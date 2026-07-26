// Standalone test: does the LEGACY I2S driver (driver/i2s.h) conflict with
// WiFi on this exact ESP32 core version? Not part of the main firmware -
// this exists purely to answer that question before any I2S mic code goes
// back into GanapatiAI.ino, since the legacy I2S driver was the confirmed
// cause of the "ADC: CONFLICT! driver_ng..." abort() crash there.
//
// Mirrors the original (now removed) initI2SMic() config exactly: same
// I2S mode, same pins, same INMP441 wiring assumptions.
#include <WiFi.h>
#include <driver/i2s.h>

#define WIFI_SSID     "VM3003995_Ext"
#define WIFI_PASSWORD "c7kQrnnrdqnf"

#define I2S_MIC_WS  32
#define I2S_MIC_SCK 33
#define I2S_MIC_SD  34
#define I2S_PORT    I2S_NUM_0
#define BUFFER_SIZE 512

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\nI2S + WiFi conflict test starting...");

  Serial.println("STEP A: Connecting WiFi first...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nSTEP A: WiFi connected. If you see this, WiFi init alone is fine.");
  } else {
    Serial.println("\nSTEP A: WiFi did not connect, continuing anyway to test I2S.");
  }

  Serial.println("STEP B: Installing legacy I2S driver now (this is the suspected conflict point)...");
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_MIC_SCK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SD
  };

  esp_err_t installResult = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  Serial.print("STEP B: i2s_driver_install() returned: ");
  Serial.println(installResult == ESP_OK ? "ESP_OK" : String(installResult));

  esp_err_t pinResult = i2s_set_pin(I2S_PORT, &pin_config);
  Serial.print("STEP B: i2s_set_pin() returned: ");
  Serial.println(pinResult == ESP_OK ? "ESP_OK" : String(pinResult));

  Serial.println("STEP C: If you're reading this line on Serial, BOTH WiFi and the legacy I2S driver initialized without crashing.");
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    int16_t buf[BUFFER_SIZE];
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, buf, sizeof(buf), &bytesRead, 100);
    Serial.print("Still alive. WiFi status: ");
    Serial.print(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "NOT CONNECTED");
    Serial.print(" | I2S bytes read: ");
    Serial.println(bytesRead);
  }
}
