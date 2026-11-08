#pragma once

#include <cstdint>

namespace alsa_playback {

struct MixerConfig {
    std::uint32_t outputChannels = 2;
    bool sortByPriority = true;
    bool clampOutput = true;
};

}  // namespace alsa_playback
