// Standalone PIR (AM312) wiring/response diagnostic - not part of the main
// GanapatiAI firmware. Prints every HIGH/LOW transition on the PIR pin so
// motion detection can be confirmed before troubleshooting anything else
// in the full state machine. Uses the same pin as the main sketch (GPIO13).
#define PIR_PIN 13

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nPIR Test starting...");
  Serial.println("AM312 needs ~30-60s after power-on to calibrate to the room -");
  Serial.println("ignore any HIGH readings in the first minute.");
  pinMode(PIR_PIN, INPUT);
}

void loop() {
  static int lastState = -1;
  int pirState = digitalRead(PIR_PIN);

  if (pirState != lastState) {
    if (pirState == HIGH) {
      Serial.println("MOTION DETECTED (pin went HIGH)");
    } else {
      Serial.println("Motion cleared (pin went LOW)");
    }
    lastState = pirState;
  }

  delay(50);
}
