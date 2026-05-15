// =============================================================================
// RFID MATCH / NO MATCH
// =============================================================================
// Scan a card -> if it matches the authorized UID, OLED shows "MATCH" and
// buzzer plays a happy double-beep. Otherwise shows "NO MATCH" and a long
// unhappy beep. After 5 seconds, OLED clears and waits for the next scan.
// =============================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <MFRC522.h>

// ----- CONFIG -- paste your card's UID from the helper sketch -----
const byte AUTHORIZED_UID[4] = {0xA2, 0x6A, 0x6C, 0x06};   // <-- CHANGE
const byte AUTHORIZED_CARD[4] = {0xF1, 0x2E, 0x12, 0x01};   // <-- CHANGE

// ----- PINS -----
#define BUZZER_PIN  27
#define RFID_SS     5
#define RFID_RST    4

// ----- HARDWARE OBJECTS -----
Adafruit_SSD1306 display(128, 64, &Wire, -1);
MFRC522 rfid(RFID_SS, RFID_RST);

// ----- STATE -----
unsigned long showUntilMs = 0;   // when to clear the screen (0 = already clear)


void setup() {
  Serial.begin(115200);
  Serial.println("\n=== RFID Match / No Match ===");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (true);
  }
  display.setTextColor(SSD1306_WHITE);

  // RFID
  SPI.begin();
  rfid.PCD_Init();

  showIdle();
}


// ---- Display helpers ----
void showIdle() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.println("  Tap a card...");
  display.display();
}

void showMatch() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(15, 20);
  display.println("MATCH");
  display.display();
}

void showNoMatch() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 8);
  display.println("NO MATCH");
  display.setTextSize(1);
  display.setCursor(10, 44);
  display.println("Unknown card");
  display.display();
}


// ---- Buzzer patterns (blocking; runs to completion) ----
void beepMatch() {
  // Two short happy beeps
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(100);
    digitalWrite(BUZZER_PIN, LOW);  delay(80);
  }
}

void beepNoMatch() {
  // One long unhappy beep
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);
}


// ---- Check if scanned card matches authorized UID ----
bool isAuthorized() {
  if (rfid.uid.size != 4) return false;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != AUTHORIZED_UID[i]) return false;
  }
  return true;
}


// ---- Print the scanned UID to Serial (helpful for debugging) ----
void printScannedUid() {
  Serial.print("Scanned UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}


void loop() {
  // 1. Clear the screen 5 seconds after the last result
  if (showUntilMs != 0 && millis() >= showUntilMs) {
    showIdle();
    showUntilMs = 0;
  }

  // 2. Check for a new card scan
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial())   return;

  printScannedUid();

  if (isAuthorized()) {
    Serial.println("--> MATCH");
    showMatch();
    beepMatch();
  } else {
    Serial.println("--> NO MATCH");
    showNoMatch();
    beepNoMatch();
  }

  // Schedule the screen to clear in 5 seconds
  showUntilMs = millis() + 5000;

  rfid.PICC_HaltA();   // tell the card to stop responding
}