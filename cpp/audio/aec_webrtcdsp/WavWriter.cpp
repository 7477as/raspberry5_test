#include "WavWriter.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

constexpr int kBitsPerSample = 16;
constexpr int kBytesPerSample = kBitsPerSample / 8;

void WriteU32Le(FILE* file, unsigned int value) {
    unsigned char bytes[4] = {
        static_cast<unsigned char>(value & 0xFF),
        static_cast<unsigned char>((value >> 8) & 0xFF),
        static_cast<unsigned char>((value >> 16) & 0xFF),
        static_cast<unsigned char>((value >> 24) & 0xFF),
    };
    std::fwrite(bytes, 1, sizeof(bytes), file);
}

void WriteU16Le(FILE* file, unsigned short value) {
    unsigned char bytes[2] = {
        static_cast<unsigned char>(value & 0xFF),
        static_cast<unsigned char>((value >> 8) & 0xFF),
    };
    std::fwrite(bytes, 1, sizeof(bytes), file);
}

short FloatToPcm16(float sample) {
    float clamped = std::clamp(sample, -1.0f, 1.0f);
    return static_cast<short>(std::lrintf(clamped * 32767.0f));
}

}  // namespace

WavWriter::~WavWriter() {
    Close();
}

bool WavWriter::Open(const std::string& path, int sampleRate, int channels) {
    if (sampleRate <= 0 || channels <= 0) {
        return false;
    }
    Close();

    mFile = std::fopen(path.c_str(), "wb");
    if (mFile == nullptr) {
        return false;
    }

    mSampleRate = sampleRate;
    mChannels = channels;
    mFrameCount = 0;
    WritePlaceholderHeader();
    return true;
}

bool WavWriter::AppendInterleaved(const float* samples, int frameCount) {
    if (mFile == nullptr || samples == nullptr || frameCount <= 0) {
        return false;
    }

    std::vector<short> pcm;
    pcm.reserve(static_cast<size_t>(frameCount) * mChannels);
    for (int frame = 0; frame < frameCount; ++frame) {
        for (int channel = 0; channel < mChannels; ++channel) {
            pcm.push_back(FloatToPcm16(samples[frame * mChannels + channel]));
        }
    }

    const size_t written = std::fwrite(pcm.data(), kBytesPerSample, pcm.size(), mFile);
    if (written != pcm.size()) {
        return false;
    }

    mFrameCount += frameCount;
    return true;
}

bool WavWriter::AppendMono(const std::vector<float>& samples) {
    if (mFile == nullptr || mChannels != 1) {
        return false;
    }
    if (samples.empty()) {
        return true;
    }
    return AppendInterleaved(samples.data(), static_cast<int>(samples.size()));
}

bool WavWriter::Close() {
    if (mFile == nullptr) {
        return true;
    }

    PatchHeaderSizes();
    const bool ok = std::fclose(mFile) == 0;
    mFile = nullptr;
    return ok;
}

void WavWriter::WritePlaceholderHeader() {
    std::fwrite("RIFF", 1, 4, mFile);
    WriteU32Le(mFile, 0);
    std::fwrite("WAVE", 1, 4, mFile);

    std::fwrite("fmt ", 1, 4, mFile);
    WriteU32Le(mFile, 16);
    WriteU16Le(mFile, 1);                                        // PCM
    WriteU16Le(mFile, static_cast<unsigned short>(mChannels));
    WriteU32Le(mFile, static_cast<unsigned int>(mSampleRate));
    WriteU32Le(mFile, static_cast<unsigned int>(mSampleRate * mChannels * kBytesPerSample));
    WriteU16Le(mFile, static_cast<unsigned short>(mChannels * kBytesPerSample));
    WriteU16Le(mFile, kBitsPerSample);

    std::fwrite("data", 1, 4, mFile);
    WriteU32Le(mFile, 0);
}

void WavWriter::PatchHeaderSizes() {
    const unsigned int dataBytes =
        static_cast<unsigned int>(mFrameCount * mChannels * kBytesPerSample);

    if (std::fseek(mFile, 4, SEEK_SET) != 0) {
        return;
    }
    WriteU32Le(mFile, 36 + dataBytes);

    if (std::fseek(mFile, 40, SEEK_SET) != 0) {
        return;
    }
    WriteU32Le(mFile, dataBytes);

    std::fseek(mFile, 0, SEEK_END);
}