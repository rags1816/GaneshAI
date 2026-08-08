#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Bump this any time GanapatiAI.ino/config.h/web_dashboard.h change - the
// boot log prints it, so the Serial Monitor is the definitive proof of
// what's actually flashed on the board, independent of any download or
// browser-cache issue on the file-sync side.
#define FIRMWARE_VERSION "2026-08-08-r87"

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
#define WISH_PAD_CONNECTED  false

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

#define OLED_MODEL       OLED_SH1106_I2C

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

// DFPlayer Mini SD card layout: all tracks live in a folder literally named
// "mp3" (0001.mp3, 0002.mp3, ...) - played via playMp3Folder(), NOT play().
#define BELL_TRACK       3      // Ganapathibell.mp3 - the only track not in mantraTracks[]/AARTI_TRACK
#define AARTI_TRACK      16     // GaneshAarti.mp3 - the only track not in mantraTracks[]
#define AARTI_DURATION   240000 // ~4 min, matches web dashboard's AARTI_FALLBACK_DURATION_MS

#endif // CONFIG_H
