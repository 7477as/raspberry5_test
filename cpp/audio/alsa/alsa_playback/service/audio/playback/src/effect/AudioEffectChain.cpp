#include "effect/AudioEffectChain.h"

#include <utility>

namespace alsa_playback {

void AudioEffectChain::AddEffect(std::unique_ptr<AudioEffect> effect) {
    if (effect != nullptr) {
        mEffects.emplace_back(std::move(effect));
    }
}

void AudioEffectChain::Process(std::vector<float>& samples,
                               std::uint32_t frames,
                               std::uint32_t channels) {
    for (const auto& effect : mEffects) {
        effect->Process(samples, frames, channels);
    }
}

bool AudioEffectChain::SetBypassed(const std::string& effectName, bool bypassed) {
    for (const auto& effect : mEffects) {
        if (effect->Name() == effectName) {
            effect->SetBypassed(bypassed);
            return true;
        }
    }
    return false;
}

std::vector<std::string> AudioEffectChain::EffectNames() const {
    std::vector<std::string> names;
    names.reserve(mEffects.size());
    for (const auto& effect : mEffects) {
        names.push_back(effect->Name());
    }
    return names;
}

std::size_t AudioEffectChain::Size() const {
    return mEffects.size();
}

}  // namespace alsa_playback
