#pragma once

#include "device/AlsaDeviceCatalog.h"
#include "format/AudioFormat.h"
#include "player/PlayerConfig.h"
#include "player/PlayerEngine.h"
#include "session/AudioSessionConfig.h"
#include "session/SessionTypes.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace alsa_playback {

struct PlaybackSessionConfig {
    std::string deviceHint = "plughw:3,0";
    AudioFormat outputFormat{};
    AudioSessionConfig session{};
    std::size_t maxSessions = 8;
    std::size_t mixBlockFrames = 1024;
};

class PlaybackSession {
public:
    PlaybackSession();
    ~PlaybackSession();

    // PlaybackSession(const PlaybackSession&) = delete;
    // PlaybackSession& operator=(const PlaybackSession&) = delete;
    // PlaybackSession(PlaybackSession&&) = delete;
    // PlaybackSession& operator=(PlaybackSession&&) = delete;

    bool Open(const PlaybackSessionConfig& config, std::string& error);
    PushResult Write(const float* samples,
                     std::size_t frameCount,
                     std::chrono::milliseconds timeout);
    bool Close(std::string& error);
    bool IsDeviceOpen() const;

    bool SetVolume(float volume);
    bool SetMuted(bool muted);
    std::size_t QueuedFrames() const;

    static bool EnumeratePlaybackDevices(std::vector<AlsaDeviceInfo>& devices,
                                         std::string& error);

private:
    static std::shared_ptr<PlayerEngine> SharedEngine(const PlaybackSessionConfig& config,
                                                      std::string& error);
    static void ReleaseEngine();

    std::shared_ptr<PlayerEngine> mEngine;
    SessionId mSessionId = kInvalidSessionId;
    AudioFormat mOutputFormat{};
    std::chrono::milliseconds mDefaultTimeout{100};
};

}  // namespace alsa_playback
