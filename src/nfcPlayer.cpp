#include "nfcPlayer.hpp"
#include <stdio.h>
#include "Arduino.h" // Dependancy for "Serial" object

#ifdef NFC_PLAYER_DEBUG_ON
  #define DEBUG(input) { Serial.print(input); }
  #define DEBUGln(input) { Serial.println(input); }
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif


// Constructor
nfcPlayer::nfcPlayer() {
  // Init audio
  //out = new AudioOutputI2S();
  /*mp32 = new AudioGeneratorMP3();
  mp3 = mp32;
  buff = buff2;*/
};


// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void nfcPlayer::callbackMetadata(void *cbData, const char *type, bool isUnicode, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  // TODO: Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  DEBUG("METADATA("); DEBUG(ptr); DEBUG(") '"); DEBUG(s1); DEBUG("' = '"); DEBUG(s2); DEBUG("'\n");
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void nfcPlayer::callbackStatus(void *cbData, int code, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  // TODO: Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  DEBUG("STATUS("); DEBUG(ptr); DEBUG(") '"); DEBUG(code); DEBUG("' = '"); DEBUG(s1); DEBUG("'\n");
  // TODO: Serial.flush();
}


bool nfcPlayer::isRunning(AudioGeneratorMP3 *mp3) {
  return mp3->isRunning();
}

bool nfcPlayer::loop(AudioGeneratorMP3 *mp3) {
  bool loop = mp3->loop();
  DEBUG("Loop:"); DEBUGln(loop);
  return loop;
}

bool nfcPlayer::stop(AudioGeneratorMP3 *mp3) {
  return mp3->stop();
}

/*
void nfcPlayer::stopPlaying(void) {
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
*/
void nfcPlayer::readAudio(AudioGeneratorMP3 *mp3, AudioFileSourceBuffer *buff, AudioFileSourceICYStream *file, AudioOutputI2S *out) {
  // stopPlaying();
  Serial.println("After stop");
  file = new AudioFileSourceICYStream("http://iopush.net/nfcAudio/mp3/LaPetitePouleRousse.mp3");
  file->RegisterMetadataCB(callbackMetadata, (void*)"ICY");
  buff = new AudioFileSourceBuffer(file, 4096);
  buff->RegisterStatusCB(callbackStatus, (void*)"buffer");
  out->SetGain(5.0/100.0);
  mp3->RegisterStatusCB(callbackStatus, (void*)"mp3");
  mp3->begin(buff, out);
}