#pragma once

namespace alsa_playback {

enum class PlayerState {
    Closed,
    Opened,
    Running,
    Stopping,
    Error,
};

}  // namespace alsa_playback
