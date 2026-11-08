#pragma once

#include "device/AlsaDeviceCatalog.h"
#include "player/PlayerConfig.h"
#include "player/PlayerState.h"
#include "session/AudioSessionConfig.h"
#include "session/SessionTypes.h"
#include "statistics/AudioStatistics.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace alsa_playback {

class PlayerEngine;

class AlsaAudioPlayer {
public:
    AlsaAudioPlayer();
    ~AlsaAudioPlayer();

    AlsaAudioPlayer(const AlsaAudioPlayer&) = delete;
    AlsaAudioPlayer& operator=(const AlsaAudioPlayer&) = delete;

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
    std::unique_ptr<PlayerEngine> mEngine;
};

}  // namespace alsa_playback
