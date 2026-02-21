#define BLYNK_TEMPLATE_ID "TMPL3nsQ49PWe"
#define BLYNK_TEMPLATE_NAME "Smart Lock"
#define BLYNK_AUTH_TOKEN "sdKbLaxBHUlhmgDcjdUte-ZJne3oiJ9X"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <MFRC522.h>
#include <SPI.h>
#include <LittleFS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

char ssid[] = "ASIF CHAUDHRY";
char pass[] = "11223344";

// Updated Pin Assignments
#define PIN_IR        D1   
#define PIN_RELAY     D3   
#define PIN_BUZZER    D4    // Buzzer moved here
#define PIN_LED_RED   D0   
#define PIN_EMERGENCY 3     // Button moved to RX (GPIO 3)
#define SS_PIN        D8   
#define RST_PIN       D2   

MFRC522 mfrc522(SS_PIN, RST_PIN);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800); 

String masterCode = "1234";
const String cardUID = "D9 D0 C8 06"; 
String enteredCode = "";
int attempts = 0;
bool systemActive = false;
bool firstConnect = true; 
unsigned long activationTimer = 0;
const long activePeriod = 10000; 

// LED Signals
void ledSignal(String type) {
  if (type == "GRANTED") {
    for (int i = 0; i < 10; i++) { digitalWrite(PIN_LED_RED, !digitalRead(PIN_LED_RED)); delay(200); }
  } else if (type == "DENIED") {
    digitalWrite(PIN_LED_RED, HIGH); delay(2000);
  } else if (type == "ALERT") {
    for (int i = 0; i < 10; i++) { digitalWrite(PIN_LED_RED, HIGH); delay(200); digitalWrite(PIN_LED_RED, LOW); delay(200); }
  }
  digitalWrite(PIN_LED_RED, LOW);
}

// Internal Logging
void logToInternal(String message) {
  timeClient.update();
  String timestamp = timeClient.getFormattedTime(); 
  File logFile = LittleFS.open("/log.txt", "a");
  if (logFile) { 
    logFile.println("[" + timestamp + "] " + message); 
    logFile.close(); 
  }
}

// Success Logic
void handleSuccess(String method) {
  attempts = 0; systemActive = false;
  Blynk.virtualWrite(V12, "GRANTED: " + method);
  logToInternal("GRANTED: " + method);
  digitalWrite(PIN_RELAY, LOW);   
  Blynk.virtualWrite(V13, 1);
  digitalWrite(PIN_BUZZER, HIGH); delay(200); digitalWrite(PIN_BUZZER, LOW);
  ledSignal("GRANTED");
  delay(5000);                    
  digitalWrite(PIN_RELAY, HIGH);  
  Blynk.virtualWrite(V13, 0);
}

// Failure Logic
void handleFailure(String reason) {
  attempts++;
  String failMsg = "Denied: " + reason;
  Blynk.virtualWrite(V12, failMsg); 
  logToInternal(failMsg);
  digitalWrite(PIN_BUZZER, HIGH); 
  if (attempts >= 3) {
    Blynk.logEvent("intruder_alert");
    Blynk.virtualWrite(V12, "SYSTEM LOCKDOWN");
    ledSignal("ALERT"); 
    attempts = 0;
  } else { 
    ledSignal("DENIED"); 
  }
  digitalWrite(PIN_BUZZER, LOW); 
}

// Connection Signal
BLYNK_CONNECTED() {
  if (firstConnect) {
    Blynk.virtualWrite(V12, "ACTIVE");
    // Startup double beep on D4
    digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
    delay(100);
    digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
    firstConnect = false;
  }
  timeClient.update();
}

// V13 - Remote Unlock/Lock
BLYNK_WRITE(V13) {
  bool s = param.asInt();
  digitalWrite(PIN_BUZZER, HIGH); delay(150); digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_RELAY, s ? LOW : HIGH); 
  if(s) { 
    Blynk.virtualWrite(V12, "Remote Unlock");
    logToInternal("APP UNLOCK"); 
    ledSignal("GRANTED"); 
  } else {
    Blynk.virtualWrite(V12, "Remote Lock");
    logToInternal("APP LOCK");
  }
}

// V14 - Manual Buzzer Control
BLYNK_WRITE(V14) {
  int buzzerState = param.asInt();
  if (buzzerState == 1) {
    digitalWrite(PIN_BUZZER, HIGH);
    Blynk.virtualWrite(V12, "Buzzer ON");
  } else {
    digitalWrite(PIN_BUZZER, LOW);
    Blynk.virtualWrite(V12, "Buzzer OFF");
  }
}

// Logs V16
BLYNK_WRITE(V16) {
  if (param.asInt() == 1) {
    File logFile = LittleFS.open("/log.txt", "r");
    if (!logFile) { Blynk.virtualWrite(V12, "No logs."); return; }
    Blynk.virtualWrite(V12, "--- LOG HISTORY ---");
    while (logFile.available()) { Blynk.virtualWrite(V12, logFile.readStringUntil('\n')); }
    logFile.close();
  }
}

// Clear V17
BLYNK_WRITE(V17) {
  if (param.asInt() == 1) {
    LittleFS.remove("/log.txt");
    Blynk.virtualWrite(V12, "Storage Cleared");
  }
}

// Keypad
BLYNK_WRITE_DEFAULT() {
  int pin = request.pin;
  if (param.asInt() == 1 && systemActive && pin <= 11) {
    if (pin <= 9) {
      enteredCode += String(pin);
      digitalWrite(PIN_BUZZER, HIGH); delay(80); digitalWrite(PIN_BUZZER, LOW);
    }
    else if (pin == 10) enteredCode = "";
    else if (pin == 11) {
      if (enteredCode == masterCode) handleSuccess("Keypad");
      else handleFailure("Wrong Code");
      enteredCode = "";
    }
  }
}

void setup() {
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_IR, INPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  // Important: Input Pullup for the button on RX
  pinMode(PIN_EMERGENCY, INPUT_PULLUP); 
  
  digitalWrite(PIN_RELAY, HIGH); 
  digitalWrite(PIN_BUZZER, LOW);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  ArduinoOTA.setHostname("SmartDoorLock");
  ArduinoOTA.begin();

  SPI.begin(); 
  mfrc522.PCD_Init(); 
  LittleFS.begin();
  timeClient.begin();
}

void loop() {
  Blynk.run();
  ArduinoOTA.handle();

  // Emergency Button Check (RX Pin)
  if (digitalRead(PIN_EMERGENCY) == LOW) {
    delay(50); // Debounce
    if (digitalRead(PIN_EMERGENCY) == LOW) {
      bool isLocked = digitalRead(PIN_RELAY);
      digitalWrite(PIN_RELAY, !isLocked); 
      Blynk.virtualWrite(V13, isLocked); 
      digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
      logToInternal(isLocked ? "BTN UNLOCK" : "BTN LOCK");
      while(digitalRead(PIN_EMERGENCY) == LOW) delay(10);
    }
  }

  // IR Wakeup
  if (digitalRead(PIN_IR) == LOW && !systemActive) {
    systemActive = true; 
    activationTimer = millis();
    digitalWrite(PIN_BUZZER, HIGH); delay(20); digitalWrite(PIN_BUZZER, LOW);
    Blynk.virtualWrite(V12, "System Active");
  }

  if (systemActive && (millis() - activationTimer > activePeriod)) {
    systemActive = false;
    Blynk.virtualWrite(V12, "System Standby");
  }

  // RFID Reading
  if (systemActive && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
      uid += String(mfrc522.uid.uidByte[i], HEX);
      if (i < mfrc522.uid.size - 1) uid += " ";
    }
    uid.toUpperCase();
    if (uid == cardUID) handleSuccess("RFID");
    else handleFailure("Wrong Card (" + uid + ")");
    mfrc522.PICC_HaltA();
  }
}