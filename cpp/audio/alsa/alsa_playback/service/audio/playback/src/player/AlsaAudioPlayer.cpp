#include "player/AlsaAudioPlayer.h"

#include "player/PlayerEngine.h"

namespace alsa_playback {

AlsaAudioPlayer::AlsaAudioPlayer()
    : mEngine(std::make_unique<PlayerEngine>()) {
}

AlsaAudioPlayer::~AlsaAudioPlayer() = default;

bool AlsaAudioPlayer::Open(const PlayerConfig& config, std::string& error) {
    return mEngine->Open(config, error);
}

bool AlsaAudioPlayer::Start(std::string& error) {
    return mEngine->Start(error);
}

void AlsaAudioPlayer::Stop() {
    mEngine->Stop();
}

void AlsaAudioPlayer::Close() {
    mEngine->Close();
}

PlayerState AlsaAudioPlayer::State() const {
    return mEngine->State();
}

SessionId AlsaAudioPlayer::CreateSession(const AudioSessionConfig& config, std::string& error) {
    return mEngine->CreateSession(config, error);
}

bool AlsaAudioPlayer::DestroySession(SessionId sessionId) {
    return mEngine->DestroySession(sessionId);
}

bool AlsaAudioPlayer::FlushSession(SessionId sessionId) {
    return mEngine->FlushSession(sessionId);
}

std::size_t AlsaAudioPlayer::QueuedFrames(SessionId sessionId) const {
    return mEngine->QueuedFrames(sessionId);
}

PushResult AlsaAudioPlayer::PushPcm(SessionId sessionId,
                                    const void* inputData,
                                    std::size_t frameCount,
                                    const AudioFormat& inputFormat) {
    return mEngine->PushPcm(sessionId, inputData, frameCount, inputFormat);
}

PushResult AlsaAudioPlayer::PushPcmBlocking(SessionId sessionId,
                                            const void* inputData,
                                            std::size_t frameCount,
                                            const AudioFormat& inputFormat,
                                            std::chrono::milliseconds timeout) {
    return mEngine->PushPcmBlocking(sessionId, inputData, frameCount, inputFormat, timeout);
}

bool AlsaAudioPlayer::SetSessionVolume(SessionId sessionId, float volume) {
    return mEngine->SetSessionVolume(sessionId, volume);
}

bool AlsaAudioPlayer::SetSessionMuted(SessionId sessionId, bool muted) {
    return mEngine->SetSessionMuted(sessionId, muted);
}

AudioStatisticsSnapshot AlsaAudioPlayer::Statistics() const {
    return mEngine->Statistics();
}

}  // namespace alsa_playback
