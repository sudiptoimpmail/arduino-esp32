# sketch_rfid_finder.ino

A helper sketch that prints the UID of any RFID card or tag tapped on the RC522 reader. Use this to discover the UIDs of your cards before hardcoding them into other projects.

## What it does

On boot, initializes the RC522 RFID reader and prints "Tap a card on the reader..." to Serial Monitor. Each time a card is held near the reader, it prints the card's 4-byte unique identifier (UID) in hexadecimal.

## Hardware

The RC522 is a 3.3V SPI device. **Connect VCC to 3.3V, not 5V — 5V will damage the chip.**

| RC522 pin | ESP32 pin |
|---|---|
| VCC | 3V3 (⚠️ NOT 5V) |
| GND | GND |
| RST | GPIO 4 |
| SDA | GPIO 5 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| IRQ | (leave disconnected) |

Note: the "SDA" pin on the RC522 is **not** I2C SDA. It's the SPI chip-select line — confusing naming, but standard for this module.

## Required library

Install via Arduino IDE → Tools → Manage Libraries:

- **MFRC522** by GithubCommunity

SPI is built into Arduino's core libraries.

## Usage

1. Wire the RC522 as shown above
2. Upload the sketch
3. Open Serial Monitor at **115200 baud**
4. Press the EN button on the ESP32
5. You should see:
   ```
   Tap a card on the reader...
   ```
6. Hold an RFID card or tag flat against the RC522 module
7. The card's UID prints to Serial:
   ```
   UID: 0xA3, 0x4F, 0x12, 0x8B
   ```
8. Write down the four hex values — you'll paste them into other sketches as the authorized UID

## What the UID looks like

Standard MIFARE Classic cards (the ones that come with the RC522 kit) have 4-byte UIDs. Newer or larger cards may have 7 bytes. The sketch handles either length but the format above assumes 4.

Each card has a unique factory-assigned UID. Two cards from the same pack of 100 will have different UIDs. This is what makes RFID useful for identification — every card is distinguishable.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Nothing in Serial Monitor | Baud rate not 115200, or wrong port selected |
| Serial shows gibberish | Baud rate dropdown set wrong (set to 115200) |
| Sketch runs but never detects a card | RC522 wiring issue — see diagnostic sketch below |
| Detects something but UID is all zeros or all FF | SPI not working — likely MOSI/MISO swapped or loose wire |

## Diagnostic — is the RC522 even responding?

If tapping cards produces no output, the chip might not be communicating. Run this minimal SPI test:

```cpp
#include <SPI.h>

#define RFID_SS  5
#define RFID_RST 4

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(RFID_SS, OUTPUT);
  pinMode(RFID_RST, OUTPUT);
  digitalWrite(RFID_SS, HIGH);
  digitalWrite(RFID_RST, HIGH);

  SPI.begin();

  // Hard reset
  digitalWrite(RFID_RST, LOW);  delay(50);
  digitalWrite(RFID_RST, HIGH); delay(50);

  // Read RC522 version register (address 0x37)
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(RFID_SS, LOW);
  SPI.transfer(0x80 | (0x37 << 1));
  byte ver = SPI.transfer(0);
  digitalWrite(RFID_SS, HIGH);
  SPI.endTransaction();

  Serial.print("Version register: 0x");
  Serial.println(ver, HEX);
}

void loop() {}
```

A healthy RC522 returns `0x91`, `0x92`, or `0x12`. Anything else (especially `0x00` or `0xFF`) means the chip isn't responding — usually a wiring issue.

## What to do with the UID

Once you have a card's UID, you can use it in any sketch that needs to check for an authorized card. The typical pattern:

```cpp
const byte AUTHORIZED_UID[4] = {0xA3, 0x4F, 0x12, 0x8B};   // your UID

bool isAuthorized() {
  if (rfid.uid.size != 4) return false;
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != AUTHORIZED_UID[i]) return false;
  }
  return true;
}
```

## Multiple authorized cards

To support more than one card, change the constant to a 2D array:

```cpp
const byte AUTHORIZED_UIDS[][4] = {
  {0xA3, 0x4F, 0x12, 0x8B},   // card 1
  {0xC1, 0x22, 0x99, 0x07},   // card 2
};
const int NUM_AUTHORIZED = sizeof(AUTHORIZED_UIDS) / sizeof(AUTHORIZED_UIDS[0]);

bool isAuthorized() {
  if (rfid.uid.size != 4) return false;
  for (int card = 0; card < NUM_AUTHORIZED; card++) {
    bool match = true;
    for (byte i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != AUTHORIZED_UIDS[card][i]) { match = false; break; }
    }
    if (match) return true;
  }
  return false;
}
```

## Notes

- The MIFARE cards in the standard kit are read-only at the UID level — you can't change a card's UID, only read it.
- Some RFID cards include data sectors that can be read and written, but this sketch only reads the UID.
- The reading range is roughly 2-3 cm. Hold the card flat against the module for best results.
- Phones with NFC may also be detectable (though typically not with their MIFARE-compatible UID).
