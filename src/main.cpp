#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <Wire.h>
#include <SPI.h>
#include "Adafruit_PN532.h"

#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

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
uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
uint8_t success = 0;
bool interruptTriggered = false;

// Audio
AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2S *out;

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}


void setup()
{
  Serial.begin(115200);
  delay(5000);
  
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

  // Init audio
  out = new AudioOutputI2S();
  mp3 = new AudioGeneratorMP3();

  // Register IRQ
  pinMode(PN532_IRQ, INPUT_PULLUP);
  nfc.readPassiveTargetIDNonBlocking(PN532_MIFARE_ISO14443A);
  delay(500);
  attachInterrupt(digitalPinToInterrupt(PN532_IRQ), handleInterrupt, FALLING);
}

void handleInterrupt() {
  Serial.println("Interrupt");
  interruptTriggered = true;
}


void StopPlaying() {
  if (mp3) {
    if (mp3->isRunning()) {
      mp3->stop();
      delete mp3;
      mp3 = NULL;
    }
  }
  if (buff) {
    buff->close();
    delete buff;
    buff = NULL;
  }
  if (file) {
    file->close();
    delete file;
    file = NULL;
  }
}

void readAudio() {
  StopPlaying();
  Serial.println("After stop");
  file = new AudioFileSourceICYStream("http://iopush.net/nfcAudio/mp3/LaPetitePouleRousse.mp3");
  file->RegisterMetadataCB(MDCallback, (void*)"ICY");
  buff = new AudioFileSourceBuffer(file, 4096);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
  out->SetGain(5.0/100.0);
  //mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
  mp3->begin(buff, out);
}


void processUid(uint8_t* uid, uint8_t uidLength) {
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
      Serial.println(line);
      readAudio();
    }
  }
}

void loop()
{

    
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  // if (!mp3->isRunning()) nfc.readPassiveTargetIDNonBlocking(PN532_MIFARE_ISO14443A);

  if (interruptTriggered == true) {
    success = nfc.readPassiveTargetIDNonBlockingAfterInterrupt(uid, &uidLength);
    delay(500);
    interruptTriggered = false;
    // detachInterrupt(PN532_IRQ);
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
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
      processUid(uid, 4);
    }
    
    if (uidLength == 7) {
      // We probably have a Mifare Ultralight card ...
      Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");
      processUid(uid, 7);
    }

    success = 0;
    
    nfc.readPassiveTargetIDNonBlocking(PN532_MIFARE_ISO14443A);
    
  }

  // Audio stuff
  static int lastms = 0;

  if (mp3->isRunning()) {
    if (millis()-lastms > 1000) {
      lastms = millis();
      Serial.printf("Running for %d ms...\n", lastms);
      Serial.flush();
     }
    if (!mp3->loop()) mp3->stop();
  } else {
    Serial.printf("MP3 done\n");
    delay(1000);
  }
}

