#pragma once

#include "format/AudioFormat.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace alsa_playback {

class PcmConverter {
public:
    static bool ConvertToFloat(const void* inputData,
                               std::size_t frameCount,
                               const AudioFormat& inputFormat,
                               std::vector<float>& outputSamples,
                               std::string& error);

    static bool ConvertFromFloat(const std::vector<float>& inputSamples,
                                 const AudioFormat& outputFormat,
                                 std::vector<std::uint8_t>& outputBytes,
                                 std::string& error);
};

}  // namespace alsa_playback
