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
#include "config.h"
#include "web_dashboard.h"

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

// OLED Display (SH1106 1.3" I2C)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);

// LED Ring Array
CRGB leds[NUM_LEDS];

// DFPlayer Mini
#define dfSerial Serial2
DFRobotDFPlayerMini myDFPlayer;

// System States
SystemState currentState = STATE_STANDBY;
unsigned long stateTimer = 0;
unsigned long stateDuration = 0;

// Sensor Tracking
unsigned long lastMotionTrigger = 0;
unsigned long lastTouchTrigger = 0;
bool motionDetected = false;
bool feetTouched = false;
bool backTouched = false;

// Set right before an Aarti chant that should end in STATE_TEMPLE_CLOSED
// (idle timeout, or an explicit close request) rather than returning to
// STATE_AMBIENT (an Aarti triggered directly via /api/control?action=aarti
// with no close intent).
bool aartiThenClose = false;

// Track Struct Definition
struct MantraTrack {
  int dfTrack;
  unsigned long duration;
};

// 14 Devotional Tracks mapping
const int NUM_TRACKS = 14;
MantraTrack mantraTracks[NUM_TRACKS] = {
  {1, 19121},  // Ganapathimantrai.mp3 (Vakratundaya - actual measured length, was overestimated at 24s)
  {2, 28390},  // Ganpathimantra1.mp3 (28s)
  {4, 27360},  // Ganapathimantra2.mp3 (27s)
  {5, 55350},  // Ganeshmantra3.mp3 (55s)
  {6, 155530}, // Ganeshmantra4.mp3 (155s)
  {7, 99600},  // Ganeshmantra5.mp3 (99s)
  {8, 60160},  // Ganeshmantra6.mp3 (60s)
  {9, 136700}, // Ganeshmantra7.mp3 (136s)
  {10, 125520},// Ganeshmantra8.mp3 (125s)
  {11, 48800}, // Ganeshmantra9.mp3 (48s)
  {12, 27380}, // Ganeshmantra10.mp3 (27s)
  {13, 79280}, // Ganeshmantra11.mp3 (79s)
  {14, 175730},// Ganeshmantra12.mp3 (175s)
  {15, 28400}  // Ganeshmantra13.mp3 (28s)
};

// Dynamic Playlist Counters
int mouseStep = 0; 
int feetStep = 0;  

// Customization & Settings
int blessingCounter = 0;
int currentBrightness = DEFAULT_BRIGHT;
int currentPattern = 0; 
int currentVolume = DEFAULT_VOLUME;
bool pirEnabled = true;

// Language Settings: 0 = English, 1 = Marathi/Sanskrit, 2 = Tamil
int selectedLang = 1; 
// Theme of the Day: 0 = Tue (Ganesha), 1 = Mon (Shiva), 2 = Wed (Wisdom), 3 = Thu (Guru), 4 = Fri (Shakti), 5 = Sat (Discipline), 6 = Sun (Sun)
int selectedTheme = 0;

// OLED Text Buffer
// Fixed-size buffer, not String: drawOLED() runs ~30x/sec and continuous
// String reallocation there was the cause of a heap-fragmentation abort()
// crash after ~7 minutes of uptime.
char scrollText[300] = "";
int scrollX = 128;
unsigned long lastScrollUpdate = 0;

// 12-second Display lock helper for Feet touch
unsigned long feetDisplayTimer = 0;
bool feetDisplayLocked = false;

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
void updateStateMachine();
void animateLeds();
void drawOLED();
void setSystemState(SystemState newState, unsigned long duration = 0);
void triggerMantra();
void triggerFeetMantra();
void triggerAarti();
void triggerAartiThenClose();
void openTempleFromClosed();
void stopAudioAndStandby();

// ==========================================
// Setup Function
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1500); // Give serial monitor time to connect
  Serial.println("\n==================================");
  Serial.println("  Initializing Advanced Ganapati  ");
  Serial.println("==================================");

  // Unconditional, earliest-possible sanity check: dump the raw RTC memory
  // value before any crash-check logic touches it, so we know whether RTC
  // memory is behaving as expected on this board/core at all.
  Serial.print("DEBUG: raw stageMagic on boot = 0x");
  Serial.println(stageMagic, HEX);
  Serial.print("DEBUG: raw lastStage on boot = ");
  Serial.println(lastStage);

  bool prevRunCrashed = (stageMagic == CRASH_TRACER_MAGIC);
  int crashedStage = lastStage;
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
  pinMode(PIR_PIN, INPUT);
  Serial.println("DEBUG: PIR_PIN configured.");
  pinMode(TOUCH_FEET_PIN, INPUT);
  Serial.println("DEBUG: TOUCH_FEET_PIN configured.");
  pinMode(TOUCH_BACK_PIN, INPUT);
  Serial.println("DEBUG: TOUCH_BACK_PIN configured.");
  Serial.println("DEBUG: Sensor pins configured successfully.");

  // 2. Initialize OLED Display
  Serial.println("DEBUG: Initializing OLED Display...");
  u8g2.begin();
  Serial.println("DEBUG: u8g2.begin() returned.");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(5, 20, "Ganapati AI 2026");
  u8g2.drawStr(5, 35, "Voice Init...");
  u8g2.sendBuffer();
  Serial.println("DEBUG: OLED Display initialized successfully.");

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
  Serial.println("DEBUG: Initializing FastLED...");
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(currentBrightness);
  fill_solid(leds, NUM_LEDS, CRGB(0, 242, 254));
  FastLED.show();
  delay(400);
  FastLED.clear();
  FastLED.show();
  Serial.println("DEBUG: FastLED initialized successfully.");

  // 4. Microphone (I2S driver removed - see git history; was an unused,
  // never-called legacy driver/i2s.h include suspected of conflicting with
  // WiFi's ADC-based RF calibration and causing an early abort() crash)
  u8g2.drawStr(5, 48, "Microphone: OK");
  u8g2.sendBuffer();

  // 5. Initialize DFPlayer Mini
  Serial.println("STEP 5: Initializing DFPlayer Mini (Serial2, GPIO16/17)...");
  dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  u8g2.drawStr(5, 60, "Audio Board: Connecting...");
  u8g2.sendBuffer();

  bool dfReady = false;
  int retries = 0;
  while (retries < 5) {
    Serial.print("STEP 5: begin() attempt ");
    Serial.print(retries + 1);
    Serial.println(" of 5 - calling now (if this is the LAST line you ever see, begin() itself is hanging)...");

    if (myDFPlayer.begin(dfSerial)) {
      dfReady = true;
      Serial.println("STEP 5: DFPlayer responded - begin() returned true.");
      break;
    }

    Serial.println("STEP 5: begin() returned false (no/bad response yet), retrying in 500ms...");
    delay(500);
    retries++;
  }
  if (!dfReady) {
    Serial.println("STEP 5: DFPlayer NOT detected after 5 attempts - continuing WITHOUT audio so the rest of the device still works.");
  }

  if (dfReady) {
    myDFPlayer.volume(currentVolume);
    u8g2.drawStr(5, 60, "Audio Board: OK           ");
  } else {
    u8g2.drawStr(5, 60, "Audio Board: Missing SD   ");
  }
  u8g2.sendBuffer();
  delay(800);

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
  if (dfReady) {
    myDFPlayer.playMp3Folder(3);
    Serial.println("STEP 8: Startup sound played!");
  } else {
    Serial.println("STEP 8: Startup sound SKIPPED (DFPlayer not ready)");
  }

  // 9. Set Standby State
  Serial.println("STEP 9: Setting system state to standby...");
  setSystemState(STATE_STANDBY);
  Serial.println("STEP 9: System state set to standby!");
}

// ==========================================
// Main Loop
// ==========================================
void loop() {
  lastStage = 1;
  server.handleClient();
  lastStage = 2;
  checkSensors();
  lastStage = 3;
  updateStateMachine();
  lastStage = 4;
  animateLeds();
  lastStage = 5;
  drawOLED();
  delay(10);
}

// ==========================================
// Web Server API Routes
// ==========================================
void handleWebRoutes() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/api/state", HTTP_GET, []() {
    const char* stateStr;
    switch(currentState) {
      case STATE_STANDBY: stateStr = "STANDBY"; break;
      case STATE_AMBIENT: stateStr = "AMBIENT"; break;
      case STATE_MANTRA_ACTIVE: stateStr = "MANTRA_ACTIVE"; break;
      case STATE_FEET_ACTIVE: stateStr = "FEET_ACTIVE"; break;
      case STATE_AARTI: stateStr = "AARTI_MODE"; break;
      case STATE_TEMPLE_CLOSED: stateStr = "TEMPLE_CLOSED"; break;
      default: stateStr = "STANDBY"; break;
    }

    char json[220];
    snprintf(json, sizeof(json),
      "{\"state\":\"%s\",\"blessings\":%d,\"brightness\":%d,\"pattern\":%d,\"volume\":%d,\"pirEnabled\":%s,\"lang\":%d,\"theme\":%d}",
      stateStr, blessingCounter, currentBrightness, currentPattern, currentVolume,
      pirEnabled ? "true" : "false", selectedLang, selectedTheme);
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
    } else if (action == "stop") {
      stopAudioAndStandby();
    }
    server.send(200, "text/plain", "OK");
  });

  // One-off diagnostic route for finding the real playMp3Folder() track
  // numbers on this SD card without re-flashing between guesses. Not part
  // of normal operation - safe to leave in, it does nothing unless called.
  //   /api/test?track=N     -> stop current playback, play mp3-folder track N
  //   /api/test?filecount=1 -> total file count DFPlayer sees on the SD card
  server.on("/api/test", HTTP_GET, []() {
    char msg[64];
    if (server.hasArg("track")) {
      int n = server.arg("track").toInt();
      myDFPlayer.stop();
      delay(50);
      myDFPlayer.playMp3Folder(n);
      snprintf(msg, sizeof(msg), "Playing mp3 folder track %d", n);
      Serial.println(msg);
      server.send(200, "text/plain", msg);
    } else if (server.hasArg("filecount")) {
      int count = myDFPlayer.readFileCounts();
      snprintf(msg, sizeof(msg), "Total files on SD card: %d", count);
      Serial.println(msg);
      server.send(200, "text/plain", msg);
    } else {
      server.send(400, "text/plain", "Usage: /api/test?track=N  or  /api/test?filecount=1");
    }
  });

  server.on("/api/leds", HTTP_GET, []() {
    if (server.hasArg("brightness")) {
      currentBrightness = server.arg("brightness").toInt();
      FastLED.setBrightness(currentBrightness);
    }
    if (server.hasArg("pattern")) {
      currentPattern = server.arg("pattern").toInt();
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/api/audio", HTTP_GET, []() {
    if (server.hasArg("volume")) {
      currentVolume = server.arg("volume").toInt();
      myDFPlayer.volume(currentVolume);
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
    server.send(200, "text/plain", "OK");
  });
}

// ==========================================
// Sensor Reading & Debouncing
// ==========================================
void checkSensors() {
  unsigned long now = millis();

  // Read in both STANDBY (wakes it) and AMBIENT (resets the idle timer so
  // continued presence delays the auto-close Aarti) - previously only read
  // during STANDBY, which left AMBIENT's "someone's still here" branch dead.
  if (pirEnabled && (currentState == STATE_STANDBY || currentState == STATE_AMBIENT)) {
    int pirState = digitalRead(PIR_PIN);
    if (pirState == HIGH && (now - lastMotionTrigger > MOTION_DEBOUNCE)) {
      motionDetected = true;
      lastMotionTrigger = now;
    }
  }

  // Always check touch inputs (allows skipping tracks)
  if (now - lastTouchTrigger > TOUCH_DEBOUNCE) {
    if (digitalRead(TOUCH_FEET_PIN) == HIGH) {
      feetTouched = true;
      lastTouchTrigger = now;
    }
    else if (digitalRead(TOUCH_BACK_PIN) == HIGH) {
      backTouched = true;
      lastTouchTrigger = now;
    }
  }
}

// ==========================================
// State Machine Transitions
// ==========================================
void updateStateMachine() {
  unsigned long now = millis();

  // If 12 seconds have passed since Feet touch, unlock and resume rolling loop
  if (feetDisplayLocked && (now - feetDisplayTimer > 12000)) {
    feetDisplayLocked = false;
    strlcpy(scrollText, oledAmbientLoopText, sizeof(scrollText)); // Resume the combined rolling blessings
    Serial.println("OLED: 12 seconds elapsed, resumed blessings roll.");
  }

  switch (currentState) {
    
    case STATE_STANDBY:
      if (backTouched) {
        backTouched = false;
        triggerMantra();
      } else if (feetTouched) {
        feetTouched = false;
        triggerFeetMantra();
      } else if (motionDetected) {
        motionDetected = false;
        setSystemState(STATE_AMBIENT, AMBIENT_TIMEOUT);
      }
      break;

    case STATE_AMBIENT:
      if (backTouched) {
        backTouched = false;
        triggerMantra();
      } else if (feetTouched) {
        feetTouched = false;
        triggerFeetMantra();
      } else if (motionDetected) {
        motionDetected = false;
        stateTimer = now;
      }

      if (now - stateTimer > stateDuration) {
        // Nobody around for AMBIENT_TIMEOUT - close the temple for the
        // night with the Aarti chant first, same as a manual close.
        triggerAartiThenClose();
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

      if (now - stateTimer > stateDuration) {
        myDFPlayer.stop();
        setSystemState(STATE_AMBIENT, AMBIENT_TIMEOUT);
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

      if (now - stateTimer > stateDuration) {
        myDFPlayer.stop();
        setSystemState(STATE_AMBIENT, AMBIENT_TIMEOUT);
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

      if (now - stateTimer > stateDuration) {
        myDFPlayer.stop();
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
      // dashboard's TEMPLE_CLOSED design).
      if (backTouched) {
        backTouched = false;
        triggerMantra();
      } else if (feetTouched) {
        feetTouched = false;
        triggerFeetMantra();
      }
      break;
  }

  motionDetected = feetTouched = backTouched = false;
}

// ==========================================
// State Handlers
// ==========================================
void setSystemState(SystemState newState, unsigned long duration) {
  currentState = newState;
  stateTimer = millis();
  stateDuration = duration;
  scrollX = 128;

  if (newState == STATE_STANDBY || newState == STATE_TEMPLE_CLOSED) {
    feetDisplayLocked = false;
    FastLED.clear();
    FastLED.show();
    u8g2.setPowerSave(1);
  } else {
    u8g2.setPowerSave(0); 
    if (newState == STATE_AMBIENT) {
      feetDisplayLocked = false;
      // In Ambient, roll the entire combined blessings loop
      strlcpy(scrollText, oledAmbientLoopText, sizeof(scrollText));
    }
  }
}

void triggerMantra() {
  blessingCounter++;
  
  // Stop whatever is playing before starting the next track
  myDFPlayer.stop();
  delay(50);
  
  feetDisplayLocked = false; // Reset feet lock when mouse back is touched
  
  // Get track from struct array
  int trackIndex = mouseStep;
  int dfTrack = mantraTracks[trackIndex].dfTrack;
  unsigned long duration = mantraTracks[trackIndex].duration;
  
  setSystemState(STATE_MANTRA_ACTIVE, duration);
  myDFPlayer.playMp3Folder(dfTrack);
  
  // Display continues rolling blessings loop during Mouse play
  strlcpy(scrollText, oledAmbientLoopText, sizeof(scrollText));
  
  mouseStep = (mouseStep + 1) % NUM_TRACKS;
}

void triggerFeetMantra() {
  blessingCounter++;
  
  // Stop whatever is playing before starting the next track
  myDFPlayer.stop();
  delay(50);
  
  // Lock screen display timer (12 seconds)
  feetDisplayTimer = millis();
  feetDisplayLocked = true;
  
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
  myDFPlayer.playMp3Folder(dfTrack);
  
  feetStep = (feetStep + 1) % NUM_TRACKS;
}

void triggerAarti() {
  // Fired autonomously by AMBIENT_TIMEOUT idle above, by the /api/control
  // ?action=close route, or directly via ?action=aarti (dashboard button or
  // manual test) - in every case this just needs to start the physical
  // chant and reflect AARTI_MODE back via /api/state.
  myDFPlayer.stop();
  delay(50);

  feetDisplayLocked = false;
  strlcpy(scrollText, "   \xE2\x9C\xA8 A moment of Aarti \xE2\x9C\xA8   ", sizeof(scrollText));

  setSystemState(STATE_AARTI, AARTI_DURATION);
  myDFPlayer.playMp3Folder(AARTI_TRACK);
}

// Aarti that ends in STATE_TEMPLE_CLOSED once it finishes, instead of
// returning to STATE_AMBIENT - the idle-timeout auto-close and the
// dashboard's manual "Close Temple" button both go through this.
void triggerAartiThenClose() {
  aartiThenClose = true;
  triggerAarti();
}

// Manually reopen after STATE_TEMPLE_CLOSED (dashboard's "Open Temple"
// button) - a touch already does this on its own, this just lets it be
// triggered remotely too.
void openTempleFromClosed() {
  if (currentState == STATE_TEMPLE_CLOSED) {
    setSystemState(STATE_AMBIENT, AMBIENT_TIMEOUT);
  }
}

void stopAudioAndStandby() {
  myDFPlayer.stop();
  setSystemState(STATE_STANDBY);
}

// ==========================================
// LED Light Patterns (FastLED)
// ==========================================
// Pattern IDs match the web dashboard's pattern-select exactly (see
// web_dashboard.h drawLeds()/peacockWaveColor()/etc.) so /api/leds?pattern=N
// looks the same on the physical ring as it does in the browser simulation:
//   0 Peacock Wave, 1 Circuit Pulse, 2 Golden Aura, 3 Rainbow Dream, 4 Diya Flicker
void animateLeds() {
  if (currentState == STATE_STANDBY || currentState == STATE_TEMPLE_CLOSED) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    return;
  }

  // Continuously-growing float, not the wrapping uint8_t hueOffset used
  // elsewhere - these patterns feed it straight into sin(), and a uint8_t
  // wrap would show up as a visible glitch once per cycle.
  static float animHue = 0;
  animHue += 1.5f;

  if (currentState == STATE_AMBIENT || currentState == STATE_MANTRA_ACTIVE || currentState == STATE_FEET_ACTIVE || currentState == STATE_AARTI) {
    CRGB c1, c2;
    switch(selectedTheme) {
      case 0: c1 = CRGB(128, 0, 32);   c2 = CRGB(255, 215, 0); break;
      case 1: c1 = CRGB(0, 242, 254);  c2 = CRGB(79, 172, 254); break;
      case 2: c1 = CRGB(0, 180, 219);  c2 = CRGB(0, 255, 135); break;
      case 3: c1 = CRGB(255, 215, 0);  c2 = CRGB(255, 100, 0);   break;
      case 4: c1 = CRGB(236, 0, 140);  c2 = CRGB(185, 43, 39);   break;
      case 5: c1 = CRGB(75, 0, 130);   c2 = CRGB(79, 172, 254); break;
      case 6: c1 = CRGB(255, 75, 75);  c2 = CRGB(255, 215, 0); break;
    }

    // Same state->pattern overrides as the dashboard: Feet touch always
    // shows Peacock Wave, Mantra/Aarti chanting always shows Circuit Pulse.
    // Only AMBIENT actually reflects the user's chosen currentPattern.
    int activePattern = currentPattern;
    if (currentState == STATE_FEET_ACTIVE) {
      activePattern = 0;
    } else if (currentState == STATE_MANTRA_ACTIVE || currentState == STATE_AARTI) {
      activePattern = 1;
    }

    for (int i = 0; i < NUM_LEDS; i++) {
      switch (activePattern) {
        case 0: { // Peacock Wave - traveling sine blend between c1/c2
          float wave = (sinf(animHue * 0.04f + i * (2.0f * PI / NUM_LEDS)) + 1.0f) / 2.0f;
          leds[i] = blend(c1, c2, (uint8_t)(wave * 255));
          break;
        }
        case 2: { // Golden Aura - uniform 60/40 c1/c2 blend, slow breathing
          float breath = sinf(animHue * 0.03f) * 0.4f + 0.6f;
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
  if (currentState == STATE_STANDBY || currentState == STATE_TEMPLE_CLOSED) return;

  unsigned long now = millis();
  static unsigned long lastOledDraw = 0;
  if (now - lastOledDraw < 33) return;
  lastOledDraw = now;

  u8g2.clearBuffer();
  
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.drawHLine(0, 14, 128);
  u8g2.drawHLine(0, 48, 128);

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "GANAPATI AI 2026");
  
  if (currentState == STATE_AMBIENT) {
    u8g2.drawStr(90, 10, "[AMBIENT]");
  } else if (currentState == STATE_MANTRA_ACTIVE) {
    u8g2.drawStr(90, 10, "[MANTRA]");
  } else if (currentState == STATE_FEET_ACTIVE) {
    u8g2.drawStr(90, 10, "[MANTRA]");
  } else if (currentState == STATE_AARTI) {
    u8g2.drawStr(90, 10, "[AARTI]");
  }

  char countStr[40];
  snprintf(countStr, sizeof(countStr), "Devotional Hits: %d", blessingCounter);
  u8g2.drawStr(6, 57, countStr);

  u8g2.setFont(u8g2_font_6x12_tf);

  int textWidth = u8g2.getUTF8Width(scrollText);
  u8g2.drawUTF8(scrollX, 34, scrollText);
  
  if (now - lastScrollUpdate > TEXT_SCROLL_SPD) {
    lastScrollUpdate = now;
    scrollX -= 2;
    if (scrollX < -textWidth) {
      scrollX = 124;
    }
  }

  u8g2.sendBuffer();
}