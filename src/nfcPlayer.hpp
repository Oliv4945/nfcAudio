#ifndef NFC_PLAYER
#define NFC_PLAYER

#include <stdint.h>
#include "Arduino.h" // Dependancy for "Serial" object

#include "AudioFileSourceICYStream.h"
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
    /*void stopPlaying(void);
    void readAudio(void);
  */
   void readAudio(AudioGeneratorMP3 *mp3, AudioFileSourceBuffer *buff, AudioFileSourceICYStream *file, AudioOutputI2S *out);
   bool isRunning(AudioGeneratorMP3 *mp3);
   bool loop(AudioGeneratorMP3 *mp3);
   bool stop(AudioGeneratorMP3 *mp3);
  private:
    //AudioGeneratorMP3 *mp3;
    //AudioFileSourceICYStream *file;
    //AudioFileSourceBuffer *buff;
    //AudioOutputI2S *out;
};

#endif