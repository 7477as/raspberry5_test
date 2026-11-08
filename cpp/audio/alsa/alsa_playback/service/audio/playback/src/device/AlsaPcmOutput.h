#pragma once

#include "device/AlsaPcmParameters.h"

#include <alsa/asoundlib.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace alsa_playback {

class AlsaPcmOutput {
public:
    AlsaPcmOutput() = default;
    ~AlsaPcmOutput();

    AlsaPcmOutput(const AlsaPcmOutput&) = delete;
    AlsaPcmOutput& operator=(const AlsaPcmOutput&) = delete;

    bool Open(const std::string& deviceName, AlsaPcmParameters parameters, std::string& error);
    void Close();
    bool IsOpen() const;
    const AlsaPcmParameters& Parameters() const;
    bool Write(const std::vector<float>& samples, std::size_t frameCount, std::string& error);
    std::uint64_t RecoveryCount() const;

private:
    bool Configure(std::string& error);
    bool Recover(int errorCode, std::string& error);

    snd_pcm_t* mPcm = nullptr;
    std::string mDeviceName;
    AlsaPcmParameters mParameters{};
    std::uint64_t mRecoveryCount = 0;
};

}  // namespace alsa_playback
