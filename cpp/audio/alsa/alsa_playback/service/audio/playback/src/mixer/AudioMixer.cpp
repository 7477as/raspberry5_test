#include "mixer/AudioMixer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace alsa_playback {

AudioMixer::AudioMixer(MixerConfig config)
    : mConfig(config) {
}

bool AudioMixer::Mix(const std::vector<std::shared_ptr<AudioSession>>& sessions,
                     std::size_t frameCount,
                     std::vector<float>& outputSamples) {
    if (frameCount == 0 || mConfig.outputChannels == 0) {
        outputSamples.clear();
        return false;
    }
    outputSamples.assign(frameCount * mConfig.outputChannels, 0.0F);
    mOrderedSessions = sessions;
    if (mConfig.sortByPriority) {
        std::stable_sort(mOrderedSessions.begin(), mOrderedSessions.end(),
                         [](const auto& left, const auto& right) {
                             return left->Priority() > right->Priority();
                         });
    }
    for (const auto& session : mOrderedSessions) {
        if (session == nullptr || session->QueueChannels() != mConfig.outputChannels) {
            continue;
        }
        const std::size_t availableFrames = session->ReadFrames(frameCount, mSessionSamples);
        if (availableFrames == 0 || session->IsMuted()) {
            continue;
        }
        const float volume = session->Volume();
        const std::size_t sampleCount = availableFrames * mConfig.outputChannels;
        for (std::size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
            outputSamples[sampleIndex] += mSessionSamples[sampleIndex] * volume;
        }
    }
    if (mConfig.clampOutput) {
        for (float& sample : outputSamples) {
            sample = std::clamp(sample, -1.0F, 1.0F);
        }
    }
    return true;
}

}  // namespace alsa_playback
