#include "ServiceImpl.h"

#include "device/AlsaDeviceCatalog.h"
#include "format/AudioFormat.h"
#include "player/PlayerEngine.h"
#include "player/PlayerConfig.h"
#include "session/AudioSessionConfig.h"
#include "session/SessionTypes.h"

namespace audio_service {
namespace {

constexpr std::size_t kDefaultMaxSessions = 8;
constexpr std::size_t kDefaultMixBlockFrames = 1024;
constexpr std::size_t kDefaultMaxQueueFrames = 48000;

alsa_playback::AudioFormat ToInternalFormat(const AudioFormat& format) {
    return alsa_playback::AudioFormat{format.sampleRate, format.channels,
                                      alsa_playback::SampleFormat::F32LE,
                                      alsa_playback::ChannelLayout::Interleaved};
}

AudioFormat ToPublicFormat(const alsa_playback::AudioFormat& format) {
    return AudioFormat{format.sampleRate, format.channels};
}

alsa_playback::PlayerConfig ToInternalConfig(const ServiceConfig& config) {
    alsa_playback::PlayerConfig internal{};
    internal.deviceName = config.deviceHint;
    internal.pcm.format.sampleRate = config.outputFormat.sampleRate;
    internal.pcm.format.channels = config.outputFormat.channels;
    internal.pcm.format.sampleFormat = alsa_playback::SampleFormat::F32LE;
    internal.pcm.format.channelLayout = alsa_playback::ChannelLayout::Interleaved;
    internal.maxSessions = kDefaultMaxSessions;
    internal.mixBlockFrames = kDefaultMixBlockFrames;
    return internal;
}

alsa_playback::AudioSessionConfig ToInternalSession(const SessionConfig& config) {
    alsa_playback::AudioSessionConfig internal{};
    internal.name = config.name;
    internal.maxQueueFrames = kDefaultMaxQueueFrames;
    internal.volume = config.volume;
    internal.muted = config.muted;
    internal.priority = config.priority;
    return internal;
}

PushResult ToPublicResult(const alsa_playback::PushResult& result) {
    PushResult output{};
    output.ok = result.IsSuccess();
    output.acceptedFrames = result.acceptedFrames;
    output.message = result.message;
    return output;
}

PlaybackStatistics ToPublicStatistics(const alsa_playback::AudioStatisticsSnapshot& snapshot) {
    PlaybackStatistics output{};
    output.acceptedFrames = snapshot.acceptedFrames;
    output.droppedFrames = snapshot.droppedFrames;
    output.outputFrames = snapshot.outputFrames;
    return output;
}

bool ToInternalFormatChecked(const AudioFormat& format,
                             alsa_playback::AudioFormat& internal,
                             std::string& error) {
    if (format.sampleRate == 0 || format.sampleRate > 192000 || format.channels == 0 ||
        format.channels > 32) {
        error = "输入音频格式无效";
        return false;
    }
    internal = ToInternalFormat(format);
    return true;
}

}

bool AudioPlaybackService::EnumeratePlaybackDevices(std::vector<PlaybackDeviceInfo>& devices,
                                                    std::string& error) {
    std::vector<alsa_playback::AlsaDeviceInfo> internalDevices;
    if (!alsa_playback::AlsaDeviceCatalog::EnumeratePlaybackDevices(internalDevices, error)) {
        return false;
    }
    devices.clear();
    for (const auto& device : internalDevices) {
        devices.push_back({device.name, device.description});
    }
    return true;
}

std::unique_ptr<AudioPlaybackService> CreateAudioPlaybackService() {
    return std::make_unique<alsa_playback::ServiceImpl>();
}

}  // namespace audio_service

namespace alsa_playback {

ServiceImpl::ServiceImpl() : mEngine(std::make_unique<PlayerEngine>()) {
}

ServiceImpl::~ServiceImpl() {
    Close();
}

bool ServiceImpl::Open(const audio_service::ServiceConfig& config, std::string& error) {
    if (config.deviceHint.empty() || !config.outputFormat.IsValid()) {
        error = "播放服务配置无效";
        return false;
    }
    mOutputFormat = config.outputFormat;
    return mEngine->Open(audio_service::ToInternalConfig(config), error);
}

bool ServiceImpl::Start(std::string& error) {
    return mEngine->Start(error);
}

void ServiceImpl::Stop() {
    mEngine->Stop();
}

void ServiceImpl::Close() {
    mEngine->Close();
}

audio_service::SessionHandle ServiceImpl::CreateSession(const audio_service::SessionConfig& config,
                                                        std::string& error) {
    auto internal = audio_service::ToInternalSession(config);
    internal.inputFormat = audio_service::ToInternalFormat(mOutputFormat);
    const auto sessionId = mEngine->CreateSession(internal, error);
    return sessionId == kInvalidSessionId
               ? audio_service::kInvalidSession
               : static_cast<audio_service::SessionHandle>(sessionId);
}

bool ServiceImpl::DestroySession(audio_service::SessionHandle session) {
    return mEngine->DestroySession(static_cast<SessionId>(session));
}

audio_service::PushResult ServiceImpl::PushBlocking(audio_service::SessionHandle session,
                                                     const void* inputData,
                                                     std::size_t frameCount,
                                                     const audio_service::AudioFormat& inputFormat,
                                                     std::chrono::milliseconds timeout) {
    AudioFormat internalFormat{};
    std::string error;
    if (!audio_service::ToInternalFormatChecked(inputFormat, internalFormat, error)) {
        return {false, 0, error};
    }
    const auto result = mEngine->PushPcmBlocking(static_cast<SessionId>(session), inputData,
                                                  frameCount, internalFormat, timeout);
    return audio_service::ToPublicResult(result);
}

audio_service::PushResult ServiceImpl::Push(audio_service::SessionHandle session,
                                             const void* inputData,
                                             std::size_t frameCount,
                                             const audio_service::AudioFormat& inputFormat) {
    AudioFormat internalFormat{};
    std::string error;
    if (!audio_service::ToInternalFormatChecked(inputFormat, internalFormat, error)) {
        return {false, 0, error};
    }
    const auto result =
        mEngine->PushPcm(static_cast<SessionId>(session), inputData, frameCount, internalFormat);
    return audio_service::ToPublicResult(result);
}

bool ServiceImpl::SetSessionVolume(audio_service::SessionHandle session, float volume) {
    return mEngine->SetSessionVolume(static_cast<SessionId>(session), volume);
}

bool ServiceImpl::SetSessionMuted(audio_service::SessionHandle session, bool muted) {
    return mEngine->SetSessionMuted(static_cast<SessionId>(session), muted);
}

bool ServiceImpl::FlushSession(audio_service::SessionHandle session) {
    return mEngine->FlushSession(static_cast<SessionId>(session));
}

std::size_t ServiceImpl::QueuedFrames(audio_service::SessionHandle session) const {
    return mEngine->QueuedFrames(static_cast<SessionId>(session));
}

audio_service::PlaybackStatistics ServiceImpl::Statistics() const {
    return audio_service::ToPublicStatistics(mEngine->Statistics());
}

}  // namespace alsa_playback
