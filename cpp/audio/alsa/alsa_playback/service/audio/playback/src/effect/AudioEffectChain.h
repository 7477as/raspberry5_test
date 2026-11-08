#pragma once

#include "effect/AudioEffect.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace alsa_playback {

class AudioEffectChain {
public:
    void AddEffect(std::unique_ptr<AudioEffect> effect);
    void Process(std::vector<float>& samples,
                 std::uint32_t frames,
                 std::uint32_t channels);
    bool SetBypassed(const std::string& effectName, bool bypassed);
    std::vector<std::string> EffectNames() const;
    std::size_t Size() const;

private:
    std::vector<std::unique_ptr<AudioEffect>> mEffects;
};

}  // namespace alsa_playback
