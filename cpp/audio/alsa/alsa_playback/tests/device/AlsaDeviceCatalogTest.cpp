#include "device/AlsaDeviceCatalog.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace alsa_playback::tests {

void RunAlsaDeviceCatalogTest() {
    std::vector<AlsaDeviceInfo> devices;
    std::string error;
    if (!AlsaDeviceCatalog::EnumeratePlaybackDevices(devices, error)) {
        throw std::runtime_error("ALSA device enumeration failed: " + error);
    }
    for (const auto& device : devices) {
        if (device.name.empty()) {
            throw std::runtime_error("ALSA device name is empty");
        }
    }
}

}  // namespace alsa_playback::tests
