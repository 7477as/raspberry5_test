#pragma once

#include "device/AlsaPcmOutput.h"
#include "effect/AudioEffectChain.h"
#include "mixer/AudioMixer.h"
#include "player/PlayerConfig.h"
#include "player/PlayerState.h"
#include "session/SessionRegistry.h"
#include "statistics/AudioStatistics.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace alsa_playback {

class PlayerEngine {
public:
    PlayerEngine();
    ~PlayerEngine();

    bool Open(const PlayerConfig& config, std::string& error);
    bool Start(std::string& error);
    void Stop();
    void Close();
    PlayerState State() const;
    SessionId CreateSession(const AudioSessionConfig& config, std::string& error);
    bool DestroySession(SessionId sessionId);
    bool FlushSession(SessionId sessionId);
    std::size_t QueuedFrames(SessionId sessionId) const;
    PushResult PushPcm(SessionId sessionId,
                       const void* inputData,
                       std::size_t frameCount,
                       const AudioFormat& inputFormat);
    PushResult PushPcmBlocking(SessionId sessionId,
                               const void* inputData,
                               std::size_t frameCount,
                               const AudioFormat& inputFormat,
                               std::chrono::milliseconds timeout);
    bool SetSessionVolume(SessionId sessionId, float volume);
    bool SetSessionMuted(SessionId sessionId, bool muted);
    AudioStatisticsSnapshot Statistics() const;

private:
    PushResult PushPcmInternal(SessionId sessionId,
                               const void* inputData,
                               std::size_t frameCount,
                               const AudioFormat& inputFormat,
                               std::chrono::milliseconds timeout,
                               bool blocking);
    void Run();
    bool ValidateInputFormat(const AudioFormat& inputFormat, std::string& error) const;
    void BuildEffectChain();

    PlayerConfig mConfig{};
    PlayerState mState = PlayerState::Closed;
    mutable std::mutex mStateMutex;
    std::condition_variable mWakeCondition;
    std::mutex mWakeMutex;
    std::atomic<bool> mStopRequested{false};
    std::thread mThread;
    std::unique_ptr<SessionRegistry> mSessions;
    std::unique_ptr<AudioMixer> mMixer;
    std::unique_ptr<AlsaPcmOutput> mOutput;
    AudioEffectChain mEffectChain;
    AudioStatistics mStatistics;
    std::vector<float> mMixSamples;
    std::string mRuntimeError;
};

}  // namespace alsa_playback
