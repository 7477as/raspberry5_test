#pragma once

#include "format/PcmBlock.h"
#include "mixer/MixerConfig.h"
#include "session/AudioSession.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace alsa_playback {

class AudioMixer {
public:
    explicit AudioMixer(MixerConfig config);

    bool Mix(const std::vector<std::shared_ptr<AudioSession>>& sessions,
             std::size_t frameCount,
             std::vector<float>& outputSamples);

private:
    MixerConfig mConfig;
    std::vector<std::shared_ptr<AudioSession>> mOrderedSessions;
    std::vector<float> mSessionSamples;
};

}  // namespace alsa_playback
