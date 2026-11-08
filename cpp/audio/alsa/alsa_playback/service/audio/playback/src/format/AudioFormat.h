#pragma once

#include <cstddef>
#include <cstdint>

namespace alsa_playback {

enum class SampleFormat {
    S16LE,
    S24LE,
    S32LE,
    F32LE,
};

enum class ChannelLayout {
    Interleaved,
};

struct AudioFormat {
    std::uint32_t sampleRate = 48000;
    std::uint32_t channels = 2;
    SampleFormat sampleFormat = SampleFormat::F32LE;
    ChannelLayout channelLayout = ChannelLayout::Interleaved;

    bool IsValid() const;
    std::size_t BytesPerSample() const;
    std::size_t BytesPerFrame() const;
    bool operator==(const AudioFormat& other) const;
    bool operator!=(const AudioFormat& other) const;
};

}  // namespace alsa_playback
