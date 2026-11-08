#include "format/AudioFormat.h"

namespace alsa_playback {

bool AudioFormat::IsValid() const {
    return sampleRate > 0 && sampleRate <= 192000 && channels > 0 && channels <= 32 &&
           channelLayout == ChannelLayout::Interleaved;
}

std::size_t AudioFormat::BytesPerSample() const {
    switch (sampleFormat) {
    case SampleFormat::S16LE:
        return 2;
    case SampleFormat::S24LE:
        return 3;
    case SampleFormat::S32LE:
    case SampleFormat::F32LE:
        return 4;
    }
    return 0;
}

std::size_t AudioFormat::BytesPerFrame() const {
    return BytesPerSample() * channels;
}

bool AudioFormat::operator==(const AudioFormat& other) const {
    return sampleRate == other.sampleRate && channels == other.channels &&
           sampleFormat == other.sampleFormat && channelLayout == other.channelLayout;
}

bool AudioFormat::operator!=(const AudioFormat& other) const {
    return !(*this == other);
}

}  // namespace alsa_playback
