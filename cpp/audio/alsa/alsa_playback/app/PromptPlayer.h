#pragma once

#include "AudioPlaybackService.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

namespace alsa_playback {

class PromptPlayer {
public:
    PromptPlayer() = default;
    ~PromptPlayer();

    PromptPlayer(const PromptPlayer&) = delete;
    PromptPlayer& operator=(const PromptPlayer&) = delete;

    bool Open(audio_service::AudioPlaybackService& playbackService,
              const audio_service::AudioFormat& outputFormat,
              const std::string& filePath,
              std::chrono::milliseconds pushTimeout,
              std::string& error);
    void Play(std::chrono::steady_clock::time_point triggerTime);
    void Close(std::string& error);

private:
    static constexpr std::size_t kPromptPushFrames = 1024;

    void Run(std::chrono::steady_clock::time_point triggerTime);
    void StopThread();

    audio_service::AudioPlaybackService* mService = nullptr;
    audio_service::SessionHandle mSessionId = audio_service::kInvalidSession;
    audio_service::AudioFormat mOutputFormat{};
    std::chrono::milliseconds mPushTimeout{100};
    std::vector<float> mSamples;
    std::atomic<bool> mStopRequested{true};
    std::atomic<bool> mDone{true};
    bool mSucceeded = true;
    std::string mError;
    std::thread mThread;
};

}  // namespace alsa_playback
