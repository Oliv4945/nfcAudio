#ifndef NFC_PLAYER
#define NFC_PLAYER

#include <stdint.h>
#include "Arduino.h" // Dependancy for "Serial" object

#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// Comment to avoid debugging prints
#define NFC_PLAYER_DEBUG_ON

class nfcPlayer {
  public:
    nfcPlayer();
    static void callbackMetadata(void *cbData, const char *type, bool isUnicode, const char *string);
    static void callbackStatus(void *cbData, int code, const char *string);
    void stopPlaying();
    void readAudio(String mp3Url);
    bool isRunning(void);
    bool loop(void);
    bool stop(void);
    void volumeDown(void);
    void volumeUp(void);
  private:
    AudioGeneratorMP3 *mp3;
    AudioFileSourceHTTPStream *file;
    AudioFileSourceBuffer *buff;
    AudioOutputI2S *out;
    uint8_t volume;
};

#endif