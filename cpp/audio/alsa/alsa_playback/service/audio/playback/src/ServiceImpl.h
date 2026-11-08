#pragma once

#include "AudioPlaybackService.h"

#include <memory>

namespace alsa_playback {

class PlayerEngine;

class ServiceImpl final : public audio_service::AudioPlaybackService {
public:
    ServiceImpl();
    ~ServiceImpl() override;

    bool Open(const audio_service::ServiceConfig& config, std::string& error) override;
    bool Start(std::string& error) override;
    void Stop() override;
    void Close() override;

    audio_service::SessionHandle CreateSession(const audio_service::SessionConfig& config,
                                                std::string& error) override;
    bool DestroySession(audio_service::SessionHandle session) override;

    audio_service::PushResult PushBlocking(audio_service::SessionHandle session,
                                            const void* inputData,
                                            std::size_t frameCount,
                                            const audio_service::AudioFormat& inputFormat,
                                            std::chrono::milliseconds timeout) override;
    audio_service::PushResult Push(audio_service::SessionHandle session,
                                   const void* inputData,
                                   std::size_t frameCount,
                                   const audio_service::AudioFormat& inputFormat) override;

    bool SetSessionVolume(audio_service::SessionHandle session, float volume) override;
    bool SetSessionMuted(audio_service::SessionHandle session, bool muted) override;
    bool FlushSession(audio_service::SessionHandle session) override;

    std::size_t QueuedFrames(audio_service::SessionHandle session) const override;
    audio_service::PlaybackStatistics Statistics() const override;

private:
    std::unique_ptr<PlayerEngine> mEngine;
    audio_service::AudioFormat mOutputFormat{};
};

}  // namespace alsa_playback
