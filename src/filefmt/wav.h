#ifndef WAV_H
#define WAV_H

#include "cvtstream.h"
#include "fileprober.h"

constexpr int wavDefaultNumChannels = 2;
constexpr int wavDefaultSampleRate = 48000;
constexpr int wavDefaultBitsPerSample = 16;

constexpr int wavMaxNumChannels = 6;
constexpr int wavMaxSampleRate = 96000;

struct WavParams final
{
    int numChannels   = wavDefaultNumChannels;
    int sampleRate    = wavDefaultSampleRate;
    int bitsPerSample = wavDefaultBitsPerSample;
};

class WavFileReader:
        public IRandomReader,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<WavFileReader>;

    virtual void open(const std::string & fileName) = 0;
    virtual void close() = 0;

    virtual auto params() const -> WavParams = 0;

private:
    class Impl;
    explicit WavFileReader() = default;
};

class WavFileWriter:
        public IRandomWriter,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<WavFileWriter>;

    virtual void open(const std::string & fileName, bool trunc, const WavParams & params = {}) = 0;
    virtual void close() = 0;

    virtual auto params() const -> WavParams = 0;

private:
    class Impl;
    explicit WavFileWriter() = default;
};

class WavFileProber:
        public FileProber
{
public:
    static auto create() -> std::unique_ptr<WavFileProber>;

private:
    class Impl;
    explicit WavFileProber() = default;
};

class WavCvt:
        public ICvt,
        public Logger
{
public:
    static auto create() -> std::unique_ptr<WavCvt>;

    virtual void setSourceParams(const WavParams & params) = 0;
    virtual void setTargetParams(const WavParams & params) = 0;
    virtual void setSourceChannel(int channel) = 0;

private:
    class Impl;
    explicit WavCvt() = default;
};

#endif // WAV_H
