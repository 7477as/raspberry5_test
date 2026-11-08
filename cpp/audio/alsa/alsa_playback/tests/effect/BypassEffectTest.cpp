#include "effect/BypassEffect.h"

#include <cmath>
#include <stdexcept>

namespace alsa_playback::tests {

void RunBypassEffectTest() {
    BypassEffect effect("test");
    std::vector<float> samples = {-0.5F, 0.0F, 0.75F};
    const auto original = samples;
    effect.Process(samples, 1, 3);
    if (samples != original || !effect.IsBypassed()) {
        throw std::runtime_error("bypass effect test failed");
    }
}

}  // namespace alsa_playback::tests
