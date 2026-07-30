#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Bump this any time GanapatiAI.ino/config.h/web_dashboard.h change - the
// boot log prints it, so the Serial Monitor is the definitive proof of
// what's actually flashed on the board, independent of any download or
// browser-cache issue on the file-sync side.
#define FIRMWARE_VERSION "2026-07-30-r23"

// ==========================================
// Hardware Pin Definitions
// ==========================================

// Sensors
// PIR moved 13 -> 19. GPIO13 doubles as JTAG MTCK, and after the sensor
// and wiring were both replaced the pin STILL showed ~55Hz noise (i.e.
// mains hum on an undriven line) - even when forced high. GPIO19 is a
// plain bidirectional GPIO with no strapping, JTAG, or flash role and
// nothing else on this build uses it. REWIRE the AM312's OUT lead to
// D19; VIN and GND stay where they are.
#define PIR_PIN          19   // AM312 Motion Sensor Pin (was 13 = JTAG MTCK)

// DISABLED. On both GPIO13 and a fresh GPIO19 the line measured only
// 27-40% duty - i.e. the AM312 is not driving the pin at all (an undriven
// wire picking up mains hum), which no pin choice or firmware change can
// fix. The temple is fully functional without it: STANDBY -> AMBIENT still
// happens via the dashboard's "Trigger PIR" button (/api/control?action=pir,
// deliberately independent of this flag) or any touch on the pads. Set back
// to true only after the sensor is confirmed to actually output a signal.
#define PIR_CONNECTED    false

// Touch pads (TP223 modules). Feet was originally GPIO12 (MTDI /
// VDD_SDIO strapping pin) - with the sensor unplugged that pin floats,
// and if noise pulls it HIGH at reset the ESP32 selects the wrong flash
// voltage and hangs before setup() ever runs (no Serial, no OLED). Moved
// to GPIO27, a plain non-strapping input.
#define TOUCH_FEET_PIN   27   // Touch Sensor 2: Idol Feet (Alternating Mantra trigger)
#define TOUCH_BACK_PIN   15   // Touch Sensor 3: Mouse Back Stroke (Mantra trigger)

// FALSE until each pad's OUT lead is actually landed on its GPIO above.
// These were briefly set true on the strength of "pads are soldered", but
// soldered is not the same as connected to the board: with the leads off,
// GPIO27/15 float and pick up the same mains hum as the PIR line, firing
// phantom touches. That put the device into FEET_ACTIVE by itself, with
// no one touching anything. Flip each one to true only after that pad's
// wire is on its pin, and expect random self-starting mantras if you turn
// one on early.
// Feet pad is wired (it was producing a real signal on GPIO27), so this
// stays true - having to re-enable it after every download was itself
// causing "the pad does nothing" reports. Mouse-back stays false until
// its lead is actually landed on GPIO15.
#define TOUCH_FEET_CONNECTED   true
#define TOUCH_BACK_CONNECTED   false

// NeoPixel LEDs
#define LED_PIN          18   // WS2812B NeoPixel Data Pin
#define NUM_LEDS         24   // 24 LEDs in the ring

// OLED Display (I2C)
#define OLED_SDA         21   // I2C Data Pin
#define OLED_SCL         22   // I2C Clock Pin

// DFPlayer Mini MP3 Player
#define DFPLAYER_RX      25   // Connect to DFPlayer TX
#define DFPLAYER_TX      26   // Connect to DFPlayer RX (via 1k resistor)

// I2S Microphone (INMP441)
#define I2S_MIC_WS       32   // I2S Word Select / LRC
#define I2S_MIC_SCK      33   // I2S Serial Clock / BCLK
#define I2S_MIC_SD       34   // I2S Serial Data / DATA

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
