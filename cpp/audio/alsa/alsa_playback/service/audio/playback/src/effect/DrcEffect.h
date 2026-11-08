#pragma once

#include "effect/AudioEffect.h"

#include <cstdint>
#include <string>
#include <vector>

namespace alsa_playback {

class DrcEffect final : public AudioEffect {
public:
    struct Config {
        float thresholdDb = -12.0F;
        float ratio = 4.0F;
        bool bypassed = true;
    };

    DrcEffect();
    explicit DrcEffect(Config config);

    std::string Name() const override;
    bool IsBypassed() const override;
    void SetBypassed(bool bypassed) override;
    void Process(std::vector<float>& samples,
                 std::uint32_t frames,
                 std::uint32_t channels) override;

private:
    Config mConfig;
};

}  // namespace alsa_playback
