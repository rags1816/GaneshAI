/*
 * ExpansionBench - standalone bring-up test for the LED ring, wish pad,
 * and mic, all together on a SEPARATE ESP32 (not the main Ganesha board).
 *
 * Same idea as SensorBench: prove each part works on its own, with
 * nothing else running, before any of it touches the main firmware.
 *
 * Uses the modern ESP_I2S.h library for the mic, not the legacy
 * driver/i2s.h - see diagnostics/I2SWiFiTest, which found the legacy
 * driver crashes when combined with WiFi on this exact board. This
 * sketch has no WiFi so that crash can't happen here either way, but
 * the mic code is written the way it'll need to be once it moves to
 * the main board, which does run WiFi.
 *
 * Wiring (same pin numbers as the main board's config.h, so nothing
 * needs renumbering later):
 *   LED ring : DI through a 330 ohm resistor -> D18. Ring's own 5V/GND
 *              from a separate Wago supply, NOT the ESP32. 1000uF cap
 *              across the ring's own 5V/GND terminals.
 *   Wish pad : TP223, VCC->3.3V, GND->GND, I-O->D4
 *   Mic      : VDD->3.3V, GND->GND, L/R->GND, WS->D32, SCK->D33, SD->D34
 *
 * Clap detection (new): a real loud clap measured on this exact rig
 * peaked around 120000-131000 on this sketch's scaling, against a quiet
 * -room floor of ~0 - see CLAP_THRESHOLD below. That's ONE data point
 * from one room, so treat the thresholds as a starting guess to tune
 * against your real room, not a finished value. Each clap toggles a
 * simulated open/closed state and the LED ring visibly reflects it
 * (rainbow when "open", off when "closed") - proving clap -> state ->
 * light works end to end before this logic moves to the main board.
 *
 * Serial Monitor at 115200.
 */

#include <FastLED.h>
#include "ESP_I2S.h"

// ---- LED ring ----
#define LED_PIN            18
#define NUM_LEDS           24   // matches the real ring - change if yours differs
#define LED_MAX_MILLIAMPS  1500
CRGB leds[NUM_LEDS];
uint8_t ledHue = 0;
uint32_t lastLedStepMs = 0;

// ---- Wish pad (TP223, momentary mode) ----
#define WISH_PIN           4
bool wishRaw = false, wishStable = false;
uint32_t wishChangedAt = 0, wishPulses = 0;
#define TOUCH_SETTLE_MS    60   // same settle time the main firmware uses

// ---- Mic (I2S, e.g. INMP441) ----
#define MIC_SCK_PIN        33
#define MIC_WS_PIN         32
#define MIC_SD_PIN         34
I2SClass I2S;
bool micReady = false;
int32_t micBuf[256];
int32_t micPeakSinceReport = 0;

// ---- Clap detection ----
// Starting guess from one real clap on this rig (quiet ~0, clap ~120k-131k).
// Tune these two against your actual room once you're watching it live.
#define CLAP_THRESHOLD     25000  // a frame louder than this counts as "loud"
#define CLAP_REARM_LEVEL   6000   // must drop back below this before the next clap can fire
#define CLAP_REFRACTORY_MS 400    // minimum gap between two separate claps
bool clapArmed = true;
uint32_t lastClapMs = 0;
bool templeOpen = false;   // simulated state for this bench test only

uint32_t lastReport = 0;
#define REPORT_MS          1000

void setup();
void loop();

void setup() {
  Serial.begin(115200);
  delay(400);

  Serial.println();
  Serial.println("=========================================================");
  Serial.println(" ExpansionBench - LED ring + Wish pad + Mic bring-up test");
  Serial.println("=========================================================");
  Serial.println(" LED ring : off to start (\"closed\"). Clap once to \"open\" -");
  Serial.println("            ring starts cycling rainbow colors. Clap again");
  Serial.println("            to \"close\" - ring goes dark. Same clap, toggles.");
  Serial.println(" Wish pad : tap it, watch the pulse counter increment.");
  Serial.println(" Mic      : talk or clap near it, watch the peak level");
  Serial.println("            jump well above the quiet-room baseline.");
  Serial.println("---------------------------------------------------------");
  Serial.println();

  // LED ring
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MILLIAMPS);
  FastLED.setBrightness(80);
  FastLED.clear();
  FastLED.show();

  // Wish pad
  pinMode(WISH_PIN, INPUT);

  // Mic
  I2S.setPins(MIC_SCK_PIN, MIC_WS_PIN, -1, MIC_SD_PIN);
  micReady = I2S.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);
  Serial.println(micReady ? "MIC: I2S bus initialized." : "MIC: FAILED to initialize I2S bus - check wiring.");
}

void loop() {
  uint32_t now = millis();

  // --- LED: rainbow sweep while "open", dark while "closed". The clap
  // detector below is what flips templeOpen. ---
  if (templeOpen) {
    if (now - lastLedStepMs >= 30) {
      lastLedStepMs = now;
      fill_rainbow(leds, NUM_LEDS, ledHue, 8);
      FastLED.show();
      ledHue++;
    }
  }

  // --- Wish pad: same debounced-edge pattern as SensorBench's touch pads ---
  bool raw = (digitalRead(WISH_PIN) == HIGH);
  if (raw != wishRaw) { wishRaw = raw; wishChangedAt = now; }
  if (now - wishChangedAt >= TOUCH_SETTLE_MS && wishStable != wishRaw) {
    wishStable = wishRaw;
    if (wishStable) {
      wishPulses++;
      Serial.printf(">>> WISH PAD PRESSED (total %lu)\n", (unsigned long)wishPulses);
    }
  }

  // --- Mic: read whatever's arrived this pass, and act on it immediately
  // for clap detection (a clap is a ~50-200ms transient - waiting for the
  // once-a-second report would blur or miss it entirely). ---
  if (micReady) {
    size_t bytesRead = I2S.readBytes((char *)micBuf, sizeof(micBuf));
    size_t samples = bytesRead / sizeof(int32_t);
    int32_t frameLoudest = 0;
    for (size_t i = 0; i < samples; i++) {
      int32_t scaled = micBuf[i] >> 14;  // 32-bit raw sample -> a human-readable range
      int32_t mag = abs(scaled);
      if (mag > frameLoudest) frameLoudest = mag;
    }
    if (frameLoudest > micPeakSinceReport) micPeakSinceReport = frameLoudest;

    // Arm-on-quiet, fire-on-loud: stops one clap's decay tail (or a
    // sustained loud noise) from registering as several claps in a row.
    if (clapArmed && frameLoudest > CLAP_THRESHOLD && (now - lastClapMs) > CLAP_REFRACTORY_MS) {
      clapArmed = false;
      lastClapMs = now;
      templeOpen = !templeOpen;
      if (!templeOpen) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
      }
      Serial.printf(">>> CLAP DETECTED (peak %ld) - temple now %s\n",
                    (long)frameLoudest, templeOpen ? "OPEN" : "CLOSED");
    }
    if (!clapArmed && frameLoudest < CLAP_REARM_LEVEL) {
      clapArmed = true;
    }
  }

  if (now - lastReport >= REPORT_MS) {
    lastReport = now;
    Serial.printf("[%6lus] Temple:%-6s | Wish pad pulses:%-4lu | Mic peak:%-6ld %s\n",
                  (unsigned long)(now / 1000), templeOpen ? "OPEN" : "CLOSED",
                  (unsigned long)wishPulses,
                  (long)micPeakSinceReport, micReady ? "" : "(mic not initialized)");
    micPeakSinceReport = 0;
  }
}
