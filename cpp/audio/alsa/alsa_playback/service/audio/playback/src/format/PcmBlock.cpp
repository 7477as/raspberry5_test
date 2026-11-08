#include "format/PcmBlock.h"

#include <cstddef>

namespace alsa_playback {

PcmBlock::PcmBlock(std::uint32_t frameCount, std::uint32_t channelCount) {
    Resize(frameCount, channelCount);
}

void PcmBlock::Resize(std::uint32_t frameCount, std::uint32_t channelCount) {
    frames = frameCount;
    channels = channelCount;
    samples.assign(static_cast<std::size_t>(frames) * channels, 0.0F);
}

bool PcmBlock::IsValid() const {
    return frames > 0 && channels > 0 && samples.size() == static_cast<std::size_t>(frames) * channels;
}

}  // namespace alsa_playback
