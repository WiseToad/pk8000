#ifndef AUDIO_H
#define AUDIO_H

#include "video.h"
#include <cstdint>
#include <vector>

// there will be equal number of samples per frame
const int samplesPerFrame = 48000 / fps;
const int sampleRate = samplesPerFrame * fps;

using Sample16 = int16_t;

enum struct MediaMode {
    stop, record, play, speedup, fwdScan, revScan
};

void initAudio();
void resetAudio();
void closeAudio();

void startAudioFrame();
void renderAudioFrame();
void endAudioFrame();

void setMediaMode(MediaMode mode);

void beepBit(bool bit);
void tapeBit(bool bit);
bool tapeBit();

extern std::vector<Sample16> audioBuf; // 2-channel (stereo) interleaved buffer

#endif // AUDIO_H
