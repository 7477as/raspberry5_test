#pragma once

#include "effect/AudioEffect.h"

#include <cstdint>
#include <string>
#include <vector>

namespace alsa_playback {

class EqualizerEffect final : public AudioEffect {
public:
    struct Config {
        bool bypassed = true;
    };

    EqualizerEffect();
    explicit EqualizerEffect(Config config);

    std::string Name() const override;
    bool IsBypassed() const override;
    void SetBypassed(bool bypassed) override;
    void Process(std::vector<float>& samples,
                 std::uint32_t frames,
                 std::uint32_t channels) override;

private:
    bool mBypassed;
};

}  // namespace alsa_playback
