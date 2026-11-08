#include "device/AlsaDeviceCatalog.h"

#include <alsa/asoundlib.h>

#include <cstdlib>

namespace alsa_playback {

bool AlsaDeviceCatalog::EnumeratePlaybackDevices(std::vector<AlsaDeviceInfo>& devices,
                                                 std::string& error) {
    devices.clear();
    void** hints = nullptr;
    const int result = snd_device_name_hint(-1, "pcm", &hints);
    if (result < 0) {
        error = snd_strerror(result);
        return false;
    }
    for (void** current = hints; current != nullptr && *current != nullptr; ++current) {
        char* name = snd_device_name_get_hint(*current, "NAME");
        char* description = snd_device_name_get_hint(*current, "DESC");
        char* io = snd_device_name_get_hint(*current, "IOID");
        const bool isPlayback = io == nullptr || std::string(io) == "Output";
        if (isPlayback && name != nullptr && name[0] != '\0') {
            devices.push_back({name, description != nullptr ? description : "", {}, {}});
        }
        std::free(name);
        std::free(description);
        std::free(io);
    }
    snd_device_name_free_hint(hints);
    return true;
}

}  // namespace alsa_playback
