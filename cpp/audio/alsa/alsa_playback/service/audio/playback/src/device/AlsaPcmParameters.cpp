#include "device/AlsaPcmParameters.h"

namespace alsa_playback {

bool ValidateAlsaPcmParameters(const AlsaPcmParameters& parameters, std::string& error) {
    if (!parameters.format.IsValid()) {
        error = "ALSA 音频格式无效";
        return false;
    }
    if (parameters.periodFrames == 0 || parameters.bufferFrames < parameters.periodFrames * 2) {
        error = "ALSA period/buffer 参数无效";
        return false;
    }
    if (parameters.latencyUs == 0) {
        error = "ALSA latency 参数无效";
        return false;
    }
    return true;
}

}  // namespace alsa_playback
