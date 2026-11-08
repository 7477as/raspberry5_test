#pragma once

#include "device/AlsaPcmParameters.h"

#include <cstddef>
#include <string>

namespace alsa_playback {

struct PlayerConfig {
    std::string deviceName = "plughw:3,0";
    // std::string deviceName = "default";
    AlsaPcmParameters pcm{};
    std::size_t maxSessions = 8;
    std::size_t mixBlockFrames = 1024;
};

}  // namespace alsa_playback
