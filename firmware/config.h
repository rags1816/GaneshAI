#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Bump this any time GanapatiAI.ino/config.h/web_dashboard.h change - the
// boot log prints it, so the Serial Monitor is the definitive proof of
// what's actually flashed on the board, independent of any download or
// browser-cache issue on the file-sync side.
#define FIRMWARE_VERSION "2026-09-01-r169"

// ==========================================
// Hardware Pin Definitions
// ==========================================

// Sensors
// PIR moved 13 -> 19. GPIO13 doubles as JTAG MTCK and stayed noisy even
// after the sensor and wiring were replaced. GPIO19 is a plain
// bidirectional GPIO with no strapping, JTAG, or flash role and nothing
// else on this build uses it.
#define PIR_PIN          19   // AM312 Motion Sensor Pin (was 13 = JTAG MTCK)

// ENABLED - the sensor was never faulty. Every earlier "dead PIR" reading
// (on GPIO13 AND on a fresh GPIO19, ~27-40% floating duty each time) was
// this specific compact AM312 module being wired in the WRONG PIN ORDER.
// Unlike the larger HC-SR501-style PIR boards, this module's own
// silkscreen reads, left to right on the back with the dome up:
//   GND - OUT - VIN
// not the VIN/GND/OUT order assumed earlier. OUT (middle pin) -> D19 was
// always right; GND and VIN just needed to land on the correct outer
// pins. Confirmed working on SensorBench once wired to the silkscreen:
// clean 0%-quiet idle, 100%-HELD-HIGH on motion, clean pulse counts.
#define PIR_CONNECTED    true

// Touch pads (TP223 modules). Feet was originally GPIO12 (MTDI /
// VDD_SDIO strapping pin) - with the sensor unplugged that pin floats,
// and if noise pulls it HIGH at reset the ESP32 selects the wrong flash
// voltage and hangs before setup() ever runs (no Serial, no OLED). Moved
// to GPIO27, a plain non-strapping input.
#define TOUCH_FEET_PIN   27   // Touch Sensor 2: Idol Feet (Alternating Mantra trigger)

// Mouse-back moved 15 -> 23. GPIO15 is the MTDO strapping pin - the same
// class of pin the feet pad was moved off (GPIO12) for the same reason.
// It never triggered on GPIO15 even with its flag enabled, and there is
// corroborating evidence in the boot log: GPIO15 held LOW at reset
// SUPPRESSES the ESP32 boot messages, yet the boot log printed normally,
// so that pin was not being driven low by an idle TP223 as it should have
// been. GPIO23 is a plain GPIO with no strapping, JTAG, or flash role.
// REWIRE the mouse-back TP223's OUT lead from D15 to D23.
#define TOUCH_BACK_PIN   23   // Touch Sensor 3: Mouse Back Stroke (was 15 = MTDO strapping pin)

// Both pads are physically wired, so both stay enabled here. Turning one
// off should only ever be a deliberate act (an unwired pin floats and
// fires phantom touches); leaving them false by default was itself
// generating false "the pad is dead" reports, which cost more time than
// the phantom touches ever did.
#define TOUCH_FEET_CONNECTED   true
#define TOUCH_BACK_CONNECTED   true

// Wish Pad (TP223) - a devotee touching this at the altar, the way one
// would touch a deity's feet, triggers a fresh Claude-generated blessing
// spoken through the amp (see triggerWishPadBlessing() in GanapatiAI.ino)
// - no offering, no typed/spoken wish, just a silent prayer acknowledged
// aloud. GPIO4 reclaimed from the OLED_SPI_DIN reservation (genuinely
// unused - this build runs OLED_SH1106_I2C, not the SPI variant).
// FALSE by default like every other not-yet-wired sensor in this file -
// an unwired pin floats and fires phantom touches, which reads as "the
// pad is dead" and costs more time than the phantom touches would.
// Set TRUE once the TP223's OUT lead is actually wired to D4.
#define WISH_PAD_PIN        4
#define WISH_PAD_CONNECTED  true

// Mouse Eye LEDs (fiber-optic coupled) - two high-intensity 5mm LEDs at
// the altar base, each butted against its own 2mm end-glow fiber running
// into one of the mouse idol's eyes; the idol carries only bare fiber,
// nothing electrical (see the mouse-eye-led diagram from this build's
// planning). Both LED anodes (each through its own 100ohm resistor - see
// EYE_LED_RESISTOR_OHMS, r147) tie to this single GPIO; cathodes share
// GND. GPIO16 is printed "RX2" on
// most DevKit boards, not "D16" - wire to the pin marked RX2. Genuinely
// free: every other GPIO is already claimed (see the amp/mic/touch/OLED
// definitions above and below), and 16 was only ever reserved for the
// SPI-wired OLED mode, which this build does not use.
// Bench-tested and confirmed working (Green, 100ohm, GPIO16) - wired into
// the sketch's state machine ahead of the idol build finishing, at the
// user's explicit request. Lights for the duration of the mouse-back
// chant (triggerMantra()) only - not the feet mantra, not the wish pad,
// not the temple-reopening welcome mantra (openTempleFromClosed() also
// uses STATE_MANTRA_ACTIVE, but never turns this pin on). See
// setSystemState() (turns it off on every state change, unconditionally)
// and triggerMantra() (the only place that turns it on) below.
#define EYE_LED_PIN         16   // printed "RX2" on the board silkscreen
// r121: restored to true for a clean re-test - after r120's rollback,
// the user discovered the ESP32 itself was not fully/properly seated in
// its socket, likely since the last physical handling around the r119
// flash. A loose board seating is a real, independent, equally-plausible
// explanation for r119's broad "everything stopped working" symptom
// (intermittent contact affecting multiple pins/peripherals at once),
// separate from the ledcAttach()/GPIO16 theory in r120's comment above.
// Re-testing with a confirmed-solid seating, on the live device, is the
// only way to tell which explanation was actually correct. If the same
// broad breakage recurs even with solid seating, revert to false
// immediately and treat r120's ledcAttach/GPIO16 theory as confirmed.
#define EYE_LED_CONNECTED   true

// Breathing animation (not a flat on/off) - see updateEyeLedBreathing()
// in GanapatiAI.ino. Driven via the ESP32's LEDC PWM peripheral instead
// of plain digitalWrite() so brightness can fade smoothly rather than
// snap on/off, matching the calm/no-abrupt-transition feel used
// elsewhere (the mood LED closure pulse, the generic fade sweep).
// Confirmed on the user's installed ESP32 board package (3.1.3, core
// 3.x): that core replaced the old channel-based ledcSetup()/
// ledcAttachPin()/ledcWrite(channel, ...) API with a single
// ledcAttach(pin, freq, resolution) call, and ledcWrite()/ledcRead() now
// take the PIN number directly instead of a channel number - no
// EYE_LED_PWM_CHANNEL constant needed at all on this core.
#define EYE_LED_PWM_FREQ_HZ  5000
#define EYE_LED_PWM_RESOLUTION  8   // 8-bit: brightness range 0-255

// Confirmed working on real hardware, then refined per direct feedback:
// the eyes should never go fully dark - a dim, always-on glow, brighter
// breathing pulse only while the mouse-back chant plays, settling back
// to the same dim glow afterward (not off). r125: the ~10% resting glow
// (old EYE_LED_BASE_BRIGHTNESS=25) read as too dim on the physical fiber
// runs, so the resting glow is now raised to 80% - EYE_LED_BASE_BRIGHTNESS
// is that steady resting level AND the top of the breathing pulse (the
// eye is already at its brightest at rest, so a touch dips it down rather
// than raising it further); EYE_LED_BREATH_LOW_BRIGHTNESS (10%) is the
// bottom of that dip, replacing the old EYE_LED_PEAK_BRIGHTNESS role.
#define EYE_LED_BASE_BRIGHTNESS       204
#define EYE_LED_BREATH_LOW_BRIGHTNESS  26
// r138: reported on hardware as "very gradual" at the original 3000ms full
// cycle - halved so the 80%->10%->80% dip reads as a clear pulse instead
// of a slow drift. Raise this back toward 3000 if it now feels too fast.
#define EYE_LED_BREATH_PERIOD_MS     1500

// r145: power draw ESTIMATE assumptions - this board has no current/power
// sensor at all, so none of this is measured. The LED ring figure is
// real math against the ring's actual current pixel colors/brightness
// (FastLED's own power model, calculate_unscaled_power_mW() - genuinely
// accurate for the ring specifically). Everything else below is a fixed
// datasheet-typical constant, since there's no way to measure the
// ESP32/OLED/DFPlayer+amp/eye-LEDs' real draw without adding real
// hardware (an INA219/INA226 current sensor would give real numbers -
// not present on this build). Correct these constants if you know your
// actual parts' real specs; see getEstimatedPowerMw() in GanapatiAI.ino.
#define POWER_SUPPLY_VOLTS          5.0  // assumed main rail (USB/adapter) everything below is estimated against
#define POWER_ESP32_BASE_MW         700  // ESP32 dev board, Wi-Fi connected, typical - real Wi-Fi TX bursts spike well above this momentarily
#define POWER_OLED_MW               75   // SSD1309 128x64 monochrome, typical mixed content
#define POWER_SENSORS_MW            5    // 3x TP223 touch pads + AM312 PIR combined - each individually negligible
#define POWER_DFPLAYER_IDLE_MW      100  // DFPlayer Mini, not currently playing
#define POWER_DFPLAYER_PLAYING_MW   500  // DFPlayer Mini + I2S amp + speaker, typical mid-volume while playing - real draw varies a lot with volume/content
// Mouse eye LEDs: computed live from the actual PWM duty (ledcRead), not
// fixed - but still needs these assumed electrical specs. Two LEDs run
// in parallel off the single EYE_LED_PIN GPIO (see its own wiring
// comment), each through its own resistor.
// r147: corrected from an assumed 120 to the real wired value, confirmed
// by the user (100ohm per LED, both resistors' other ends tied together
// to EYE_LED_PIN) - matches r108's original bench-test note ("Green,
// 100ohm"), which the wiring comment on EYE_LED_PIN above had drifted
// from at some point.
#define EYE_LED_RESISTOR_OHMS       100  // per LED
// r148: corrected again, from the r146 datasheet-rated-current figure
// (3.1V, only valid AT the LED's rated 20mA test current) to a REAL
// measured value - a diode's Vf drops well below its rated-current spec
// at much lower actual currents (exponential I-V curve, not a fixed
// drop). Measured on a spare/isolated ESP32+LED-only bench rig running
// this exact PWM code at its 80% resting duty: 0.31V across the 100ohm
// resistor -> 3.1mA average -> 3.875mA on-state (peak) current once
// corrected for the 80% duty -> Vf = 3.3V - (3.875mA * 100ohm) = 2.91V.
// See this version's CLAUDE.md entry for the full worked numbers,
// including an earlier arithmetic slip (dividing by duty cycle twice)
// that this corrects. r149: confirmed to be a real property of the LED
// itself, not the old circuit - a second, independent measurement on
// the new 5V+transistor circuit (below) backed out the same 2.91V.
// Re-measure and correct again if a different-spec LED is substituted.
#define EYE_LED_FORWARD_VOLTAGE     2.91
#define EYE_LED_GPIO_VOLTAGE        3.3  // ESP32 GPIO high level - no longer the eye LEDs' supply as of r149, see below; kept as it's still a true fact about the pin, used for its base-drive role

// r149: the eye LED branch moved off GPIO16-as-power-source onto its own
// NPN transistor (2N2222) low-side switch fed from the 5V rail, so the
// LEDs could actually reach a meaningful fraction of their 20mA rating
// (r146/r147 found only ~2-4mA was possible straight off a 3.3V GPIO,
// regardless of resistor choice - not enough voltage headroom above the
// LED's own ~2.9V Vf). GPIO16 now only drives the transistor's base
// through EYE_LED_BASE_RESISTOR_OHMS - ledcRead(EYE_LED_PIN) still
// reads the same PWM duty as before, since GPIO16 itself is unchanged,
// only what it's driving changed. EYE_LED_TRANSISTOR_VCE_SAT is a
// typical 2N2222 saturation voltage at these low currents, not measured
// separately - the LED math above already backs out the same Vf via
// this assumption, which is the real cross-check that it's reasonable.
#define EYE_LED_SUPPLY_VOLTAGE       5.0
#define EYE_LED_TRANSISTOR_VCE_SAT   0.2
#define EYE_LED_BASE_RESISTOR_OHMS   1000

// NeoPixel LEDs
#define LED_PIN          18   // WS2812B NeoPixel Data Pin
#define NUM_LEDS         24   // 24 LEDs in the ring

// FALSE while the ring is physically disconnected. When false, FastLED is
// never initialised and never called - the boot log jumps straight past it.
//
// (An earlier version of this comment blamed FastLED for a repeating
// "...itself is hanging" log line. That was wrong - the line is our own
// DFPlayer retry message in setup(), truncated by the Serial Monitor.
// FastLED was never implicated. This flag is kept purely because there is
// no point driving a ring that is not attached, and it trims ~28KB flash.)
//
// Set true ONLY when the ring is reconnected - and give it its own 5V
// supply, never the ESP32 board's 5V pin: 24 WS2812B pixels can pull ~1.4A,
// far past what the board's regulator and USB path can deliver, which
// browns out the chip into a reboot loop.
#define LED_CONNECTED    true

// Hard current budget for the ring, in milliamps at 5V. FastLED enforces
// this by scaling brightness down automatically whenever the frame it is
// about to draw would exceed it - so the ring simply renders dimmer
// instead of browning out its supply. This makes a modest supply safe
// rather than merely hopeful.
//
// Set for a dedicated 5V 3A switching adapter, which is what this build
// uses. LOWER IT if you ever run the ring from anything weaker:
//   5V 3A switching adapter (this build)               -> 1500
//   HW-131 / MB102 breadboard supply (AMS1117 linear)  -> 450
//   USB power                                          -> do not run the
//     ring at all; USB's ~500mA cannot start 24 pixels and the resulting
//     brownout resets the ESP32 into a reboot loop.
// 24 WS2812B at full white draw ~1400mA, so 1500 leaves the ring
// unrestricted on a 3A supply while still capping a runaway frame.
#define LED_MAX_MILLIAMPS  1500

// ==========================================
// OLED Display
// ==========================================
// Two physical displays are supported. Pick ONE with OLED_MODEL below.
// BOTH panels are 128x64 pixels, so every screen layout, font, margin and
// scroll speed in the sketch is unchanged - the 2.42" is simply a bigger
// piece of glass showing the same picture, readable across a room instead
// of at arm's length.
//
//   OLED_SH1106_I2C   the original 1.3" module. 4 wires, D21/D22.
//   OLED_SSD1309_SPI  the Waveshare 2.42" module AS IT SHIPS (4-wire SPI).
//                     7 wires, but nothing to solder on the module itself.
//   OLED_SSD1309_I2C  the Waveshare 2.42" module AFTER its SPI/I2C jumper
//                     has been moved to I2C. Then it wires up exactly like
//                     the 1.3" did - 4 wires, D21/D22 - and frees the five
//                     SPI pins for the future amp/wish-pad.
//
// Check the SPI/I2C jumper printed on the back of the Waveshare board and
// set this to match it. Getting this wrong shows a blank screen and
// nothing else - it cannot damage anything.
#define OLED_SH1106_I2C   0
#define OLED_SSD1309_SPI  1
#define OLED_SSD1309_I2C  2

#define OLED_MODEL       OLED_SSD1309_I2C

// SSD1309 panels ship with one of two init sequences. If the 2.42" comes
// up blank, garbled, or shifted sideways by a few pixels, change this 0
// to a 2 and reflash - that is the whole fix, and it is the FIRST thing
// to try before suspecting the wiring.
#define OLED_SSD1309_VARIANT  0

// I2C pins - used by OLED_SH1106_I2C and OLED_SSD1309_I2C only.
#define OLED_SDA         21   // I2C Data Pin  (module pin marked SDA / DIN)
#define OLED_SCL         22   // I2C Clock Pin (module pin marked SCL / CLK)

// SPI pins - used by OLED_SSD1309_SPI only.
// Deliberately NOT the ESP32's hardware-SPI pins: those are GPIO18 (taken
// by the LED ring) and GPIO23 (taken by the mouse-back pad). Software SPI
// runs on any pins, and a 128x64 mono panel does not need the speed - a
// full-screen update costs ~8ms against a ~40ms frame.
//
// All five are on the DevKit's RIGHT-HAND column, on purpose. The obvious
// choices were D13 and D14, but on a 30-pin DevKit that column ends
// "...D27, D14, D12, D13, GND, VIN" - so D12 sits BETWEEN them. GPIO12 is
// the strapping pin that picks the flash voltage at reset: a wire pulling
// it high hangs the chip before setup() runs, with no serial and no
// display, looking exactly like a dead board. This build has already lost
// a day to that once. Keeping D12's neighbours empty means a miscounted
// hole cannot reproduce it.
//
// WATCH THE BOARD LABELS: GPIO16 and GPIO17 are NOT printed "D16"/"D17" on
// a DevKit. They are printed "RX2" and "TX2" - the board shows their
// Serial-2 role, not their number. There is no conflict with the DFPlayer,
// which also uses Serial 2: the ESP32 can route a UART to any pins, and
// Serial2 is pointed at GPIO25/26 below, leaving these two free.
// NO LONGER USABLE AS WRITTEN: D4 and D5 have since been reclaimed by
// the wish pad (WISH_PAD_PIN) and the reserved mic (I2S_MIC_WS)
// respectively, now that the display being used is the 4-pin I2C-only
// SSD1309 module (no SPI capability at all, so this mode was never
// going to be needed with it). Left defined for reference only - do NOT
// select OLED_SSD1309_SPI without first re-picking free pins here.
#define OLED_SPI_CLK     22   // module pin CLK -> board pin D22
#define OLED_SPI_DIN      4   // module pin DIN -> board pin D4 (CONFLICTS with WISH_PAD_PIN)
#define OLED_SPI_CS       5   // module pin CS  -> board pin D5 (CONFLICTS with I2S_MIC_WS)
#define OLED_SPI_DC      17   // module pin DC  -> board pin printed TX2
#define OLED_SPI_RST     16   // module pin RST -> board pin printed RX2

// DFPlayer Mini MP3 Player
#define DFPLAYER_RX      25   // Connect to DFPlayer TX
#define DFPLAYER_TX      26   // Connect to DFPlayer RX (via 1k resistor)

// I2S Amp (MAX98357A) - speaks the AI blessing text through the altar's
// own speaker when a priest approves an offering (see speakBlessingOnAmp()
// in GanapatiAI.ino). Every other GPIO is already spoken for on this
// board (DFPlayer on 25/26, touch pads on 4/23/27, PIR on 19, LED on 18,
// OLED on 21/22) - 13 and 14 are the only genuinely free general-purpose
// pins left, so DIN reclaims one pin (32) from the old I2S mic
// reservation below. CONFIRM these are actually free on your current
// board before wiring - swap them here if not.
#define AMP_BCLK_PIN     13   // module pin BCLK
#define AMP_LRC_PIN      14   // module pin LRC (Word Select)
#define AMP_DIN_PIN      32   // module pin DIN (reclaimed from the I2S mic reservation)

// CONFIRMED ON HARDWARE - two things beyond the three pins above, or the
// amp powers up, receives valid I2S data, and stays completely silent
// (this cost hours across TWO separate amp modules before being found):
//   1. Module's SD (shutdown) pin -> tie to VIN. Unlike GAIN, a floating
//      SD pin has no defined behavior on generic/clone MAX98357A
//      breakouts (only Adafruit's ties it internally) - floating, it
//      commonly settles into shutdown/muted regardless of good power and
//      good I2S data. GAIN can stay floating - that's an intentional
//      documented default (~9dB), not a problem.
//   2. Module's GND -> also wire directly to an ESP32 GND pin, not only
//      to the external adapter's ground via the WAGO. BCLK/LRC/DIN are
//      logic signals referenced to a shared ground; power alone (VIN)
//      can read a perfectly good 5V on a meter even when the adapter's
//      ground isn't actually the same electrical node as the ESP32's.

// I2S Microphone (INMP441) - RESERVED, NOT YET USED BY ANY CODE.
// These three numbers are a plan, not a driver: nothing in the sketch
// reads them, so wiring the mic up today changes nothing and proves
// nothing. They are kept here so the pins stay reserved and are not
// handed out to something else.
// The INMP441 has SIX pads: VDD -> 3V3, GND -> GND, plus L/R -> GND
// (L/R selects which stereo half it speaks on; tied to GND = left, which
// is what the ESP32 side will expect).
//
// REAL BUG FOUND while assembling the final pin plan: WS used to be
// GPIO32 here, the exact pin AMP_DIN_PIN above now also uses - the amp
// correctly reclaimed it, but this definition was never updated to
// match, so it was silently still claiming an already-used pin. Moved
// WS to GPIO5 (genuinely free - it was only ever reserved for the
// SPI-mode OLED, which this build doesn't use with an I2C-only display).
#define I2S_MIC_WS        5   // module pad WS  (Word Select / LRC) - moved from 32, which AMP_DIN_PIN now uses
#define I2S_MIC_SCK      33   // module pad SCK (Serial Clock / BCLK)
#define I2S_MIC_SD       34   // module pad SD  (Serial Data) - input-only pin, correct for a mic

// CONFIRMED ON HARDWARE - two things beyond the three pins above, or the
// amp powers up, receives valid I2S data, and stays completely silent
// (this cost hours across TWO separate amp modules before being found):
//   1. Module's SD (shutdown) pin -> tie to VIN. Unlike GAIN, a floating
//      SD pin has no defined behavior on generic/clone MAX98357A
//      breakouts (only Adafruit's ties it internally) - floating, it
//      commonly settles into shutdown/muted regardless of good power and
//      good I2S data. GAIN can stay floating - that's an intentional
//      documented default (~9dB), not a problem.
//   2. Module's GND -> also wire directly to an ESP32 GND pin, not only
//      to the external adapter's ground via the WAGO. BCLK/LRC/DIN are
//      logic signals referenced to a shared ground; power alone (VIN)
//      can read a perfectly good 5V on a meter even when the adapter's
//      ground isn't actually the same electrical node as the ESP32's.

// ==========================================
// Software & Timing Configurations
// ==========================================

// Home Wi-Fi Network Settings
#define WIFI_SSID        "VM3003995_Ext"
#define WIFI_PASSWORD    "c7kQrnnrdqnf"

// r159: backup network (phone hotspot) - tried automatically if the
// preferred network fails to connect at boot, and selectable as the
// PREFERRED network from the dashboard (see /api/control?action=wifiswitch
// in GanapatiAI.ino). Must be 2.4GHz - the ESP32 cannot join a 5GHz
// network at all.
#define WIFI_SSID_BACKUP     "AndroidAP9715"
#define WIFI_PASSWORD_BACKUP "21Vihana"

// r154: heap fragmentation / low-memory self-healing thresholds - see
// checkHeapHealth()/checkScheduledRestart() in GanapatiAI.ino. A restart
// is the actual fix (the heap allocator can't defragment memory in
// place, only a clean boot resets it to one large contiguous block) -
// these just decide WHEN that's warranted, never interrupting an active
// blessing/mantra/Aarti to do it.
#define HEAP_FRAGMENTATION_WARN_RATIO 0.5      // largest free block < 50% of total free heap = fragmented
#define HEAP_CRITICAL_FREE_BYTES     30000     // absolute low-memory floor, regardless of fragmentation
#define AUTO_RESTART_UPTIME_MS       86400000UL // 24h - routine preventive restart, idle-gated only (no NTP/wall-clock needed)

// Timing (in milliseconds)
#define AMBIENT_TIMEOUT  60000  // AMBIENT idle time before auto-triggering the closing Aarti (matches web dashboard's AARTI_IDLE_MS)
#define MOTION_DEBOUNCE  3000   // Prevent rapid PIR re-triggers
#define TOUCH_DEBOUNCE   2000   // Prevent double-clicks on touch pads
// OLED horizontal scroll step interval (ms). 80ms was tuned for the old
// small 6x12 font; after r11 tripled the glyph width (logisoso20) a
// typical offering message took ~30s to scroll across while the
// offering display window is only 12s - devotees saw two words before
// it vanished. Set BELOW drawOLED's 33ms frame time so the scroll steps
// on every frame (any value between 33 and 66 would quantize to ~66ms
// steps and halve the speed): 3px per ~33ms frame = ~90px/s, so even a
// long personalized offering (~900px) completes a full pass in ~10s,
// inside the 12s window, and blessing rotation pace returns to a
// reasonable ~7-10s per blessing.
#define TEXT_SCROLL_SPD  30

// Triggers Durations
#define MANTRA_DURATION  30000  // Mouse Back: 30-sec lockout
#define FEET_DURATION    30000  // Feet Touch: 30-sec lockout for Ganeshmantra1 & 2

// Default System Settings
#define DEFAULT_VOLUME   15     // Default MP3 Volume (0 - 30)
#define DEFAULT_BRIGHT   150    // Default LED Brightness (0 - 255)

// State machine definitions
enum SystemState {
  STATE_STANDBY,        // Everything idle (OLED & LEDs off), PIR wakes this
  STATE_AMBIENT,        // Woken up by PIR motion (peacock breathe, "Welcome")
  STATE_MANTRA_ACTIVE,  // Mouse Back touched: Plays 30-sec mantra
  STATE_FEET_ACTIVE,    // Feet touched: Plays alternating Ganeshmantra1 / Ganeshmantra2 (30s)
  STATE_AARTI,          // Idle-triggered or manually-closed Aarti chant
  STATE_TEMPLE_CLOSED   // Night mode after Aarti closes the temple - OLED/LEDs off like
                         // STANDBY, but deliberately only a touch (feet/mouse-back) wakes
                         // it, not PIR - matches the web dashboard's TEMPLE_CLOSED design.
};

// V2 Phase 4: ExperienceScene - a small, read-only snapshot of "what's
// currently happening" across state/mood/audio/display, built via
// getCurrentScene() in GanapatiAI.ino. Exists so new code (starting with
// Phase 3's LED scene engine) has ONE thing to query instead of checking
// half a dozen separate globals (currentMoodTag, blessingTaskActive,
// feetDisplayLocked, stateTimer/stateDuration...) directly and
// separately, the way animateLeds()/drawOLED() currently do.
//
// Deliberately NOT a class hierarchy or an event-driven orchestrator -
// the full V2 spec's "AI Ritual Orchestrator" (InteractionEvent/
// ExperiencePlan/Guardian/Renderer layers) was reviewed and rejected as
// over-engineered for a single ESP32 running one state machine. This is
// just a struct and one function that reads existing state - it does
// not change how or when any existing global gets SET, only gives
// read-side code one coherent view instead of several scattered checks.
// Built specifically to prevent the class of bug r99 was: LED/OLED/audio
// drifting out of sync because nothing had one shared picture of "what's
// actually playing right now."
struct ExperienceScene {
  SystemState state;
  const char *mood;         // currentMoodTag, or "none" if empty
  bool audioActive;         // blessingTaskActive || a DFPlayer track is set
  bool displayLocked;       // feetDisplayLocked
  unsigned long elapsedMs;  // millis() - stateTimer
  unsigned long durationMs; // stateDuration
};

// DFPlayer Mini SD card layout: all tracks live in a folder literally named
// "mp3" (0001.mp3, 0002.mp3, ...) - played via playMp3Folder(), NOT play().
#define BELL_TRACK       3      // Ganapathibell.mp3 - the only track not in mantraTracks[]/AARTI_TRACK/AARTI_PART2_TRACK
#define AARTI_TRACK       16     // GaneshAarti.mp3 - part 1 of the closing Aarti
#define AARTI_PART1_DURATION 206000 // real measured length of AARTI_TRACK (3:26) - triggerAarti() switches to part 2 exactly here, no gap
// r137: reinstated r132/reverted r136 - confirmed by direct correction
// that track 10 (Ganeshmantra8.mp3, formerly in mantraTracks[] - see its
// own removal comment there) DOES belong with Aarti track 16, playing
// immediately after it as one continuous ritual. AARTI_DURATION is the
// COMBINED total (both parts back to back) - this is what setSystemState()
// actually uses for STATE_AARTI, so the LED flame arc and eye breathing
// pulse (both riding on the state's own duration) naturally stretch across
// both parts as a single scripted sequence instead of resetting partway.
#define AARTI_PART2_TRACK     10
#define AARTI_PART2_DURATION  125520 // Ganeshmantra8.mp3's own measured length (2:05), matches mantraTracks[]'s old entry for this track
#define AARTI_DURATION (AARTI_PART1_DURATION + AARTI_PART2_DURATION) // r130/r132/r137: matches web dashboard's AARTI_FALLBACK_DURATION_MS

// r142: second, single-track Aarti alternative - track 22 (formerly in
// mouseChantTracks[], now removed from that rotation), per direct
// request. triggerAarti() picks between this and the AARTI_TRACK+
// AARTI_PART2_TRACK block above based on lastTouchWasMouseBack (whichever
// pad was touched most recently) - feet -> the 16+10 block, mouse-back ->
// this alternative alone. Chosen over a plain alternating rotation as
// the "whatever is easy" option that's also more meaningful: the closing
// ritual reflects who's actually been at the altar, not an arbitrary coin
// flip.
#define AARTI_ALT_TRACK      22
#define AARTI_ALT_DURATION   239000 // track 22's own real length (3:59)

// r130/r142: mouse-back's own rotating pool of chants - deliberately NOT
// part of mantraTracks[]/feet's shared playlist, so the mouse-back pad
// always plays from its own dedicated set instead of cycling through
// the feet library. r142: 22 moved out (now AARTI_ALT_TRACK above), 28
// and 29 moved out (over 2 minutes - see the "Songs" pool below, per
// direct request that anything that long is "too long with touch pads"
// and should be manual-only), and new tracks 30/31/35 added to keep the
// pool at 10. The actual MantraTrack array (mouseChantTracks[]) and its
// own rotation counter (mouseStep) live in GanapatiAI.ino right next to
// mantraTracks[]/feetStep, same convention. 18-20 are taken by the
// offline-blessing fallback tracks; 32/37 are new "Songs" (see below);
// next free track number is 40.

// r142: "Songs" - any track over 2 minutes is deliberately excluded from
// BOTH touch-triggered rotations (feet and mouse-back), per direct
// request ("too long with touch pads") - playable only on demand via the
// dashboard's Play Track control (see playTrackManually() and
// songTracks[] in GanapatiAI.ino, and allPlayableTracks in
// web_dashboard.h). This is a duration-only lookup table, never a
// rotation pool. Applies retroactively across the whole catalog, not
// just this batch - feet's former track 9 (2:16) and mouse-back's former
// tracks 28 (2:07) and 29 (4:13) all moved here too, alongside the new
// long tracks 32 (3:06) and 37 (3:28). Track 22, also over 2 minutes, is
// NOT here - it became AARTI_ALT_TRACK above instead, per direct request.

// Phase 1: Offline Blessing Fallback - see playOfflineBlessingFallback()
// in GanapatiAI.ino. Plays instead of a live AI blessing whenever Wi-Fi
// or the backend is unreachable, so a devotee gets something real
// instead of silence. Briefly reused 3 of the existing mantraTracks[]
// entries to avoid asking for new recordings - reverted because a
// mantra/chant isn't the same kind of content as a blessing acknowledging
// someone's prayer, and playing one in its place was a genre mismatch,
// not just a lower-quality stand-in. Fixed properly instead: these 3
// tracks are real spoken blessings generated ONCE via the existing
// synthesizeAudio endpoint (same Google TTS voice as every live
// blessing) and saved as static files - no live AI dependency, and no
// human voice recording needed either. Files must be placed as
// 0018.mp3/0019.mp3/0020.mp3 in the DFPlayer's MP3 folder.
#define OFFLINE_BLESSING_TRACK_COUNT 3
static const int OFFLINE_BLESSING_TRACKS[OFFLINE_BLESSING_TRACK_COUNT] = {18, 19, 20};

#endif // CONFIG_H
