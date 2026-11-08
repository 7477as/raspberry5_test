#pragma once

#include "format/AudioFormat.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace alsa_playback {

struct AudioSessionConfig {
    std::string name = "session";
    AudioFormat inputFormat{};
    std::size_t maxQueueFrames = 48000;
    float volume = 1.0F;
    bool muted = false;
    std::int32_t priority = 0;
};

}  // namespace alsa_playback
