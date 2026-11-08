#include "PlaybackSession.h"

#include <mutex>

namespace alsa_playback {
namespace {

std::mutex& EngineMutex() {
    static std::mutex mutex;
    return mutex;
}

std::shared_ptr<PlayerEngine>& EngineInstance() {
    static std::shared_ptr<PlayerEngine> engine;
    return engine;
}

std::size_t& EngineUsers() {
    static std::size_t users = 0;
    return users;
}

PlayerConfig ToPlayerConfig(const PlaybackSessionConfig& config) {
    PlayerConfig internal{};
    internal.deviceName = config.deviceHint;
    internal.pcm.format = config.outputFormat;
    internal.maxSessions = config.maxSessions;
    internal.mixBlockFrames = config.mixBlockFrames;
    return internal;
}

}

PlaybackSession::PlaybackSession() = default;

PlaybackSession::~PlaybackSession() {
    std::string ignored;
    Close(ignored);
}

bool PlaybackSession::Open(const PlaybackSessionConfig& config, std::string& error) {
    std::string ignored;
    Close(ignored);
    if (config.deviceHint.empty() || !config.outputFormat.IsValid()) {
        error = "播放会话配置无效";
        return false;
    }
    auto engine = SharedEngine(config, error);
    if (engine == nullptr) {
        return false;
    }
    AudioSessionConfig session = config.session;
    session.inputFormat = config.outputFormat;
    const SessionId sessionId = engine->CreateSession(session, error);
    if (sessionId == kInvalidSessionId) {
        ReleaseEngine();
        return false;
    }
    mEngine = std::move(engine);
    mSessionId = sessionId;
    mOutputFormat = config.outputFormat;
    return true;
}

PushResult PlaybackSession::Write(const float* samples,
                                  std::size_t frameCount,
                                  std::chrono::milliseconds timeout) {
    if (mEngine == nullptr || mSessionId == kInvalidSessionId) {
        return {PushError::InvalidSession, 0, "播放会话尚未打开"};
    }
    return mEngine->PushPcmBlocking(mSessionId, samples, frameCount, mOutputFormat, timeout);
}

bool PlaybackSession::Close(std::string& error) {
    error.clear();
    if (mEngine != nullptr && mSessionId != kInvalidSessionId) {
        mEngine->DestroySession(mSessionId);
        mSessionId = kInvalidSessionId;
    }
    if (mEngine != nullptr) {
        mEngine.reset();
        ReleaseEngine();
    }
    return true;
}

bool PlaybackSession::IsDeviceOpen() const {
    return mEngine != nullptr && mEngine->IsDeviceOpen();
}

bool PlaybackSession::SetVolume(float volume) {
    return mEngine != nullptr && mSessionId != kInvalidSessionId &&
           mEngine->SetSessionVolume(mSessionId, volume);
}

bool PlaybackSession::SetMuted(bool muted) {
    return mEngine != nullptr && mSessionId != kInvalidSessionId &&
           mEngine->SetSessionMuted(mSessionId, muted);
}

std::size_t PlaybackSession::QueuedFrames() const {
    return mEngine == nullptr || mSessionId == kInvalidSessionId
               ? 0
               : mEngine->QueuedFrames(mSessionId);
}

bool PlaybackSession::EnumeratePlaybackDevices(std::vector<AlsaDeviceInfo>& devices,
                                               std::string& error) {
    return AlsaDeviceCatalog::EnumeratePlaybackDevices(devices, error);
}

std::shared_ptr<PlayerEngine> PlaybackSession::SharedEngine(const PlaybackSessionConfig& config,
                                                            std::string& error) {
    std::lock_guard<std::mutex> lock(EngineMutex());
    auto& engine = EngineInstance();
    if (engine == nullptr) {
        engine = std::make_shared<PlayerEngine>();
        if (!engine->Open(ToPlayerConfig(config), error)) {
            engine.reset();
            return nullptr;
        }
    }
    ++EngineUsers();
    return engine;
}

void PlaybackSession::ReleaseEngine() {
    std::shared_ptr<PlayerEngine> lastEngine;
    {
        std::lock_guard<std::mutex> lock(EngineMutex());
        if (EngineUsers() == 0) {
            return;
        }
        --EngineUsers();
        if (EngineUsers() == 0) {
            lastEngine = EngineInstance();
            EngineInstance().reset();
        }
    }
    if (lastEngine != nullptr) {
        lastEngine->Close();
    }
}

}  // namespace alsa_playback
