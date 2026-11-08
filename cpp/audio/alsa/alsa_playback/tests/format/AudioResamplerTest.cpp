#include "format/AudioResampler.h"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace alsa_playback::tests {

void RunAudioResamplerTest() {
    AudioResampler resampler(44100, 48000, 1);
    std::vector<float> input(4410);
    for (std::size_t index = 0; index < input.size(); ++index) {
        input[index] = static_cast<float>(std::sin(index * 0.01));
    }
    std::vector<float> first;
    std::vector<float> second;
    resampler.Process(std::vector<float>(input.begin(), input.begin() + 2205), first);
    resampler.Process(std::vector<float>(input.begin() + 2205, input.end()), second);
    if (first.empty() || second.empty() || std::abs(static_cast<int>(first.size() + second.size()) - 4799) > 2) {
        throw std::runtime_error("audio resampler rate conversion test failed");
    }
    if (std::abs(first.back() - second.front()) > 0.1F) {
        throw std::runtime_error("audio resampler block continuity test failed");
    }
}

}
