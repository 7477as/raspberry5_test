#pragma once

#include <string>

namespace alsa_playback {

struct AlsaError {
    int code = 0;
    std::string message;

    bool IsSuccess() const;
};

AlsaError MakeAlsaError(int code, const std::string& operation);

}  // namespace alsa_playback
