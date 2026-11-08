#pragma once

#include "format/AudioFormat.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace alsa_playback {

class WavFileReader {
public:
    bool Open(const std::string& filePath, std::string& error);
    bool ReadFrames(std::size_t requestedFrames,
                    std::vector<std::int16_t>& samples,
                    std::size_t& framesRead,
                    std::string& error);
    const AudioFormat& Format() const;
    std::uint64_t TotalFrames() const;
    bool IsEndOfFile() const;

private:
    std::ifstream mStream;
    AudioFormat mFormat{};
    std::uint64_t mTotalFrames = 0;
    std::uint64_t mFramesRead = 0;
};

}  // namespace alsa_playback
