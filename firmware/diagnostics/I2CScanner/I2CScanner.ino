// Standalone I2C bus scanner - not part of the main GanapatiAI firmware.
// Flash this alone to check whether the OLED is detected at all on the
// I2C bus before troubleshooting anything else. Uses the same SDA/SCL
// pins as the main sketch (GPIO21/22).
#include <Wire.h>

#define OLED_SDA 21
#define OLED_SCL 22

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\nI2C Scanner starting...");

  Wire.begin(OLED_SDA, OLED_SCL);
}

void loop() {
  int foundCount = 0;
  Serial.println("Scanning I2C bus...");

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      foundCount++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (foundCount == 0) {
    Serial.println("No I2C devices found - check wiring/power.");
  } else {
    Serial.print(foundCount);
    Serial.println(" device(s) found.");
  }

  Serial.println("Done. Rescanning in 5 seconds...\n");
  delay(5000);
}
