#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Bump this any time GanapatiAI.ino/config.h/web_dashboard.h change - the
// boot log prints it, so the Serial Monitor is the definitive proof of
// what's actually flashed on the board, independent of any download or
// browser-cache issue on the file-sync side.
#define FIRMWARE_VERSION "2026-07-28-r1"

// ==========================================
// Hardware Pin Definitions
// ==========================================

// Sensors
#define PIR_PIN          13   // AM312 Motion Sensor Pin
// NOTE: was GPIO12 (MTDI / VDD_SDIO strapping pin). With the touch sensor
// unplugged that pin floats, and if noise pulls it HIGH at reset the ESP32
// selects the wrong flash voltage and hangs/reboots before setup() ever
// runs (no Serial, no OLED). Moved to GPIO27, a plain non-strapping input.
// Rewire the feet sensor's signal line to GPIO27 when reconnecting it.
#define TOUCH_FEET_PIN   27   // Touch Sensor 2: Idol Feet (Alternating Mantra trigger)
#define TOUCH_BACK_PIN   15   // Touch Sensor 3: Mouse Back Stroke (Mantra trigger)

// Set to true only once each TP223 module's OUT pin is actually wired to
// its GPIO above - until then the pin is floating/uncertain and produces
// phantom touches, which was confusing testing of unrelated features
// (PIR, Ambient, Aarti). Flip independently as each gets wired for real.
#define TOUCH_FEET_CONNECTED   false
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
#define TEXT_SCROLL_SPD  80     // OLED text horizontal scrolling speed (ms)

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
