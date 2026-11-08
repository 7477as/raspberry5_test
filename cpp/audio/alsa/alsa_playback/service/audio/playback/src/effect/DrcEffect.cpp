#include "effect/DrcEffect.h"

namespace alsa_playback {

DrcEffect::DrcEffect()
    : DrcEffect(Config{}) {
}

DrcEffect::DrcEffect(Config config)
    : mConfig(config) {
}

std::string DrcEffect::Name() const {
    return "drc";
}

bool DrcEffect::IsBypassed() const {
    return mConfig.bypassed;
}

void DrcEffect::SetBypassed(bool bypassed) {
    mConfig.bypassed = bypassed;
}

void DrcEffect::Process(std::vector<float>&, std::uint32_t, std::uint32_t) {
}

}  // namespace alsa_playback
