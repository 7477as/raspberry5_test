#pragma once

#include <cstdint>
#include <vector>

namespace alsa_playback {

class AudioResampler {
public:
    AudioResampler(std::uint32_t inputRate, std::uint32_t outputRate, std::uint32_t channels);
    void Process(const std::vector<float>& input, std::vector<float>& output);
    void Reset();

private:
    std::uint32_t mInputRate;
    std::uint32_t mOutputRate;
    std::uint32_t mChannels;
    double mNextPosition = 0.0;
    std::vector<float> mPreviousFrame;
    bool mHasPreviousFrame = false;
};

}  // namespace alsa_playback
