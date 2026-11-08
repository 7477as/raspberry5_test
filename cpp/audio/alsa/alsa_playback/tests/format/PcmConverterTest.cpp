#include "format/PcmConverter.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace alsa_playback::tests {

void RunPcmConverterTest() {
    const std::int16_t input[] = {-32768, 0, 32767, 16384};
    AudioFormat format;
    format.sampleFormat = SampleFormat::S16LE;
    std::vector<float> converted;
    std::string error;
    if (!PcmConverter::ConvertToFloat(input, 2, format, converted, error) || converted.size() != 4 ||
        std::abs(converted[0] + 1.0F) > 0.001F || std::abs(converted[2] - 0.999969F) > 0.001F) {
        throw std::runtime_error("PCM conversion to float test failed");
    }
    std::vector<std::uint8_t> output;
    if (!PcmConverter::ConvertFromFloat(converted, format, output, error) || output.size() != sizeof(input)) {
        throw std::runtime_error("PCM conversion from float test failed");
    }
}

}  // namespace alsa_playback::tests
