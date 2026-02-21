# 🚪 Smart IoT RFID & Keypad Door Lock System

An advanced, internet-connected security solution I engineered using the **ESP8266 (NodeMCU)** platform. This project bridges physical security with cloud-based control, featuring multi-factor authentication (MFA), local flash data persistence, and remote maintenance via OTA.

---

## 📑 Table of Contents
1. [Project Gallery & Visuals](#-project-gallery--visuals)
2. [System Logic Flow](#-system-logic-flow)
3. [Key Features](#-key-features)
4. [Hardware Architecture](#-hardware-architecture)
5. [Engineering Trade-offs & Component Selection](#-engineering-trade-offs--component-selection)
6. [Technical Challenges & Solutions](#-technical-challenges--solutions)
7. [Installation & Setup](#-installation--setup)
8. [Future Scope](#-future-scope)
9. [Full Source Code](#-full-source-code)

---

### 🎥 Project Demo Video
Click the image below to watch the full system in action:

[![Smart Lock Demo Video](./images/video_thumbnail.jpg)](https://www.youtube.com/watch?v=YOUR_VIDEO_ID)

## 🛠️ Project Gallery & Visuals

I have documented the physical construction and digital interface to provide a transparent view of the system’s integration.

### I. Hardware Implementation
![Physical Build View](Smart_IoT_RFID_-_Keypad_Door_Lock_System
/images/connections.jpg)
>Shows the Breadboard View.

### II. Circuit Schematic
I mapped the hardware to avoid GPIO pins that interfere with the ESP8266 boot process.

![Circuit Schematic](./images/crkt.png)
> **Circuit Diagram:** Clean schematic detailing the connections between the NodeMCU, MFRC522, IR sensor, and Active-Low Relay.

### III. Digital Interface (Blynk IoT)
I designed a custom mobile dashboard to manage the lock remotely and view real-time logs.

![Blynk App Layout](./images/blynk_dashboard.png)
> **Blynk Dashboard:** Features a Remote Unlock switch, Live Event Terminal, and Storage Management controls.

---

## 📂 System Logic Flow

To ensure the system is both secure and responsive, I designed a specific logic flow that prioritizes power efficiency and strict security validation.

    LOGIC DESIGN:-
     ---[Standby Mode] -->|IR Sensor Detection| -->[Wake-up: Active Mode 10s]
     -->{Authentication}
     -->|RFID Scan| ->[Compare UID] / |Blynk Keypad| ->[Compare Master Code]
     -->|Match| [GRANTED: Unlock Solenoid]
     -->|Mismatch| [DENIED: Sound Buzzer]
     -->[Log Event to LittleFS + NTP Sync]
     -->[Log Denial + Increment Attempts]
     -->|Attempts >= 3| -->[Intruder Alert + Lockdown]
     -->[Standby Mode]

---

## 🔑 Key Features
* **Dual-Layer Authentication:** Physical access via **RFID (MFRC522)** tags combined with a **Virtual Keypad** on the Blynk App.
* **Remote Management:** Real-time lock/unlock capabilities and system status monitoring via the **Blynk IoT Cloud**.
* **Intelligent Power Management:** IR proximity sensor triggers "Active" mode, saving processor cycles during standby.
* **Event Logging:** Persistent log history stored locally in flash memory (**LittleFS**) with **NTP-synchronized** timestamps.
* **Over-the-Air (OTA) Updates:** Wireless firmware updates to eliminate the need for physical access to the module after installation.
* **Intruder Alert:** Automated system lockdown and cloud notifications triggered after 3 consecutive failed attempts.

---

## 🔌 Hardware Architecture

| Component | NodeMCU (ESP8266) Pin | Role |
| :--- | :--- | :--- |
| **Relay (Solenoid)** | **D3** | Actuator control (Active-Low) |
| **Buzzer** | **D4** | Audible feedback |
| **Emergency Button** | **RX (GPIO 3)** | Physical override (Safe for RX during boot) |
| **IR Sensor** | **D1** | System Wake-up/Proximity detection |
| **RFID Reader (SDA/SS)** | **D8** | SPI Chip Select |
| **RFID Reader (SCK)** | **D5** | SPI Clock |
| **RFID Reader (MOSI)** | **D7** | SPI Master Out Slave In |
| **RFID Reader (MISO)** | **D6** | SPI Master In Slave Out |
| **RFID Reader (RST)** | **D2** | SPI Reset |
| **LED** | **D0** | Visual Status Indication |

---

## ⚙️ Engineering Trade-offs & Component Selection

I specifically chose the **ESP8266 (NodeMCU)** over the standard Arduino Uno for several technical reasons:
* **Integrated Connectivity:** Built-in WiFi stack allowed for NTP time-stamping and cloud synchronization without requiring external shields.
* **Memory Management:** 4MB of onboard Flash allowed for **LittleFS** implementation, ensuring local data persistence during power outages.
* **Efficiency:** High clock speed (80MHz) ensured low-latency processing of SPI data while simultaneously handling WiFi background tasks.

---

## ⚠️ Technical Challenges & Solutions

### 1. Resolving Pin Conflicts (Internal Flash)
* **Problem:** Initially, the buzzer was connected to SD3, which caused continuous boot loops. 
* **Solution:** I diagnosed the conflict (SD3 is tied to the internal SPI flash) and relocated the buzzer to **D4** and the button to **RX** to ensure stable boot states.

### 2. Serial Interface Conflicts
* **Problem:** Using the default RX/TX pins for output devices prevented code uploads via USB.
* **Solution:** I moved the buzzer to D4 and utilized the button on RX. Because a physical push-button acts as an "open circuit" by default, it no longer interferes with the USB-to-Serial data transfer.

### 3. Implementing Wireless Maintenance (OTA)
* **Problem:** Physical access to the USB port is difficult post-installation.
* **Solution:** I integrated **ArduinoOTA**, allowing me to push firmware updates wirelessly over the local network.

---

## 🚀 Installation & Setup

To replicate this firmware, the following C++ libraries are required via the Arduino Library Manager:
* `Blynk` by Volodymyr Shymanskyy
* `MFRC522` by GithubCommunity
* `NTPClient` by Fabrice Weinberg

### Blynk Configuration
1. Create a new template in the Blynk Web Dashboard.
2. Set up the following Datastreams (Virtual Pins):
   * `V12` - Terminal (String)
   * `V13` - Switch (Integer 0/1) for Remote Lock/Unlock
   * `V14` - Switch (Integer 0/1) for Manual Buzzer
   * `V16` - Button (Integer 0/1) for View Logs
   * `V17` - Button (Integer 0/1) for Clear Logs
   * `V0 to V11` - Buttons for Virtual Keypad (0-9, Clear, OK)

---

## 🔮 Future Scope
* **Migration to ESP32:** Transitioning the architecture to utilize integrated BLE (Bluetooth Low Energy) and dual-core processing.
* **Biometric Integration:** Adding a capacitive fingerprint sensor for a third factor of authentication in high-security environments.
* **Camera Integration:** Utilizing an ESP32-CAM module to capture and log photos of individuals scanning tags.

---

## 💻 Full Source Code

```cpp
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

#define PIN_IR        D1   
#define PIN_RELAY     D3   
#define PIN_BUZZER    D4    
#define PIN_LED_RED   D0   
#define PIN_EMERGENCY 3     
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

void logToInternal(String message) {
  timeClient.update();
  String timestamp = timeClient.getFormattedTime(); 
  File logFile = LittleFS.open("/log.txt", "a");
  if (logFile) { 
    logFile.println("[" + timestamp + "] " + message); 
    logFile.close(); 
  }
}

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

BLYNK_CONNECTED() {
  if (firstConnect) {
    Blynk.virtualWrite(V12, "ACTIVE");
    digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
    delay(100);
    digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
    firstConnect = false;
  }
  timeClient.update();
}

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

BLYNK_WRITE(V16) {
  if (param.asInt() == 1) {
    File logFile = LittleFS.open("/log.txt", "r");
    if (!logFile) { Blynk.virtualWrite(V12, "No logs."); return; }
    Blynk.virtualWrite(V12, "--- LOG HISTORY ---");
    while (logFile.available()) { Blynk.virtualWrite(V12, logFile.readStringUntil('\n')); }
    logFile.close();
  }
}

BLYNK_WRITE(V17) {
  if (param.asInt() == 1) {
    LittleFS.remove("/log.txt");
    Blynk.virtualWrite(V12, "Storage Cleared");
  }
}

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
  pinMode(PIN_EMERGENCY, INPUT_PULLUP);
  
  digitalWrite(PIN_RELAY, HIGH); 
  digitalWrite(PIN_BUZZER, LOW);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  ArduinoOTA.setHostname("SmartDoorLock-Production");
  ArduinoOTA.begin();

  SPI.begin(); 
  mfrc522.PCD_Init(); 
  LittleFS.begin();
  timeClient.begin();
}

void loop() {
  Blynk.run();
  ArduinoOTA.handle();

  if (digitalRead(PIN_EMERGENCY) == LOW) {
    delay(50);
    if (digitalRead(PIN_EMERGENCY) == LOW) {
      bool isLocked = digitalRead(PIN_RELAY);
      digitalWrite(PIN_RELAY, !isLocked); 
      Blynk.virtualWrite(V13, isLocked); 
      digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
      logToInternal(isLocked ? "BTN UNLOCK" : "BTN LOCK");
      while(digitalRead(PIN_EMERGENCY) == LOW) delay(10);
    }
  }

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

