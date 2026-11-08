#pragma once

#include "effect/AudioEffect.h"

namespace alsa_playback {

class BypassEffect final : public AudioEffect {
public:
    explicit BypassEffect(std::string name);

    std::string Name() const override;
    bool IsBypassed() const override;
    void SetBypassed(bool bypassed) override;
    void Process(std::vector<float>& samples,
                 std::uint32_t frames,
                 std::uint32_t channels) override;

private:
    std::string mName;
    bool mBypassed = true;
};

}  // namespace alsa_playback
