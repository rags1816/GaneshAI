/*
 * SensorBench - standalone sensor validator for GanapatiAI
 *
 * Runs on a BARE ESP32 with only the sensor under test attached. No WiFi,
 * no OLED, no DFPlayer, no LEDs - so it boots in under a second and its
 * serial output is nothing but sensor truth. The main firmware reports the
 * same facts, but buried in ~15 seconds of boot chatter and behind a state
 * machine that only acts on a sensor in certain states.
 *
 * Use it to answer, definitively:
 *   - is this PIR outputting anything at all?
 *   - is this touch pad actually connected and pulsing?
 *   - is this pin quiet, or is it picking up mains hum?
 *
 * Wire ONLY what you are testing. An unconnected pin floats and reports
 * noise - that is expected and the output says so.
 *
 * Serial Monitor at 115200.
 */

#define PIR_PIN         19   // AM312 OUT
#define TOUCH_FEET_PIN  27   // TP223 OUT (feet)
#define TOUCH_BACK_PIN  23   // TP223 OUT (mouse back)

// Duty cycle over this window separates a driven line from a floating one.
// Mains hum on an undriven wire lands near 50%; a genuinely driven pin sits
// at ~0% or ~100%.
#define WINDOW_MS       250
#define REPORT_MS       1000

struct Line {
  const char* name;
  uint8_t pin;
  uint32_t samples, highs;
  int dutyPct;
  bool stable, lastRead;
  uint32_t changedAt;
  uint32_t pulses;      // confirmed LOW->HIGH transitions since boot
};

Line lines[] = {
  { "PIR        (D19)", PIR_PIN,        0, 0, 0, false, false, 0, 0 },
  { "Feet pad   (D27)", TOUCH_FEET_PIN, 0, 0, 0, false, false, 0, 0 },
  { "Mouse pad  (D23)", TOUCH_BACK_PIN, 0, 0, 0, false, false, 0, 0 },
};
const int NUM_LINES = sizeof(lines) / sizeof(lines[0]);

uint32_t windowStart = 0, lastReport = 0;

// Same 60ms settle the main firmware uses, so a pulse counted here means
// a pulse the firmware would also have accepted.
#define SETTLE_MS 60

void setup() {
  Serial.begin(115200);
  delay(400);
  for (int i = 0; i < NUM_LINES; i++) pinMode(lines[i].pin, INPUT);

  Serial.println();
  Serial.println("=========================================================");
  Serial.println(" SensorBench - GanapatiAI sensor validator");
  Serial.println("=========================================================");
  Serial.println(" Wire ONLY the sensor you are testing. Then:");
  Serial.println("   PIR      - wave a hand, watch for duty ~100% + pulses");
  Serial.println("   Touch pad- tap it, watch the pulse counter increment");
  Serial.println();
  Serial.println(" Reading the duty column:");
  Serial.println("   0%       quiet, nothing driving it (correct when idle)");
  Serial.println("   ~30-70%  FLOATING - not connected, picking up mains hum");
  Serial.println("   100%     line held high (real detection, or stuck)");
  Serial.println("---------------------------------------------------------");
  Serial.println();
}

void loop() {
  uint32_t now = millis();

  for (int i = 0; i < NUM_LINES; i++) {
    Line &L = lines[i];
    bool raw = (digitalRead(L.pin) == HIGH);

    L.samples++;
    if (raw) L.highs++;

    // Debounced edge count - the number that actually matters for a pad.
    if (raw != L.lastRead) { L.lastRead = raw; L.changedAt = now; }
    if (now - L.changedAt >= SETTLE_MS && L.stable != L.lastRead) {
      L.stable = L.lastRead;
      if (L.stable) {
        L.pulses++;
        Serial.printf(">>> PULSE  %s   (total %lu)\n", L.name, (unsigned long)L.pulses);
      }
    }
  }

  if (now - windowStart >= WINDOW_MS) {
    for (int i = 0; i < NUM_LINES; i++) {
      Line &L = lines[i];
      L.dutyPct = L.samples ? (int)((L.highs * 100UL) / L.samples) : 0;
      L.samples = L.highs = 0;
    }
    windowStart = now;
  }

  if (now - lastReport >= REPORT_MS) {
    lastReport = now;
    Serial.printf("[%6lus] ", (unsigned long)(now / 1000));
    for (int i = 0; i < NUM_LINES; i++) {
      Line &L = lines[i];
      const char* verdict;
      if (L.dutyPct <= 5)        verdict = "quiet";
      else if (L.dutyPct >= 95)  verdict = "HELD-HIGH";
      else                       verdict = "FLOATING?";
      Serial.printf("%s %3d%% %-9s pulses:%-4lu | ",
                    L.name, L.dutyPct, verdict, (unsigned long)L.pulses);
    }
    Serial.println();
  }

  delay(10);
}
