#include "device/AlsaError.h"

#include <alsa/asoundlib.h>

namespace alsa_playback {

bool AlsaError::IsSuccess() const {
    return code >= 0;
}

AlsaError MakeAlsaError(int code, const std::string& operation) {
    if (code >= 0) {
        return {code, {}};
    }
    return {code, operation + ": " + snd_strerror(code)};
}

}  // namespace alsa_playback
