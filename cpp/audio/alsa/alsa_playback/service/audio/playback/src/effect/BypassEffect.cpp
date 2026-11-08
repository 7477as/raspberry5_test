#include "effect/BypassEffect.h"

#include <utility>

namespace alsa_playback {

BypassEffect::BypassEffect(std::string name)
    : mName(std::move(name)) {
}

std::string BypassEffect::Name() const {
    return mName;
}

bool BypassEffect::IsBypassed() const {
    return mBypassed;
}

void BypassEffect::SetBypassed(bool bypassed) {
    mBypassed = bypassed;
}

void BypassEffect::Process(std::vector<float>&, std::uint32_t, std::uint32_t) {
}

}  // namespace alsa_playback
