#pragma once

#include <cstdint>
#include <vector>

namespace alsa_playback {

struct PcmBlock {
    std::uint32_t frames = 0;
    std::uint32_t channels = 0;
    std::vector<float> samples;

    PcmBlock() = default;
    PcmBlock(std::uint32_t frameCount, std::uint32_t channelCount);

    void Resize(std::uint32_t frameCount, std::uint32_t channelCount);
    bool IsValid() const;
};

}  // namespace alsa_playback
