#pragma once

#include <cstdio>
#include <string>
#include <vector>

class WavWriter {
public:
    WavWriter() = default;
    ~WavWriter();

    WavWriter(const WavWriter&) = delete;
    WavWriter& operator=(const WavWriter&) = delete;

    bool Open(const std::string& path, int sampleRate, int channels);

    // samples: 交错排列的 [-1.0, 1.0] 浮点样本，长度必须是 frameCount * channels 的整数倍
    bool AppendInterleaved(const float* samples, int frameCount);

    bool AppendMono(const std::vector<float>& samples);

    bool Close();

    long long GetFrameCount() const { return mFrameCount; }

private:
    void WritePlaceholderHeader();
    void PatchHeaderSizes();

    FILE* mFile = nullptr;
    int mSampleRate = 0;
    int mChannels = 0;
    long long mFrameCount = 0;
};