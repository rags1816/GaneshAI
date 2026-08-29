/*
 * Ganapati AI - Main Arduino Sketch (2026 Dual Touch Mantra Theme)
 * 
 * Target Board: ESP32-S3 Dev Module or ESP32 NodeMCU
 * Required Libraries:
 *   - FastLED (by Daniel Garcia)
 *   - U8g2 (by Oliver)
 *   - DFRobotDFPlayerMini (by DFRobot)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>
#define FASTLED_ESP32_HAS_UART 0
#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>
#include <esp_task_wdt.h>
#include "ESP_I2S.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "config.h"
#include "web_dashboard.h"
#include "puja_page.h"
#include "indic_fonts.h"

#ifdef U8G2_HAVE_HW_I2C
#include <Wire.h>
#endif

// ==========================================
// RTC-memory crash tracer
// ==========================================
// Survives a SW_CPU_RESET (crash-triggered reboot, NOT a full power cycle).
// stageMagic is only ever this value if the previous run got far enough to
// set it; if the next boot's raw read doesn't match, either this is a fresh
// power-on or the crash happened before this code could run at all.
#define CRASH_TRACER_MAGIC 0xC0FFEE42
RTC_NOINIT_ATTR uint32_t stageMagic;
RTC_NOINIT_ATTR int lastStage;

// ==========================================
// Object Instances & Globals
// ==========================================
WebServer server(80);

// OLED Display. Which panel is compiled in is chosen by OLED_MODEL in
// config.h - see the comment block there. Both panels are 128x64, so
// nothing below this line cares which one is fitted.
#if OLED_MODEL == OLED_SH1106_I2C
  // Original 1.3" SH1106, hardware I2C on D21/D22.
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);
#elif OLED_MODEL == OLED_SSD1309_I2C
  // Waveshare 2.42" SSD1309 with its jumper moved to I2C. Same two wires
  // as the 1.3", so this is a drop-in swap.
  #if OLED_SSD1309_VARIANT == 2
    U8G2_SSD1309_128X64_NONAME2_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);
  #else
    U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);
  #endif
#elif OLED_MODEL == OLED_SSD1309_SPI
  // Waveshare 2.42" SSD1309 in its factory 4-wire SPI mode. Software SPI,
  // so the pins are free choices and avoid the LED ring and touch pads.
  #if OLED_SSD1309_VARIANT == 2
    U8G2_SSD1309_128X64_NONAME2_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ OLED_SPI_CLK, /* data=*/ OLED_SPI_DIN, /* cs=*/ OLED_SPI_CS, /* dc=*/ OLED_SPI_DC, /* reset=*/ OLED_SPI_RST);
  #else
    U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ OLED_SPI_CLK, /* data=*/ OLED_SPI_DIN, /* cs=*/ OLED_SPI_CS, /* dc=*/ OLED_SPI_DC, /* reset=*/ OLED_SPI_RST);
  #endif
#else
  #error "OLED_MODEL in config.h is not one of OLED_SH1106_I2C / OLED_SSD1309_SPI / OLED_SSD1309_I2C"
#endif

// LED Ring Array
CRGB leds[NUM_LEDS];

// DFPlayer Mini
#define dfSerial Serial2
DFRobotDFPlayerMini myDFPlayer;

// Set true only if myDFPlayer.begin() actually succeeds at boot (see
// setup()). Real crash found tonight: every myDFPlayer.stop()/
// playMp3Folder() call went straight through unconditionally, and the
// DFPlayer library's sendStack() has an unbounded `while (_isSending)`
// wait for the module's ACK with no timeout of its own - if the module
// isn't responding (unplugged, not detected at boot, wiring fault),
// that call hangs forever, and the only thing that ever stops it is the
// ESP32's own 8s hardware watchdog forcibly panicking and rebooting the
// whole board. These wrapper functions below are the fix: skip the call
// entirely when the DFPlayer was never confirmed present, the same way
// PIR_CONNECTED/TOUCH_*_CONNECTED already gate other optional hardware.
bool dfPlayerReady = false;
void dfStop() { if (dfPlayerReady) myDFPlayer.stop(); }
void dfPlay(int track) { if (dfPlayerReady) myDFPlayer.playMp3Folder(track); }
void dfSetVolume(int v) { if (dfPlayerReady) myDFPlayer.volume(v); }
int dfReadFileCounts() { return dfPlayerReady ? myDFPlayer.readFileCounts() : 0; }

// I2S amp (MAX98357A) - speaks the AI-generated blessing text through the
// altar's own speaker when a priest approves an offering (see
// speakBlessingOnAmp() below). Set true only if I2S itself initializes;
// same "skip entirely rather than hang" pattern as dfPlayerReady above -
// a missing/faulty amp should never be able to stall the whole device.
I2SClass ampI2S;
bool ampReady = false;

// Set true for the whole life of the background task that speaks a
// blessing through the amp (see speakBlessingOnAmpAsync()/
// speakGenericBlessingOnAmpAsync() below) - declared up here, ahead of
// updateStateMachine(), so its offering-resume logic can check it before
// restarting the interrupted mantra on the DFPlayer. Without that check,
// the mantra resumed on a fixed ~6s display timer that had no idea
// whether the amp's blessing (a full network round trip: Claude, then
// Google TTS, then streaming the WAV back) had actually finished -
// confirmed on hardware as the mantra and the wish-pad blessing audibly
// overlapping when the network call ran long.
volatile bool blessingTaskActive = false;

// Timestamp of the most recent blessingTaskActive=true - see
// checkBlessingTaskHealth() below. Every background task that sets the
// flag true is supposed to set it back to false itself when done
// (fetchBlessingImage()/postAndStreamAudioToAmp() all have their own
// internal timeouts), but https.POST() hanging past its own configured
// timeout is a real, documented ESP32 WiFiClientSecure/TLS-handshake
// failure mode, not just a theoretical one. If that ever happens, the
// task never reaches its own "set false" line, and with no safety net
// EVERY future offering/wish-pad touch would silently skip the spoken
// blessing forever (bell still rings - it isn't gated on this flag -
// but speakBlessingOnAmpAsync() etc. all bail out on
// "a blessing is already playing") until the board is physically
// rebooted. checkBlessingTaskHealth() is that safety net.
unsigned long blessingTaskStartMs = 0;

// One-off hardware sanity check for the physical INMP441 mic - see
// micTestTaskFn() below and the /api/test?mic=1 route. Separate I2S
// instance from ampI2S (RX vs TX, and the ESP32 has two independent I2S
// peripherals so both can exist at once); guarded the same way the amp
// task is, so a second request while one is already running can't start
// a conflicting I2S RX config on the same pins.
I2SClass micI2S;
volatile bool micTestRunning = false;

// System States
SystemState currentState = STATE_STANDBY;
unsigned long stateTimer = 0;
unsigned long stateDuration = 0;

// Human-readable state name for Serial logs. Bare enum numbers in the log
// ("state=3") are genuinely ambiguous while debugging - it cost a whole
// test round working out that state=3 meant FEET_ACTIVE fired by a
// phantom touch on a floating pin, not motion detection working.
const char* stateName(SystemState s) {
  switch (s) {
    case STATE_STANDBY:       return "STANDBY";
    case STATE_AMBIENT:       return "AMBIENT";
    case STATE_MANTRA_ACTIVE: return "MANTRA_ACTIVE";
    case STATE_FEET_ACTIVE:   return "FEET_ACTIVE";
    case STATE_AARTI:         return "AARTI";
    case STATE_TEMPLE_CLOSED: return "TEMPLE_CLOSED";
    default:                  return "?";
  }
}

// Sensor Tracking
unsigned long lastMotionTrigger = 0;
unsigned long lastTouchTrigger = 0;
bool motionDetected = false;

// Runtime Wi-Fi health check. Set true only if setup() actually connected
// in station mode (never for the AP fallback - that path is left alone
// deliberately, to avoid flip-flopping between AP and station on a device
// that already gave up on the home network once). Nothing in this
// firmware previously re-checked Wi-Fi after boot - a drop at any point
// in a 10-15 day unattended run (weak signal, router hiccup, DHCP lease
// issue - all ordinary events) meant permanent disconnection until
// someone found and power-cycled the device. checkWiFiHealth() below
// closes that gap.
bool wifiStationMode = false;
unsigned long lastWifiCheck = 0;
#define WIFI_CHECK_INTERVAL_MS 15000

// Free-heap logging - a data point this firmware has never collected.
// server.arg() and several offering/prayer parameters use the Arduino
// String class, which is a known, well-documented source of gradual
// heap fragmentation over long ESP32 uptimes (unlike tonight's specific
// hang, this is a MULTI-DAY concern - the exact failure mode a 10-15 day
// unattended festival run would actually hit). This does not fix
// anything by itself - it makes the trend visible in the Serial log, so
// a slow decline over the festival's run is something that can be SEEN
// happening rather than only inferred after a crash with no evidence.
unsigned long lastHeapLog = 0;
#define HEAP_LOG_INTERVAL_MS 60000
bool feetTouched = false;
bool backTouched = false;

// Bell-first wake (agreed design): the touch that WAKES the temple - from
// STANDBY or TEMPLE_CLOSED - rings the bell, and the mantra starts once
// the bell has had time to sound. Touches while already awake keep the
// old behaviour and start their mantra immediately. Non-blocking (no
// delay()) so the web server and sensors stay live during the bell.
// 0 = nothing pending, 1 = feet pad's mantra, 2 = mouse-back's mantra.
int pendingWakeMantra = 0;
unsigned long pendingWakeAt = 0;
#define WAKE_BELL_LEAD_MS 2500

// Per-pad debounce state for the edge-triggered touch handling in
// checkSensors(). "Stable" is the level a pad has actually held for
// TOUCH_SETTLE_MS; brief bounces from a rough solder joint never reach it.
#define TOUCH_SETTLE_MS 60
bool feetLastRead = false, feetStable = false;
bool backLastRead = false, backStable = false;
unsigned long feetChangedAt = 0, backChangedAt = 0;
bool wishPadLastRead = false, wishPadStable = false;
unsigned long wishPadChangedAt = 0;
unsigned long lastWishPadTrigger = 0;

// Separate, UNGATED tracking of the wish pad's raw pin - ignores
// WISH_PAD_CONNECTED/wishPadEnabled entirely, unlike wishPadRead above.
// See the event-triggered diagnostic in checkSensors() below: with the
// dashboard's Wish Pad toggle confirmed ON and the module's own LED
// confirmed lighting on a real touch, yet nothing reaching the firmware
// at all (no trigger, no log line), the only way to see whether GPIO4
// itself is actually moving is to watch the pin directly, gate-free.
bool wishPadRawLastRead = false, wishPadRawStable = false;
unsigned long wishPadRawChangedAt = 0;

// Advances one pad's debounce. Returns true only on the reading a pad's
// stable level actually CHANGES, so callers see one event per real
// press or release rather than a continuous "still HIGH" every pass.
bool settleTouch(bool raw, bool &lastRead, unsigned long &changedAt, bool &stable, unsigned long now) {
  if (raw != lastRead) {
    lastRead = raw;
    changedAt = now;
  }
  if (now - changedAt >= TOUCH_SETTLE_MS && stable != lastRead) {
    stable = lastRead;
    return true;
  }
  return false;
}

// Set right before an Aarti chant fired via an explicit close request
// (dashboard's "Close Temple" button) so it ends in STATE_TEMPLE_CLOSED
// rather than returning to STATE_AMBIENT (an Aarti triggered directly via
// /api/control?action=aarti with no close intent).
bool aartiThenClose = false;

// r137: true once the closing Aarti has switched from part 1
// (AARTI_TRACK) to part 2 (AARTI_PART2_TRACK) - see triggerAarti() (resets
// this false) and updateStateMachine()'s STATE_AARTI case (flips it true
// and starts part 2 at exactly AARTI_PART1_DURATION elapsed, no gap).
bool aartiPart2Playing = false;

// Offering-approval pause/resume tracking (see triggerPersonalizedOffering()):
// when a priest approves an offering while a mantra/Aarti is playing, the
// interrupted state and its FULL original duration are remembered here so
// the 12-second offering display can resume it afterward.
//
// This stores the FULL duration, not "how much was left" - a real bug
// found on hardware: since resuming replays the track from the
// beginning (see triggerPersonalizedOffering() for why), giving it only
// the time that was LEFT before the interruption meant an offering
// approved near the end of a track cut the resumed replay off almost
// immediately - it needs the full track length again, not the remainder.
bool offeringDisplayActive = false;
bool offeringInterrupted = false;
SystemState offeringPausedState = STATE_STANDBY;
unsigned long offeringPausedDurationMs = 0;

// r126: triggerWishPadBlessing() deliberately reuses STATE_FEET_ACTIVE's
// existing display-lock/interruption timing rather than adding a whole
// new SystemState (see its own comment) - but that meant the OLED's
// top-right tag and /api/state's exposed state string couldn't tell a
// real feet touch apart from a wish-pad blessing, both showing "FEET"/
// "FEET_ACTIVE". This flag is set true only by triggerWishPadBlessing(),
// right after its own setSystemState(STATE_FEET_ACTIVE, ...) call, and
// cleared unconditionally at the top of every setSystemState() transition
// (same "one place that always knows a transition just happened" pattern
// used for the eye LED) - so it's true for exactly the duration of a wish
// pad blessing and never leaks into the next real feet touch.
bool wishPadBlessingActive = false;

// Track Struct Definition
struct MantraTrack {
  int dfTrack;
  unsigned long duration;
};

// r137: reinstated r132 (reverted by r136, then confirmed by direct
// correction that track 10 DOES belong with Aarti) - track 10 moved OUT
// of feet's rotation and into the closing Aarti itself (see triggerAarti()'s
// AARTI_PART2_TRACK below). 13 tracks, not 14.
const int NUM_TRACKS = 13;
MantraTrack mantraTracks[NUM_TRACKS] = {
  {1, 19121},  // Ganapathimantrai.mp3 (Vakratundaya - actual measured length, was overestimated at 24s)
  {2, 28390},  // Ganpathimantra1.mp3 (28s)
  {4, 27360},  // Ganapathimantra2.mp3 (27s)
  {5, 55350},  // Ganeshmantra3.mp3 (55s)
  {6, 155530}, // Ganeshmantra4.mp3 (155s)
  {7, 99600},  // Ganeshmantra5.mp3 (99s)
  {8, 60160},  // Ganeshmantra6.mp3 (60s)
  {9, 136700}, // Ganeshmantra7.mp3 (136s)
  {11, 48800}, // Ganeshmantra9.mp3 (48s)
  {12, 27380}, // Ganeshmantra10.mp3 (27s)
  {13, 79280}, // Ganeshmantra11.mp3 (79s)
  {14, 102000},// Ganeshmantra12.mp3 - r131: intentionally re-recorded shorter (1:42), confirmed by user
  {15, 28400}  // Ganeshmantra13.mp3 (28s)
};

// r130: mouse-back's own rotating pool, separate from mantraTracks[]
// above - see config.h's comment for the full history. Real measured
// durations from the actual SD card file listing (MM:SS resolution,
// converted to ms) - not placeholders.
const int NUM_MOUSE_TRACKS = 10;
MantraTrack mouseChantTracks[NUM_MOUSE_TRACKS] = {
  {17, 12000},  // Ganapati Bappa Morya... + short Vakratunda Maha Kaya (0:12)
  {21, 39000},  // 0:39
  {22, 239000}, // 3:59 - unusually long next to the rest of this pool; worth double-checking it's the intended file
  {23, 21000},  // 0:21
  {24, 88000},  // 1:28
  {25, 102000}, // 1:42
  {26, 45000},  // 0:45
  {27, 86000},  // 1:26
  {28, 127000}, // 2:07
  {29, 253000}  // 4:13
};

// Dynamic Playlist Counters - feet touch and mouse-back each rotate
// through their own separate pool (mantraTracks[]/mouseChantTracks[]).
int feetStep = 0;
int mouseStep = 0;

// The DFPlayer folder-track number (matches mantraTracks[].dfTrack, or
// AARTI_TRACK) currently audibly playing - 0 means nothing from the
// devotional library is playing right now (BELL_TRACK doesn't count).
// Exposed via /api/state as "track" so the dashboard's Now Playing/Song
// Library panel can highlight the right entry even when a mantra was
// started by a PHYSICAL touch the browser never saw a local trigger for
// - previously the dashboard only knew this if IT had started the track
// itself locally. Also doubles as the "what to resume" memory for
// triggerPersonalizedOffering() below.
int currentPlayingTrack = 0;

// Phase 1: Offline Blessing Fallback counters - see
// playOfflineBlessingFallback() below and /api/state's offlineFallbackCount
// field. Declared up here (not next to the function itself) since
// /api/state's handler, registered in setup() well before that function's
// definition later in the file, reads it directly - Arduino's automatic
// function-prototype generation doesn't extend to global variables.
unsigned long offlineFallbackCount = 0;
unsigned long lastOfflineFallbackMs = 0;

// V2 Phase 5: captured once in setup() from the crash-tracer's local
// prevRunCrashed/crashedStage (see the comment there) - stageMagic/
// lastStage themselves get overwritten immediately after for THIS run's
// own tracking, so these globals are the only way /api/health can report
// on the previous run's crash status later.
bool bootCrashedLastRun = false;
int bootCrashedStage = 0;

// Eye LED breathing animation state - see updateEyeLedBreathing() below.
// eyeLedBreathingActive is the only thing triggerMantra()/setSystemState()
// touch directly; the actual brightness curve is computed continuously
// from eyeLedBreathingStartMs each loop() pass, same non-blocking pattern
// as animateLeds()'s animHue.
bool eyeLedBreathingActive = false;
unsigned long eyeLedBreathingStartMs = 0;

// Customization & Settings
int blessingCounter = 0;
int currentBrightness = DEFAULT_BRIGHT;
int currentPattern = 0; 
int currentVolume = DEFAULT_VOLUME;
bool pirEnabled = true;

// Runtime admin on/off toggles (dashboard "System Settings") for the
// remaining components, same pattern as pirEnabled above - these are
// independent of the compile-time *_CONNECTED flags in config.h (which
// mean "is this hardware physically wired up at all"); these mean "is it
// currently switched on" for hardware that IS wired up. No Mic toggle
// here - the main board has no mic functionality built into it at all
// yet (that only exists in the separate diagnostics/ExpansionBench
// sketch), so there's nothing on this board to switch off.
bool displayEnabled = true;
bool ledEnabled = true;
bool touchFeetEnabled = true;
bool touchBackEnabled = true;
bool wishPadEnabled = true;

// LED "opening" transition - a brief bloom played by animateLeds() right
// after waking from closed/standby into any active state, before settling
// into that state's normal pattern. Set by setSystemState() on the actual
// state-change edge (old state was closed/standby, new state isn't); 0
// means no transition is currently playing. The "closing" transition
// (temple going TO closed/standby) is played synchronously by
// playLedClosingSweep() instead, since it's a one-off fade rather than
// something that needs to keep animating through later frames.
unsigned long ledOpeningTransitionStart = 0;
#define LED_OPENING_TRANSITION_MS 1200

// Language Settings: 0 = English, 1 = Sanskrit/Hindi, 2 = Tamil, 3 = Marathi
// (Marathi used to share code 1 with Sanskrit/Hindi - see LANG_TO_CODE in
// web_dashboard.h - split out to its own code so the wish pad can
// actually speak Marathi instead of always falling back to Hindi.)
int selectedLang = 1;
// Theme of the Day: 0 = Tue (Ganesha), 1 = Mon (Shiva), 2 = Wed (Wisdom), 3 = Thu (Guru), 4 = Fri (Shakti), 5 = Sat (Discipline), 6 = Sun (Sun)
int selectedTheme = 0;

// V2 Phase 6: Temple Atmosphere - 0=Day, 1=Evening, 2=Night, 3=Festival.
// A manual dashboard toggle only, deliberately no NTP/time-of-day
// automation - per the reviewed V2 feedback, auto-switching by clock
// time can surprise a devotee with the wrong mode after a Wi-Fi outage
// or an incorrect timezone; a manual choice is predictable and the
// operator stays in control. Applied in animateLeds() as a color wash
// layered on top of whatever Theme of the Day already picked, for
// STATE_AMBIENT specifically - it doesn't touch Mantra/Feet/Aarti/mood
// colors, which stay exactly as before.
int selectedAtmosphere = 0;

// OLED Text Buffer
// Fixed-size buffer, not String: drawOLED() runs ~30x/sec and continuous
// String reallocation there was the cause of a heap-fragmentation abort()
// crash after ~7 minutes of uptime.
//
// 500, not 300 - a ~40-word blessing in Devanagari (Marathi/Hindi) or
// other 3-byte-per-character UTF-8 scripts can run noticeably longer in
// bytes than the same word count in English; 300 left too little margin
// against snprintf() silently truncating a longer non-English blessing
// (and potentially mid-character - the same class of bug fixed in
// /api/state's JSON encoder, see checkSensors()/that handler below).
char scrollText[500] = "";
int scrollX = 128;
unsigned long lastScrollUpdate = 0;

// Which language scrollText's CONTENT is actually in right now - drives
// which OLED font drawOLED() loads before drawing it (see fontForScrollLang()
// and OLED_UNSUPPORTED_SCRIPT_LANGS below). Everything in this firmware is
// English EXCEPT a personalized offering's real prayer/blessing text
// (triggerPersonalizedOffering() when prayer.length() > 0) - every other
// scrollText assignment in the file sets this back to "en" alongside it,
// so a leftover non-Latin font can never bleed into an unrelated English
// message once the offering that needed it has ended.
char scrollTextLang[4] = "en";

// Sentiment-aware LED mood - set from the backend's X-Blessing-Mood
// response header (see postAndStreamAudioToAmp()'s outMood parameter and
// askClaudeForBlessing()/THEME_TO_MOOD in index.js) whenever a wish-pad
// touch or offering has one available. Empty string means "no mood
// known" - animateLeds() falls back to the plain saffron/gold blessing
// palette in that case (network/backend failure, or the phone-relayed
// offering path where a mood wasn't captured - see the mood query param
// in triggerPersonalizedOffering()). Cleared back to "" everywhere
// offeringDisplayActive itself gets cleared, so a mood color can never
// bleed into an unrelated later blessing.
char currentMoodTag[16] = "";

// Correctly-shaped blessing bitmap for scripts U8g2 can't render properly
// on its own (see renderTextToXbm()/SCRIPT_FONTS in backend/functions/
// index.js and fetchBlessingImage() below) - non-NULL exactly when
// drawOLED() should draw+scroll THIS image instead of scrollText.
// Heap-allocated because size varies with text length; freeBlessingImage()
// must be called before ever overwriting these, and at every point
// offeringDisplayActive itself gets cleared, so a stale image can never
// bleed into an unrelated later blessing or leak memory.
uint8_t *blessingImageBuf = NULL;
int blessingImageWidth = 0;
int blessingImageHeight = 0;
int blessingImageBytesPerRow = 0;
int blessingImageScrollX = 128;

void freeBlessingImage() {
  if (blessingImageBuf != NULL) {
    free(blessingImageBuf);
    blessingImageBuf = NULL;
  }
  blessingImageWidth = 0;
  blessingImageHeight = 0;
  blessingImageBytesPerRow = 0;
}

// Display lock helper for Feet touch / Mouse Back / offerings. The lock
// holds ONE message on screen (no blessing rotation) for feetDisplayLockMs
// before the rotation resumes. 12s for an ordinary touch; an offering sets
// it long and the state machine clears it explicitly once the message has
// finished a real scroll pass, because a long devotee name+prayer cannot
// physically finish a pass in 12s and devotees were only ever seeing a
// fragment of their own offering.
unsigned long feetDisplayTimer = 0;
unsigned long feetDisplayLockMs = 12000;
bool feetDisplayLocked = false;

// Pixels moved per drawOLED frame. Deliberately NOT used to predict how
// long a scroll pass will take: drawOLED's 33ms gate plus loop()'s
// delay(10) makes real frames land every ~40ms, so any arithmetic
// estimate runs ~20% fast and clipped the tail off offering messages.
// Code that needs to know a pass finished waits on scrollPassComplete,
// which drawOLED sets from the actual scroll position.
#define SCROLL_PX_PER_STEP 3

// Ambient blessing rotation - cycles through the same 48-message list
// (oledChildBlessingsList + oledAdultBlessingsList) the web dashboard
// rotates through, instead of showing one fixed scrolling sentence.
unsigned long lastAmbientBlessingRotate = 0;
int ambientBlessingIdx = 0;
// Was a fixed 15s timer - broke once the OLED font got much bigger (see
// drawOLED()): wider glyphs mean a typical blessing sentence takes far
// longer than 15s to fully scroll across a 128px screen, so most
// blessings were getting replaced mid-scroll, sometimes after only a
// couple of words were visible. Rotation is now driven by
// scrollPassComplete (set by drawOLED() when the CURRENT text has
// actually finished one full scroll pass) instead - this is just the
// minimum floor so a very short string can't flicker instantly.
#define MIN_BLESSING_DISPLAY_MS 2000
bool scrollPassComplete = false;

// Diagnostics counters/flags exposed via /api/pins and /api/test?oled=1.
// The touch counters count ACCEPTED touches (past settle + debounce), i.e.
// exactly the ones that print a TOUCH: line - so the browser can verify a
// pad without a serial cable. oledTestUntil, while in the future, makes
// drawOLED paint a solid white test card regardless of state - the most
// visible thing a marginal panel can show, and drawn even in STANDBY when
// the screen is normally off.
uint32_t feetTouchCount = 0;
uint32_t backTouchCount = 0;
unsigned long oledTestUntil = 0;

// ==========================================
// Text Database
// ==========================================
const char* oledWelcome[] = {
  "Welcome!",      
  "swagatam!",     
  "varaverpu!"     
};

// Mouse Back: Ganapathimantrai.mp3 lyrics (Maintained for reference)
const char* oledMantra[] = {
  "   Om Gan Ganapataye Namaha! May the Lord of Wisdom optimize your life's neural networks.   ",
  "   Om Gan Ganapataye Namaha! vakratunda mahakaya suryakoti samaprabha. mangalamurti morya!   ",
  "   Om Gam Ganapataye Namaha! mangala murthi moraiya!   "
};

// Real Blessings database from Blessings1.docx
const char* oledAdultBlessingsList[] = {
  "I bless you with the removal of professional and personal roadblocks to ensure success.",
  "I grant you the grace of financial stability, wealth, and career growth.",
  "I bless you with the emotional capacity to handle high-stress situations with calm.",
  "I bless you with the wisdom to weigh choices objectively and make sound judgments.",
  "I bless you with a peaceful, loving, and supportive family environment.",
  "I grant you the ability to be thankful for life's blessings and maintain self-improvement.",
  "I bless you with the discipline to avoid greed, anger, or unhealthy attachments.",
  "I bless you to forgive easily and help those in need.",
  "I grant you the wisdom to release things that are beyond your control.",
  "I wish you a deeper connection to your inner self and finding peace amidst a busy life.",
  "I bless you with profound clarity of mind, patient wisdom, and inner strength.",
  "May all professional obstacles dissolve, opening wide doors to prosperity and success.",
  "I grant you emotional resilience and a peaceful heart, shielding you from anxiety.",
  "May My divine energy rejuvenate your physical body, infusing you with robust health.",
  "I bless your home with harmony, lasting unity, and a deep sense of security.",
  "When the weights of responsibility feel too heavy, surrender your burdens into My hands.",
  "May you always possess the integrity, humility, and patience to handle hard situations.",
  "I bless your hard work so that it bears rich fruit, ensuring you never lack resources.",
  "May My presence be a constant anchor in your life, grounding you in spiritual peace.",
  "I bless your journey with continuous growth, purposeful action, and vibrant well-being.",
  "May your heart beat with steady strength and your body remain flexible and resilient.",
  "I bless you with the wisdom to balance hard work with mindful rest, protecting your vitality."
};

const char* oledChildBlessingsList[] = {
  "I bless you to explore, learn, and absorb new knowledge eagerly.",
  "I bless you with the boon of concentration to stay grounded during studies.",
  "I grant you the courage to bounce back quickly when faced with difficult subjects.",
  "I bless you with the grace to remain polite and grounded, much like Ganesha's nature.",
  "I grant you the inspiration to think outside the box and express yourself freely.",
  "I grant you the wisdom to make healthy, positive lifestyle choices from a young age.",
  "I bless you with inner strength to overcome fears in making friends or public speaking.",
  "I grant you the ability to find simple, childlike happiness in everyday moments.",
  "I grant you the blessing of attracting honest, supportive, and kind friends.",
  "I bless you with the shield of grace to keep you safe and guide your journey.",
  "I bless you to be guided safely past financial, mental, and physical hurdles.",
  "I bless you with clarity, sharp focus, and success in studies or new ventures.",
  "I bless you with attracting material success, abundance, and good fortune.",
  "I grant your wish of creating a balanced, calm, and positive environment.",
  "I bless your young mind with sharp focus, memory, and joyful curiosity to learn.",
  "May your heart always be fearless and filled with kind thoughts for everyone.",
  "Whenever a school lesson feels too difficult, remember that I am right beside you.",
  "I grant you the wisdom of My large ears to listen carefully and grow wise.",
  "May My blessings protect you from harm and guide your steps safely.",
  "I fill your spirit with boundless energy to play and explore the world with joy.",
  "May you always speak words as sweet as the modaks I love, spreading happiness.",
  "I bless your body with robust health, deep immunity, and strong glowing energy.",
  "When you feel sad or alone, close your eyes and call My name for instant comfort.",
  "I bless your entire childhood with endless wonder, creative ideas, and a bright smile.",
  "I grant you deep, peaceful sleep at night so your body can rest and wake up full of energy.",
  "I bless every meal you eat to nourish your bones, sharpen your mind, and make you strong."
};

// Generate random combined rolling blessings text for Ambient & Mouse loop playing
const char* oledAmbientLoopText = 
  "   [WELCOME] sukh-samriddhi labho! Wishing you deep intellect, peace, and spiritual growth. khoop abhyas kar, motha ho ani nehami hasat raha! Happy coding! vidyam dadati vinayam. ungal vazhvil anaithu thadaigalum neengi vetri perattum.   ";

// ==========================================
// Function Declarations
// ==========================================
void handleWebRoutes();
void checkSensors();
void checkWiFiHealth();
void checkHeapHealth();
void checkBlessingTaskHealth();
void updateStateMachine();
void animateLeds();
void playLedClosingSweep();
void drawOLED();
void setSystemState(SystemState newState, unsigned long duration = 0);
void triggerMantra();
void triggerFeetMantra();
void triggerPersonalizedOffering(String name, String offeringType, String prayer, String lang, String mood);
void triggerWishPadBlessing();
void micTestTaskFn(void *pvParameters);
void fetchBlessingImage(const String &text, const String &lang);
void freeBlessingImage();
void micRecordPlaybackTaskFn(void *pvParameters);
void triggerAarti();
void triggerAartiThenClose();
void openTempleFromClosed();
void stopAudioAndStandby();
void playOfflineBlessingFallback(const String &lang);
void playTrackManually(int n);
ExperienceScene getCurrentScene();
bool moodColorsFor(const char *mood, CRGB &c1, CRGB &c2);
int moodPatternFor(const char *mood);
void playMoodClosurePulse(const char *mood);
void updateEyeLedBreathing();

// ==========================================
// Setup Function
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1500); // Give serial monitor time to connect
  Serial.println("\n==================================");
  Serial.println("  Initializing Advanced Ganapati  ");
  Serial.println("==================================");
  // Bump FIRMWARE_VERSION any time GanapatiAI.ino/config.h changes, so the
  // boot log itself proves what's actually flashed - no download/caching
  // ambiguity possible, unlike checking a saved local file.
  Serial.print("FIRMWARE_VERSION: ");
  Serial.println(FIRMWARE_VERSION);

  // Unconditional, earliest-possible sanity check: dump the raw RTC memory
  // value before any crash-check logic touches it, so we know whether RTC
  // memory is behaving as expected on this board/core at all.
  Serial.print("DEBUG: raw stageMagic on boot = 0x");
  Serial.println(stageMagic, HEX);
  Serial.print("DEBUG: raw lastStage on boot = ");
  Serial.println(lastStage);

  bool prevRunCrashed = (stageMagic == CRASH_TRACER_MAGIC);
  int crashedStage = lastStage;
  // V2 Phase 5: saved into globals (stageMagic/lastStage themselves get
  // overwritten a few lines below, for THIS run's own tracking) so the
  // dashboard health panel can report it later via /api/health - this
  // was previously Serial-only, visible only if someone happened to be
  // watching the monitor at the exact moment of reboot.
  bootCrashedLastRun = prevRunCrashed;
  bootCrashedStage = crashedStage;
  if (prevRunCrashed) {
    Serial.print("DEBUG: previous run crashed near stage ");
    Serial.println(crashedStage);
  } else {
    Serial.println("DEBUG: no crash detected on previous run (or first boot after power-on).");
  }

  // Mark that we've entered setup(); a crash between here and the first
  // loop() stage update will now be attributed to stage 0 on the next boot.
  stageMagic = CRASH_TRACER_MAGIC;
  lastStage = 0;

  // Seed random generator using ESP32 hardware RNG
  randomSeed(esp_random());
  Serial.println("DEBUG: Random seed set successfully.");

  // 1. Initialize Sensor Pins
  // Plain INPUT, no pull. An internal pull-down (tried in r15) can drag
  // the AM312's very weak output (~100uA) below the HIGH threshold and
  // mute a perfectly good sensor; the AM312 drives both HIGH and LOW on
  // its own, so it needs no pull with sound wiring.
  pinMode(PIR_PIN, INPUT);
  Serial.printf("DEBUG: PIR on GPIO%d, PIR_CONNECTED=%s\n",
                PIR_PIN, PIR_CONNECTED ? "true" : "false");
  // INPUT_PULLDOWN, not plain INPUT - confirmed on hardware tonight as a
  // real problem, not a theoretical one: WISH PAD RAW logged GPIO4
  // flipping HIGH/LOW on its own with nobody touching it, clustering
  // right around unrelated activity (a feet touch on a different pin).
  // A plain high-impedance INPUT floats and picks up ambient/coupled
  // noise the moment its connection is even slightly marginal; the
  // ESP32's own internal pull-down holds the pin firmly LOW whenever
  // nothing is actively driving it, without blocking a genuinely
  // connected TP223 (push-pull output) from still driving HIGH normally.
  // Applied to all three touch pins since the same risk applies equally
  // to each - not just the one that happened to show it first.
  pinMode(TOUCH_FEET_PIN, INPUT_PULLDOWN);
  Serial.println("DEBUG: TOUCH_FEET_PIN configured.");
  pinMode(TOUCH_BACK_PIN, INPUT_PULLDOWN);
  Serial.println("DEBUG: TOUCH_BACK_PIN configured.");
  pinMode(WISH_PAD_PIN, INPUT_PULLDOWN);
  Serial.printf("DEBUG: WISH_PAD_PIN configured, WISH_PAD_CONNECTED=%s\n",
                WISH_PAD_CONNECTED ? "true" : "false");
  if (EYE_LED_CONNECTED) {
    // LEDC PWM, not plain digitalWrite - see the breathing animation
    // comment in config.h. Core 3.x single-call API (confirmed on the
    // user's installed 3.1.3 board package).
    ledcAttach(EYE_LED_PIN, EYE_LED_PWM_FREQ_HZ, EYE_LED_PWM_RESOLUTION);
    ledcWrite(EYE_LED_PIN, EYE_LED_BASE_BRIGHTNESS); // dim steady glow, never fully dark
    Serial.println("DEBUG: EYE_LED_PIN configured (PWM).");
  }
  Serial.println("DEBUG: Sensor pins configured successfully.");
  // Printed explicitly because these two config.h flags being left false
  // is the single most common reason touch "does nothing" after wiring a
  // sensor - checkSensors() completely skips reading a pin whose
  // _CONNECTED flag is false, on purpose (see config.h), and that's easy
  // to forget to flip after wiring. If either shows "false" here and you
  // HAVE wired that sensor, that's very likely the whole problem.
  Serial.printf("DEBUG: TOUCH_FEET_CONNECTED=%s  TOUCH_BACK_CONNECTED=%s\n",
                TOUCH_FEET_CONNECTED ? "true" : "false", TOUCH_BACK_CONNECTED ? "true" : "false");

  // 2. Initialize OLED Display
  Serial.println("DEBUG: Initializing OLED Display...");
  u8g2.begin();
  Serial.println("DEBUG: u8g2.begin() returned.");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(5, 20, "Ganapati AI 2026");
  u8g2.drawStr(5, 35, "Voice Init...");
  u8g2.sendBuffer();
  // Deliberately NOT "initialized successfully": on SPI the ESP32 only ever
  // writes to the panel - there is no reply line - so this code cannot tell
  // whether a display is attached at all. An earlier message here claimed
  // success and sent debugging down the wrong road.
  Serial.println("DEBUG: OLED init + first frame SENT. SPI is write-only: this proves");
  Serial.println("DEBUG:   nothing about the panel - only your eyes can. If the screen is");
  Serial.println("DEBUG:   dark, browse to /api/test?oled=1 to force a solid-white test card.");

  // If the previous run crashed, hold that fact on-screen (and on Serial)
  // for ~30s so it's actually catchable before the boot proceeds further.
  if (prevRunCrashed) {
    char crashMsg[32];
    snprintf(crashMsg, sizeof(crashMsg), "CRASHED near stage %d", crashedStage);
    Serial.print("DEBUG: displaying crash tracer: ");
    Serial.println(crashMsg);
    unsigned long crashBlinkStart = millis();
    bool blinkOn = true;
    while (millis() - crashBlinkStart < 30000) {
      u8g2.clearBuffer();
      if (blinkOn) {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(5, 20, "PREV RUN CRASHED:");
        u8g2.drawStr(5, 35, crashMsg);
      }
      u8g2.sendBuffer();
      blinkOn = !blinkOn;
      delay(500);
    }
  }

  // 3. Initialize LEDs (FastLED)
  // Skipped entirely when LED_CONNECTED is false: no point driving a ring
  // that is not attached, and it keeps ~28KB of FastLED out of the build.
  // (Not a FastLED bug - an earlier comment here wrongly blamed FastLED for
  // a repeating "...itself is hanging" line that is actually the DFPlayer
  // retry message below, truncated by the Serial Monitor.)
  if (LED_CONNECTED) {
    Serial.println("DEBUG: Initializing FastLED...");
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    // Enforce the supply's current budget BEFORE any show(). FastLED
    // scales brightness down on the fly whenever a frame would exceed
    // this, so an undersized supply produces a dimmer ring rather than a
    // sagging rail. Without it, a bright frame on a modest supply browns
    // out - which is exactly what took the board down once already.
    FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MILLIAMPS);
    FastLED.setBrightness(currentBrightness);
    fill_solid(leds, NUM_LEDS, CRGB(0, 242, 254));
    FastLED.show();
    delay(400);
    FastLED.clear();
    FastLED.show();
    Serial.println("DEBUG: FastLED initialized successfully.");
  } else {
    Serial.println("DEBUG: FastLED SKIPPED (LED_CONNECTED=false in config.h).");
  }

  // 4. Microphone (I2S driver removed - see git history; was an unused,
  // never-called legacy driver/i2s.h include suspected of conflicting with
  // WiFi's ADC-based RF calibration and causing an early abort() crash)
  u8g2.drawStr(5, 48, "Microphone: OK");
  u8g2.sendBuffer();

  // 5. Initialize DFPlayer Mini
  // Pins printed from the actual macros, never hardcoded. This line used
  // to read "GPIO16/17" - stale text from before the pins were moved to
  // 25/26 (16/17 are merely ESP32 Serial2 defaults). Anyone wiring the
  // module by that message would have connected it to the wrong pins.
  Serial.printf("STEP 5: Initializing DFPlayer Mini (Serial2, ESP32 RX=GPIO%d <- DFPlayer TX, ESP32 TX=GPIO%d -> DFPlayer RX)...\n",
                DFPLAYER_RX, DFPLAYER_TX);
  dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  u8g2.drawStr(5, 60, "Audio Board: Connecting...");
  u8g2.sendBuffer();

  int retries = 0;
  while (retries < 5) {
    Serial.print("STEP 5: begin() attempt ");
    Serial.print(retries + 1);
    Serial.println(" of 5 - calling now (if this is the LAST line you ever see, begin() itself is hanging)...");

    if (myDFPlayer.begin(dfSerial)) {
      dfPlayerReady = true;
      Serial.println("STEP 5: DFPlayer responded - begin() returned true.");
      break;
    }

    Serial.println("STEP 5: begin() returned false (no/bad response yet), retrying in 500ms...");
    delay(500);
    retries++;
  }
  if (!dfPlayerReady) {
    Serial.println("STEP 5: DFPlayer NOT detected after 5 attempts - continuing WITHOUT audio so the rest of the device still works.");
  }

  if (dfPlayerReady) {
    myDFPlayer.volume(currentVolume);
    u8g2.drawStr(5, 60, "Audio Board: OK           ");
  } else {
    u8g2.drawStr(5, 60, "Audio Board: Missing SD   ");
  }
  u8g2.sendBuffer();
  delay(800);

  // 5.5 Initialize the I2S amp (MAX98357A) - speaks AI blessings through
  // the altar's own speaker. Fixed format (16kHz/16-bit/mono) matching
  // exactly what the backend's Google TTS call always requests - see
  // speakBlessingOnAmp() and backend/functions/index.js's synthesizeSpeech().
  Serial.printf("STEP 5.5: Initializing I2S amp (BCLK=GPIO%d, LRC=GPIO%d, DIN=GPIO%d)...\n",
                AMP_BCLK_PIN, AMP_LRC_PIN, AMP_DIN_PIN);
  ampI2S.setPins(AMP_BCLK_PIN, AMP_LRC_PIN, AMP_DIN_PIN);
  ampReady = ampI2S.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);
  Serial.println(ampReady ? "STEP 5.5: Amp I2S bus initialized." : "STEP 5.5: Amp FAILED to initialize - continuing without spoken blessings.");

  // 6. Initialize Wi-Fi Connection to Home Router
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);
  
  u8g2.clearBuffer();
  u8g2.drawStr(5, 20, "Connecting Wi-Fi...");
  u8g2.drawStr(5, 35, WIFI_SSID);
  u8g2.sendBuffer();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int wRetries = 0;
  while (WiFi.status() != WL_CONNECTED && wRetries < 20) {
    delay(500);
    Serial.print(".");
    wRetries++;
  }

  IPAddress myIP;
  if (WiFi.status() == WL_CONNECTED) {
    myIP = WiFi.localIP();
    wifiStationMode = true;
    Serial.println("\nWi-Fi Connected!");
    Serial.print("IP Address: ");
    Serial.println(myIP);
    
    u8g2.clearBuffer();
    u8g2.drawStr(5, 20, "Wi-Fi Connected!");
    u8g2.drawStr(5, 35, WIFI_SSID);
    String ipStr = "IP: " + myIP.toString();
    u8g2.drawStr(5, 50, ipStr.c_str());
    u8g2.sendBuffer();
  } else {
    Serial.println("\nWi-Fi connection failed. Starting AP Mode...");
    WiFi.softAP("Ganapati_AI_AP", "Morya2026");
    myIP = WiFi.softAPIP();
    Serial.print("AP IP Address: ");
    Serial.println(myIP);
    
    u8g2.clearBuffer();
    u8g2.drawStr(5, 20, "Wi-Fi Failed.");
    u8g2.drawStr(5, 35, "AP Mode Active:");
    String ipStr = "IP: " + myIP.toString();
    u8g2.drawStr(5, 50, ipStr.c_str());
    u8g2.sendBuffer();
  }
  delay(3000);

  // 6. Register Web Server routes
  Serial.println("STEP 6: Registering Web Server routes...");
  handleWebRoutes();
  Serial.println("STEP 6: Web Server routes registered!");
  
  // 7. Start Web Server
  Serial.println("STEP 7: Starting Web Server...");
  server.begin();
  Serial.println("STEP 7: Web Server started!");

  // 8. Play Startup Sound
  Serial.println("STEP 8: Playing startup sound...");
  if (dfPlayerReady) {
    myDFPlayer.playMp3Folder(BELL_TRACK);
    Serial.println("STEP 8: Startup sound played!");
  } else {
    Serial.println("STEP 8: Startup sound SKIPPED (DFPlayer not ready)");
  }

  // 9. Set Standby State
  Serial.println("STEP 9: Setting system state to standby...");
  setSystemState(STATE_STANDBY);
  Serial.println("STEP 9: System state set to standby!");

  // Repeated at the END of boot on purpose. These flags decide whether a
  // sensor is read at all, so "nothing happens when I touch the pad" is
  // usually just one of them being false - but printed once at STEP 1
  // they scroll far out of view behind the WiFi/DFPlayer/FastLED chatter,
  // which is exactly how a false flag went unnoticed. Last lines of the
  // boot log, where they are actually visible.
  Serial.println("----------------------------------------------------");
  Serial.printf("CONFIG  %s\n", FIRMWARE_VERSION);
  Serial.printf("  PIR (GPIO%d)          : %s\n", PIR_PIN,
                PIR_CONNECTED ? "ENABLED" : "disabled - not read");
  // The pin's ACTUAL level is printed alongside, because "enabled but
  // silent" and "enabled but stuck" look identical otherwise. A TP223 in
  // momentary mode idles LOW and pulses HIGH on touch, so at rest an
  // enabled pad should read LOW here. Reading HIGH at rest means a stuck
  // line (solder bridge, or the module set to self-lock mode); a pad that
  // reads LOW but never logs a TOUCH when pressed is not receiving the
  // module's output at all.
  Serial.printf("  Feet pad (GPIO%d)     : %s, reads %s at rest\n", TOUCH_FEET_PIN,
                TOUCH_FEET_CONNECTED ? "ENABLED" : "disabled - not read",
                digitalRead(TOUCH_FEET_PIN) == HIGH ? "HIGH <- unexpected" : "LOW (normal idle)");
  Serial.printf("  LED ring (GPIO%d)     : %s (budget %dmA @5V)\n", LED_PIN,
                LED_CONNECTED ? "ENABLED" : "disabled - FastLED not initialised",
                LED_MAX_MILLIAMPS);
  Serial.printf("  Mouse-back pad (GPIO%d): %s, reads %s at rest\n", TOUCH_BACK_PIN,
                TOUCH_BACK_CONNECTED ? "ENABLED" : "disabled - not read",
                digitalRead(TOUCH_BACK_PIN) == HIGH ? "HIGH <- unexpected" : "LOW (normal idle)");
  // Which panel is compiled in, and on which wires. A blank screen is the
  // same symptom for "wrong panel selected" and "wrong pins", so the log
  // has to say which one the firmware believes is fitted.
#if OLED_MODEL == OLED_SH1106_I2C
  Serial.printf("  OLED                  : 1.3\" SH1106, I2C on SDA=GPIO%d SCL=GPIO%d\n",
                OLED_SDA, OLED_SCL);
#elif OLED_MODEL == OLED_SSD1309_I2C
  Serial.printf("  OLED                  : 2.42\" SSD1309 (variant %d), I2C on SDA=GPIO%d SCL=GPIO%d\n",
                OLED_SSD1309_VARIANT, OLED_SDA, OLED_SCL);
#elif OLED_MODEL == OLED_SSD1309_SPI
  Serial.printf("  OLED                  : 2.42\" SSD1309 (variant %d), SPI CLK=GPIO%d DIN=GPIO%d CS=GPIO%d DC=GPIO%d RST=GPIO%d\n",
                OLED_SSD1309_VARIANT, OLED_SPI_CLK, OLED_SPI_DIN, OLED_SPI_CS, OLED_SPI_DC, OLED_SPI_RST);
#endif
  Serial.println("  (blank screen? try OLED_SSD1309_VARIANT 0 <-> 2 in");
  Serial.println("   config.h BEFORE suspecting the wiring)");
  Serial.println("  (a pad showing 'disabled' can never trigger - set its");
  Serial.println("   *_CONNECTED flag in config.h to true once it is wired)");
  Serial.println("----------------------------------------------------");

  // Watchdog - started HERE, deliberately after boot is fully done, not at
  // the top of setup(). The DFPlayer retry loop above (5 attempts, each
  // observed on hardware taking up to ~2.2s to time out) plus a slow Wi-Fi
  // connect can legitimately take longer than any single runtime operation
  // ever should - starting the watchdog before that would risk a false
  // reboot-loop on the exact "no DFPlayer attached" boots this project has
  // had all day. 8000ms comfortably clears the longest known deliberate
  // block once running (900ms, the bell-before-mantra delay in
  // openTempleFromClosed()) with better than 8x margin, while still
  // recovering in seconds rather than needing someone to notice a hung,
  // unattended device and power-cycle it by hand - the actual failure
  // mode reported on hardware tonight (Serial AND the dashboard both went
  // silent together, consistent with loop() itself stalling, cause not
  // yet found). This does not fix that unknown cause - it makes a hang
  // survivable until it is found.
  // REAL BUG FOUND ON HARDWARE: esp_task_wdt_init() below was silently
  // failing every boot - "TWDT already initialized" - because the
  // Arduino framework itself already initializes the watchdog before
  // setup() ever runs, with ITS OWN defaults (confirmed against this
  // core's sdkconfig: 5000ms timeout, not the 8000ms this code believed
  // it was setting). So this device has been running on a 5-second
  // watchdog with idle tasks monitored, the whole time - not the 8s/no-
  // idle-monitoring config written here. esp_task_wdt_reconfigure() is
  // the correct call once the TWDT is already running (esp_task_wdt_init()
  // is only for the very first init) - this is what actually applies the
  // intended settings. Falls back to init() + add() in case some other
  // environment genuinely hasn't initialized it yet.
  esp_task_wdt_config_t twdt_config = {
    .timeout_ms = 8000,
    .idle_core_mask = 0,
    .trigger_panic = true,
  };
  esp_err_t wdtErr = esp_task_wdt_reconfigure(&twdt_config);
  if (wdtErr != ESP_OK) {
    Serial.printf("WATCHDOG: reconfigure() failed (%d), falling back to init()...\n", wdtErr);
    esp_task_wdt_init(&twdt_config);
  }
  // REGRESSION FOUND ON HARDWARE (r64): this call was accidentally left
  // inside the fallback branch above, so on the normal/successful path
  // (reconfigure() returning ESP_OK) the main loop task was NEVER
  // subscribed to the watchdog at all - every esp_task_wdt_reset() call,
  // including the pre-existing one at the top of loop(), then failed with
  // "task not found", flooding the log and silently disabling ALL crash
  // protection (worse than before the r64 fix, not better). Must run on
  // every path, not just the fallback.
  esp_task_wdt_add(NULL);
  Serial.printf("WATCHDOG: reconfigure() returned %d - now actually 8000ms, idle tasks NOT monitored.\n", wdtErr);
}

// ==========================================
// Main Loop
// ==========================================
void loop() {
  esp_task_wdt_reset();
  lastStage = 1;
  server.handleClient();
  checkWiFiHealth();
  checkHeapHealth();
  checkBlessingTaskHealth();
  lastStage = 2;
  checkSensors();
  lastStage = 3;
  updateStateMachine();
  lastStage = 4;
  animateLeds();
  updateEyeLedBreathing();
  lastStage = 5;
  drawOLED();
  delay(10);
}

// ==========================================
// Web Server API Routes
// ==========================================
void handleWebRoutes() {
  // Both pages change with nearly every firmware update tonight, and
  // phone browsers were caching stale copies (missing the latest
  // features) even after a fresh reflash - explicitly forbidding caching
  // means a reload always gets the version actually running right now.
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send_P(200, "text/html", INDEX_HTML);
  });

  // Devotee-facing offering submission page - the QR code on the admin
  // dashboard links here so devotees can submit from their own phones.
  server.on("/puja", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send_P(200, "text/html", PUJA_HTML);
  });

  server.on("/api/state", HTTP_GET, []() {
    const char* stateStr;
    switch(currentState) {
      case STATE_STANDBY: stateStr = "STANDBY"; break;
      case STATE_AMBIENT: stateStr = "AMBIENT"; break;
      case STATE_MANTRA_ACTIVE: stateStr = "MANTRA_ACTIVE"; break;
      // r127: report a wish-pad blessing as its own "WISH_ACTIVE" instead
      // of "FEET_ACTIVE" - the dashboard previously couldn't tell a real
      // feet touch apart from a wish-pad blessing, same underlying bug as
      // the OLED tag fixed in r126, just on the dashboard side instead of
      // the physical display.
      case STATE_FEET_ACTIVE: stateStr = wishPadBlessingActive ? "WISH_ACTIVE" : "FEET_ACTIVE"; break;
      case STATE_AARTI: stateStr = "AARTI_MODE"; break;
      case STATE_TEMPLE_CLOSED: stateStr = "TEMPLE_CLOSED"; break;
      default: stateStr = "STANDBY"; break;
    }

    // "blessing" makes the firmware the single source of truth for what
    // text is currently showing - the dashboard displays this verbatim
    // instead of running its own independent rotation, so the physical
    // OLED and the dashboard always show the exact same line at the exact
    // same time (previously each picked randomly on its own timer).
    // "elapsed"/"duration" let the dashboard's song timer mirror THIS
    // device's actual playback clock instead of running its own local
    // wall-clock timer - the local timer kept counting through offering
    // interruptions and past the real end of tracks (both reported on
    // hardware). stateTimer/stateDuration reset on every state entry,
    // including the offering's 12s display and the resume afterward, so
    // the dashboard timer restarts exactly when the device's does.
    unsigned long stateElapsed = millis() - stateTimer;
    if (stateElapsed > stateDuration) stateElapsed = stateDuration;

    char json[850];
    int n = snprintf(json, sizeof(json),
      "{\"firmware\":\"%s\",\"state\":\"%s\",\"blessings\":%d,\"brightness\":%d,\"pattern\":%d,\"volume\":%d,\"pirEnabled\":%s,\"displayEnabled\":%s,\"ledEnabled\":%s,\"touchFeetEnabled\":%s,\"touchBackEnabled\":%s,\"wishPadEnabled\":%s,\"lang\":%d,\"theme\":%d,\"atmosphere\":%d,\"track\":%d,\"elapsed\":%lu,\"duration\":%lu,\"offlineFallbackCount\":%lu,\"blessing\":\"",
      FIRMWARE_VERSION, stateStr, blessingCounter, currentBrightness, currentPattern, currentVolume,
      pirEnabled ? "true" : "false", displayEnabled ? "true" : "false", ledEnabled ? "true" : "false",
      touchFeetEnabled ? "true" : "false", touchBackEnabled ? "true" : "false", wishPadEnabled ? "true" : "false",
      selectedLang, selectedTheme, selectedAtmosphere, currentPlayingTrack,
      stateElapsed, stateDuration, offlineFallbackCount);

    // Minimal JSON string escaping - none of today's ENGLISH blessing/
    // welcome text needs it, but a future text edit could introduce a
    // quote/backslash and silently break the dashboard's JSON.parse()
    // without this. Bound reserves room for a full 2-byte escape
    // sequence PLUS the closing "} written after the loop.
    //
    // UTF-8 AWARE on purpose - confirmed on hardware as the cause of a
    // Marathi (Devanagari) offering showing correctly on the physical
    // OLED (which reads scrollText directly) but not on the dashboard's
    // mirrored text: the old version copied byte-by-byte and could stop
    // mid-character right at the buffer boundary, since Devanagari is
    // 3 bytes/character in UTF-8 while this loop only ever thought in
    // single bytes. A truncated multi-byte sequence left invalid UTF-8
    // in the JSON string, which the dashboard's response.json() then
    // failed to render correctly. Now a whole character is always
    // copied or none of it is - never split across the boundary.
    for (const char* s = scrollText; *s; ) {
      unsigned char b = (unsigned char)*s;
      int charLen = 1;
      if ((b & 0xE0) == 0xC0) charLen = 2;      // 2-byte UTF-8 (e.g. many European accents)
      else if ((b & 0xF0) == 0xE0) charLen = 3; // 3-byte UTF-8 (Devanagari, Tamil, etc.)
      else if ((b & 0xF8) == 0xF0) charLen = 4; // 4-byte UTF-8 (emoji)

      // Worst case every byte of this character needs a backslash escape
      // (never actually true for multi-byte bytes, which are always
      // >= 0x80 and can't match '"'/'\\', but cheap to be conservative).
      if (n + (charLen * 2) > (int)sizeof(json) - 6) break; // whole character wouldn't fit - stop here, don't split it

      for (int k = 0; k < charLen && s[k]; k++) {
        unsigned char cb = (unsigned char)s[k];
        if (cb == '"' || cb == '\\') json[n++] = '\\';
        if (cb >= 0x20) json[n++] = (char)cb;
      }
      s += charLen;
    }
    n += snprintf(json + n, sizeof(json) - n, "\"}");

    server.send(200, "application/json", json);
  });

  // V2 Phase 4: exposes ExperienceScene (see config.h/getCurrentScene())
  // for manual verification that the snapshot actually matches reality -
  // same testability convention as every other /api/* route here.
  server.on("/api/scene", HTTP_GET, []() {
    ExperienceScene scene = getCurrentScene();
    char json[200];
    snprintf(json, sizeof(json),
      "{\"state\":\"%s\",\"mood\":\"%s\",\"audioActive\":%s,\"displayLocked\":%s,\"elapsedMs\":%lu,\"durationMs\":%lu}",
      stateName(scene.state), scene.mood, scene.audioActive ? "true" : "false",
      scene.displayLocked ? "true" : "false", scene.elapsedMs, scene.durationMs);
    server.send(200, "application/json", json);
  });

  // V2 Phase 5: Guardian-style health panel data. Deliberately just
  // surfaces signals that already exist (checkWiFiHealth()'s reconnect,
  // checkHeapHealth()'s logging, checkBlessingTaskHealth()'s 120s
  // self-heal, the crash tracer) - this endpoint reports, it does not
  // add any new automatic recovery behavior. Per the reviewed V2
  // feedback: conservative recovery stays limited to Wi-Fi/API retry
  // (already exactly what checkWiFiHealth() does); everything else here
  // is for a human to read, not for the firmware to act on unprompted.
  server.on("/api/health", HTTP_GET, []() {
    unsigned long blessingTaskMs = blessingTaskActive ? (millis() - blessingTaskStartMs) : 0;
    char json[640];
    snprintf(json, sizeof(json),
      "{\"wifiConnected\":%s,\"wifiRSSI\":%d,\"freeHeap\":%lu,\"minFreeHeap\":%lu,"
      "\"uptimeSec\":%lu,\"ampReady\":%s,\"blessingTaskActive\":%s,\"blessingTaskMs\":%lu,"
      "\"offlineFallbackCount\":%lu,\"lastOfflineFallbackAgoSec\":%ld,"
      "\"bootCrashedLastRun\":%s,\"bootCrashedStage\":%d,"
      "\"dfPlayerReady\":%s,\"ledConnected\":%s,\"ledEnabled\":%s,"
      "\"pirConnected\":%s,\"pirEnabled\":%s}",
      (WiFi.status() == WL_CONNECTED) ? "true" : "false", WiFi.RSSI(),
      (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(),
      millis() / 1000, ampReady ? "true" : "false",
      blessingTaskActive ? "true" : "false", blessingTaskMs,
      offlineFallbackCount,
      lastOfflineFallbackMs == 0 ? -1L : (long)((millis() - lastOfflineFallbackMs) / 1000),
      bootCrashedLastRun ? "true" : "false", bootCrashedStage,
      dfPlayerReady ? "true" : "false",
      LED_CONNECTED ? "true" : "false", ledEnabled ? "true" : "false",
      PIR_CONNECTED ? "true" : "false", pirEnabled ? "true" : "false");
    server.send(200, "application/json", json);
  });

  server.on("/api/control", HTTP_GET, []() {
    String action = server.arg("action");
    if (action == "mantra") {
      triggerMantra();
    } else if (action == "feet") {
      triggerFeetMantra();
    } else if (action == "aarti") {
      triggerAarti();
    } else if (action == "close") {
      triggerAartiThenClose();
    } else if (action == "open") {
      openTempleFromClosed();
    } else if (action == "pir") {
      // Dashboard's "Simulate PIR Detection" button, remote-triggered -
      // mirrors exactly what a real motion detection does in checkSensors(),
      // including the stop()+delay(50) settle gap - see the matching fix
      // there for why.
      if (currentState == STATE_STANDBY) {
        dfStop();
        delay(50);
        dfPlay(BELL_TRACK);
        setSystemState(STATE_AMBIENT, AMBIENT_TIMEOUT);
      }
    } else if (action == "stop") {
      stopAudioAndStandby();
    } else if (action == "playtrack") {
      // r133: proper on-demand playback for the dashboard's "Play Track"
      // control - unlike /api/test?track=N (a raw diagnostic that just
      // calls dfPlay() with no state change at all), this actually shows
      // something coherent on the OLED/LED and updates currentPlayingTrack
      // so the dashboard's own Now Playing panel reflects it too.
      playTrackManually(server.arg("track").toInt());
    } else if (action == "offering") {
      // Fired when the priest approves a devotee's submission from the
      // Priest Queue (see approveQueueItem() in web_dashboard.h) - shows
      // their actual name/offering/prayer on the physical OLED, the same
      // way puja.html's submission ends up in the queue in the first place.
      String name = server.arg("name");
      String offeringType = server.arg("offering");
      String prayer = server.arg("prayer");
      String lang = server.hasArg("lang") ? server.arg("lang") : "en";
      // Set only when the phone's own generateBlessing call already
      // determined one (see X-Blessing-Mood in requestAiBlessing(),
      // puja_page.h) - empty when it hasn't (older queue items, or the
      // priest approved before that call finished). triggerPersonalizedOffering()
      // falls back to determining its own mood live in that case.
      String mood = server.hasArg("mood") ? server.arg("mood") : "";
      triggerPersonalizedOffering(name, offeringType, prayer, lang, mood);
    }
    server.send(200, "text/plain", "OK");
  });

  // One-off diagnostic route for finding the real playMp3Folder() track
  // numbers on this SD card without re-flashing between guesses. Not part
  // of normal operation - safe to leave in, it does nothing unless called.
  //   /api/test?track=N     -> stop current playback, play mp3-folder track N
  //   /api/test?filecount=1 -> total file count DFPlayer sees on the SD card
  // Live pin health, readable from any phone/laptop on the network - no
  // serial cable needed. Reports each sensor pin's level RIGHT NOW plus
  // how many accepted touches since boot. The intermittent-wire test:
  // hold a finger on a pad and refresh - its level must show 1. Refresh
  // again after letting go - back to 0, and count went up by 1.
  server.on("/api/pins", HTTP_GET, []() {
    char json[300];
    snprintf(json, sizeof(json),
      "{\"firmware\":\"%s\",\"state\":\"%s\",\"uptime_s\":%lu,"
      "\"feet\":{\"gpio\":%d,\"enabled\":%s,\"level\":%d,\"touches\":%lu},"
      "\"back\":{\"gpio\":%d,\"enabled\":%s,\"level\":%d,\"touches\":%lu},"
      "\"pir\":{\"gpio\":%d,\"enabled\":%s,\"level\":%d}}",
      FIRMWARE_VERSION, stateName(currentState), (unsigned long)(millis() / 1000),
      TOUCH_FEET_PIN, TOUCH_FEET_CONNECTED ? "true" : "false",
      digitalRead(TOUCH_FEET_PIN), (unsigned long)feetTouchCount,
      TOUCH_BACK_PIN, TOUCH_BACK_CONNECTED ? "true" : "false",
      digitalRead(TOUCH_BACK_PIN), (unsigned long)backTouchCount,
      PIR_PIN, PIR_CONNECTED ? "true" : "false", digitalRead(PIR_PIN));
    server.send(200, "application/json", json);
  });

  server.on("/api/test", HTTP_GET, []() {
    char msg[64];
    if (server.hasArg("oled")) {
      if (server.arg("oled").toInt() != 0) {
        oledTestUntil = millis() + 10000;
        Serial.println("OLED TEST: solid-white card for 10s (via /api/test?oled=1)");
        server.send(200, "text/plain",
          "OLED test card ON for 10 seconds - the screen should be SOLID WHITE "
          "with 'DISPLAY WORKS' in black. Still dark = panel/wiring, not firmware.");
      } else {
        oledTestUntil = 1; // in the past -> cleanup on next drawOLED pass
        server.send(200, "text/plain", "OLED test card off.");
      }
      return;
    }
    if (server.hasArg("mic")) {
      if (micTestRunning) {
        server.send(200, "text/plain", "Mic test already running - check Serial Monitor.");
        return;
      }
      micTestRunning = true;
      BaseType_t created = xTaskCreatePinnedToCore(micTestTaskFn, "micTest", 4096, NULL, 1, NULL, 1);
      if (created != pdPASS) {
        micTestRunning = false;
        server.send(500, "text/plain", "Failed to start mic test task.");
        return;
      }
      server.send(200, "text/plain",
        "Mic test started - open the Serial Monitor now, watch for ~4 seconds, "
        "and clap or speak near the mic. A MIC LEVEL bar that moves means it "
        "works; flat at 0 the whole time means check the wiring.");
      return;
    }
    if (server.hasArg("micplay")) {
      if (micTestRunning || blessingTaskActive) {
        server.send(200, "text/plain", "A mic or amp test is already running - wait a few seconds and try again.");
        return;
      }
      if (!ampReady) {
        server.send(200, "text/plain", "Amp not ready - can't play back a recording.");
        return;
      }
      micTestRunning = true;
      blessingTaskActive = true;
      blessingTaskStartMs = millis();
      BaseType_t created = xTaskCreatePinnedToCore(micRecordPlaybackTaskFn, "micPlayback", 8192, NULL, 1, NULL, 1);
      if (created != pdPASS) {
        micTestRunning = false;
        blessingTaskActive = false;
        server.send(500, "text/plain", "Failed to start mic playback task.");
        return;
      }
      server.send(200, "text/plain",
        "Recording 3 seconds now - speak or clap near the mic. It will play "
        "back through the amp speaker a few seconds after this page loads.");
      return;
    }
    if (server.hasArg("track")) {
      int n = server.arg("track").toInt();
      dfStop();
      delay(50);
      dfPlay(n);
      snprintf(msg, sizeof(msg), "Playing mp3 folder track %d", n);
      Serial.println(msg);
      server.send(200, "text/plain", msg);
    } else if (server.hasArg("filecount")) {
      int count = dfReadFileCounts();
      snprintf(msg, sizeof(msg), "Total files on SD card: %d", count);
      Serial.println(msg);
      server.send(200, "text/plain", msg);
    } else if (server.hasArg("offline")) {
      // Manually previews the offline blessing fallback (Phase 1) without
      // needing to actually kill Wi-Fi or the backend - picks a random
      // OFFLINE_BLESSING_TRACKS entry, mood and OLED phrase exactly like a
      // real failure would.
      playOfflineBlessingFallback("en");
      snprintf(msg, sizeof(msg), "Offline fallback preview triggered (count=%lu)", offlineFallbackCount);
      Serial.println(msg);
      server.send(200, "text/plain", msg);
    } else {
      server.send(400, "text/plain", "Usage: /api/test?track=N  or  /api/test?filecount=1  or  /api/test?mic=1  or  /api/test?micplay=1  or  /api/test?oled=1  or  /api/test?offline=1");
    }
  });

  server.on("/api/leds", HTTP_GET, []() {
    if (server.hasArg("brightness")) {
      currentBrightness = server.arg("brightness").toInt();
      if (LED_CONNECTED) FastLED.setBrightness(currentBrightness);
    }
    if (server.hasArg("pattern")) {
      currentPattern = server.arg("pattern").toInt();
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/api/audio", HTTP_GET, []() {
    if (server.hasArg("volume")) {
      currentVolume = server.arg("volume").toInt();
      dfSetVolume(currentVolume);
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/api/settings", HTTP_GET, []() {
    if (server.hasArg("pir")) {
      pirEnabled = (server.arg("pir").toInt() == 1);
    }
    if (server.hasArg("reset")) {
      blessingCounter = 0;
    }
    if (server.hasArg("lang")) {
      selectedLang = server.arg("lang").toInt();
    }
    if (server.hasArg("theme")) {
      selectedTheme = server.arg("theme").toInt();
    }
    if (server.hasArg("atmosphere")) {
      selectedAtmosphere = server.arg("atmosphere").toInt();
    }
    // Logged unconditionally on every toggle below (not just failures) -
    // reported on hardware that a dashboard toggle can show OFF in the
    // browser while the device keeps behaving as if it's still ON, with
    // no way to tell from the UI alone whether the request ever reached
    // the ESP32 at all. These lines make that directly checkable on the
    // Serial Monitor instead of guessing between a JS-side and a
    // firmware-side cause.
    if (server.hasArg("display")) {
      displayEnabled = (server.arg("display").toInt() == 1);
      Serial.printf("SETTINGS: display -> %s\n", displayEnabled ? "true" : "false");
      if (!displayEnabled) {
        // Blank it once immediately, rather than waiting for drawOLED()
        // to notice - otherwise whatever was on screen stays frozen
        // there until the next state change happens to redraw it.
        u8g2.clearBuffer();
        u8g2.sendBuffer();
      }
    }
    if (server.hasArg("led")) {
      ledEnabled = (server.arg("led").toInt() == 1);
      Serial.printf("SETTINGS: led -> %s\n", ledEnabled ? "true" : "false");
      if (!ledEnabled && LED_CONNECTED) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
      }
    }
    if (server.hasArg("touchFeet")) {
      touchFeetEnabled = (server.arg("touchFeet").toInt() == 1);
      Serial.printf("SETTINGS: touchFeet -> %s\n", touchFeetEnabled ? "true" : "false");
    }
    if (server.hasArg("touchBack")) {
      touchBackEnabled = (server.arg("touchBack").toInt() == 1);
      Serial.printf("SETTINGS: touchBack -> %s\n", touchBackEnabled ? "true" : "false");
    }
    if (server.hasArg("wishPad")) {
      wishPadEnabled = (server.arg("wishPad").toInt() == 1);
      Serial.printf("SETTINGS: wishPad -> %s\n", wishPadEnabled ? "true" : "false");
    }
    server.send(200, "text/plain", "OK");
  });
}

// ==========================================
// Wi-Fi Health Check
// ==========================================
// Runs at most once every WIFI_CHECK_INTERVAL_MS - checking WiFi.status()
// every single loop() pass would be wasteful and isn't needed; a dropped
// connection sitting undetected for up to 15s is a non-issue against the
// alternative (permanently undetected until manual power-cycle). Only
// acts if this device connected in station mode at boot - the AP
// fallback path is left alone on purpose, see wifiStationMode's comment.
void checkWiFiHealth() {
  if (!wifiStationMode) return;
  unsigned long now = millis();
  if (now - lastWifiCheck < WIFI_CHECK_INTERVAL_MS) return;
  lastWifiCheck = now;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI: connection lost - attempting reconnect...");
    WiFi.reconnect();
  }
}

// Logs free heap once a minute - see lastHeapLog's comment. A healthy,
// stable value across days is reassuring; a steady downward trend is
// exactly the early warning a String-fragmentation problem gives before
// it becomes a crash with no diagnostic trail.
void checkHeapHealth() {
  unsigned long now = millis();
  if (now - lastHeapLog < HEAP_LOG_INTERVAL_MS) return;
  lastHeapLog = now;
  Serial.printf("HEAP: %lu bytes free, %lu min-ever-free (uptime %lus)\n",
                (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getMinFreeHeap(), now / 1000);
}

// Safety net for blessingTaskActive getting stuck true forever - see its
// declaration/comment near ampReady above. 120s is deliberately generous:
// legitimate calls can legitimately take a while (image fetch + Claude +
// Google TTS + reverb DSP + streaming a slow-paced ~500-char blessing's
// audio can genuinely add up to a large chunk of that), and clearing the
// flag while the task is STILL actually running and about to write to
// ampI2S risks exactly the concurrent-write collision blessingTaskActive
// exists to prevent in the first place - so this only ever fires well
// after any real call would have finished, not as a normal-case timeout.
// The task itself, if it later does return from whatever hung, just sets
// blessingTaskActive = false again redundantly - harmless.
#define BLESSING_TASK_STUCK_MS 120000UL
void checkBlessingTaskHealth() {
  if (!blessingTaskActive) return;
  if (millis() - blessingTaskStartMs < BLESSING_TASK_STUCK_MS) return;
  Serial.println("AMP: blessing task has been active for 120s+ without finishing - "
                  "assuming it's hung (likely a stalled network call) and force-clearing "
                  "blessingTaskActive so future offerings aren't silently skipped forever.");
  blessingTaskActive = false;
}

// ==========================================
// Sensor Reading & Debouncing
// ==========================================
void checkSensors() {
  unsigned long now = millis();

  // ---- PIR motion sensing ------------------------------------------
  // PIR_CONNECTED=false (config.h) skips the sensor completely - no reads,
  // no logs, no wake. Everything else (touch pads, dashboard, offerings,
  // rituals) works without it, so a misbehaving sensor can never block or
  // muddy testing of unrelated features.
  //
  // Detection is by DUTY CYCLE over a 250ms window, not "pin is HIGH" and
  // not "HIGH sustained for 100ms". Real hardware showed this pin toggling
  // ~55 times/second - mains-hum frequency on an undriven wire, and
  // impossible for an AM312, whose output holds HIGH for ~2 SECONDS per
  // detection. Any single digitalRead() of such a line returns HIGH about
  // half the time, which is exactly why a log line could say "HIGH" while
  // nothing ever legitimately triggered. Duty cycle separates the cases:
  //   ~40-60%  -> line floating/noisy, sensor is NOT driving it (wiring)
  //   >=90%    -> line genuinely held HIGH = real motion
  // The window is ~25 samples (loop runs every ~10ms) and far shorter than
  // the AM312's ~2s pulse, so a true detection always fills a whole window.
  //
  // Logging is rate-limited to one line per 2s. An earlier per-edge print
  // was itself a serious bug: on a noisy line it emitted ~30 lines/second,
  // and once the serial TX buffer saturates Serial.printf BLOCKS, stalling
  // the main loop - which also drives the OLED scroll, the state machine,
  // and the web server. That one flood made the display sluggish and the
  // dashboard buttons unresponsive.
  static unsigned int pirSamples = 0;
  static unsigned int pirHighSamples = 0;
  static unsigned long pirWindowStart = 0;
  static unsigned long lastPirReport = 0;
  static int pirDutyPct = 0;
  static bool pirMotionLive = false;

  if (PIR_CONNECTED) {
    pirSamples++;
    if (digitalRead(PIR_PIN) == HIGH) pirHighSamples++;

    if (now - pirWindowStart >= 250) {
      pirDutyPct = pirSamples ? (int)((pirHighSamples * 100UL) / pirSamples) : 0;
      pirMotionLive = (pirDutyPct >= 90);
      pirSamples = 0;
      pirHighSamples = 0;
      pirWindowStart = now;

      if (pirDutyPct > 5 && now - lastPirReport > 2000) {
        lastPirReport = now;
        const char* verdict = pirMotionLive
            ? "MOTION (line driven HIGH)"
            : "NOISE - sensor not driving the pin, check OUT wire/pin";
        Serial.printf("PIR: duty %d%% over 250ms -> %s  (state=%s, pirEnabled=%s)\n",
                      pirDutyPct, verdict, stateName(currentState), pirEnabled ? "true" : "false");
      }
    }
  } else {
    pirMotionLive = false;
  }

  bool pirConfirmed = pirMotionLive;

  // STANDBY only: PIR wakes the temple, but must NOT also reset AMBIENT's
  // idle-to-Aarti timer. Tried that (reading PIR during AMBIENT too, to let
  // continued presence delay the close) and confirmed by simulation that it
  // backfires - almost any real PIR sensor picks up occasional ambient
  // motion/heat/light changes over a few minutes, which kept resetting the
  // 60s countdown and left AMBIENT stuck forever, never reaching Aarti at
  // all. Only an actual touch (feet/mouse-back) counts as "someone's here."
  // Also gated on pendingWakeMantra == 0 (r32's bell-first-wake): a touch
  // sets pendingWakeMantra and leaves currentState at STANDBY for the
  // ~2.5s WAKE_BELL_LEAD_MS window until the promised mantra actually
  // starts. Real PIR motion landing in that window was setting
  // motionDetected here, which the STANDBY case then acted on BEFORE the
  // pending mantra fired - a second bell plus setSystemState(AMBIENT),
  // which the pending mantra then fired on top of regardless (it runs
  // unconditional on state, by design) - bell rings twice, state left
  // confused between AMBIENT and whichever mantra actually started.
  if (pirEnabled && currentState == STATE_STANDBY && pendingWakeMantra == 0) {
    if (pirConfirmed && (now - lastMotionTrigger > MOTION_DEBOUNCE)) {
      motionDetected = true;
      lastMotionTrigger = now;
    }
  }

  // Touch pads: EDGE-triggered (fires once on untouched -> touched), not
  // level-triggered. Reading "is the pin HIGH?" meant a pad that stayed
  // HIGH - a devotee resting a hand on it, a TP223 in self-locking mode,
  // or a marginal solder joint holding the line up - re-fired on every
  // TOUCH_DEBOUNCE expiry, restarting the mantra from the top every 2
  // seconds so it never actually played. Confirmed on hardware: an
  // identical TOUCH line every 2.05s, exactly the debounce interval.
  //
  // Each pad is also settled independently: a reading must hold steady
  // for TOUCH_SETTLE_MS before it counts as the new level, which rejects
  // the short bounces a rough solder joint produces. TOUCH_DEBOUNCE then
  // rate-limits genuine repeat taps.
  bool feetRead = TOUCH_FEET_CONNECTED && touchFeetEnabled && (digitalRead(TOUCH_FEET_PIN) == HIGH);
  bool backRead = TOUCH_BACK_CONNECTED && touchBackEnabled && (digitalRead(TOUCH_BACK_PIN) == HIGH);
  // Both advanced every pass, before any branching - putting these calls
  // inside the if/else-if would let short-circuit evaluation skip one
  // pad's debounce on any pass the other pad reported an edge.
  bool feetEdge = settleTouch(feetRead, feetLastRead, feetChangedAt, feetStable, now);
  bool backEdge = settleTouch(backRead, backLastRead, backChangedAt, backStable, now);
  // Both gated on !blessingTaskActive - confirmed on hardware as a real
  // overlap, not hypothetical: a devotee's spoken blessing (wish pad or
  // an approved offering) plays through the I2S amp on its own speaker,
  // completely independent of the DFPlayer mantra speaker, so nothing
  // previously stopped a FRESH feet/back touch from immediately starting
  // a brand-new mantra right on top of a still-playing blessing. (This is
  // different from the existing blessingTaskActive check elsewhere, which
  // only delays RESUMING a mantra that was already interrupted - it never
  // covered a new touch arriving mid-blessing.) Simply dropping the touch
  // here, rather than queuing it, matches the same "skip this one" choice
  // speakBlessingOnAmpAsync() already makes for a colliding AMP request -
  // the devotee can just touch again once the blessing finishes.
  if (feetEdge && feetStable) {
    if (blessingTaskActive) {
      Serial.println("TOUCH: feet pad pressed but a blessing is speaking - ignoring so it doesn't play over it");
    } else if (now - lastTouchTrigger > TOUCH_DEBOUNCE) {
      feetTouched = true;
      lastTouchTrigger = now;
      // A TOUCH line with nobody touching means that pad's lead is not
      // properly landed on its pin - set its _CONNECTED flag back to false
      // rather than working around it.
      Serial.printf("TOUCH: feet pad pressed (GPIO%d) while %s\n", TOUCH_FEET_PIN, stateName(currentState));
      feetTouchCount++;
    }
  } else if (backEdge && backStable) {
    if (blessingTaskActive) {
      Serial.println("TOUCH: mouse-back pad pressed but a blessing is speaking - ignoring so it doesn't play over it");
    } else if (now - lastTouchTrigger > TOUCH_DEBOUNCE) {
      backTouched = true;
      lastTouchTrigger = now;
      Serial.printf("TOUCH: mouse-back pad pressed (GPIO%d) while %s\n", TOUCH_BACK_PIN, stateName(currentState));
      backTouchCount++;
    }
  }

  // Wish pad: deliberately self-contained, NOT wired into feetTouched/
  // backTouched or the wake/resume state-machine logic those feed - a
  // silent prayer touch shouldn't need to understand mantra playback
  // state at all. Own debounce state, own trigger, calls
  // triggerWishPadBlessing() directly (see below).
  //
  // EVENT-TRIGGERED DIAGNOSTIC (not periodic - only prints when GPIO4
  // actually changes level, so it won't flood the monitor the way the
  // old once-a-second WISH PAD RAW did): with the dashboard's Wish Pad
  // toggle confirmed on and the module's own LED confirmed lighting on a
  // real touch, yet nothing at all reaching the firmware, this is the
  // only way left to see whether the pin itself is really moving.
  bool wishPadRawNow = (digitalRead(WISH_PAD_PIN) == HIGH);
  bool wishPadRawEdge = settleTouch(wishPadRawNow, wishPadRawLastRead, wishPadRawChangedAt, wishPadRawStable, now);
  if (wishPadRawEdge) {
    Serial.printf("WISH PAD RAW EDGE: GPIO%d now %s  WISH_PAD_CONNECTED=%s  wishPadEnabled=%s\n",
                  WISH_PAD_PIN, wishPadRawStable ? "HIGH" : "LOW",
                  WISH_PAD_CONNECTED ? "true" : "false", wishPadEnabled ? "true" : "false");
  }

  bool wishPadRead = WISH_PAD_CONNECTED && wishPadEnabled && (digitalRead(WISH_PAD_PIN) == HIGH);
  bool wishPadEdge = settleTouch(wishPadRead, wishPadLastRead, wishPadChangedAt, wishPadStable, now);
  if (wishPadEdge && wishPadStable) {
    // Same !blessingTaskActive gate as the feet/back touches above -
    // triggerWishPadBlessing() itself unconditionally restarts the bell
    // and display even though speakGenericBlessingOnAmpAsync() already
    // refuses to double up the actual spoken audio, so a repeat touch
    // mid-blessing would still audibly re-ring the bell over the
    // still-playing blessing without this.
    if (blessingTaskActive) {
      Serial.println("TOUCH: wish pad pressed but a blessing is speaking - ignoring so it doesn't play over it");
    } else if (now - lastWishPadTrigger > TOUCH_DEBOUNCE) {
      lastWishPadTrigger = now;
      Serial.printf("TOUCH: wish pad pressed (GPIO%d) while %s\n", WISH_PAD_PIN, stateName(currentState));
      triggerWishPadBlessing();
    }
  }

  // A pad stuck HIGH for a long stretch is almost always a wiring/solder
  // fault (or a TP223 left in self-lock mode). Edge triggering means it no
  // longer breaks playback, but it does mean that pad is dead until it
  // releases - so say so, rarely, instead of failing silently.
  static unsigned long lastStuckWarn = 0;
  if ((feetStable || backStable) && now - lastTouchTrigger > 30000 && now - lastStuckWarn > 30000) {
    lastStuckWarn = now;
    Serial.printf("TOUCH: %s%s held HIGH for 30s+ - likely a solder/wiring fault or a TP223 in self-lock mode; that pad won't trigger again until it releases\n",
                  feetStable ? "feet " : "", backStable ? "mouse-back" : "");
  }
}

// ==========================================
// State Machine Transitions
// ==========================================
void updateStateMachine() {
  unsigned long now = millis();

  // If 12 seconds have passed since a Feet or Mouse Back touch, unlock and
  // let the blessing rotation immediately show a fresh one (rather than
  // falling back to the fixed welcome sentence). feetDisplayLocked/Timer
  // are shared by both touches - only one is ever active at a time.
  if (feetDisplayLocked && (now - feetDisplayTimer > feetDisplayLockMs)) {
    feetDisplayLocked = false;
    feetDisplayLockMs = 12000; // back to the default for the next touch
    lastAmbientBlessingRotate = 0;
    scrollPassComplete = true; // force the rotation below to fire immediately
    Serial.println("OLED: display lock elapsed, resumed blessings roll.");
  }

  // While Ambient or a Mouse Back mantra is playing (nobody needs a
  // locked personalized blessing right now), rotate through the same
  // 48-message list the web dashboard uses, instead of one fixed
  // scrolling sentence. Feet Touch's own 12s-locked blessing takes
  // priority over this while feetDisplayLocked is set.
  //
  // Gated on scrollPassComplete (set by drawOLED() once the CURRENT text
  // has actually finished scrolling all the way across), not a fixed
  // timer - see the MIN_BLESSING_DISPLAY_MS comment for why a fixed
  // timer broke once the font got bigger. MIN_BLESSING_DISPLAY_MS is
  // just a floor against flickering on a very short string.
  if ((currentState == STATE_AMBIENT || currentState == STATE_MANTRA_ACTIVE || currentState == STATE_FEET_ACTIVE) && !feetDisplayLocked &&
      scrollPassComplete && (now - lastAmbientBlessingRotate > MIN_BLESSING_DISPLAY_MS)) {
    lastAmbientBlessingRotate = now;
    scrollPassComplete = false;
    if (ambientBlessingIdx % 2 == 0) {
      int r = random(0, 26);
      snprintf(scrollText, sizeof(scrollText), "   [BLESSING] %s   ", oledChildBlessingsList[r]);
      strlcpy(scrollTextLang, "en", sizeof(scrollTextLang));
    } else {
      int r = random(0, 22);
      snprintf(scrollText, sizeof(scrollText), "   [BLESSING] %s   ", oledAdultBlessingsList[r]);
      strlcpy(scrollTextLang, "en", sizeof(scrollTextLang));
    }
    scrollX = 128;
    ambientBlessingIdx++;
  }

  // A wake-touch's bell has finished sounding - start the mantra it
  // promised. Runs before the switch so it fires regardless of what
  // state the bell interval ended in.
  if (pendingWakeMantra != 0 && now >= pendingWakeAt) {
    int pad = pendingWakeMantra;
    pendingWakeMantra = 0;
    // Previously silent - the WAKE: line at arm-time was the only trace
    // of this mechanism in the log, so there was no way to tell from
    // Serial alone whether the bell fired but the promised mantra never
    // did. This closes that gap.
    Serial.printf("WAKE: bell finished, starting %s mantra now (state was %s)\n",
                  pad == 2 ? "mouse-back" : "feet", stateName(currentState));
    if (pad == 2) triggerMantra();
    else          triggerFeetMantra();
    // ROOT CAUSE of "bell rings, mantra never audibly starts": triggerMantra()/
    // triggerFeetMantra() contain delay(50) and then call setSystemState(),
    // which sets stateTimer via its OWN fresh millis() call - ending up
    // slightly AHEAD of the `now` snapshot taken at the top of this
    // function, before that delay ran. Execution falls straight through
    // into the switch() below on the SAME stale `now`, and the new
    // state's very own "has the duration elapsed?" check computes
    // now - stateTimer with now < stateTimer - unsigned subtraction, so
    // instead of going negative it wraps to just under 2^32 (confirmed on
    // hardware: 4294967245 = 4294967295 - 50, the exact 50ms of that
    // delay). That's far past any real duration, so the freshly-created
    // mantra state is destroyed on the very same pass it was created.
    // Refreshing `now` here closes the gap the stale snapshot left open.
    now = millis();
  }

  switch (currentState) {

    case STATE_STANDBY:
      if (backTouched || feetTouched) {
        // Waking touch: bell now, mantra when the bell has rung
        // (WAKE_BELL_LEAD_MS). A second touch during the bell is ignored
        // rather than queued twice.
        if (pendingWakeMantra == 0) {
          pendingWakeMantra = backTouched ? 2 : 1;
          pendingWakeAt = now + WAKE_BELL_LEAD_MS;
          dfPlay(BELL_TRACK);
          Serial.printf("WAKE: bell first (%s pad), mantra in %dms\n",
                        backTouched ? "mouse-back" : "feet", WAKE_BELL_LEAD_MS);
        }
        backTouched = feetTouched = false;
      } else if (motionDetected) {
        motionDetected = false;
        // Every OTHER trigger in this file (both touch pads, Aarti,
        // offerings) calls stop()+delay(50) before playMp3Folder() -
        // some DFPlayer Mini clones drop a command sent without that
        // settle gap. This was the one exception. Reported on hardware:
        // PIR correctly woke the temple (state, display, dashboard all
        // moved to AMBIENT) but the bell itself never sounded - matches
        // exactly what a dropped play command looks like.
        // Unlike the touch-wake branch above, this had NO log line at
        // all - reported again as "PIR wakes it but no bell" with no way
        // to tell from Serial whether this branch even runs.
        Serial.println("WAKE: PIR motion detected - playing bell, moving to AMBIENT");
        dfStop();
        delay(50);
        dfPlay(BELL_TRACK);
        setSystemState(STATE_AMBIENT, AMBIENT_TIMEOUT);
      }
      break;

    case STATE_AMBIENT:
      // motionDetected is never set true here (checkSensors() only reads
      // PIR during STANDBY) - only an actual touch resets the idle timer,
      // deliberately, so a sensitive PIR can't block the temple from ever
      // closing for the night.
      if (backTouched) {
        backTouched = false;
        triggerMantra();
        now = millis(); // see the matching comment on the pending-wake
                         // mechanism above - same stale-now-after-a-
                         // delay()-inside-trigger hazard applies here too,
                         // and this exact path was NOT the one caught on
                         // hardware yet: a touch from AMBIENT would fall
                         // through to the stateDuration check below on a
                         // stale `now`, underflow, and spuriously trigger
                         // the idle-timeout branch below the instant the
                         // mantra it just started was created.
      } else if (feetTouched) {
        feetTouched = false;
        triggerFeetMantra();
        now = millis(); // see above
      }

      if (now - stateTimer > stateDuration) {
        // r128: used to auto-close the temple with a full ~4-minute Aarti
        // right here - reported directly as "too much": this device has
        // no clock/NTP, so PIR waking AMBIENT at ANY time of day (a
        // daytime passer-by, not just at actual night) triggered the same
        // full closing ritual every single time nobody touched anything
        // within AMBIENT_TIMEOUT. Closing the temple is manual-only now -
        // the dashboard's "Close Temple" button and ?action=close still
        // call triggerAartiThenClose() directly, unaffected - AMBIENT
        // idling out on its own just goes back to sleep like any other
        // idle timeout, ready for the next PIR trigger.
        setSystemState(STATE_STANDBY);
      }
      break;

    case STATE_MANTRA_ACTIVE:
      if (backTouched) {
        backTouched = false;
        triggerMantra();
        return;
      } else if (feetTouched) {
        feetTouched = false;
        triggerFeetMantra();
        return;
      }

      // One whole mantra done, nobody touched again - go back to sleep.
      // Only PIR (from STANDBY) or a fresh touch wakes things up again;
      // AMBIENT is no longer the automatic landing state after a mantra.
      if (now - stateTimer > stateDuration) {
        Serial.printf("MANTRA: duration elapsed (%lums of %lums) - back to standby\n",
                      now - stateTimer, stateDuration);
        dfStop();
        setSystemState(STATE_STANDBY);
      }
      break;

    case STATE_FEET_ACTIVE:
      if (backTouched) {
        backTouched = false;
        triggerMantra();
        return;
      } else if (feetTouched) {
        feetTouched = false;
        triggerFeetMantra();
        return;
      }

      // An offering's display ends only once the message has ACTUALLY
      // finished one full scroll pass (scrollPassComplete, set by
      // drawOLED) - not when a pre-computed duration expires. The
      // computed estimate was consistently short because drawOLED's 33ms
      // gate combined with loop()'s delay(10) makes real frames land
      // every ~40ms, so the text scrolls ~20% slower than any 33ms-based
      // calculation predicts - which clipped the last word or two off
      // every offering. Waiting on the real scroll state is immune to
      // that drift entirely. stateDuration still sets the 12s minimum,
      // and the hard cap stops a pathologically long message (or a
      // stalled scroll) from holding the display forever.
      {
        bool offeringMinElapsed = (now - stateTimer > stateDuration);
        bool offeringHardCap    = (now - stateTimer > stateDuration + 20000);
        // Also wait for the amp's background task to finish speaking
        // (blessingTaskActive) before resuming the interrupted mantra on
        // the DFPlayer - the display's own timer/scroll can easily finish
        // before the amp's network round trip does, and resuming the
        // mantra early plays it right on top of the still-speaking amp.
        // offeringHardCap overrides this too, so a stuck amp task can
        // never hold the display hostage forever.
        bool offeringDone = offeringMinElapsed &&
                             ((scrollPassComplete && !blessingTaskActive) || offeringHardCap);
        bool plainDone    = (now - stateTimer > stateDuration);

      if (offeringDisplayActive ? offeringDone : plainDone) {
        if (offeringDisplayActive) {
          // Offering display finished. If it interrupted an actual mantra
          // mid-play, resume it by replaying the SAME track
          // (currentPlayingTrack, untouched by the BELL_TRACK the
          // offering played) - see the note in
          // triggerPersonalizedOffering() for why this restarts the track
          // instead of using DFPlayer's pause()/start() (unreliable on
          // many DFPlayer Mini clones).
          offeringDisplayActive = false;
          currentMoodTag[0] = '\0'; // don't let this blessing's mood color bleed into the resumed mantra
          freeBlessingImage(); // same reasoning - don't let a stale image show through the resumed mantra
          // Release the display lock explicitly so the resumed mantra's
          // blessing rotation starts again immediately, and reset the
          // lock window for the next ordinary touch.
          feetDisplayLocked = false;
          feetDisplayLockMs = 12000;
          scrollPassComplete = true;
          Serial.printf("OFFERING: display done after %lums (cap hit: %s), offeringInterrupted=%s, resumeTrack=%d, resumeState=%s\n",
                        now - stateTimer, offeringHardCap ? "yes" : "no",
                        offeringInterrupted ? "true" : "false", currentPlayingTrack, stateName(offeringPausedState));
          if (offeringInterrupted) {
            offeringInterrupted = false;
            dfStop();
            delay(100); // was 50ms - some DFPlayer Mini clones drop a command sent too soon after stop()
            dfPlay(currentPlayingTrack);
            // Resuming into Aarti specifically: restore its fixed display
            // text, since it was overwritten by the offering's "[OFFERING]
            // ..." line and (unlike Ambient/Mantra/Feet) Aarti doesn't
            // have a rotating blessing loop to naturally replace it.
            if (offeringPausedState == STATE_AARTI) {
              strlcpy(scrollText, "   \xE2\x9C\xA8 A moment of Aarti \xE2\x9C\xA8   ", sizeof(scrollText));
      strlcpy(scrollTextLang, "en", sizeof(scrollTextLang));
            }
            setSystemState(offeringPausedState, offeringPausedDurationMs);
          } else {
            dfStop();
            setSystemState(STATE_STANDBY);
          }
        } else {
          // One whole mantra done, nobody touched again - go back to sleep.
          Serial.printf("MANTRA: duration elapsed (%lums of %lums) - back to standby\n",
                        now - stateTimer, stateDuration);
          dfStop();
          setSystemState(STATE_STANDBY);
        }
      }
      }
      break;

    case STATE_AARTI:
      if (backTouched) {
        backTouched = false;
        aartiThenClose = false;
        triggerMantra();
        return;
      } else if (feetTouched) {
        feetTouched = false;
        aartiThenClose = false;
        triggerFeetMantra();
        return;
      }

      // r137: switch to part 2 (AARTI_PART2_TRACK) the instant part 1's
      // own real length elapses - immediately back-to-back, no gap, so it
      // reads as one continuous Aarti block rather than two separate
      // chants. stateDuration itself is unchanged (AARTI_DURATION, the
      // combined total) - only which physical track is sounding changes.
      if (!aartiPart2Playing && (now - stateTimer >= AARTI_PART1_DURATION)) {
        aartiPart2Playing = true;
        currentPlayingTrack = AARTI_PART2_TRACK;
        dfPlay(AARTI_PART2_TRACK);
      }

      if (now - stateTimer > stateDuration) {
        dfStop();
        if (aartiThenClose) {
          aartiThenClose = false;
          setSystemState(STATE_TEMPLE_CLOSED);
        } else {
          setSystemState(STATE_AMBIENT, AMBIENT_TIMEOUT);
        }
      }
      break;

    case STATE_TEMPLE_CLOSED:
      // Deliberately only a touch wakes the closed temple - not PIR
      // (motionDetected is ignored here on purpose, matching the web
      // dashboard's TEMPLE_CLOSED design). Same bell-first wake as
      // STANDBY: this is also a waking touch.
      if (backTouched || feetTouched) {
        if (pendingWakeMantra == 0) {
          pendingWakeMantra = backTouched ? 2 : 1;
          pendingWakeAt = now + WAKE_BELL_LEAD_MS;
          dfPlay(BELL_TRACK);
          Serial.printf("WAKE: bell first (%s pad), mantra in %dms\n",
                        backTouched ? "mouse-back" : "feet", WAKE_BELL_LEAD_MS);
        }
        backTouched = feetTouched = false;
      }
      break;
  }

  motionDetected = feetTouched = backTouched = false;
}

// Fades whatever's currently showing on the ring down to black over
// ~800ms, rather than the abrupt cut to black setSystemState() used to do
// on every close - played once, synchronously, right before that cut.
// Cheap (24 CRGB = 72 bytes on the stack) and short enough not to be a
// concern under the watchdog.
void playLedClosingSweep() {
  CRGB original[NUM_LEDS];
  memcpy(original, leds, sizeof(original));
  for (int step = 20; step >= 0; step--) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = original[i];
      leds[i].nscale8((uint8_t)(step * 255 / 20));
    }
    FastLED.show();
    delay(40);
  }
}

// ==========================================
// State Handlers
// ==========================================
void setSystemState(SystemState newState, unsigned long duration) {
  SystemState oldState = currentState;
  currentState = newState;
  stateTimer = millis();
  stateDuration = duration;
  scrollX = 128;

  // r126: cleared unconditionally on every transition, same reasoning as
  // the eye LED reset just below - this is the one place that always
  // knows a transition just happened. triggerWishPadBlessing() is the
  // only place that sets this true, immediately after its own call into
  // this function, so it stays true for exactly one wish-pad blessing.
  wishPadBlessingActive = false;

  // Eye LEDs (mouse's eyes, fiber-coupled) - settle to the dim steady
  // EYE_LED_BASE_BRIGHTNESS glow on every single state change,
  // unconditionally, since this is the one place that always knows a
  // transition just happened - never fully dark, per direct feedback
  // after seeing the original off/on version on real hardware.
  // triggerMantra() is the only place that starts the brighter breathing
  // pulse, immediately after its own call into this function - every
  // other trigger (feet, offerings, wish pad, Aarti, reopening from
  // closed) just settles back to the resting glow, without needing its
  // own explicit reset. A brief (~250ms) blocking fade DOWN TO BASE, not
  // an instant cut, but ONLY when breathing was actually active - every
  // other state change (the vast majority) just confirms the PWM
  // channel is already at the resting level, no delay added.
  // r125: EYE_LED_BASE_BRIGHTNESS is now the CEILING of the breathing
  // dip (see config.h), so an interrupted chant is usually caught
  // mid-dip, below base, not above it like the old low-base scheme -
  // this fade now steps toward base in whichever direction is needed,
  // instead of only ever fading downward.
  if (EYE_LED_CONNECTED) {
    if (eyeLedBreathingActive) {
      int startB = ledcRead(EYE_LED_PIN);
      int step = (startB > EYE_LED_BASE_BRIGHTNESS) ? -15 : 15;
      for (int b = startB; (step < 0) ? (b > EYE_LED_BASE_BRIGHTNESS) : (b < EYE_LED_BASE_BRIGHTNESS); b += step) {
        ledcWrite(EYE_LED_PIN, b);
        delay(15);
      }
      eyeLedBreathingActive = false;
    }
    ledcWrite(EYE_LED_PIN, EYE_LED_BASE_BRIGHTNESS);
  }
  // Every fresh state entry starts a fresh scroll pass for whatever text
  // was just set - the blessing rotation below must not fire again until
  // THIS text has had its own chance to fully scroll across the screen.
  scrollPassComplete = false;

  // LED open/close transitions - detected here since setSystemState() is
  // the one place that always knows both the old and new state.
  bool wasClosed = (oldState == STATE_STANDBY || oldState == STATE_TEMPLE_CLOSED);
  bool nowClosed = (newState == STATE_STANDBY || newState == STATE_TEMPLE_CLOSED);
  if (wasClosed && !nowClosed && LED_CONNECTED && ledEnabled) {
    ledOpeningTransitionStart = millis(); // animateLeds() plays the bloom on its next frames
  }

  if (newState == STATE_STANDBY || newState == STATE_TEMPLE_CLOSED) {
    feetDisplayLocked = false;
    currentPlayingTrack = 0;
    if (LED_CONNECTED) {
      if (!wasClosed && ledEnabled) {
        // V2 Phase 3: a mood-driven blessing (offering/wish-pad, not a
        // plain mantra/feet touch) gets one gentle pulse of its own
        // color first - a clear completion cue - before the existing
        // generic fade takes over.
        bool wasMoodBearing = (oldState == STATE_MANTRA_ACTIVE || oldState == STATE_FEET_ACTIVE) &&
                               currentMoodTag[0] != '\0';
        if (wasMoodBearing) {
          playMoodClosurePulse(currentMoodTag);
        }
        playLedClosingSweep(); // fade out whatever was showing, rather than an abrupt cut to black
      }
      FastLED.clear();
      FastLED.show();
    }
    // Blank the panel BEFORE sleeping it. setPowerSave(1) alone leaves the
    // last-drawn image latched on some panels, so closing the temple froze
    // whatever text happened to be mid-scroll on screen (reported on
    // hardware). An explicit empty frame makes "asleep" also mean "blank"
    // on every panel, regardless of how it implements power-save.
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    u8g2.setPowerSave(1);
  } else {
    u8g2.setPowerSave(0); 
    if (newState == STATE_AMBIENT) {
      feetDisplayLocked = false;
      // Show the welcome sentence first; updateStateMachine() switches to
      // rotating blessings once this text finishes one scroll pass.
      strlcpy(scrollText, oledAmbientLoopText, sizeof(scrollText));
      strlcpy(scrollTextLang, "en", sizeof(scrollTextLang));
      lastAmbientBlessingRotate = stateTimer;
    }
  }
}

// V2 Phase 4: ExperienceScene - see the struct's own comment in config.h.
// Pure read - takes a snapshot of existing globals, changes nothing.
// currentPlayingTrack != 0 is used as a proxy for "some audio is set to
// be playing" alongside blessingTaskActive, since a plain mantra/feet/
// Aarti track plays without ever setting blessingTaskActive (that flag
// only covers the background AI-blessing network task).
ExperienceScene getCurrentScene() {
  ExperienceScene scene;
  scene.state = currentState;
  scene.mood = (currentMoodTag[0] != '\0') ? currentMoodTag : "none";
  scene.audioActive = blessingTaskActive || (currentPlayingTrack != 0);
  scene.displayLocked = feetDisplayLocked;
  scene.elapsedMs = millis() - stateTimer;
  scene.durationMs = stateDuration;
  return scene;
}

void triggerMantra() {
  blessingCounter++;

  // A fresh touch always wins over any pending offering resume.
  offeringDisplayActive = false;
  offeringInterrupted = false;
  currentMoodTag[0] = '\0';
  freeBlessingImage();

  // Stop whatever is playing before starting the next track
  dfStop();
  delay(50);

  // Lock screen display timer (12 seconds) - same pattern as Feet Touch:
  // show one personalized blessing first, then let the rotation resume
  // for however much longer the (often much longer) mantra keeps playing.
  feetDisplayTimer = millis();
  feetDisplayLocked = true;
  feetDisplayLockMs = 12000; // ordinary touch: never inherit an offering's long window

  // r129: rotates through mouseChantTracks[] - mouse-back's own small
  // pool, separate from mantraTracks[] (feet's shared playlist). r105
  // had made this a single fixed chant instead of a rotation; r129
  // reintroduces rotation now that more than one mouse-specific chant
  // exists, using its own mouseStep counter so it never touches feetStep.
  int trackIndex = mouseStep;
  int dfTrack = mouseChantTracks[trackIndex].dfTrack;
  unsigned long duration = mouseChantTracks[trackIndex].duration;

  // Same random child/adult blessing text as before - unrelated to
  // which track plays, just what shows on the OLED while it does.
  int r;
  if (random(0, 2) == 0) {
    r = random(0, 26);
    snprintf(scrollText, sizeof(scrollText), "   [BLESSING] %s   ", oledChildBlessingsList[r]);
  } else {
    r = random(0, 22);
    snprintf(scrollText, sizeof(scrollText), "   [BLESSING] %s   ", oledAdultBlessingsList[r]);
  }

  setSystemState(STATE_MANTRA_ACTIVE, duration);
  // Eyes breathe for exactly this chant's duration - setSystemState()
  // just turned this off unconditionally above. triggerAarti() (r128)
  // also turns this back on for its own chant, but a feet touch, an
  // offering, or the temple-reopening welcome mantra (which also uses
  // STATE_MANTRA_ACTIVE, via openTempleFromClosed()) never light it.
  // updateEyeLedBreathing() (called from loop()) does the actual
  // animating from here - this just starts the clock.
  if (EYE_LED_CONNECTED) {
    eyeLedBreathingActive = true;
    eyeLedBreathingStartMs = millis();
  }
  currentPlayingTrack = dfTrack;
  dfPlay(dfTrack);

  mouseStep = (mouseStep + 1) % NUM_MOUSE_TRACKS;
}

void triggerFeetMantra() {
  blessingCounter++;

  // A fresh touch always wins over any pending offering resume.
  offeringDisplayActive = false;
  offeringInterrupted = false;
  currentMoodTag[0] = '\0';
  freeBlessingImage();

  // Stop whatever is playing before starting the next track
  dfStop();
  delay(50);
  
  // Lock screen display timer (12 seconds)
  feetDisplayTimer = millis();
  feetDisplayLocked = true;
  feetDisplayLockMs = 12000; // ordinary touch: never inherit an offering's long window
  
  // Get track from struct array
  int trackIndex = feetStep;
  int dfTrack = mantraTracks[trackIndex].dfTrack;
  unsigned long duration = mantraTracks[trackIndex].duration;
  
  // Alternate child and adult blessings based on track index parity
  int r;
  if (trackIndex % 2 == 0) {
    r = random(0, 26);
    snprintf(scrollText, sizeof(scrollText), "   [BLESSING] %s   ", oledChildBlessingsList[r]);
  } else {
    r = random(0, 22);
    snprintf(scrollText, sizeof(scrollText), "   [BLESSING] %s   ", oledAdultBlessingsList[r]);
  }
  
  setSystemState(STATE_FEET_ACTIVE, duration);
  currentPlayingTrack = dfTrack;
  dfPlay(dfTrack);

  feetStep = (feetStep + 1) % NUM_TRACKS;
}

// Minimal JSON string escaper - only needs to handle a single text field
// going into a hand-built JSON body (see speakBlessingOnAmp() below), so a
// full JSON library isn't worth adding as a new dependency. Multi-byte
// UTF-8 (Hindi/Tamil/etc. blessing text) needs no special handling here -
// every continuation byte is >= 0x80 and passes through unescaped, which
// is exactly what valid JSON expects.
String jsonEscape(const String &s) {
  String out;
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) {
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", (uint8_t)c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

// Scans an HTTP response stream for the WAV "data" subchunk marker and
// consumes it plus its 4-byte length, leaving the stream positioned right
// at the first raw PCM sample. Scanning for the marker (rather than
// assuming a fixed 44-byte header) works regardless of exactly how Google
// TTS formats its WAV header. Bounded by deadlineMs so a stalled/odd
// response can never hang forever - the same unbounded-wait mistake that
// caused tonight's earlier DFPlayer crash, not repeated here.
bool skipToWavData(WiFiClient *stream, unsigned long deadlineMs) {
  uint8_t window[4] = {0, 0, 0, 0};
  while (millis() < deadlineMs) {
    if (!stream->available()) {
      delay(5);
      continue;
    }
    uint8_t b;
    if (stream->readBytes(&b, 1) != 1) continue;
    window[0] = window[1];
    window[1] = window[2];
    window[2] = window[3];
    window[3] = b;
    if (window[0] == 'd' && window[1] == 'a' && window[2] == 't' && window[3] == 'a') {
      uint8_t sizeBytes[4];
      size_t got = 0;
      while (got < 4 && millis() < deadlineMs) {
        if (stream->available()) {
          got += stream->readBytes(sizeBytes + got, 4 - got);
        } else {
          delay(5);
        }
      }
      return got == 4;
    }
  }
  return false;
}

// Fetches a correctly-shaped bitmap for text/lang from the backend's
// renderTextImage endpoint (see SCRIPT_FONTS/renderTextToXbm() in
// backend/functions/index.js - U8g2 has no text-shaping engine, so
// Devanagari/Tamil/etc. text drawn directly looks fragmented; this
// endpoint renders it correctly server-side instead) and stores it in
// blessingImageBuf/Width/Height/BytesPerRow for drawOLED() to draw+
// scroll in place of scrollText. Best-effort only, same philosophy as
// postAndStreamAudioToAmp() below: any failure here just means the OLED
// falls back to whatever scrollText already holds (the plain English
// "Blessing spoken - see dashboard for text" notice) - never a crash or
// a hang, and never blocks the spoken blessing, which is unaffected by
// whatever happens here.
void fetchBlessingImage(const String &text, const String &lang) {
  freeBlessingImage(); // always start clean - a stale image from a previous blessing must never show through a failed fetch

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10000);

  HTTPClient https;
  https.setTimeout(10000);
  if (!https.begin(client, "https://us-central1-ganapatiai.cloudfunctions.net/renderTextImage")) {
    Serial.println("IMAGE: https.begin() failed");
    return;
  }
  https.addHeader("Content-Type", "application/json");

  String body = "{\"text\":\"" + jsonEscape(text) + "\",\"lang\":\"" + lang + "\"}";
  int code = https.POST(body);
  if (code == 204) {
    Serial.println("IMAGE: backend has no dedicated font for this language - staying on plain text");
    https.end();
    return;
  }
  if (code != 200) {
    Serial.printf("IMAGE: backend returned HTTP %d\n", code);
    https.end();
    return;
  }

  WiFiClient *stream = https.getStreamPtr();

  // 6-byte header: width, height, bytesPerRow, each uint16 little-endian
  // (see renderTextImage's own comment in index.js).
  uint8_t header[6];
  int got = 0;
  unsigned long headerDeadline = millis() + 8000;
  while (got < 6 && millis() < headerDeadline) {
    if (stream->available()) {
      got += stream->readBytes(header + got, 6 - got);
    } else if (!https.connected()) {
      break;
    } else {
      delay(5);
    }
  }
  if (got != 6) {
    Serial.println("IMAGE: timed out or stream closed before the 6-byte header arrived");
    https.end();
    return;
  }

  int w = header[0] | (header[1] << 8);
  int h = header[2] | (header[3] << 8);
  int bpr = header[4] | (header[5] << 8);
  int dataLen = bpr * h;

  // Sanity bounds before ever malloc()ing based on network-supplied
  // numbers, on a device this RAM-constrained - a corrupted response
  // must never be trusted with an unbounded allocation. A full-width
  // (128px-wide screen worth of rows) blessing at the font size this
  // renders at would be nowhere near these limits even for a very long
  // blessing, so this is generous headroom, not a real constraint.
  if (w <= 0 || w > 4000 || h <= 0 || h > 200 || dataLen <= 0 || dataLen > 40000) {
    Serial.printf("IMAGE: header out of sane bounds (w=%d h=%d bytesPerRow=%d) - aborting\n", w, h, bpr);
    https.end();
    return;
  }

  uint8_t *buf = (uint8_t *)malloc(dataLen);
  if (buf == NULL) {
    Serial.println("IMAGE: malloc failed - out of heap, staying on plain text");
    https.end();
    return;
  }

  int readSoFar = 0;
  unsigned long lastDataMs = millis();
  while (readSoFar < dataLen) {
    size_t avail = stream->available();
    if (avail == 0) {
      if (!https.connected() || millis() - lastDataMs > 8000) {
        Serial.println("IMAGE: stream ended/stalled before the full image arrived");
        free(buf);
        https.end();
        return;
      }
      delay(5);
      continue;
    }
    size_t toRead = avail < (size_t)(dataLen - readSoFar) ? avail : (size_t)(dataLen - readSoFar);
    int n = stream->readBytes(buf + readSoFar, toRead);
    if (n <= 0) break;
    readSoFar += n;
    lastDataMs = millis();
  }
  https.end();

  if (readSoFar != dataLen) {
    Serial.printf("IMAGE: incomplete (%d of %d bytes) - discarding\n", readSoFar, dataLen);
    free(buf);
    return;
  }

  blessingImageBuf = buf;
  blessingImageWidth = w;
  blessingImageHeight = h;
  blessingImageBytesPerRow = bpr;
  blessingImageScrollX = 128;
  Serial.printf("IMAGE: received %dx%d blessing bitmap (%d bytes)\n", w, h, dataLen);
}

// Fetches spoken-blessing audio for the given (already-decided) text from
// the backend's synthesizeAudio endpoint and streams it straight to the
// physical altar speaker over I2S, without ever buffering the whole clip
// in RAM. Best-effort only: any failure here just means the temple stays
// silent for this offering - the OLED display (triggered separately by
// the caller) still shows the text regardless, and every wait in here is
// bounded so a slow/stalled network can never hang the device.
//
// NOTE: client.setInsecure() skips TLS certificate validation. Accepted
// tradeoff for this project - avoids maintaining Google's rotating root
// CA on-device - but means a network-level attacker on the same Wi-Fi
// could in principle substitute the audio. Low stakes for a home
// devotional altar; revisit if that ever changes.
// Shared by speakBlessingOnAmp() and speakGenericBlessingOnAmp() below -
// both just POST a JSON body to a Cloud Function and stream whatever WAV
// audio comes back straight to the I2S amp, never buffering the whole
// clip in RAM; only the URL and body actually differ between them.
//
// outMood is optional (pass NULL to ignore) - populated from the
// X-Blessing-Mood response header when the backend sends one (see
// askClaudeForBlessing()/THEME_TO_MOOD in index.js), so animateLeds()
// can switch the LED ring to match the blessing's actual emotional tone
// instead of a fixed color regardless of what was prayed for. Captured
// right after the headers arrive, before the (possibly multi-second)
// audio stream starts, so the LED can react as early as possible - the
// synthesizeAudio endpoint (speakBlessingOnAmp's fast path) never sets
// this header at all, so outMood is left untouched for that caller,
// which is fine since that caller doesn't pass one.
//
// Returns true if any real audio was actually heard, false otherwise -
// callers use this to decide whether to fall back to
// playOfflineBlessingFallback() (Phase 1: offline blessing library).
// A stream that stalls AFTER some real bytes already played counts as
// true (something genuine was heard, just cut short) - only a total
// failure before any audio played triggers the fallback, so a devotee
// never hears two different blessings back to back.
bool postAndStreamAudioToAmp(const String &url, const String &jsonBody, String *outMood = NULL) {
  // Logged before the TLS handshake/POST even starts, since both can take
  // several seconds on ESP32 (WiFiClientSecure re-handshakes every call,
  // even with setInsecure()) and that whole stretch was otherwise silent -
  // easy to misread as a hang when it's just an in-flight connection.
  Serial.println("AMP: connecting...");

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10000);

  HTTPClient https;
  https.setTimeout(10000);
  if (!https.begin(client, url)) {
    Serial.println("AMP: https.begin() failed");
    return false;
  }
  https.addHeader("Content-Type", "application/json");
  if (outMood != NULL) {
    const char *headerKeys[] = {"X-Blessing-Mood"};
    https.collectHeaders(headerKeys, 1);
  }

  int code = https.POST(jsonBody);
  if (code != 200) {
    // The body on a failure is the actual error text (e.g. "Speech
    // synthesis failed: Google TTS returned 400: ...") - logging only
    // the numeric code meant a real diagnosis (the Chirp3-HD pitch
    // rejection found tonight) needed guessing instead of just reading
    // it. Bounded to 200 chars so a large/unexpected body can't flood
    // the monitor.
    String errBody = https.getString();
    Serial.printf("AMP: backend returned HTTP %d: %s\n", code, errBody.substring(0, 200).c_str());
    https.end();
    return false;
  }

  if (outMood != NULL && https.hasHeader("X-Blessing-Mood")) {
    *outMood = https.header("X-Blessing-Mood");
    Serial.printf("AMP: blessing mood = %s\n", outMood->c_str());
  }

  WiFiClient *stream = https.getStreamPtr();
  if (!skipToWavData(stream, millis() + 8000)) {
    Serial.println("AMP: could not find WAV data chunk, aborting playback");
    https.end();
    return false;
  }

  Serial.println("AMP: playing spoken blessing...");
  uint8_t buf[1024];
  int bytesPlayed = 0;
  unsigned long lastDataMs = millis();
  while (https.connected() || stream->available()) {
    size_t avail = stream->available();
    if (avail == 0) {
      if (millis() - lastDataMs > 8000) {
        Serial.println("AMP: stream stalled, stopping playback");
        break;
      }
      delay(5);
      continue;
    }
    size_t toRead = avail < sizeof(buf) ? avail : sizeof(buf);
    int n = stream->readBytes(buf, toRead);
    if (n <= 0) break;
    ampI2S.write(buf, n);
    bytesPlayed += n;
    lastDataMs = millis();
    delay(1); // real yield, unlike delay(0) - see speakBlessingOnAmpAsync() for why this whole function no longer runs on the watchdog-subscribed task at all
  }

  https.end();
  Serial.printf("AMP: playback finished, %d bytes played\n", bytesPlayed);
  return bytesPlayed > 0;
}

// ==========================================
// Phase 1: Offline Blessing Fallback
// ==========================================
// Fires whenever postAndStreamAudioToAmp() couldn't play any real audio
// at all (Wi-Fi down, backend unreachable, or a hard failure before any
// bytes streamed) - plays one of a small set of pre-recorded, generic
// blessing chants from the SD card instead of leaving the temple
// silent. NOT personalized or translated like every other spoken
// blessing on this device - these are fixed recordings, so a devotee
// still gets a real spoken blessing when the network can't reach
// Claude/Google TTS at all, rather than silence. Real blessing content
// (not a mantra/chant - a genre mismatch was tried and reverted, see
// config.h), generated once via the existing synthesizeAudio voice
// rather than a human recording - see OFFLINE_BLESSING_TRACKS in
// config.h for the exact generation steps.
// offlineFallbackCount/lastOfflineFallbackMs (declared up near
// blessingCounter) are plumbing for the dashboard health panel
// (Phase 5) - not surfaced there yet, only in /api/state for now.
void playOfflineBlessingFallback(const String &lang) {
  int track = OFFLINE_BLESSING_TRACKS[random(0, OFFLINE_BLESSING_TRACK_COUNT)];

  // Same reasoning as the wish pad's deterministic mood pick (see
  // THEME_TO_MOOD in index.js): there's no real sentiment behind a
  // network failure for Claude to read even if it were reachable, so a
  // random pick from the same 6 moods beats defaulting to one fixed
  // color every time.
  static const char *OFFLINE_MOODS[] = {"joyful", "hopeful", "comforting", "peaceful", "empowering", "grateful"};
  strlcpy(currentMoodTag, OFFLINE_MOODS[random(0, 6)], sizeof(currentMoodTag));

  // Same random child/adult flavor text already used for a plain mantra
  // touch, so the OLED still shows something warm and varied instead of
  // one fixed "offline" notice every single time (r109 planning: "local
  // variation" without any AI/network dependency).
  freeBlessingImage(); // always the fixed local text below, never a fetched image
  int r;
  if (random(0, 2) == 0) {
    r = random(0, 26);
    snprintf(scrollText, sizeof(scrollText), "   [BLESSING] %s   ", oledChildBlessingsList[r]);
  } else {
    r = random(0, 22);
    snprintf(scrollText, sizeof(scrollText), "   [BLESSING] %s   ", oledAdultBlessingsList[r]);
  }
  strlcpy(scrollTextLang, "en", sizeof(scrollTextLang));

  Serial.printf("OFFLINE: live blessing unavailable - playing local track %d instead (requested lang=%s, not translated)\n",
                track, lang.c_str());

  offlineFallbackCount++;
  lastOfflineFallbackMs = millis();

  dfStop();
  delay(50);
  dfPlay(track);
  currentPlayingTrack = track;
}

// Speaks text that's ALREADY been decided (a blessing already shown/heard
// by the devotee's phone, read back out of the priest queue) - calls the
// lightweight TTS-only synthesizeAudio endpoint, no Claude call.
void speakBlessingOnAmp(const String &text, const String &lang) {
  if (!ampReady) {
    Serial.println("AMP: not initialized, skipping spoken blessing");
    return;
  }
  if (text.length() == 0) return;

  String body = "{\"text\":\"" + jsonEscape(text) + "\",\"lang\":\"" + lang + "\"}";
  bool ok = postAndStreamAudioToAmp("https://us-central1-ganapatiai.cloudfunctions.net/synthesizeAudio", body);
  if (!ok) playOfflineBlessingFallback(lang);
}

// Fired by the wish pad (see triggerWishPadBlessing() below) - a devotee
// touched it in silent prayer, with no offering and no typed/spoken wish
// at all, so there's no text yet to speak. Calls the FULL generateBlessing
// endpoint (Claude + TTS) with touchOnly:true, so Bappa writes a fresh
// blessing on the spot - never the same words twice, since it's a real
// LLM call each time rather than a fixed recorded phrase.
void speakGenericBlessingOnAmp(const String &lang) {
  if (!ampReady) {
    Serial.println("AMP: not initialized, skipping spoken blessing");
    return;
  }

  String body = "{\"name\":\"a devotee\",\"offering\":\"\",\"prayer\":\"\",\"standardWish\":\"\",\"lang\":\"" + lang + "\",\"touchOnly\":true}";
  String mood;
  bool ok = postAndStreamAudioToAmp("https://us-central1-ganapatiai.cloudfunctions.net/generateBlessing", body, &mood);
  if (!ok) {
    playOfflineBlessingFallback(lang);
    return;
  }
  if (mood.length() > 0) strlcpy(currentMoodTag, mood.c_str(), sizeof(currentMoodTag));
}

// Fallback for triggerPersonalizedOffering() when the priest approves an
// item before it has any already-decided blessing text (item.prayer is
// still empty) - confirmed on hardware as offerings that show correctly
// on the OLED but play NOTHING on the amp, since speakBlessingOnAmpAsync
// silently skips empty text. Two real ways this happens: the devotee
// submitted with no typed wish AND no quick-blessing dropdown pick (a
// genuinely silent offering), or the priest approved faster than the
// phone's own generateBlessing round trip finished. Rather than staying
// silent either way, this generates a fresh blessing on the spot - same
// idea as the wish pad's touchOnly path, but WITH the real name/offering
// context (touchOnly:false), so it reads as "your offering", not a blind
// touch.
void speakOfferingFallbackBlessingOnAmp(const String &name, const String &offeringType, const String &lang) {
  if (!ampReady) {
    Serial.println("AMP: not initialized, skipping spoken blessing");
    return;
  }

  String body = "{\"name\":\"" + jsonEscape(name) + "\",\"offering\":\"" + jsonEscape(offeringType) +
                "\",\"prayer\":\"\",\"standardWish\":\"\",\"lang\":\"" + lang + "\",\"touchOnly\":false}";
  String mood;
  bool ok = postAndStreamAudioToAmp("https://us-central1-ganapatiai.cloudfunctions.net/generateBlessing", body, &mood);
  if (!ok) {
    playOfflineBlessingFallback(lang);
    return;
  }
  if (mood.length() > 0) strlcpy(currentMoodTag, mood.c_str(), sizeof(currentMoodTag));
}

// Two real crashes found on hardware tonight (both watchdog panics) came
// from speakBlessingOnAmp() running inline inside the HTTP request
// handler - on the SAME task as loop()/server.handleClient(), which is
// subscribed to the watchdog. A TLS handshake, a slow synthesizeAudio
// response, or - worst case - ampI2S.write() itself blocking forever if
// the amp isn't actually consuming data (a wiring fault: the ESP32's I2S
// DMA buffer fills up and write() waits for space that never frees) could
// all run past the watchdog's timeout and reboot the whole board, freezing
// the OLED and every other sensor along with it for however long it took.
//
// Running it on its own FreeRTOS task instead, NOT subscribed to the
// watchdog, fixes the actual failure mode: a stuck or faulty amp can now
// only ever produce silence, never a crash. blessingTaskActive (declared
// near ampReady, above) guards against two offerings overlapping, since
// ampI2S is a single shared instance not safe for concurrent writes from
// two tasks at once.

struct BlessingTaskParams {
  String text;
  String lang;
};

void speakBlessingTaskFn(void *pvParameters) {
  BlessingTaskParams *params = (BlessingTaskParams *)pvParameters;
  // Same text/lang already known for the speech itself - reused here so
  // there's no separate plumbing needed. fetchBlessingImage() is a
  // no-op (204 from the backend) for English or any language with no
  // dedicated font, so this is safe to call unconditionally rather than
  // duplicating the "which languages get an image" list on this side
  // too - the backend's SCRIPT_FONTS is the one source of truth for that.
  fetchBlessingImage(params->text, params->lang);
  speakBlessingOnAmp(params->text, params->lang);
  delete params;
  blessingTaskActive = false;
  vTaskDelete(NULL);
}

void speakBlessingOnAmpAsync(const String &text, const String &lang) {
  if (blessingTaskActive) {
    Serial.println("AMP: a blessing is already playing, skipping this one");
    return;
  }
  if (!ampReady || text.length() == 0) return;

  BlessingTaskParams *params = new BlessingTaskParams{text, lang};
  blessingTaskActive = true;
  blessingTaskStartMs = millis();
  BaseType_t created = xTaskCreatePinnedToCore(speakBlessingTaskFn, "blessingAmp", 8192, params, 1, NULL, 1);
  if (created != pdPASS) {
    Serial.println("AMP: failed to create blessing playback task");
    delete params;
    blessingTaskActive = false;
  }
}

// Same background-task pattern again, for speakOfferingFallbackBlessingOnAmp()
// above - see triggerPersonalizedOffering()'s call site for when this
// fires instead of the normal already-decided-text path.
struct OfferingFallbackTaskParams {
  String name;
  String offeringType;
  String lang;
};

void speakOfferingFallbackTaskFn(void *pvParameters) {
  OfferingFallbackTaskParams *params = (OfferingFallbackTaskParams *)pvParameters;
  speakOfferingFallbackBlessingOnAmp(params->name, params->offeringType, params->lang);
  delete params;
  blessingTaskActive = false;
  vTaskDelete(NULL);
}

void speakOfferingFallbackBlessingOnAmpAsync(const String &name, const String &offeringType, const String &lang) {
  if (blessingTaskActive) {
    Serial.println("AMP: a blessing is already playing, skipping this one");
    return;
  }
  if (!ampReady) return;

  OfferingFallbackTaskParams *params = new OfferingFallbackTaskParams{name, offeringType, lang};
  blessingTaskActive = true;
  blessingTaskStartMs = millis();
  BaseType_t created = xTaskCreatePinnedToCore(speakOfferingFallbackTaskFn, "offeringFallback", 8192, params, 1, NULL, 1);
  if (created != pdPASS) {
    Serial.println("AMP: failed to create blessing playback task");
    delete params;
    blessingTaskActive = false;
  }
}

// Same background-task pattern as speakBlessingOnAmpAsync() above, for
// the wish pad's fresh-each-time generic blessing. Shares
// blessingTaskActive with it deliberately - both ultimately write to the
// same single ampI2S instance, so a wish-pad touch and a priest approval
// landing at the same moment must never play concurrently.
struct GenericBlessingTaskParams {
  String lang;
};

void speakGenericBlessingTaskFn(void *pvParameters) {
  GenericBlessingTaskParams *params = (GenericBlessingTaskParams *)pvParameters;
  speakGenericBlessingOnAmp(params->lang);
  delete params;
  blessingTaskActive = false;
  vTaskDelete(NULL);
}

void speakGenericBlessingOnAmpAsync(const String &lang) {
  if (blessingTaskActive) {
    Serial.println("AMP: a blessing is already playing, skipping this one");
    return;
  }
  if (!ampReady) return;

  GenericBlessingTaskParams *params = new GenericBlessingTaskParams{lang};
  blessingTaskActive = true;
  blessingTaskStartMs = millis();
  BaseType_t created = xTaskCreatePinnedToCore(speakGenericBlessingTaskFn, "wishPadBlessing", 8192, params, 1, NULL, 1);
  if (created != pdPASS) {
    Serial.println("AMP: failed to create blessing playback task");
    delete params;
    blessingTaskActive = false;
  }
}

// One-off hardware check for the physical INMP441 mic - triggered by
// browsing to /api/test?mic=1, NOT run automatically at boot or in the
// main loop (the mic has no feature using it yet - see the config.h
// comment on I2S_MIC_WS/SCK/SD). Runs on its own background task, same
// reasoning as the amp: an I2S read that never returns (bad wiring, dead
// module) must never be able to stall the watchdog-subscribed main loop.
// Prints a live level bar to Serial for ~4 seconds so you can watch it
// react in real time while clapping/speaking near the mic, rather than
// just a pass/fail verdict at the end.
void micTestTaskFn(void *pvParameters) {
  Serial.println("MIC TEST: initializing I2S RX...");
  micI2S.setPins(I2S_MIC_SCK, I2S_MIC_WS, -1, I2S_MIC_SD);
  bool ok = micI2S.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
  if (!ok) {
    Serial.println("MIC TEST: I2S init FAILED - check WS/SCK/SD/VDD/GND wiring on the INMP441.");
    micTestRunning = false;
    vTaskDelete(NULL);
    return;
  }

  Serial.println("MIC TEST: running for 4 seconds - clap or speak near the mic now.");
  static int16_t buf[1600]; // 100ms at 16kHz mono 16-bit
  for (int chunk = 0; chunk < 40; chunk++) {
    size_t bytesRead = micI2S.readBytes((char *)buf, sizeof(buf));
    size_t samples = bytesRead / sizeof(int16_t);
    int peak = 0;
    for (size_t i = 0; i < samples; i++) {
      int v = abs((int)buf[i]); // widen before abs() - abs(INT16_MIN) as int16_t overflows
      if (v > peak) peak = v;
    }
    int barLen = map(peak, 0, 32767, 0, 40);
    char bar[41];
    memset(bar, '#', barLen);
    bar[barLen] = '\0';
    Serial.printf("MIC LEVEL: [%-40s] peak=%d samples=%u\n", bar, peak, (unsigned)samples);
  }

  micI2S.end();
  Serial.println("MIC TEST: done. A bar that moved with your voice/claps means the mic works; "
                  "flat at 0 the whole time means check the wiring before assuming the module is bad.");
  micTestRunning = false;
  vTaskDelete(NULL);
}

// Records 3 real seconds from the mic and plays it straight back out the
// amp speaker, so the mic can be judged by ear instead of a number -
// triggered by /api/test?micplay=1. Holds BOTH micTestRunning and
// blessingTaskActive for its whole life: it touches both shared I2S
// instances (micI2S for the recording, ampI2S for playback) and neither
// guard alone would stop it colliding with the other kind of test/use.
void micRecordPlaybackTaskFn(void *pvParameters) {
  Serial.println("MIC PLAYBACK: initializing I2S RX...");
  micI2S.setPins(I2S_MIC_SCK, I2S_MIC_WS, -1, I2S_MIC_SD);
  bool ok = micI2S.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
  if (!ok) {
    Serial.println("MIC PLAYBACK: I2S init FAILED - check wiring.");
    micTestRunning = false;
    blessingTaskActive = false;
    vTaskDelete(NULL);
    return;
  }

  Serial.println("MIC PLAYBACK: recording 3 seconds now - speak or clap near the mic...");
  size_t wavSize = 0;
  uint8_t *wavBuf = micI2S.recordWAV(3, &wavSize);
  micI2S.end();

  if (wavBuf == NULL || wavSize == 0) {
    Serial.println("MIC PLAYBACK: recording FAILED - check wiring.");
    micTestRunning = false;
    blessingTaskActive = false;
    vTaskDelete(NULL);
    return;
  }

  Serial.printf("MIC PLAYBACK: recorded %u bytes, playing it back on the amp now...\n", (unsigned)wavSize);
  if (ampReady) {
    ampI2S.playWAV(wavBuf, wavSize);
    Serial.println("MIC PLAYBACK: done playing.");
  } else {
    Serial.println("MIC PLAYBACK: amp not ready, can't play back.");
  }
  free(wavBuf);

  micTestRunning = false;
  blessingTaskActive = false;
  vTaskDelete(NULL);
}

// Fired when the priest approves a devotee's submission from the Priest
// Queue (see /api/control?action=offering, wired from approveQueueItem()
// in web_dashboard.h) - shows their actual name/offering/prayer on the
// physical OLED for 12s, the same personalized text already shown
// locally in the browser dashboard when a priest approves an item, and
// speaks it aloud through the altar's own speaker (see
// speakBlessingOnAmp() above).
void triggerPersonalizedOffering(String name, String offeringType, String prayer, String lang, String mood) {
  blessingCounter++;

  // If a mantra OR the Aarti chant is actively playing, remember it so it
  // can resume once the 12-second offering display finishes - both which
  // track (currentPlayingTrack, untouched by the BELL_TRACK played below)
  // and how much of its state timer was left. Otherwise (idle/Ambient/
  // etc.) there's nothing to resume, just show the offering and settle
  // back to standby.
  //
  // NOTE: this used to call myDFPlayer.pause() and resume with .start(),
  // which the DFPlayer Mini datasheet documents but which many cheap
  // DFPlayer Mini clones do NOT reliably honor in practice (a widely
  // reported issue - start() silently does nothing on affected boards, so
  // the mantra never came back after an offering). Restarting the SAME
  // track from the beginning is less elegant (loses the exact playback
  // position) but works on every DFPlayer clone, which matters more.
  //
  // AARTI was missing from this condition until now - an offering
  // approved mid-Aarti was silently treated as "nothing to resume" and
  // just stopped for good, matching the reported "pauses the Aarti but
  // doesn't play it back" bug. aartiThenClose is left untouched here on
  // purpose - it's not reset anywhere during this interruption, so
  // whichever ritual intent (close the temple afterward, or not) the
  // Aarti had before being interrupted is still correct once resumed.
  offeringInterrupted = (currentState == STATE_MANTRA_ACTIVE || currentState == STATE_FEET_ACTIVE || currentState == STATE_AARTI);
  if (offeringInterrupted) {
    offeringPausedState = currentState;
    // The FULL duration, not the remainder - see the note on
    // offeringPausedDurationMs above. Replaying from the start needs the
    // whole track's time again, regardless of how far in it was when
    // this offering interrupted it.
    offeringPausedDurationMs = stateDuration;
  }
  Serial.printf("OFFERING: approved while state=%s, offeringInterrupted=%s, pausedTrack=%d, fullDurationMs=%lu\n",
                stateName(currentState), offeringInterrupted ? "true" : "false", currentPlayingTrack, offeringPausedDurationMs);
  dfStop();
  offeringDisplayActive = true;
  delay(50);
  dfPlay(BELL_TRACK);

  feetDisplayTimer = millis();
  feetDisplayLocked = true;

  if (name.length() == 0) name = "Anonymous Devotee";

  if (prayer.length() > 0) {
    snprintf(scrollText, sizeof(scrollText), "   [OFFERING] %s: %s   ", name.c_str(), prayer.c_str());
    // The only place in the whole firmware where scrollText's content is
    // ever anything but English - see fontForScrollLang() in drawOLED().
    strlcpy(scrollTextLang, lang.c_str(), sizeof(scrollTextLang));
    // Mood was already decided when the phone's own generateBlessing call
    // ran (see X-Blessing-Mood in requestAiBlessing()/puja_page.h) - use
    // it directly, no extra network round trip needed. Empty when an
    // older queue item never captured one, or the dashboard's own
    // quick-offering panel doesn't relay it - animateLeds() treats an
    // empty mood the same as "not known", same as before this feature.
    strlcpy(currentMoodTag, mood.c_str(), sizeof(currentMoodTag));
    Serial.printf("OFFERING: mood param received = \"%s\"\n", mood.c_str());
  } else {
    snprintf(scrollText, sizeof(scrollText), "   [OFFERING] Thank you, %s, for your %s offering!   ", name.c_str(), offeringType.c_str());
    strlcpy(scrollTextLang, "en", sizeof(scrollTextLang)); // fixed English template regardless of lang
    // Not known yet in this branch - speakOfferingFallbackBlessingOnAmpAsync()
    // below determines its own mood live and fills this in once its
    // network call returns, same async-update pattern as the wish pad.
    currentMoodTag[0] = '\0';
    // This branch always shows the fixed English template above, never
    // a fetched image - clear out any leftover image from a PREVIOUS
    // offering so it can't show through underneath this one.
    freeBlessingImage();
  }

  // 12s is only the MINIMUM here. The display actually ends when the
  // message has finished a real scroll pass (see STATE_FEET_ACTIVE in
  // updateStateMachine) - trying to pre-compute the pass duration was
  // what clipped the last word or two, because real frame timing runs
  // ~20% slower than the arithmetic predicted. feetDisplayLockMs is set
  // generously so the blessing rotation can't overwrite the offering
  // mid-pass; the state machine clears the lock explicitly when the
  // offering display genuinely finishes.
  u8g2.setFont(u8g2_font_logisoso20_tf);
  Serial.printf("OFFERING: showing a %dpx message, min 12s, ends on full scroll pass\n",
                u8g2.getUTF8Width(scrollText));
  feetDisplayLockMs = 600000UL; // effectively "until the offering ends"

  setSystemState(STATE_FEET_ACTIVE, 12000);

  // Speak the blessing through the altar's own speaker, on its own
  // background task (see speakBlessingOnAmpAsync() above) - two real
  // watchdog-panic crashes on hardware tonight both traced back to this
  // running inline on the same task as loop()/server.handleClient().
  //
  // `prayer` here is normally the AI blessing text already decided by the
  // devotee's phone (see upgradeOfferingText() in puja_page.h) - the fast
  // path just re-speaks it. But it arrives EMPTY whenever that phone-side
  // call hasn't landed yet (the priest approved faster than the network
  // round trip) or genuinely failed - and empty text made
  // speakBlessingOnAmpAsync silently do nothing, so the OLED showed the
  // offering while the speaker stayed silent. Confirmed on hardware.
  // Falling back to generating a blessing live (same idea as the wish
  // pad, but with the real name/offering context) means an approved
  // offering can never end in silence.
  if (prayer.length() > 0) {
    speakBlessingOnAmpAsync(prayer, lang);
  } else {
    Serial.println("OFFERING: no already-decided blessing text yet - generating one live instead of staying silent.");
    speakOfferingFallbackBlessingOnAmpAsync(name, offeringType, lang);
  }
}

// Fired by a touch on the physical wish pad (see checkSensors()) - the
// way one would touch a deity's feet in silent prayer: no name, no
// offering, no typed/spoken wish. Deliberately lighter-weight than
// triggerPersonalizedOffering() - a brief generic acknowledgment on the
// OLED rather than a 12s personalized display, since there's no specific
// text to show. Reuses the SAME interrupt/resume mechanism (offeringDisplayActive
// etc.) so a wish-pad touch mid-mantra pauses and resumes exactly like an
// offering approval does.
void triggerWishPadBlessing() {
  offeringInterrupted = (currentState == STATE_MANTRA_ACTIVE || currentState == STATE_FEET_ACTIVE || currentState == STATE_AARTI);
  if (offeringInterrupted) {
    offeringPausedState = currentState;
    offeringPausedDurationMs = stateDuration;
  }
  Serial.printf("WISH PAD: touched while state=%s, offeringInterrupted=%s\n",
                stateName(currentState), offeringInterrupted ? "true" : "false");
  dfStop();
  offeringDisplayActive = true;
  delay(50);
  dfPlay(BELL_TRACK);

  feetDisplayTimer = millis();
  feetDisplayLocked = true;
  strlcpy(scrollText, "   \xF0\x9F\x99\x8F Your silent prayer is heard...   ", sizeof(scrollText));
  strlcpy(scrollTextLang, "en", sizeof(scrollTextLang));
  // Not known yet - speakGenericBlessingOnAmpAsync() below determines the
  // mood live (deterministically, from the same random theme it picks
  // for the blessing text) and fills this in once its network call
  // returns, same as the LED starting on the default palette until then.
  currentMoodTag[0] = '\0';
  freeBlessingImage(); // this display is always the fixed English message above, never a fetched image
  feetDisplayLockMs = 600000UL; // cleared by updateStateMachine() when the display genuinely finishes, same as an offering

  setSystemState(STATE_FEET_ACTIVE, 6000);
  wishPadBlessingActive = true; // r126: see its own declaration comment - lets the OLED tag/API tell this apart from a real feet touch

  // Reuses the dashboard's own language setting (selectedLang, 0/1/2 -
  // see its declaration above) rather than always English - the wish pad
  // has no language picker of its own, unlike puja.html's 9-language
  // dropdown (which also includes Punjabi/Gujarati/etc - none of those
  // reach the wish pad, only this 4-option dashboard setting does), but
  // devotees touching it still hear it in whatever language the altar is
  // currently set to. Code 1 (Sanskrit/Hindi) maps to Hindi since the
  // backend's LANGUAGE_CONFIG has no dedicated Sanskrit voice; code 3
  // (Marathi) maps to the backend's real Marathi voice - these used to
  // both be code 1, which meant Marathi silently played as Hindi
  // (confirmed on hardware) until LANG_TO_CODE gave Marathi its own code.
  const char *wishPadLang = (selectedLang == 1) ? "hi" : (selectedLang == 2) ? "ta" : (selectedLang == 3) ? "mr" : "en";
  speakGenericBlessingOnAmpAsync(wishPadLang);
}

// r133: on-demand playback of any known track (dashboard's "Play Track"
// control) - looks up the real duration from whichever pool the track
// number belongs to (feet/mouse/Aarti's two parts) so the OLED/LED render
// something coherent for the actual length instead of a generic guess.
// Reuses STATE_FEET_ACTIVE purely for its saffron/gold display and timer,
// same as several other manual triggers - this is an admin convenience,
// not a devotional touch flow, so it doesn't need its own SystemState.
void playTrackManually(int n) {
  unsigned long duration = 60000; // fallback for a track not in any known list below
  for (int i = 0; i < NUM_TRACKS; i++) {
    if (mantraTracks[i].dfTrack == n) { duration = mantraTracks[i].duration; break; }
  }
  for (int i = 0; i < NUM_MOUSE_TRACKS; i++) {
    if (mouseChantTracks[i].dfTrack == n) { duration = mouseChantTracks[i].duration; break; }
  }
  if (n == AARTI_TRACK) duration = AARTI_PART1_DURATION;
  else if (n == AARTI_PART2_TRACK) duration = AARTI_PART2_DURATION;

  dfStop();
  delay(50);
  offeringDisplayActive = false;
  offeringInterrupted = false;
  currentMoodTag[0] = '\0';
  freeBlessingImage();
  feetDisplayLocked = true;
  feetDisplayTimer = millis();
  feetDisplayLockMs = 12000;
  strlcpy(scrollText, "   \xF0\x9F\x8E\xB5 Manual playback \xF0\x9F\x8E\xB5   ", sizeof(scrollText));
  strlcpy(scrollTextLang, "en", sizeof(scrollTextLang));

  setSystemState(STATE_FEET_ACTIVE, duration);
  currentPlayingTrack = n;
  dfPlay(n);
}

void triggerAarti() {
  // Fired by the dashboard's manual "Close Temple" button (?action=close,
  // via triggerAartiThenClose() below) or directly via ?action=aarti
  // (dashboard button or manual test) - in every case this just needs to
  // start the physical chant and reflect AARTI_MODE back via /api/state.
  // r128: no longer fired autonomously by AMBIENT_TIMEOUT idle - see that
  // removal's own comment in updateStateMachine()'s STATE_AMBIENT case.
  dfStop();
  delay(50);

  feetDisplayLocked = false;
  strlcpy(scrollText, "   \xE2\x9C\xA8 A moment of Aarti \xE2\x9C\xA8   ", sizeof(scrollText));
      strlcpy(scrollTextLang, "en", sizeof(scrollTextLang));

  setSystemState(STATE_AARTI, AARTI_DURATION);
  // r128: eyes now flash/breathe through the whole Aarti chant too, not
  // just a mouse-back touch - direct request that the guardian's eyes
  // react while the LED ring's flame animation (see animateLeds()'s
  // dedicated STATE_AARTI palette/intensity arc) visibly changes color
  // around it, instead of sitting static at the plain resting glow.
  // Same updateEyeLedBreathing() cycle as the mouse chant - it just
  // keeps repeating for AARTI_DURATION's full combined duration (r137:
  // both parts back to back) instead of 30s.
  if (EYE_LED_CONNECTED) {
    eyeLedBreathingActive = true;
    eyeLedBreathingStartMs = millis();
  }
  // r137: reset here (not just where it's set true in updateStateMachine())
  // so a fresh Aarti always starts on part 1 even if the previous one was
  // interrupted by a touch mid-part-2.
  aartiPart2Playing = false;
  currentPlayingTrack = AARTI_TRACK;
  dfPlay(AARTI_TRACK);
}

// Aarti that ends in STATE_TEMPLE_CLOSED once it finishes, instead of
// returning to STATE_AMBIENT - the dashboard's manual "Close Temple"
// button (?action=close) is the only caller as of r128 (the idle-timeout
// auto-close that used to also call this was removed - see its own
// comment in updateStateMachine()'s STATE_AMBIENT case).
void triggerAartiThenClose() {
  aartiThenClose = true;
  triggerAarti();
}

// Manually reopen after STATE_TEMPLE_CLOSED (dashboard's "Open Temple"
// button, or a touch while closed) - a quick bell + one short mantra
// (mantraTracks[0] specifically, picked for testing - not the rotating
// mouseStep), NOT the full ~4-minute Aarti chant. Opening and closing are
// not symmetric: closing is meant to be a deliberate, unhurried ritual,
// but reusing that same long ritual for opening meant the temple could
// idle its way into ANOTHER auto-close within minutes of just opening.
//
// Settles into STATE_MANTRA_ACTIVE (like any other mantra) rather than
// jumping straight into STATE_AMBIENT - once this short welcome mantra
// ends it falls through to STATE_STANDBY same as any other mantra, and
// from there PIR is what takes it to AMBIENT. Landing directly in AMBIENT
// here was the bug: AMBIENT's own idle timer would then close the temple
// again within about a minute of it just opening, with no chance to ever
// see STANDBY or test PIR at all.
void openTempleFromClosed() {
  // Accepts STANDBY as well as TEMPLE_CLOSED. It used to reject anything
  // but TEMPLE_CLOSED, which meant pressing "Open Temple" while the idol
  // sat in STANDBY - now the normal resting state after every mantra -
  // returned instantly: no bell, no state change, a button that looked
  // dead. That also left no way to wake the temple at all once the PIR
  // was disabled. Opening from STANDBY is the same welcoming gesture, so
  // it runs the same bell + welcome mantra.
  if (currentState != STATE_TEMPLE_CLOSED && currentState != STATE_STANDBY) return;

  offeringDisplayActive = false;
  offeringInterrupted = false;
  currentMoodTag[0] = '\0';
  freeBlessingImage();

  dfStop();
  delay(50);
  dfPlay(BELL_TRACK);
  delay(900); // let the bell ring out before the mantra starts

  dfStop();
  delay(50);
  dfPlay(mantraTracks[0].dfTrack);
  currentPlayingTrack = mantraTracks[0].dfTrack;

  blessingCounter++;
  feetDisplayTimer = millis();
  feetDisplayLocked = true;
  feetDisplayLockMs = 12000;
  strlcpy(scrollText, oledAmbientLoopText, sizeof(scrollText));
      strlcpy(scrollTextLang, "en", sizeof(scrollTextLang));
  setSystemState(STATE_MANTRA_ACTIVE, mantraTracks[0].duration);
}

void stopAudioAndStandby() {
  // Was completely silent - if a stray /api/control?action=stop ever
  // arrives from anywhere (a stale browser tab still running its own
  // local timers, a second dashboard session, anything hitting that URL)
  // it would force STANDBY with zero trace in the log, indistinguishable
  // from a genuine unexplained state revert. Reported on hardware: a
  // touch's mantra started correctly, then state was back at STANDBY
  // within under a second, with no error anywhere - this is exactly what
  // an untraced external stop would look like.
  Serial.printf("STOP: stopAudioAndStandby() called (state was %s)\n", stateName(currentState));
  dfStop();
  setSystemState(STATE_STANDBY);
}

// ==========================================
// LED Light Patterns (FastLED)
// ==========================================
// Pattern IDs match the web dashboard's pattern-select exactly (see
// web_dashboard.h drawLeds()/peacockWaveColor()/etc.) so /api/leds?pattern=N
// looks the same on the physical ring as it does in the browser simulation:
//   0 Peacock Wave, 1 Circuit Pulse, 2 Golden Aura, 3 Rainbow Dream, 4 Diya Flicker

// Sentiment-aware LED colors for an offering/wish-pad blessing - see
// currentMoodTag and MOODS/THEME_TO_MOOD in backend/functions/index.js.
// Six moods, kept as a small fixed set matched exactly to the backend's
// list so every value Claude/the theme mapping can produce has a real
// color here; anything unrecognized (empty string - mood not known yet
// or the backend call failed - or a future backend change this build
// predates) returns false and the caller keeps the plain saffron/gold
// default, same as before this feature existed.
// Redesigned after real hardware feedback: the original 6 pairs put
// hopeful/peaceful both in the blue family and joyful/grateful both in
// the gold family, so two moods often looked like "no difference at
// all" on the ring even when classified correctly. Every mood below now
// anchors on its own distinct hue family - gold/magenta, teal, purple,
// green, red/orange, warm amber-white - so any two moods are
// distinguishable at a glance, not just on paper.
bool moodColorsFor(const char *mood, CRGB &c1, CRGB &c2) {
  if (strcmp(mood, "joyful") == 0) {
    c1 = CRGB(255, 200, 0);   // bright gold
    c2 = CRGB(255, 20, 147);  // hot magenta - was a soft pink, too close to comforting's
  } else if (strcmp(mood, "hopeful") == 0) {
    c1 = CRGB(64, 224, 208);  // turquoise - was sky blue, too close to peaceful's
    c2 = CRGB(255, 255, 153); // pale yellow
  } else if (strcmp(mood, "comforting") == 0) {
    c1 = CRGB(147, 112, 219); // soft purple
    c2 = CRGB(221, 160, 221); // lavender/plum - was soft pink, too close to joyful's
  } else if (strcmp(mood, "peaceful") == 0) {
    c1 = CRGB(46, 139, 87);   // sea green - was deep sky blue, too close to hopeful's
    c2 = CRGB(152, 251, 152); // pale mint
  } else if (strcmp(mood, "empowering") == 0) {
    c1 = CRGB(220, 20, 60);   // crimson
    c2 = CRGB(255, 140, 0);   // orange
  } else if (strcmp(mood, "grateful") == 0) {
    c1 = CRGB(255, 191, 0);   // warm amber - was bright gold, too close to joyful's
    c2 = CRGB(255, 244, 214); // warm white
  } else {
    return false;
  }
  return true;
}

// r136: ties the AMBIENT ring's motion pattern to the last real blessing's
// mood, instead of Pattern sitting as a third fully independent manual
// pick alongside Theme/Atmosphere - direct follow-up after discussing
// that all three only ever affect the ring during the same narrow
// AMBIENT window. Reuses the same 6 moods as moodColorsFor() just above
// (see its own comment for why these six specifically); called from
// animateLeds() only when currentMoodTag is actually set - a fresh boot
// or a state that clears the tag (a plain mantra/feet touch, an offering
// resuming) falls back to the manual "AMBIENT LED Pattern" dropdown
// instead, same as before this existed.
int moodPatternFor(const char *mood) {
  if (strcmp(mood, "joyful") == 0)     return 3; // Rainbow Dream - vibrant, playful
  if (strcmp(mood, "hopeful") == 0)    return 0; // Peacock Wave - flowing, aspirational
  if (strcmp(mood, "comforting") == 0) return 2; // Golden Aura - gentle breathing
  if (strcmp(mood, "peaceful") == 0)   return 2; // Golden Aura - gentle breathing
  if (strcmp(mood, "empowering") == 0) return 1; // Circuit Pulse - energetic
  if (strcmp(mood, "grateful") == 0)   return 4; // Diya Flicker - warm devotional lamp
  return 0; // unrecognized - Peacock Wave, same default as currentPattern's own default
}

// V2 Phase 3: LED scene phases - Closure. One gentle brightness pulse in
// the blessing's own mood color (rises then eases back to dark, ~400ms),
// called from setSystemState() right before the existing generic
// playLedClosingSweep() fade - so a mood-driven blessing gets a clear,
// warm "signing off" cue instead of just fading from whatever colors
// happened to be showing. Deliberately narrow: only fires for a REAL
// mood, not a plain mantra/feet touch (saffron/gold, no distinct mood)
// - same reasoning as why that plain touch never picks up mood colors
// at all (r109: "a mantra touch should always feel the same and
// special"). No-op if the mood string isn't one of the 6 known moods.
void playMoodClosurePulse(const char *mood) {
  CRGB c1, c2;
  if (!moodColorsFor(mood, c1, c2)) return;
  for (int step = 0; step <= 20; step++) {
    uint8_t b = (uint8_t)(sinf(step / 20.0f * PI) * 255); // rises then eases back to 0
    for (int i = 0; i < NUM_LEDS; i++) {
      CRGB c = (i % 2 == 0) ? c1 : c2;
      c.nscale8(b);
      leds[i] = c;
    }
    FastLED.show();
    delay(20);
  }
}

// V2 Phase 6: Temple Atmosphere - a color wash layered on top of
// whatever Theme of the Day already picked for STATE_AMBIENT, not a
// replacement for it. Manual only (selectedAtmosphere, set via the
// dashboard's /api/settings?atmosphere=N) - see its declaration for why
// this deliberately isn't NTP/clock-driven.
void applyAtmosphere(CRGB &c1, CRGB &c2) {
  // r125: reported on hardware as "no visible effect" even while watching
  // the correct AMBIENT window - the original blend amounts (100/180/120
  // out of 255) left Theme of the Day's own colors dominant enough that
  // the wash read as a minor tint, not a distinct atmosphere. Pushed all
  // three well past the halfway point so the wash color clearly leads.
  switch (selectedAtmosphere) {
    case 1: // Evening - warm gold wash
      c1 = blend(c1, CRGB(255, 180, 60), 200);
      c2 = blend(c2, CRGB(255, 130, 20), 200);
      break;
    case 2: // Night - deep blue-teal, deliberately dim and quiet
      c1 = blend(c1, CRGB(10, 40, 60), 220);
      c2 = blend(c2, CRGB(10, 60, 70), 220);
      c1.nscale8(90);
      c2.nscale8(90);
      break;
    case 3: // Festival - vibrant gold+magenta wash, full energy
      c1 = blend(c1, CRGB(255, 200, 0), 210);
      c2 = blend(c2, CRGB(255, 0, 150), 210);
      break;
    default: // 0 = Day - Theme of the Day's own colors, unmodified
      break;
  }
}

// Mouse eye LED breathing - breathes gently (EYE_LED_BREATH_PERIOD_MS per
// cycle) for as long as eyeLedBreathingActive stays true, using the same
// single Green LED, no color change. Called every loop() pass (see loop()
// above), same non-blocking pattern as animateLeds().
// r125: the resting glow (EYE_LED_BASE_BRIGHTNESS, 80%) is now the SAME
// level as the top of this pulse, not its bottom - the eye is already at
// its brightest when a touch starts the chant, so there's no ramp-up to
// do; the pulse instead dips DOWN to EYE_LED_BREATH_LOW_BRIGHTNESS (10%)
// and back up to the 80% resting level, repeating for as long as the
// chant plays. The sine is phase-shifted (+PI/2) so brightness starts
// and ends each cycle exactly at the 80% resting level, with no jump
// when breathing starts or when setSystemState() takes back over once
// the chant ends. r138: period halved (3000ms -> EYE_LED_BREATH_PERIOD_MS,
// 1500ms) after direct feedback that the dip read as "very gradual".
void updateEyeLedBreathing() {
  if (!EYE_LED_CONNECTED) return;
  if (!eyeLedBreathingActive) return;
  unsigned long elapsed = millis() - eyeLedBreathingStartMs;
  const float lo = EYE_LED_BREATH_LOW_BRIGHTNESS;
  const float hi = EYE_LED_BASE_BRIGHTNESS;
  float phase = (elapsed / (float)EYE_LED_BREATH_PERIOD_MS) * 2.0f * PI + (PI / 2.0f);
  float mid = (lo + hi) / 2.0f;
  float amp = (hi - lo) / 2.0f;
  uint8_t brightness = (uint8_t)(mid + sinf(phase) * amp);
  ledcWrite(EYE_LED_PIN, brightness);
}

void animateLeds() {
  if (!LED_CONNECTED) return; // FastLED never initialised - see setup()
  if (!ledEnabled) return; // admin-disabled - blanked once already in the /api/settings handler
  if (currentState == STATE_STANDBY || currentState == STATE_TEMPLE_CLOSED) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    return;
  }

  // Opening bloom - plays for LED_OPENING_TRANSITION_MS right after
  // waking from closed/standby (set by setSystemState()), then falls
  // through to the state's normal pattern below once it's done.
  if (ledOpeningTransitionStart != 0) {
    unsigned long elapsed = millis() - ledOpeningTransitionStart;
    if (elapsed < LED_OPENING_TRANSITION_MS) {
      float progress = elapsed / (float)LED_OPENING_TRANSITION_MS;
      uint8_t brightness = (uint8_t)(sinf(progress * PI) * 255); // rises then eases back down
      CRGB bloom = CRGB(255, 223, 130);
      bloom.nscale8(brightness);
      fill_solid(leds, NUM_LEDS, bloom);
      FastLED.show();
      return;
    }
    ledOpeningTransitionStart = 0; // done - normal per-state rendering resumes below
  }

  // Continuously-growing float, not the wrapping uint8_t hueOffset used
  // elsewhere - these patterns feed it straight into sin(), and a uint8_t
  // wrap would show up as a visible glitch once per cycle.
  static float animHue = 0;

  if (currentState == STATE_AARTI) {
    // Dedicated flame palette with a scripted intensity arc over the
    // chant's known duration, rather than reacting to live audio (true
    // tempo-matching needs the mic, parked for now) - builds toward a
    // peak partway through, eases back for the close.
    float aartiProgress = constrain((millis() - stateTimer) / (float)stateDuration, 0.0f, 1.0f);
    float intensity = sinf(aartiProgress * PI * 0.9f);
    animHue += 1.0f + intensity * 2.5f; // faster pulse at the peak, calmer at start/end
  } else {
    animHue += 1.5f;
  }

  if (currentState == STATE_AMBIENT || currentState == STATE_MANTRA_ACTIVE || currentState == STATE_FEET_ACTIVE || currentState == STATE_AARTI) {
    CRGB c1, c2;
    if (currentState == STATE_MANTRA_ACTIVE || currentState == STATE_FEET_ACTIVE) {
      // A plain feet/mouse-back touch mantra always shows the same warm
      // saffron/gold - "a mantra touch should always feel the same and
      // special". An offering or wish-pad blessing (offeringDisplayActive)
      // is different: if a real mood came back with it, show that
      // instead, so the LED reflects what was actually prayed for rather
      // than looking identical regardless of the blessing's content.
      bool usedMoodColor = false;
      if (offeringDisplayActive && currentMoodTag[0] != '\0') {
        usedMoodColor = moodColorsFor(currentMoodTag, c1, c2);
      }
      if (!usedMoodColor) {
        c1 = CRGB(255, 140, 0);  // saffron
        c2 = CRGB(255, 215, 0);  // gold
      }
    } else if (currentState == STATE_AARTI) {
      c1 = CRGB(180, 20, 0);   // deep ember red
      c2 = CRGB(255, 120, 0);  // bright flame orange
    } else {
      switch(selectedTheme) {
        case 0: c1 = CRGB(128, 0, 32);   c2 = CRGB(255, 215, 0); break;
        case 1: c1 = CRGB(0, 242, 254);  c2 = CRGB(79, 172, 254); break;
        case 2: c1 = CRGB(0, 180, 219);  c2 = CRGB(0, 255, 135); break;
        case 3: c1 = CRGB(255, 215, 0);  c2 = CRGB(255, 100, 0);   break;
        case 4: c1 = CRGB(236, 0, 140);  c2 = CRGB(185, 43, 39);   break;
        case 5: c1 = CRGB(75, 0, 130);   c2 = CRGB(79, 172, 254); break;
        case 6: c1 = CRGB(255, 75, 75);  c2 = CRGB(255, 215, 0); break;
      }
      applyAtmosphere(c1, c2); // V2 Phase 6 - AMBIENT only, doesn't touch Mantra/Feet/Aarti/mood colors above
    }

    // Same state->pattern overrides as the dashboard: Feet touch always
    // shows Peacock Wave, Mantra/Aarti chanting always shows Circuit Pulse.
    // Only AMBIENT actually reflects a chosen pattern - r136: and even
    // then, prefers the last real blessing's mood (moodPatternFor()) over
    // the manual "AMBIENT LED Pattern" dropdown, which is now just the
    // fallback for before any blessing has happened yet.
    int activePattern = currentPattern;
    if (currentState == STATE_FEET_ACTIVE) {
      activePattern = 0;
    } else if (currentState == STATE_MANTRA_ACTIVE || currentState == STATE_AARTI) {
      activePattern = 1;
    } else if (currentMoodTag[0] != '\0') {
      activePattern = moodPatternFor(currentMoodTag);
    }

    for (int i = 0; i < NUM_LEDS; i++) {
      switch (activePattern) {
        case 0: { // Peacock Wave - traveling sine blend between c1/c2
          float wave = (sinf(animHue * 0.04f + i * (2.0f * PI / NUM_LEDS)) + 1.0f) / 2.0f;
          leds[i] = blend(c1, c2, (uint8_t)(wave * 255));
          break;
        }
        case 2: { // Golden Aura - uniform 60/40 c1/c2 blend, breathing
          // Reported on hardware as "not happening" - the original 0.2-1.0
          // brightness swing over ~1.4s was real but too subtle to read as
          // an animation against a bright blend. Widened to a near-black
          // trough and sped up so it's unmistakable within a second or two.
          float breath = sinf(animHue * 0.045f) * 0.5f + 0.5f;
          leds[i] = blend(c1, c2, 102); // 0.4 * 255
          leds[i].nscale8_video((uint8_t)(breath * 255));
          break;
        }
        case 3: { // Rainbow Dream - full-saturation rotating rainbow
          uint8_t hue8 = (uint8_t)((long)animHue + (i * 256 / NUM_LEDS));
          leds[i] = CHSV(hue8, 255, 255);
          break;
        }
        case 4: { // Diya Flicker - warm oil-lamp flicker, ignores theme on purpose
          float base = 0.55f + sinf(animHue * 0.07f + i * 1.3f) * 0.15f;
          float flicker = (random(0, 100) / 100.0f) * 0.25f;
          float intensity = constrain(base + flicker, 0.15f, 1.0f);
          leds[i] = CRGB((uint8_t)(255 * intensity), (uint8_t)(140 * intensity * 0.75f), (uint8_t)(20 * intensity * 0.3f));
          break;
        }
        case 1: // Circuit Pulse
        default: {
          float breath = sinf(animHue * 0.05f) * 0.5f + 0.5f;
          leds[i] = c1;
          leds[i].nscale8_video((uint8_t)(breath * 255));
          break;
        }
      }
    }

    // Circuit Pulse's occasional single-LED spark to c2
    if (activePattern == 1 && random(0, 100) < 5) {
      leds[random(0, NUM_LEDS)] = c2;
    }

    FastLED.show();
    return;
  }
}

// ==========================================
// OLED Text Drawing & Scrolling (U8g2)
// ==========================================
void drawOLED() {
  if (!displayEnabled) return; // admin-disabled - blanked once already in the /api/settings handler

  // Hardware test card (/api/test?oled=1) outranks everything, including
  // the states where the screen is normally off - its whole job is to
  // answer "is this panel alive?" with the most visible image possible.
  if (oledTestUntil != 0) {
    if (millis() < oledTestUntil) {
      u8g2.clearBuffer();
      u8g2.drawBox(0, 0, 128, 64);          // solid white, edge to edge
      u8g2.setDrawColor(0);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(19, 36, "DISPLAY WORKS");
      u8g2.setDrawColor(1);
      u8g2.sendBuffer();
      return;
    }
    oledTestUntil = 0;                       // test over - blank once, resume
    u8g2.clearBuffer();
    u8g2.sendBuffer();
  }

  if (currentState == STATE_STANDBY || currentState == STATE_TEMPLE_CLOSED) return;

  unsigned long now = millis();
  static unsigned long lastOledDraw = 0;
  if (now - lastOledDraw < 33) return;
  lastOledDraw = now;

  u8g2.clearBuffer();

  // Small state tag, top-right corner only. The old header band
  // ("GANAPATI AI 2026") and the "Devotional Hits" footer used to box the
  // blessing text into a 34px-tall strip drawn at 6x12 - unreadable from
  // more than arm's length on a screen this small. Both are dropped so
  // the blessing text below can use almost the full 64px height instead.
  // r126: real bug fix, not just a rename - STATE_FEET_ACTIVE was falling
  // into the same "MANTRA" tag as STATE_MANTRA_ACTIVE, so a feet touch and
  // a mouse-back touch were visually indistinguishable on the physical
  // OLED (reported directly: mouse-back touch looked like it said "mantra
  // active" instead of its own thing). Each real touch source now gets
  // its own tag; wishPadBlessingActive (see its declaration comment)
  // distinguishes a wish-pad blessing from a real feet touch even though
  // both share STATE_FEET_ACTIVE's underlying timing.
  u8g2.setFont(u8g2_font_5x7_tf);
  const char* tag = "";
  if (currentState == STATE_AMBIENT) tag = "AMBIENT";
  else if (currentState == STATE_MANTRA_ACTIVE) tag = "MOUSE BACK";
  else if (currentState == STATE_FEET_ACTIVE) tag = wishPadBlessingActive ? "WISH" : "FEET";
  else if (currentState == STATE_AARTI) tag = "AARTI";
  int tagWidth = u8g2.getStrWidth(tag);
  u8g2.drawStr(124 - tagWidth, 8, tag);

  // The actual content devotees read - now large enough to be legible
  // from across a room instead of squinting at a small font.
  //
  // blessingImageBuf takes over completely when set (see
  // fetchBlessingImage()/renderTextToXbm() in backend/functions/
  // index.js): U8g2 has no text-shaping engine, so even the correct
  // dedicated font per script (indic_fonts.h) only draws isolated
  // glyphs, never reordering a vowel mark or fusing a conjunct -
  // confirmed on hardware as real words looking "broken and not
  // complete". The backend now renders the text into a correctly-
  // shaped bitmap instead (verified against real shaping: matra
  // reordering and conjunct ligatures both confirmed correct before
  // this was wired in), so the ESP32's job here shrinks to "scroll
  // through this picture", the same drawXBMP() call U8g2 already
  // supports natively - no per-script font selection needed for
  // anything blessingImageBuf covers.
  if (blessingImageBuf != NULL) {
    u8g2.drawXBMP(blessingImageScrollX, 20, blessingImageWidth, blessingImageHeight, blessingImageBuf);
    if (now - lastScrollUpdate > TEXT_SCROLL_SPD) {
      lastScrollUpdate = now;
      blessingImageScrollX -= SCROLL_PX_PER_STEP;
      if (blessingImageScrollX < -blessingImageWidth) {
        blessingImageScrollX = 128;
        scrollPassComplete = true; // same "finished one pass" signal the text path uses - updateStateMachine()'s offering-done timing relies on it regardless of which path is active
      }
    }
    u8g2.sendBuffer();
    return;
  }

  // Font is chosen per scrollTextLang, not fixed - u8g2_font_logisoso20_tf
  // (like every "normal" u8g2 font) only has glyphs for Latin script, so
  // non-Latin text drawn with it just shows nothing for every character.
  // This path (no image, plain scrollText) now only actually runs for
  // English text, or the rare case blessingImageBuf failed to arrive
  // (network hiccup) - the per-script fonts below are kept as a graceful
  // partial fallback for that case rather than showing nothing, even
  // though they don't shape correctly on their own (see indic_fonts.h's
  // header comment) - some legible letters beat none while a retry
  // would come too late to matter for this display cycle.
  const uint8_t *scrollFont = u8g2_font_logisoso20_tf;
  const char *displayText = scrollText;
  const char *OLED_NO_FONT_MSG = "   Blessing spoken - see dashboard for text   ";
  if (strcmp(scrollTextLang, "hi") == 0 || strcmp(scrollTextLang, "mr") == 0) {
    scrollFont = u8g2_font_notosansdevanagari16_t;
  } else if (strcmp(scrollTextLang, "ml") == 0) {
    scrollFont = u8g2_font_notosansmalayalam16_t;
  } else if (strcmp(scrollTextLang, "ta") == 0) {
    scrollFont = u8g2_font_notosanstamil16_t;
  } else if (strcmp(scrollTextLang, "te") == 0) {
    scrollFont = u8g2_font_notosanstelugu16_t;
  } else if (strcmp(scrollTextLang, "gu") == 0) {
    scrollFont = u8g2_font_notosansgujarati16_t;
  } else if (strcmp(scrollTextLang, "pa") == 0) {
    scrollFont = u8g2_font_notosansgurmukhi16_t;
  } else if (strcmp(scrollTextLang, "bn") == 0) {
    scrollFont = u8g2_font_notosansbengali16_t;
  } else if (strcmp(scrollTextLang, "sd") == 0 || strcmp(scrollTextLang, "fa") == 0) {
    displayText = OLED_NO_FONT_MSG; // not in the dropdown yet, but stay safe if ever selected
  }
  u8g2.setFont(scrollFont);

  int textWidth = u8g2.getUTF8Width(displayText);
  u8g2.drawUTF8(scrollX, 46, displayText);

  if (now - lastScrollUpdate > TEXT_SCROLL_SPD) {
    lastScrollUpdate = now;
    scrollX -= SCROLL_PX_PER_STEP; // see TEXT_SCROLL_SPD note in config.h
    if (scrollX < -textWidth) {
      scrollX = 128;
      scrollPassComplete = true;
    }
  }

  u8g2.sendBuffer();
}