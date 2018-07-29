// To be renaimed in "config.h"

// WiFi credentials
const char* ssid     = "WIFI_SSID";
const char* password = "xxxxxx";
const char* host = "xxxxx.ext";
const int port = 80;


// NFC cards
const uint8_t uidVolumeLen    = 7;
const uint8_t uidVolumeUp[]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
const uint8_t uidVolumeDown[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Others
const uint32_t timeoutBetweenSameCard = 5000;