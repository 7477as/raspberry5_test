#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace alsa_playback {

struct AlsaDeviceInfo {
    std::string name;
    std::string description;
    std::string cardName;
    std::string deviceName;
};

class AlsaDeviceCatalog {
public:
    static bool EnumeratePlaybackDevices(std::vector<AlsaDeviceInfo>& devices,
                                         std::string& error);
};

}  // namespace alsa_playback
