#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <Wire.h>
#include <SPI.h>
#include "Adafruit_PN532.h"

#include "nfcPlayer.hpp"

#include "secrets.h"

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (14)
#define PN532_MISO (12)
#define PN532_MOSI (13)
#define PN532_SS   (5)

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (4)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

// Hardware SPI
Adafruit_PN532 nfc(PN532_SS);


uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
uint8_t uidPlaying[]    = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the playing UID
uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
uint8_t success = 0;
uint32_t timeoutNfc = 0;
bool interruptTriggered = false;
bool readerDisabled = false;


// Objects
nfcPlayer player;


// NFC interrupt handler
void ICACHE_RAM_ATTR handleInterrupt() {
  detachInterrupt(PN532_IRQ);
  interruptTriggered = true;
  readerDisabled = true;
}


void setup() {
  Serial.begin(115200);
  
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nIP address: ");
  Serial.println(WiFi.localIP());

  // NFC
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  // configure board to read RFID tags
  nfc.SAMConfig();
  Serial.println("Waiting for an ISO14443A Card ...");

  // Register IRQ
  pinMode(PN532_IRQ, INPUT_PULLUP);
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
  delay(500);
  attachInterrupt(digitalPinToInterrupt(PN532_IRQ), handleInterrupt, FALLING);
}


void processUid(uint8_t* uid, uint8_t uidLength) {
  String mp3Url;

  Serial.println("processUid");
  // Process volume
  if (memcmp(uid, uidVolumeDown, uidVolumeLen) == 0) {
    player.volumeDown();
    return;
  }
  if (memcmp(uid, uidVolumeUp, uidVolumeLen) == 0) {
    player.volumeUp();
    return;
  }
  // Stop playing in all cases
  if (player.isRunning()) player.stop();

  // Stop card
  if (memcmp(uid, uidStop, uidVolumeLen) == 0) {
    return;
  }

  // Try to read NTAG2xx memory and extract an URL
  // TODO: mifare classic/ultralight
  // TODO: Parsing from the app "NFC Reader" on Android Play store, not only the spec. To be validated
  // Read only the first record
  
  if (uidLength == 7) {

    uint8_t headerPage[4];
    // Read tag type, if any
    nfc.ntag2xx_ReadPage(4, headerPage);
    Serial.print("Header 4: ");
    nfc.PrintHex(headerPage, 4);
    Serial.println();
    if (headerPage[1] == 0x03) {
      Serial.println("NDEF RECORD");
      // Read header
      nfc.ntag2xx_ReadPage(5, headerPage);
      Serial.print("Header 5: ");
      nfc.PrintHex(headerPage, 4);
      Serial.println();
      if ((headerPage[3] && 0x07) == 0x01) {
        nfc.ntag2xx_ReadPage(6, headerPage);
        Serial.print("Header 6: ");
        nfc.PrintHex(headerPage, 4);
        Serial.println();
        Serial.println("NDEF - Well know record");
        if (headerPage[2] == 0x55) {
          Serial.println("NDEF - Well know URI");
          switch(headerPage[3]) {
            case 0x01:
              mp3Url = "http://www.";
              break;
            case 0x02:
              mp3Url = "https://www.";
              break;
            case 0x03:
              mp3Url = "http://";
              break;
            case 0x04:
              mp3Url = "https://";
              break;
            default:
              Serial.print("NDEF - Value: '0x");
              Serial.print(headerPage[3], HEX);
              Serial.println("' unknown.");
          }
          // Read URL
          uint8_t data[((headerPage[1]-1)/4+1)*4+1];
          for (uint8_t page = 7; page < ((headerPage[1]-1)/4+1) + 7; page ++) {
            nfc.ntag2xx_ReadPage(page, data + (page - 7) * 4);
          }
          data[headerPage[1]-1] = '\0';
          nfc.PrintHex(data, 12);
          mp3Url += String((char *) data);
          Serial.print("NDEF - URL: ");
          Serial.println(mp3Url);
        }
      }
    }
  }

  if (mp3Url.length() == 0) {
    // Try to connect to the server in order to get an URL to play
    WiFiClient client;
    if (!client.connect(host, port)) {
      Serial.println("connection failed");
      return;
    }
    String url = String("/nfcAudio/uid/0x");
    for (uint8_t i = 0; i< uidLength; i++) {
      url += String(uid[i], HEX);
    }
    Serial.print("URL: ");
    Serial.println(url);
    // Send request
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                "Host: " + host + "\r\n" +
                "Connection: close\r\n\r\n"
      );
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
    // Read all the lines of the reply from server and print them to Serial
    uint8_t lineNumber = 1;
    while (client.available()) {
      String line = client.readStringUntil('\r');
      lineNumber++;
      /*
      Serial.print(lineNumber);
      Serial.print(" - ");
      Serial.println(line);
      */
      if (lineNumber==10) {
        Serial.print("Reading: ");
        // Remove CR then LF
        mp3Url = line.substring(1,line.length()-1); // TODO - Might be better to move it for each line
      }
    }
  }

  if (mp3Url.length() > 0) {
    Serial.printf("Playing: %s\n", mp3Url.c_str());
    player.readAudio(mp3Url);
    memcpy(uidPlaying, uid, uidLength);
    timeoutNfc = millis();
  }
}


void loop() {    
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)

  if (interruptTriggered == true) {
    success = nfc.readDetectedPassiveTargetID(uid, &uidLength);
    interruptTriggered = false;
  }
  
  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (uidLength == 4) {
      // We probably have a Mifare Classic card ... 
      processUid(uid, 4);
    }
    
    if (uidLength == 7) {
      // We probably have a Mifare Ultralight card or NTAG ...
      processUid(uid, 7);
    }

    // Rearm for next tag, 
    success = 0;
    
  }

  if (readerDisabled == true) {
    // Reactivate reader after timeout
    if (abs(millis() - timeoutNfc) > timeoutAfterPlay) {
      readerDisabled = false;
      nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
      attachInterrupt(digitalPinToInterrupt(PN532_IRQ), handleInterrupt, FALLING);
    }
  }

  // Audio stuff
  static int lastms = 0;

  if (player.isRunning()) {
    if (millis()-lastms > 1000) {
      lastms = millis();
      Serial.printf("Running for %d ms...\n", lastms);
      Serial.flush();
     }
    if (!player.loop()) player.stop();
  } else {
    Serial.printf("MP3 done\n");
    delay(1000);
  }
}
