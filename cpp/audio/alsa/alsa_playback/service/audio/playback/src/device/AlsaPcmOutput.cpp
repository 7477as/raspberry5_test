#include "device/AlsaPcmOutput.h"

#include "format/PcmConverter.h"

#include <algorithm>
#include <cerrno>
#include <utility>

namespace alsa_playback {

AlsaPcmOutput::~AlsaPcmOutput() {
    Close();
}

bool AlsaPcmOutput::Open(const std::string& deviceName,
                         AlsaPcmParameters parameters,
                         std::string& error) {
    Close();
    if (deviceName.empty() || !ValidateAlsaPcmParameters(parameters, error)) {
        return false;
    }
    const int openResult = snd_pcm_open(&mPcm, deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (openResult < 0) {
        error = "打开 ALSA 设备失败: " + std::string(snd_strerror(openResult));
        mPcm = nullptr;
        return false;
    }
    mDeviceName = deviceName;
    mParameters = parameters;
    if (!Configure(error)) {
        Close();
        return false;
    }
    return true;
}

void AlsaPcmOutput::Close() {
    if (mPcm == nullptr) {
        return;
    }
    snd_pcm_drop(mPcm);
    snd_pcm_close(mPcm);
    mPcm = nullptr;
}

bool AlsaPcmOutput::IsOpen() const {
    return mPcm != nullptr;
}

const AlsaPcmParameters& AlsaPcmOutput::Parameters() const {
    return mParameters;
}

std::uint64_t AlsaPcmOutput::RecoveryCount() const {
    return mRecoveryCount;
}

bool AlsaPcmOutput::Configure(std::string& error) {
    const snd_pcm_format_t format = mParameters.format.sampleFormat == SampleFormat::S16LE
                                        ? SND_PCM_FORMAT_S16_LE
                                        : mParameters.format.sampleFormat == SampleFormat::S24LE
                                              ? SND_PCM_FORMAT_S24_3LE
                                              : mParameters.format.sampleFormat == SampleFormat::S32LE
                                                    ? SND_PCM_FORMAT_S32_LE
                                                    : SND_PCM_FORMAT_FLOAT_LE;
    const int result = snd_pcm_set_params(mPcm,
                                          format,
                                          SND_PCM_ACCESS_RW_INTERLEAVED,
                                          mParameters.format.channels,
                                          mParameters.format.sampleRate,
                                          1,
                                          mParameters.latencyUs);
    if (result < 0) {
        error = "配置 ALSA 设备失败: " + std::string(snd_strerror(result));
        return false;
    }
    return true;
}

bool AlsaPcmOutput::Recover(int errorCode, std::string& error) {
    const int recoveryResult = snd_pcm_recover(mPcm, errorCode, 1);
    if (recoveryResult < 0) {
        error = "ALSA 播放恢复失败: " + std::string(snd_strerror(recoveryResult));
        return false;
    }
    ++mRecoveryCount;
    return true;
}

bool AlsaPcmOutput::Write(const std::vector<float>& samples,
                          std::size_t frameCount,
                          std::string& error) {
    if (mPcm == nullptr || frameCount == 0 ||
        samples.size() < frameCount * mParameters.format.channels) {
        error = "ALSA 写入参数无效";
        return false;
    }
    std::vector<std::uint8_t> bytes;
    if (!PcmConverter::ConvertFromFloat(samples, mParameters.format, bytes, error)) {
        return false;
    }
    std::size_t frameOffset = 0;
    while (frameOffset < frameCount) {
        const snd_pcm_sframes_t written = snd_pcm_writei(
            mPcm,
            bytes.data() + frameOffset * mParameters.format.BytesPerFrame(),
            frameCount - frameOffset);
        if (written == -EINTR) {
            continue;
        }
        if (written < 0) {
            if (!Recover(static_cast<int>(written), error)) {
                return false;
            }
            continue;
        }
        if (written == 0) {
            error = "ALSA 写入返回 0 帧";
            return false;
        }
        frameOffset += static_cast<std::size_t>(written);
    }
    return true;
}

}  // namespace alsa_playback
