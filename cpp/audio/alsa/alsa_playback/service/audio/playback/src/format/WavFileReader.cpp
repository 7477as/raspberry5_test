#include "format/WavFileReader.h"

#include <algorithm>
#include <array>

namespace alsa_playback {
namespace {

std::uint16_t ReadU16(const std::uint8_t* bytes) {
    return static_cast<std::uint16_t>(bytes[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[1]) << 8);
}

std::uint32_t ReadU32(const std::uint8_t* bytes) {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

bool ReadExact(std::ifstream& stream, char* bytes, std::size_t size) {
    stream.read(bytes, static_cast<std::streamsize>(size));
    return stream.gcount() == static_cast<std::streamsize>(size);
}

}

bool WavFileReader::Open(const std::string& filePath, std::string& error) {
    mStream.close();
    mStream.clear();
    mStream.open(filePath, std::ios::binary);
    if (!mStream) {
        error = "无法打开 WAV 文件: " + filePath;
        return false;
    }
    std::array<std::uint8_t, 12> header{};
    if (!ReadExact(mStream, reinterpret_cast<char*>(header.data()), header.size()) ||
        std::string(reinterpret_cast<char*>(header.data()), 4) != "RIFF" ||
        std::string(reinterpret_cast<char*>(header.data() + 8), 4) != "WAVE") {
        error = "文件不是有效的 RIFF/WAVE 文件";
        return false;
    }
    bool hasFormat = false;
    bool hasData = false;
    std::uint16_t channels = 0;
    std::uint16_t bitsPerSample = 0;
    std::uint16_t audioFormat = 0;
    std::uint32_t sampleRate = 0;
    std::uint32_t dataSize = 0;
    std::streampos dataPosition = 0;
    while (mStream && (!hasFormat || !hasData)) {
        std::array<std::uint8_t, 8> chunk{};
        if (!ReadExact(mStream, reinterpret_cast<char*>(chunk.data()), chunk.size())) {
            break;
        }
        const std::uint32_t chunkSize = ReadU32(chunk.data() + 4);
        const std::streampos payloadPosition = mStream.tellg();
        const std::string chunkId(reinterpret_cast<char*>(chunk.data()), 4);
        if (chunkId == "fmt ") {
            std::array<std::uint8_t, 16> format{};
            if (chunkSize < format.size() ||
                !ReadExact(mStream, reinterpret_cast<char*>(format.data()), format.size())) {
                error = "WAV fmt chunk 长度无效";
                return false;
            }
            audioFormat = ReadU16(format.data());
            channels = ReadU16(format.data() + 2);
            sampleRate = ReadU32(format.data() + 4);
            bitsPerSample = ReadU16(format.data() + 14);
            hasFormat = true;
        } else if (chunkId == "data") {
            dataSize = chunkSize;
            dataPosition = payloadPosition;
            hasData = true;
        }
        mStream.clear();
        mStream.seekg(payloadPosition + static_cast<std::streamoff>(chunkSize + (chunkSize & 1U)));
    }
    if (!hasFormat || !hasData) {
        error = "WAV 文件缺少 fmt 或 data chunk";
        return false;
    }
    if (audioFormat != 1 || bitsPerSample != 16 || (channels != 1 && channels != 2)) {
        error = "仅支持 PCM S16LE 单声道或双声道 WAV";
        return false;
    }
    mFormat = AudioFormat{sampleRate, channels, SampleFormat::S16LE, ChannelLayout::Interleaved};
    if (!mFormat.IsValid() || dataSize % mFormat.BytesPerFrame() != 0) {
        error = "WAV 采样率或 data chunk 大小无效";
        return false;
    }
    mTotalFrames = dataSize / mFormat.BytesPerFrame();
    mFramesRead = 0;
    mStream.clear();
    mStream.seekg(dataPosition);
    if (!mStream) {
        error = "无法定位 WAV data chunk";
        return false;
    }
    return true;
}

bool WavFileReader::ReadFrames(std::size_t requestedFrames,
                               std::vector<std::int16_t>& samples,
                               std::size_t& framesRead,
                               std::string& error) {
    framesRead = 0;
    if (!mStream.is_open() || requestedFrames == 0) {
        error = "WAV 读取器未打开或请求帧数无效";
        return false;
    }
    const std::uint64_t remainingFrames = mTotalFrames - mFramesRead;
    framesRead = static_cast<std::size_t>(std::min<std::uint64_t>(requestedFrames, remainingFrames));
    samples.resize(framesRead * mFormat.channels);
    const std::size_t byteCount = framesRead * mFormat.BytesPerFrame();
    if (byteCount != 0 && !ReadExact(mStream, reinterpret_cast<char*>(samples.data()), byteCount)) {
        error = "WAV data chunk 被截断";
        framesRead = 0;
        samples.clear();
        return false;
    }
    mFramesRead += framesRead;
    return true;
}

const AudioFormat& WavFileReader::Format() const {
    return mFormat;
}

std::uint64_t WavFileReader::TotalFrames() const {
    return mTotalFrames;
}

bool WavFileReader::IsEndOfFile() const {
    return mFramesRead >= mTotalFrames;
}

}
