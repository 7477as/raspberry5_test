#include "effect/EqualizerEffect.h"

namespace alsa_playback {

EqualizerEffect::EqualizerEffect()
    : EqualizerEffect(Config{}) {
}

EqualizerEffect::EqualizerEffect(Config config)
    : mBypassed(config.bypassed) {
}

std::string EqualizerEffect::Name() const {
    return "eq";
}

bool EqualizerEffect::IsBypassed() const {
    return mBypassed;
}

void EqualizerEffect::SetBypassed(bool bypassed) {
    mBypassed = bypassed;
}

void EqualizerEffect::Process(std::vector<float>&, std::uint32_t, std::uint32_t) {
}

}  // namespace alsa_playback
