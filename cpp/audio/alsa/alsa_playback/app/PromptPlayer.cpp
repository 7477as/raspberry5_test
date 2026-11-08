#include "app/PromptPlayer.h"

#include "service/audio/playback/src/format/AudioResampler.h"
#include "service/audio/playback/src/format/ChannelMapper.h"
#include "service/audio/playback/src/format/PcmConverter.h"
#include "service/audio/playback/src/format/WavFileReader.h"

#include <algorithm>
#include <cstdint>
#include <thread>

namespace alsa_playback {

PromptPlayer::~PromptPlayer() {
    std::string ignored;
    Close(ignored);
}

bool PromptPlayer::Open(audio_service::AudioPlaybackService& playbackService,
                        const audio_service::AudioFormat& outputFormat,
                        const std::string& filePath,
                        std::chrono::milliseconds pushTimeout,
                        std::string& error) {
    std::string ignored;
    Close(ignored);
    if (!outputFormat.IsValid()) {
        error = "提示音输出格式无效";
        return false;
    }
    WavFileReader reader;
    if (!reader.Open(filePath, error)) {
        return false;
    }
    const auto inputFormat = reader.Format();
    std::vector<std::int16_t> inputSamples;
    std::size_t framesRead = 0;
    if (!reader.ReadFrames(static_cast<std::size_t>(reader.TotalFrames()), inputSamples,
                            framesRead, error) || framesRead == 0) {
        if (error.empty()) {
            error = "提示音 WAV 数据为空";
        }
        return false;
    }
    std::vector<float> inputFloat;
    if (!PcmConverter::ConvertToFloat(inputSamples.data(), framesRead, inputFormat, inputFloat,
                                      error)) {
        return false;
    }
    AudioResampler resampler(inputFormat.sampleRate, outputFormat.sampleRate,
                             inputFormat.channels);
    std::vector<float> outputFloat;
    resampler.Process(inputFloat, outputFloat);
    if (outputFloat.empty()) {
        error = "提示音重采样结果为空";
        return false;
    }
    std::vector<float> mappedSamples;
    if (!ChannelMapper::MapInterleaved(outputFloat, inputFormat.channels, outputFormat.channels,
                                      mappedSamples, error)) {
        return false;
    }
    mSamples = std::move(mappedSamples);
    mOutputFormat = outputFormat;
    mPushTimeout = pushTimeout;
    mService = &playbackService;
    audio_service::SessionConfig sessionConfig;
    sessionConfig.name = "prompt";
    sessionConfig.volume = 0.45F;
    sessionConfig.priority = 10;
    mSessionId = playbackService.CreateSession(sessionConfig, error);
    if (mSessionId == audio_service::kInvalidSession) {
        mService = nullptr;
        return false;
    }
    return true;
}

void PromptPlayer::Play(std::chrono::steady_clock::time_point triggerTime) {
    StopThread();
    if (mService == nullptr || mSessionId == audio_service::kInvalidSession) {
        return;
    }
    mStopRequested.store(false, std::memory_order_release);
    mDone.store(false, std::memory_order_release);
    mSucceeded = true;
    mError.clear();
    mThread = std::thread(&PromptPlayer::Run, this, triggerTime);
}

void PromptPlayer::Close(std::string& error) {
    StopThread();
    error = mError;
    if (mService != nullptr && mSessionId != audio_service::kInvalidSession) {
        mService->DestroySession(mSessionId);
        mSessionId = audio_service::kInvalidSession;
    }
    mSamples.clear();
    mService = nullptr;
}

void PromptPlayer::Run(std::chrono::steady_clock::time_point triggerTime) {
    std::string error;
    bool succeeded = true;
    const std::size_t totalFrames = mOutputFormat.channels == 0
                                        ? 0
                                        : mSamples.size() / mOutputFormat.channels;
    while (!mStopRequested.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < triggerTime) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::size_t submittedFrames = 0;
    while (!mStopRequested.load(std::memory_order_acquire) && submittedFrames < totalFrames) {
        const std::size_t remainingFrames = totalFrames - submittedFrames;
        const std::size_t blockFrames = std::min(kPromptPushFrames, remainingFrames);
        const float* samples = mSamples.data() + submittedFrames * mOutputFormat.channels;
        const audio_service::PushResult result =
            mService->PushBlocking(mSessionId, samples, blockFrames, mOutputFormat, mPushTimeout);
        if (!result.IsSuccess()) {
            error = result.message;
            succeeded = false;
            break;
        }
        submittedFrames += result.acceptedFrames;
    }
    mError = error;
    mSucceeded = succeeded;
    mDone.store(true, std::memory_order_release);
}

void PromptPlayer::StopThread() {
    mStopRequested.store(true, std::memory_order_release);
    if (mThread.joinable()) {
        mThread.join();
    }
}

}
