#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace alsa_playback {

class AudioEffect {
public:
    virtual ~AudioEffect() = default;

    virtual std::string Name() const = 0;
    virtual bool IsBypassed() const = 0;
    virtual void SetBypassed(bool bypassed) = 0;
    virtual void Process(std::vector<float>& samples,
                         std::uint32_t frames,
                         std::uint32_t channels) = 0;
};

}  // namespace alsa_playback
