#include "format/AudioResampler.h"

#include <algorithm>
#include <cstddef>

namespace alsa_playback {

AudioResampler::AudioResampler(std::uint32_t inputRate,
                               std::uint32_t outputRate,
                               std::uint32_t channels)
    : mInputRate(inputRate), mOutputRate(outputRate), mChannels(channels) {
}

void AudioResampler::Process(const std::vector<float>& input, std::vector<float>& output) {
    output.clear();
    if (mInputRate == 0 || mOutputRate == 0 || mChannels == 0 || input.size() % mChannels != 0) {
        return;
    }
    const std::size_t inputFrames = input.size() / mChannels;
    std::vector<float> combined;
    if (mHasPreviousFrame) {
        combined.insert(combined.end(), mPreviousFrame.begin(), mPreviousFrame.end());
    }
    combined.insert(combined.end(), input.begin(), input.end());
    const double step = static_cast<double>(mInputRate) / mOutputRate;
    while (mNextPosition + 1.0 < static_cast<double>(combined.size() / mChannels)) {
        const std::size_t leftFrame = static_cast<std::size_t>(mNextPosition);
        const std::size_t rightFrame = leftFrame + 1;
        const float fraction = static_cast<float>(mNextPosition - leftFrame);
        for (std::uint32_t channel = 0; channel < mChannels; ++channel) {
            const float left = combined[leftFrame * mChannels + channel];
            const float right = combined[rightFrame * mChannels + channel];
            output.push_back(left + (right - left) * fraction);
        }
        mNextPosition += step;
    }
    mNextPosition = std::max(0.0, mNextPosition - static_cast<double>(inputFrames));
    if (inputFrames != 0) {
        mPreviousFrame.assign(input.end() - static_cast<std::ptrdiff_t>(mChannels), input.end());
        mHasPreviousFrame = true;
    }
}

void AudioResampler::Reset() {
    mNextPosition = 0.0;
    mPreviousFrame.clear();
    mHasPreviousFrame = false;
}

}
