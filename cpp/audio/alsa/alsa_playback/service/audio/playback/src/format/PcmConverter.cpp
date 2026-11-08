#include "format/PcmConverter.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace alsa_playback {
namespace {

float ClampSample(float sample) {
    return std::clamp(sample, -1.0F, 1.0F);
}

std::int32_t ReadSigned24(const std::uint8_t* bytes) {
    std::int32_t value = static_cast<std::int32_t>(bytes[0]) |
                         (static_cast<std::int32_t>(bytes[1]) << 8) |
                         (static_cast<std::int32_t>(bytes[2]) << 16);
    if ((value & 0x00800000) != 0) {
        value |= static_cast<std::int32_t>(0xFF000000);
    }
    return value;
}

void WriteSigned24(std::uint8_t* bytes, std::int32_t value) {
    bytes[0] = static_cast<std::uint8_t>(value & 0xFF);
    bytes[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    bytes[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
}

float ReadSample(const std::uint8_t* bytes, SampleFormat format) {
    switch (format) {
    case SampleFormat::S16LE: {
        std::int16_t value = 0;
        std::memcpy(&value, bytes, sizeof(value));
        return static_cast<float>(value) / 32768.0F;
    }
    case SampleFormat::S24LE:
        return static_cast<float>(ReadSigned24(bytes)) / 8388608.0F;
    case SampleFormat::S32LE: {
        std::int32_t value = 0;
        std::memcpy(&value, bytes, sizeof(value));
        return static_cast<float>(value) / 2147483648.0F;
    }
    case SampleFormat::F32LE: {
        float value = 0.0F;
        std::memcpy(&value, bytes, sizeof(value));
        return value;
    }
    }
    return 0.0F;
}

void WriteSample(std::uint8_t* bytes, SampleFormat format, float sample) {
    const float clampedSample = ClampSample(sample);
    switch (format) {
    case SampleFormat::S16LE: {
        const auto value = static_cast<std::int16_t>(std::lrintf(clampedSample * 32767.0F));
        std::memcpy(bytes, &value, sizeof(value));
        return;
    }
    case SampleFormat::S24LE: {
        const auto value = static_cast<std::int32_t>(std::lrintf(clampedSample * 8388607.0F));
        WriteSigned24(bytes, value);
        return;
    }
    case SampleFormat::S32LE: {
        const auto value = static_cast<std::int32_t>(std::lrintf(clampedSample * 2147483647.0F));
        std::memcpy(bytes, &value, sizeof(value));
        return;
    }
    case SampleFormat::F32LE:
        std::memcpy(bytes, &clampedSample, sizeof(clampedSample));
        return;
    }
}

}  // namespace

bool PcmConverter::ConvertToFloat(const void* inputData,
                                  std::size_t frameCount,
                                  const AudioFormat& inputFormat,
                                  std::vector<float>& outputSamples,
                                  std::string& error) {
    if (inputData == nullptr || frameCount == 0 || !inputFormat.IsValid()) {
        error = "输入 PCM 数据或格式无效";
        return false;
    }
    const std::size_t sampleCount = frameCount * inputFormat.channels;
    const std::size_t bytesPerFrame = inputFormat.BytesPerFrame();
    if (bytesPerFrame == 0 || sampleCount > std::numeric_limits<std::size_t>::max() / sizeof(float)) {
        error = "输入 PCM 大小溢出";
        return false;
    }
    outputSamples.resize(sampleCount);
    const auto* inputBytes = static_cast<const std::uint8_t*>(inputData);
    const std::size_t bytesPerSample = inputFormat.BytesPerSample();
    for (std::size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        outputSamples[sampleIndex] = ReadSample(inputBytes + sampleIndex * bytesPerSample,
                                                inputFormat.sampleFormat);
    }
    return true;
}

bool PcmConverter::ConvertFromFloat(const std::vector<float>& inputSamples,
                                    const AudioFormat& outputFormat,
                                    std::vector<std::uint8_t>& outputBytes,
                                    std::string& error) {
    if (inputSamples.empty() || !outputFormat.IsValid()) {
        error = "输出 PCM 数据或格式无效";
        return false;
    }
    const std::size_t bytesPerSample = outputFormat.BytesPerSample();
    if (bytesPerSample == 0 || inputSamples.size() > std::numeric_limits<std::size_t>::max() / bytesPerSample) {
        error = "输出 PCM 大小溢出";
        return false;
    }
    outputBytes.resize(inputSamples.size() * bytesPerSample);
    for (std::size_t sampleIndex = 0; sampleIndex < inputSamples.size(); ++sampleIndex) {
        WriteSample(outputBytes.data() + sampleIndex * bytesPerSample,
                    outputFormat.sampleFormat,
                    inputSamples[sampleIndex]);
    }
    return true;
}

}  // namespace alsa_playback
