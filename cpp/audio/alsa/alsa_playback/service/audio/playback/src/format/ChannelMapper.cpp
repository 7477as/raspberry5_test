#include "format/ChannelMapper.h"

#include <algorithm>
#include <cstddef>

namespace alsa_playback {

bool ChannelMapper::MapInterleaved(const std::vector<float>& inputSamples,
                                   std::uint32_t inputChannels,
                                   std::uint32_t outputChannels,
                                   std::vector<float>& outputSamples,
                                   std::string& error) {
    if (inputChannels == 0 || outputChannels == 0 || inputSamples.size() % inputChannels != 0) {
        error = "声道参数或交错 PCM 数据无效";
        return false;
    }
    const std::size_t frameCount = inputSamples.size() / inputChannels;
    outputSamples.assign(frameCount * outputChannels, 0.0F);
    for (std::size_t frame = 0; frame < frameCount; ++frame) {
        if (inputChannels == outputChannels) {
            std::copy_n(inputSamples.data() + frame * inputChannels,
                        inputChannels,
                        outputSamples.data() + frame * outputChannels);
            continue;
        }
        if (inputChannels == 1) {
            for (std::uint32_t channel = 0; channel < outputChannels; ++channel) {
                outputSamples[frame * outputChannels + channel] = inputSamples[frame];
            }
            continue;
        }
        if (outputChannels == 1) {
            float sum = 0.0F;
            for (std::uint32_t channel = 0; channel < inputChannels; ++channel) {
                sum += inputSamples[frame * inputChannels + channel];
            }
            outputSamples[frame] = sum / static_cast<float>(inputChannels);
            continue;
        }
        const std::uint32_t copiedChannels = std::min(inputChannels, outputChannels);
        for (std::uint32_t channel = 0; channel < copiedChannels; ++channel) {
            outputSamples[frame * outputChannels + channel] =
                inputSamples[frame * inputChannels + channel];
        }
    }
    return true;
}

}  // namespace alsa_playback
