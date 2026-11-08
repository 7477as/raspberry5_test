#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace audio_service {

using SessionHandle = std::uint64_t;
constexpr SessionHandle kInvalidSession = 0;

struct AudioFormat {
    std::uint32_t sampleRate = 48000;
    std::uint32_t channels = 2;

    bool IsValid() const {
        return sampleRate > 0 && sampleRate <= 192000 && channels > 0 && channels <= 32;
    }
};

struct ServiceConfig {
    std::string deviceHint = "plughw:3,0";
    AudioFormat outputFormat{};
};

struct SessionConfig {
    std::string name = "session";
    float volume = 1.0F;
    bool muted = false;
    std::int32_t priority = 0;
};

struct PushResult {
    bool ok = false;
    std::size_t acceptedFrames = 0;
    std::string message;

    bool IsSuccess() const {
        return ok;
    }
};

struct PlaybackStatistics {
    std::uint64_t acceptedFrames = 0;
    std::uint64_t droppedFrames = 0;
    std::uint64_t outputFrames = 0;
};

struct PlaybackDeviceInfo {
    std::string id;
    std::string description;
};

class AudioPlaybackService {
public:
    virtual ~AudioPlaybackService() = default;

    virtual bool Open(const ServiceConfig& config, std::string& error) = 0;
    virtual bool Start(std::string& error) = 0;
    virtual void Stop() = 0;
    virtual void Close() = 0;

    virtual SessionHandle CreateSession(const SessionConfig& config, std::string& error) = 0;
    virtual bool DestroySession(SessionHandle session) = 0;

    virtual PushResult PushBlocking(SessionHandle session,
                                    const void* inputData,
                                    std::size_t frameCount,
                                    const AudioFormat& inputFormat,
                                    std::chrono::milliseconds timeout) = 0;
    virtual PushResult Push(SessionHandle session,
                            const void* inputData,
                            std::size_t frameCount,
                            const AudioFormat& inputFormat) = 0;

    virtual bool SetSessionVolume(SessionHandle session, float volume) = 0;
    virtual bool SetSessionMuted(SessionHandle session, bool muted) = 0;
    virtual bool FlushSession(SessionHandle session) = 0;

    virtual std::size_t QueuedFrames(SessionHandle session) const = 0;
    virtual PlaybackStatistics Statistics() const = 0;

    static bool EnumeratePlaybackDevices(std::vector<PlaybackDeviceInfo>& devices,
                                         std::string& error);
};

std::unique_ptr<AudioPlaybackService> CreateAudioPlaybackService();

}  // namespace audio_service
