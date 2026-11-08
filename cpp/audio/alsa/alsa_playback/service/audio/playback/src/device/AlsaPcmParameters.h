#pragma once

#include "format/AudioFormat.h"

#include <cstdint>
#include <string>

namespace alsa_playback {

struct AlsaPcmParameters {
    AudioFormat format{};
    std::uint32_t periodFrames = 1024;
    std::uint32_t bufferFrames = 4096;
    std::uint32_t latencyUs = 50000;
};

bool ValidateAlsaPcmParameters(const AlsaPcmParameters& parameters, std::string& error);

}  // namespace alsa_playback
