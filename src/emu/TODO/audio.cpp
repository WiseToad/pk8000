#include "audio.h"
#include "media.h"
#include "system.h"
#include "machine.h"
#include "wav.h"
#include "log.h"
#include "filesys.h"
#include "libretro.h"
#include <memory>
#include <sstream>

const int readBufferSeconds = 1; // wav read buffer capacity

const int16_t comparatorLevel = 8192;

const int minPulseFreq = 800;    // min record pulse freq = 2400 (base freq) / 2 (1's encoding) / 1.5 (some margin)
const int detectFrameCount = 3;  // frames to exactly detect record or silence (formely the detect accumulator depth)
const int framesPerGap = 40;     // silence that bios writes before, after and between tape records

const int32_t hipassRC = 500;    // highpass time constant in sample periods, >= 1
const int32_t hipassAlpha = 32768 * hipassRC / (hipassRC + 1);

const int speedupNum = 3, speedupDen = 2;

const int scanNum = 3, scanDen = 2;
const int scanCycles = 10;

    //--//

using Sample8  = uint8_t;
using Sample32 = int32_t;

enum struct SignalType {
    silence, record
};

struct AudioIn final
{
    int sampleRate;
    int samplesPerFrame;

    std::vector<Sample16> samples;
    int sampleCount;
    int samplePos;

    bool bit;

    std::vector<bool> bits;
    int bitSize;
    int bitCount;
    int bitOffset;
    int clkOffset;
    int bias;

    int pulseChunk;
    int pulseTaken;
    int detectAccum;

    SignalType signalType;
    int recordSampleCount;
    int silenceSampleCount;
};

struct AudioOut final
{
    struct Edge final {
        int pos;
        bool bit;
    };

    std::vector<Edge> edges;
    int edgeCount;
    int edgePos;

    bool bit;
    int edgeSamplePos;

    Sample32 prevSample;
    Sample32 hipassSample;

    std::vector<Sample32> samples;
    int samplePos;
};

enum struct PlayStatus {
    halt,       // no background/forefround scans or playback
    passive,    // background scan during stop or record media mode
    active,     // regular playback, speedup and forward/reverse scans
    finish      // about to deactivate and change to halt/passive status
};

enum struct ScanStatus {
    passive,    // scan for next record if silence gets longer than two 'gaps'
    start,      // scan for record start
    active,     // scan for record end
    finish      // scan for next record start
};

    //--//

void mediaStop();
void mediaStartRecord();
void mediaStartPlay(int speedupNum, int speedupDen = 1);
void mediaStartPlay();
void mediaStartSpeedup();
void mediaStartFwdScan();
void mediaStartRevScan();

int clkToPos(int clk, int samplesPerFrame, int bias = 0);

void openWavReader();
void readWav(bool startFrame, bool reverse);
void readWav();
void scanWav();

void renderAudioBit(AudioOut * out, int toPos);
void renderAudioOut(AudioOut * out, int toPos);
void renderAudioIn(AudioIn * in, AudioOut * out, int toPos);
void renderAudio(int toPos);

void compress(Sample16 * targetBuf, const Sample32 * sourceBuf, int sampleCount);

    //--//

const int edgeSampleCount = 5;
Sample16 edgeSamples[edgeSampleCount] = {-32767, -24576, 0, 24576, 32767};

std::vector<Sample16> audioBuf;

AudioIn tapeIn;
AudioOut beepOut;
AudioOut tapeOut;

std::unique_ptr<WavReader> wavReader;
std::unique_ptr<WavWriter> wavWriter;

MediaMode mediaMode;
PlayStatus playStatus;
ScanStatus scanStatus;
bool scanEof;   // whether eof reached during background scan

bool frameActive;

    //--//

void initAudio()
{
    beepOut.edges.resize(clkPerFrame / 10 + 64); // approx. max count of cpu out instructions plus some margin
    beepOut.edgeSamplePos = 0;
    beepOut.prevSample = edgeSamples[0];
    beepOut.hipassSample = 0;
    beepOut.samples.resize(samplesPerFrame);

    tapeOut.edges.resize(clkPerFrame / 10 + 64);
    tapeOut.edgeSamplePos = 0;
    tapeOut.prevSample = edgeSamples[0];
    tapeOut.hipassSample = 0;
    tapeOut.samples.resize(samplesPerFrame);

    wavReader = nullptr;
    wavWriter = nullptr;

    mediaMode = MediaMode::stop;
    playStatus = PlayStatus::halt;

    resetAudio();
}

void resetAudio()
{
    closeAudio();

    beepOut.bit = false;
    tapeOut.bit = false;

    frameActive = false;

    if(playMedia.fmt == FileFmt::wav) {
        openWavReader();
        tapeIn.bitCount = 0;
        tapeIn.samplesPerFrame = (tapeIn.sampleRate * scanNum) / (scanDen * fps);
        playStatus = PlayStatus::passive;
        scanEof = false;
    }
}

void closeAudio()
{
    wavReader = nullptr; // delete wavReader
    playStatus = PlayStatus::halt;

    setMediaMode(MediaMode::stop);
}

void startAudioFrame()
{
    beepOut.edgeCount = 0;
    beepOut.edgePos = 0;
    beepOut.samplePos = 0;

    tapeOut.edgeCount = 0;
    tapeOut.edgePos = 0;
    tapeOut.samplePos = 0;

    if(playStatus == PlayStatus::active)
        readWav();
    else
    if(playStatus == PlayStatus::passive)
        scanWav();

    frameActive = true;
}

void endAudioFrame()
{
    frameActive = false;

    if(playStatus == PlayStatus::finish) {
        setMediaMode(MediaMode::stop);
        if(!wavReader->valid()) {
            wavReader = nullptr; // delete wavReader
            playStatus = PlayStatus::halt;
        }
        else if(wavReader->eof()) {
            wavReader->seekSample(0);
            scanEof = true;
        }
    }
}

    //--//

void setMediaMode(MediaMode mode)
{
    if(mode == mediaMode)
        return;

    mediaStop();

    switch(mode) {
        default:
            mode = MediaMode::stop;
        case MediaMode::stop:
            break;
        case MediaMode::record:
            mediaStartRecord();
            break;
        case MediaMode::play:
            mediaStartPlay();
            break;
        case MediaMode::speedup:
            mediaStartSpeedup();
            break;
        case MediaMode::fwdScan:
            mediaStartFwdScan();
            break;
        case MediaMode::revScan:
            mediaStartRevScan();
            break;
    }
}

void mediaStop()
{
    // render samples with current speedup until current timepoint
    // and commit rendered samples into output stream (if open)
    if(frameActive)
        renderAudio(clkToPos(cpu.clk, samplesPerFrame));

    wavWriter = nullptr; // delete wavWriter

    if(playStatus == PlayStatus::active || playStatus == PlayStatus::finish) {
        if(frameActive) {
            tapeIn.bitOffset += clkToPos(cpu.clk - tapeIn.clkOffset,
                                         tapeIn.samplesPerFrame, tapeIn.bias);
            tapeIn.bitSize = tapeIn.bitOffset;
        }
        tapeIn.samplesPerFrame = (tapeIn.sampleRate * scanNum) / (scanDen * fps);

        // adjust sample pos if playback direction gets reversed
        if(mediaMode == MediaMode::revScan)
            ++tapeIn.samplePos;

        playStatus = PlayStatus::passive;
        scanEof = false;
    }

    mediaMode = MediaMode::stop;
}

void mediaStartRecord()
{
    // TODO: wtf below? forgot... test it ASAP!
    if(playMedia.fmt == FileFmt::wav) {
        msg("No valid media to record");
        return;
    }

    wavWriter = nullptr;
    if(saveDir.empty()) {
        log(LogLevel::error, "Save directory is unknown");
    } else
    if(mkdir(saveDir)) {
        std::stringstream fname;
        fname << filename(saveDir + "/pk8000.") << sys.started << ".save.wav";
        wavWriter = std::make_unique<WavWriter>(fname.str(), 1, sampleRate, 16);
    }
    if(!wavWriter || !wavWriter->valid()) {
        msg("Error during media record");
        wavWriter = nullptr;
        return;
    }

    msg("Recording into save directory");
    mediaMode = MediaMode::record;
}

void mediaStartPlay(int speedupNum, int speedupDen)
{
    if(playMedia.fmt != FileFmt::wav) {
        msg("No valid media to play");
        return;
    }

    bool reverse = false;
    if(speedupNum < 0) {
        reverse = !reverse;
        speedupNum = -speedupNum;
    }
    if(speedupDen < 0) {
        reverse = !reverse;
        speedupDen = -speedupDen;
    }
    if(!speedupDen) {
        log(LogLevel::error, "Invalid playback speedup denominator");
        return;
    }

    if(!wavReader) {
        openWavReader();
        tapeIn.bitCount = 0;
        tapeIn.bitOffset = 0;
    }

    tapeIn.samplesPerFrame = (tapeIn.sampleRate * speedupNum) / (speedupDen * fps);

    playStatus = PlayStatus::active;

    if(frameActive) {
        tapeIn.clkOffset = cpu.clk;
        tapeIn.bias = (cpu.clk * tapeIn.samplesPerFrame) % clkPerFrame;
        tapeIn.bitSize = tapeIn.bitOffset + clkToPos(clkPerFrame - tapeIn.clkOffset,
                                                     tapeIn.samplesPerFrame, tapeIn.bias);
        if(tapeIn.bitSize > int(tapeIn.bits.size()))
            tapeIn.bits.resize(tapeIn.bitSize);

        if(tapeIn.bitCount < tapeIn.bitSize)
            readWav(false, reverse);
    }
}

void mediaStartPlay()
{
    mediaStartPlay(1); // forward playback with normal speed
    if(playStatus != PlayStatus::active &&
            playStatus != PlayStatus::finish)
        return;

    mediaMode = MediaMode::play;
    scanStatus = ScanStatus::passive;
}

void mediaStartSpeedup()
{
    mediaStartPlay(speedupNum, speedupDen); // forward playback with 3/2 speedup
    if(playStatus != PlayStatus::active &&
            playStatus != PlayStatus::finish)
        return;

    mediaMode = MediaMode::speedup;
    scanStatus = ScanStatus::passive;
}

void mediaStartFwdScan()
{
    mediaStartPlay(scanNum, scanDen); // forward playback with double speedup
    if(playStatus != PlayStatus::active &&
            playStatus != PlayStatus::finish)
        return;

    if(tapeIn.signalType == SignalType::record) {
        scanStatus = ScanStatus::active;
    } else {
        tapeIn.silenceSampleCount = 0;
        scanStatus = ScanStatus::start;
    }

    mediaMode = MediaMode::fwdScan;
}

void mediaStartRevScan()
{
    mediaStartPlay(-scanNum, scanDen); // reverse playback with double speedup
    if(playStatus != PlayStatus::active &&
            playStatus != PlayStatus::finish)
        return;

    // adjust sample pos because playback direction gets reversed
    --tapeIn.samplePos;

    if(tapeIn.signalType == SignalType::silence) {
        scanStatus = ScanStatus::active;
    } else {
        tapeIn.recordSampleCount = 0;
        scanStatus = ScanStatus::start;
    }

    mediaMode = MediaMode::revScan;
}

    //--//

inline int clkToPos(int clk, int samplesPerFrame, int bias)
{
    return (clk * samplesPerFrame + bias) / clkPerFrame;
}

void audioBit(AudioOut * out, bool bit)
{
    if(!frameActive)
        return;

    int pos = out->edgeCount;
    int size = out->edges.size();
    if(pos >= size)
        pos = size - 1;
    else
        ++out->edgeCount;

    out->edges[pos] = AudioOut::Edge {
            clkToPos(cpu.clk, samplesPerFrame),
            bit
    };
}

void beepBit(bool bit)
{
    audioBit(&beepOut, bit);
}

void tapeBit(bool bit)
{
    if(playStatus == PlayStatus::active ||
            playStatus == PlayStatus::finish)
        return;

    audioBit(&tapeOut, bit);
}

bool tapeBit()
{
    if(!frameActive)
        return false;

    if(mediaMode != MediaMode::play &&
            mediaMode != MediaMode::speedup)
        return false;

    int pos = tapeIn.bitOffset + clkToPos(cpu.clk - tapeIn.clkOffset,
                                          tapeIn.samplesPerFrame, tapeIn.bias);
    if(pos >= tapeIn.bitCount)
        pos = tapeIn.bitCount - 1;
    if(pos < 0)
        return false;

    return tapeIn.bits[pos];
}

    //--//

void openWavReader()
{
    wavReader = std::make_unique<WavReader>(playMedia.filename);

    if(wavReader->valid()) {
        tapeIn.samplesPerFrame = wavReader->sampleRate() / fps; // there will be equal number of samples per frame
        tapeIn.sampleRate = tapeIn.samplesPerFrame * fps;
        tapeIn.samples.resize(tapeIn.sampleRate * wavReader->numChannels() * readBufferSeconds);
    } else {
        tapeIn.samplesPerFrame = 1;
        tapeIn.sampleRate = fps;
    }
    tapeIn.sampleCount = 0;
    tapeIn.samplePos = 0;
    tapeIn.bit = false;
    tapeIn.pulseChunk = 0;
    tapeIn.pulseTaken = 0;
    tapeIn.detectAccum = 0;
    tapeIn.signalType = SignalType::silence;
    tapeIn.recordSampleCount = 0;
    tapeIn.silenceSampleCount = 0;
}

void readWav(bool startFrame, bool reverse)
{
    if(startFrame) {
        // move remaining tail of bits at the beginning of the buffer
        std::copy(tapeIn.bits.begin() + tapeIn.bitSize,
                  tapeIn.bits.begin() + tapeIn.bitCount,
                  tapeIn.bits.begin());

        tapeIn.bitCount = std::max(0, tapeIn.bitCount - tapeIn.bitSize);
        tapeIn.bitSize = clkToPos(clkPerFrame, tapeIn.samplesPerFrame);
        tapeIn.bits.resize(std::max(tapeIn.bitSize, tapeIn.bitCount));

        tapeIn.bitOffset = 0;
        tapeIn.clkOffset = 0;
        tapeIn.bias = 0;
    }

    int maxPulseWidth = tapeIn.sampleRate / minPulseFreq;
    int detectSampleCount = tapeIn.samplesPerFrame * detectFrameCount;

    for(;;) {
        int sampleAvail;
        int sampleInc;
        if(!reverse) {
            sampleAvail = tapeIn.sampleCount - tapeIn.samplePos;
            sampleInc = 1;
        } else { // reverse scan
            sampleAvail = tapeIn.samplePos + 1;
            sampleInc = -1;
        }
        int bitAvail = tapeIn.bitSize - tapeIn.bitCount;
        int bitBreak = tapeIn.bitCount + std::min(sampleAvail, bitAvail);
        for(;;) {
            // apply comparator
            int bitCount = tapeIn.bitCount;
            if(tapeIn.bit) {
                while(tapeIn.bitCount < bitBreak &&
                        tapeIn.samples[tapeIn.samplePos] > -comparatorLevel)
                {
                    tapeIn.bits[tapeIn.bitCount++] = tapeIn.bit;
                    tapeIn.samplePos += sampleInc;
                }
            } else {
                while(tapeIn.bitCount < bitBreak &&
                        tapeIn.samples[tapeIn.samplePos] < comparatorLevel)
                {
                    tapeIn.bits[tapeIn.bitCount++] = tapeIn.bit;
                    tapeIn.samplePos += sampleInc;
                }
            }
            tapeIn.pulseChunk += (tapeIn.bitCount - bitCount);
            // detect signal type used then for fast record scans
            if(tapeIn.pulseChunk <= maxPulseWidth) {
                if(tapeIn.bitCount >= bitBreak)
                    break;
                int pulseTaken = tapeIn.pulseChunk - tapeIn.pulseTaken;
                tapeIn.pulseTaken = tapeIn.pulseChunk;
                tapeIn.detectAccum += pulseTaken;
                if(tapeIn.detectAccum >= detectSampleCount) {
                    if(tapeIn.signalType != SignalType::record) {
                        tapeIn.recordSampleCount = tapeIn.detectAccum - detectSampleCount;
                        tapeIn.silenceSampleCount += (pulseTaken - tapeIn.recordSampleCount);
                        tapeIn.signalType = SignalType::record;
                    } else {
                        tapeIn.recordSampleCount += pulseTaken;
                    }
                    tapeIn.detectAccum = detectSampleCount;
                } else {
                    if(tapeIn.signalType == SignalType::record)
                        tapeIn.recordSampleCount += pulseTaken;
                    else
                        tapeIn.silenceSampleCount += pulseTaken;
                }
            } else {
                int pulseTaken = tapeIn.pulseChunk - tapeIn.pulseTaken;
                tapeIn.pulseTaken = tapeIn.pulseChunk;
                tapeIn.detectAccum -= pulseTaken;
                if(tapeIn.detectAccum <= 0) {
                    if(tapeIn.signalType != SignalType::silence) {
                        tapeIn.silenceSampleCount = -tapeIn.detectAccum;
                        tapeIn.recordSampleCount += (pulseTaken - tapeIn.silenceSampleCount);
                        tapeIn.signalType = SignalType::silence;
                    } else {
                        tapeIn.silenceSampleCount += pulseTaken;
                    }
                    tapeIn.detectAccum = 0;
                } else {
                    if(tapeIn.signalType == SignalType::silence)
                        tapeIn.silenceSampleCount += pulseTaken;
                    else
                        tapeIn.recordSampleCount += pulseTaken;
                }
                if(tapeIn.bitCount >= bitBreak)
                    break;
            }
            tapeIn.pulseTaken = tapeIn.pulseChunk = 0;
            tapeIn.bit = !tapeIn.bit;
        }
        if(tapeIn.bitCount >= tapeIn.bitSize)
            return;
        // fallback to wav reader
        if(!reverse) {
            tapeIn.sampleCount = wavReader->readSamples(tapeIn.samples.data(), tapeIn.samples.size());
            tapeIn.samplePos = 0;
        } else { // reverse scan
            int sampleCount = tapeIn.samples.size();
            int samplePos = int(wavReader->samplePos()) - tapeIn.sampleCount - sampleCount;
            if(samplePos < 0) {
                sampleCount += samplePos;
                samplePos = 0;
            }
            wavReader->seekSample(samplePos);
            tapeIn.sampleCount = wavReader->readSamples(tapeIn.samples.data(), sampleCount);
            if(tapeIn.sampleCount != sampleCount)
                tapeIn.sampleCount = 0;
            tapeIn.samplePos = tapeIn.sampleCount - 1;
        }
        if(tapeIn.sampleCount <= 0)
            break;
    }

    // wav state is invalid or stream bound reached
    while(tapeIn.bitCount < tapeIn.bitSize)
        tapeIn.bits[tapeIn.bitCount++] = false;

    if(wavReader->valid() && !wavReader->bof() && !wavReader->eof())
        log(LogLevel::warn, "Inconsistent WAV reader state");

    if(playStatus == PlayStatus::passive) {
        if(wavReader->valid() && wavReader->eof() && !scanEof) {
            wavReader->seekSample(0);
            scanEof = true;
        } else {
            playStatus = PlayStatus::halt;
        }
    } else
    if(playStatus == PlayStatus::active) {
        if(!wavReader->valid())
            msg("Error during media playback");
        else
        if(wavReader->bof() || wavReader->eof())
            msg( "End of media reached");
        playStatus = PlayStatus::finish;
    }
}

void readWav()
{
    int samplesPerGap = tapeIn.samplesPerFrame * framesPerGap;

    for(int i = 0; i < scanCycles; ++i) {
        readWav(true, mediaMode == MediaMode::revScan);
        if(playStatus != PlayStatus::active)
            return;

        switch(scanStatus) {
            case ScanStatus::passive:
            default:
                if(tapeIn.signalType == SignalType::record) {
                    return;
                }
                if(tapeIn.silenceSampleCount < samplesPerGap * 2) {
                    return;
                }
                break;
            case ScanStatus::start:
                if(mediaMode == MediaMode::revScan) {
                    if(tapeIn.signalType == SignalType::record)
                        break;
                    if(tapeIn.recordSampleCount > samplesPerGap * 2 / 3) {
                        setMediaMode(MediaMode::play);
                        return;
                    }
                } else {
                    if(tapeIn.signalType == SignalType::silence)
                        break;
                    if(tapeIn.silenceSampleCount > samplesPerGap * 2 / 3) {
                        setMediaMode(MediaMode::play);
                        return;
                    }
                }
                scanStatus = ScanStatus::active;
                break;
            case ScanStatus::active:
                if(mediaMode == MediaMode::revScan) {
                    if(tapeIn.signalType == SignalType::silence)
                        break;
                } else {
                    if(tapeIn.signalType == SignalType::record)
                        break;
                }
                scanStatus = ScanStatus::finish;
                break;
            case ScanStatus::finish:
                if(mediaMode == MediaMode::revScan) {
                    if(tapeIn.signalType == SignalType::silence) {
                        setMediaMode(MediaMode::play);
                        return;
                    }
                } else {
                    if(tapeIn.signalType == SignalType::record) {
                        setMediaMode(MediaMode::play);
                        return;
                    }
                }
                break;
        }
    }
}

void scanWav()
{
    for(int i = 0; i < scanCycles; ++i) {
        if(tapeIn.signalType == SignalType::record)
            return;
        readWav(true, false);
        if(playStatus != PlayStatus::passive)
            return;
    }
}

    //--//

void renderAudioBit(AudioOut * out, int toPos)
{
    //TODO: optimize!
    int shrink = 1;
    if(mediaMode == MediaMode::fwdScan || mediaMode == MediaMode::revScan)
        shrink = 16;

    int pos = out->samplePos;
    if(out->bit) {
        while(out->edgeSamplePos < (edgeSampleCount - 1) && pos < toPos)
            out->samples[pos++] = edgeSamples[++out->edgeSamplePos] / shrink;
        while(pos < toPos)
            out->samples[pos++] = edgeSamples[edgeSampleCount - 1] / shrink;
    } else {
        while(out->edgeSamplePos > 0 && pos < toPos)
            out->samples[pos++] = edgeSamples[--out->edgeSamplePos] / shrink;
        while(pos < toPos)
            out->samples[pos++] = edgeSamples[0] / shrink;
    }

    // apply simple high-pass RC-filter for DC removal
    while(out->samplePos < toPos) {
        Sample32 sample = out->samples[out->samplePos];
        out->hipassSample =
                ((out->hipassSample + sample - out->prevSample)
                 * hipassAlpha) / 32768;
        out->prevSample = sample;
        out->samples[out->samplePos++] = out->hipassSample;
    }
}

void renderAudioOut(AudioOut * out, int toPos)
{
    while(out->edgePos < out->edgeCount) {
        AudioOut::Edge & edge = out->edges[out->edgePos];
        int pos = edge.pos;
        if(pos >= toPos)
            break;
        ++out->edgePos;

        renderAudioBit(out, pos);
        out->bit = edge.bit;
    }
    renderAudioBit(out, toPos);
}

void renderAudioIn(AudioIn * in, AudioOut * out, int toPos)
{
    int inPos  = (out->samplePos * in->samplesPerFrame) / samplesPerFrame;
    int bias = (out->samplePos * in->samplesPerFrame) % samplesPerFrame;

    // copy in into out with resampling
    for(;;) {
        bool bit = in->bits[inPos++];
        if(bit == out->bit)
            continue;

        int pos = (inPos * samplesPerFrame + bias) / in->samplesPerFrame;
        if(pos >= toPos)
            break;

        renderAudioBit(out, pos);
        out->bit = bit;
    }
    renderAudioBit(out, toPos);
}

void renderAudio(int toPos)
{
    renderAudioOut(&beepOut, toPos);

    int pos = tapeOut.samplePos;
    if(playStatus == PlayStatus::passive || playStatus == PlayStatus::halt)
        renderAudioOut(&tapeOut, toPos);
    else
        renderAudioIn(&tapeIn, &tapeOut, toPos);

    if(mediaMode == MediaMode::record &&
            wavWriter && wavWriter->valid())
    {
        int sampleCount = toPos - pos;
        std::vector<Sample16> compressBuf(sampleCount);
        compress(compressBuf.data(), tapeOut.samples.data() + pos, sampleCount);

        wavWriter->writeSamples(compressBuf.data(), sampleCount);
        if(!wavWriter->valid()) {
            msg("Error during media record");
            wavWriter = nullptr;
        }
    }
}

void renderAudioFrame()
{
    renderAudio(samplesPerFrame);

    // if some edges get out of frame
    if(beepOut.edgePos < beepOut.edgeCount)
        beepOut.bit = beepOut.edges[beepOut.edgeCount - 1].bit;
    if(tapeOut.edgePos < tapeOut.edgeCount)
        tapeOut.bit = tapeOut.edges[tapeOut.edgeCount - 1].bit;

    std::vector<Sample32> mixBuf(samplesPerFrame);
    std::vector<Sample16> compressBuf(samplesPerFrame);

    // mix two channels into single target buffer and apply compressor
    for(int i = 0; i < samplesPerFrame; ++i)
        mixBuf[i] = beepOut.samples[i] + tapeOut.samples[i];
    compress(compressBuf.data(), mixBuf.data(), samplesPerFrame);

    audioBuf.resize(samplesPerFrame * 2);

    // expand mono into target stereo buffer for libretro frontend
    for(int i = 0, j = 0; i < samplesPerFrame; ++i) {
        audioBuf[j++] = compressBuf[i];
        audioBuf[j++] = compressBuf[i];
    }
}

void compress(Sample16 * targetBuf, const Sample32 * sourceBuf, int sampleCount)
{
    for(int i = 0; i < sampleCount; ++i) {
        Sample32 source = sourceBuf[i];
        Sample32 target;
        if(source >= 0) { // positive half-wave
            target = source & 16383;                        // target = source % 0.5
            target = (65535 - target) * target / 16384 / 3; // target = (2 - target) * target * 2 / 3
            while(source >= 16384) {                        // while(source >= 0.5) {
                target = (target + 32768) / 2;              //     target = (target + 1) * 0.5
                source -= 16384;                            //     source -= 0.5
            }                                               // }
        } else {          // negative half-wave
            target = source | ~16383;                       // target = source % 0.5
            target = (65536 + target) * target / 16384 / 3; // target = (2 + target) * target * 2 / 3
            while(source < -16384) {                        // while(source < -0.5) {
                target = (target - 32768) / 2;              //     target = (target - 1) * 0.5
                source += 16384;                            //     source += 0.5
            }                                               // }
        }
        targetBuf[i] = Sample16(target);
    }
}
