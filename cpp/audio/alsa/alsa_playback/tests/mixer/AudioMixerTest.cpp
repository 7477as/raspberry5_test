#include "mixer/AudioMixer.h"
#include "session/AudioSession.h"

#include <stdexcept>

namespace alsa_playback::tests {

void RunAudioMixerTest() {
    AudioSessionConfig config;
    config.inputFormat.channels = 2;
    config.maxQueueFrames = 8;
    auto session = std::make_shared<AudioSession>(1, config, 2);
    PcmBlock block(2, 2);
    block.samples = {0.25F, 0.25F, 0.25F, 0.25F};
    if (!session->Push(std::move(block)).IsSuccess()) {
        throw std::runtime_error("session push test failed");
    }
    AudioMixer mixer(MixerConfig{2, true, true});
    std::vector<float> output;
    if (!mixer.Mix({session}, 2, output) || output.size() != 4) {
        throw std::runtime_error("mixer invocation failed");
    }
}

}  // namespace alsa_playback::tests
