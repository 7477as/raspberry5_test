#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace alsa_playback {

using SessionId = std::uint64_t;
constexpr SessionId kInvalidSessionId = 0;

enum class PushError {
    None,
    InvalidSession,
    InvalidArgument,
    QueueFull,
    NotRunning,
    FormatMismatch,
    ConversionFailed,
};

struct PushResult {
    PushError error = PushError::None;
    std::size_t acceptedFrames = 0;
    std::string message;

    bool IsSuccess() const;
};

}  // namespace alsa_playback
