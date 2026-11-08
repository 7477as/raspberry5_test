#include "effect/LimiterEffect.h"

namespace alsa_playback {

LimiterEffect::LimiterEffect()
    : LimiterEffect(Config{}) {
}

LimiterEffect::LimiterEffect(Config config)
    : mConfig(config) {
}

std::string LimiterEffect::Name() const {
    return "limiter";
}

bool LimiterEffect::IsBypassed() const {
    return mConfig.bypassed;
}

void LimiterEffect::SetBypassed(bool bypassed) {
    mConfig.bypassed = bypassed;
}

void LimiterEffect::Process(std::vector<float>&, std::uint32_t, std::uint32_t) {
}

}  // namespace alsa_playback
