#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace alsa_playback {

class ChannelMapper {
public:
    static bool MapInterleaved(const std::vector<float>& inputSamples,
                               std::uint32_t inputChannels,
                               std::uint32_t outputChannels,
                               std::vector<float>& outputSamples,
                               std::string& error);
};

}  // namespace alsa_playback
