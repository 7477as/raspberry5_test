#include "player/PlayerEngine.h"

#include "effect/BypassEffect.h"
#include "effect/DrcEffect.h"
#include "effect/EqualizerEffect.h"
#include "effect/LimiterEffect.h"
#include "format/ChannelMapper.h"
#include "format/PcmConverter.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

namespace alsa_playback {

PlayerEngine::PlayerEngine() = default;

PlayerEngine::~PlayerEngine() {
    Close();
}

bool PlayerEngine::Open(const PlayerConfig& config, std::string& error) {
    Close();
    if (config.deviceName.empty() || config.mixBlockFrames == 0 || config.maxSessions == 0 ||
        !ValidateAlsaPcmParameters(config.pcm, error)) {
        if (error.empty()) {
            error = "播放器配置无效";
        }
        return false;
    }
    auto output = std::make_unique<AlsaPcmOutput>();
    if (!output->Open(config.deviceName, config.pcm, error)) {
        return false;
    }
    mConfig = config;
    mOutput = std::move(output);
    mSessions = std::make_unique<SessionRegistry>(config.maxSessions, config.pcm.format.channels);
    mMixer = std::make_unique<AudioMixer>(MixerConfig{config.pcm.format.channels, true, true});
    mMixSamples.resize(config.mixBlockFrames * config.pcm.format.channels);
    BuildEffectChain();
    {
        std::lock_guard<std::mutex> lock(mStateMutex);
        mState = PlayerState::Opened;
    }
    return true;
}

bool PlayerEngine::Start(std::string& error) {
    std::lock_guard<std::mutex> lock(mStateMutex);
    if (mState != PlayerState::Opened) {
        error = "播放器当前状态不允许启动";
        return false;
    }
    mStopRequested.store(false, std::memory_order_release);
    if (mSessions != nullptr) {
        mSessions->ReopenAll();
    }
    try {
        mThread = std::thread(&PlayerEngine::Run, this);
    } catch (const std::exception& exception) {
        error = std::string("创建播放线程失败: ") + exception.what();
        mState = PlayerState::Error;
        return false;
    }
    mState = PlayerState::Running;
    return true;
}

void PlayerEngine::Stop() {
    mStopRequested.store(true, std::memory_order_release);
    if (mSessions != nullptr) {
        mSessions->CloseAll();
    }
    mWakeCondition.notify_all();
    if (mThread.joinable()) {
        mThread.join();
    }
    std::lock_guard<std::mutex> lock(mStateMutex);
    if (mState != PlayerState::Closed) {
        mState = mOutput != nullptr && mOutput->IsOpen() ? PlayerState::Opened : PlayerState::Closed;
    }
}

void PlayerEngine::Close() {
    Stop();
    if (mSessions != nullptr) {
        mSessions->Clear();
    }
    mMixer.reset();
    mSessions.reset();
    mOutput.reset();
    std::lock_guard<std::mutex> lock(mStateMutex);
    mState = PlayerState::Closed;
}

PlayerState PlayerEngine::State() const {
    std::lock_guard<std::mutex> lock(mStateMutex);
    return mState;
}

SessionId PlayerEngine::CreateSession(const AudioSessionConfig& config, std::string& error) {
    if (State() == PlayerState::Closed || mSessions == nullptr) {
        error = "播放器尚未打开";
        return kInvalidSessionId;
    }
    if (config.inputFormat.sampleRate != mConfig.pcm.format.sampleRate) {
        error = "当前版本暂不支持 session 采样率转换";
        return kInvalidSessionId;
    }
    if (config.inputFormat.channels == 0 || config.inputFormat.channels > mConfig.pcm.format.channels) {
        error = "session 声道数超过输出声道数";
        return kInvalidSessionId;
    }
    const auto session = mSessions->Create(config, error);
    return session == nullptr ? kInvalidSessionId : session->Id();
}

bool PlayerEngine::DestroySession(SessionId sessionId) {
    return mSessions != nullptr && mSessions->Remove(sessionId);
}

bool PlayerEngine::FlushSession(SessionId sessionId) {
    const auto session = mSessions == nullptr ? nullptr : mSessions->Find(sessionId);
    if (session == nullptr) {
        return false;
    }
    session->Flush();
    return true;
}

std::size_t PlayerEngine::QueuedFrames(SessionId sessionId) const {
    const auto session = mSessions == nullptr ? nullptr : mSessions->Find(sessionId);
    return session == nullptr ? 0 : session->QueuedFrames();
}

PushResult PlayerEngine::PushPcm(SessionId sessionId,
                                 const void* inputData,
                                 std::size_t frameCount,
                                 const AudioFormat& inputFormat) {
    return PushPcmInternal(sessionId, inputData, frameCount, inputFormat, std::chrono::milliseconds(0), false);
}

PushResult PlayerEngine::PushPcmBlocking(SessionId sessionId,
                                         const void* inputData,
                                         std::size_t frameCount,
                                         const AudioFormat& inputFormat,
                                         std::chrono::milliseconds timeout) {
    return PushPcmInternal(sessionId, inputData, frameCount, inputFormat, timeout, true);
}

PushResult PlayerEngine::PushPcmInternal(SessionId sessionId,
                                         const void* inputData,
                                         std::size_t frameCount,
                                         const AudioFormat& inputFormat,
                                         std::chrono::milliseconds timeout,
                                         bool blocking) {
    if (State() == PlayerState::Closed || mSessions == nullptr ||
        (blocking && (State() != PlayerState::Running || mStopRequested.load(std::memory_order_acquire)))) {
        return {PushError::NotRunning, 0, "播放器尚未打开"};
    }
    const auto session = mSessions->Find(sessionId);
    if (session == nullptr) {
        return {PushError::InvalidSession, 0, "session 不存在"};
    }
    std::string error;
    if (!ValidateInputFormat(inputFormat, error)) {
        return {PushError::FormatMismatch, 0, error};
    }
    std::vector<float> inputSamples;
    if (!PcmConverter::ConvertToFloat(inputData, frameCount, inputFormat, inputSamples, error)) {
        return {PushError::ConversionFailed, 0, error};
    }
    std::vector<float> mappedSamples;
    if (!ChannelMapper::MapInterleaved(inputSamples,
                                       inputFormat.channels,
                                       mConfig.pcm.format.channels,
                                       mappedSamples,
                                       error)) {
        return {PushError::ConversionFailed, 0, error};
    }
    PcmBlock block(static_cast<std::uint32_t>(frameCount), mConfig.pcm.format.channels);
    block.samples = std::move(mappedSamples);
    const PushResult result = blocking
                                  ? session->PushBlocking(std::move(block), timeout)
                                  : session->Push(std::move(block));
    mStatistics.AddPushedFrames(frameCount);
    if (result.IsSuccess()) {
        mStatistics.AddAcceptedFrames(result.acceptedFrames);
        mStatistics.UpdatePeakQueuedFrames(session->QueuedFrames());
        mWakeCondition.notify_one();
    } else if (!blocking) {
        mStatistics.AddDroppedFrames(frameCount);
    }
    return result;
}

bool PlayerEngine::SetSessionVolume(SessionId sessionId, float volume) {
    const auto session = mSessions == nullptr ? nullptr : mSessions->Find(sessionId);
    if (session == nullptr) {
        return false;
    }
    session->SetVolume(volume);
    return true;
}

bool PlayerEngine::SetSessionMuted(SessionId sessionId, bool muted) {
    const auto session = mSessions == nullptr ? nullptr : mSessions->Find(sessionId);
    if (session == nullptr) {
        return false;
    }
    session->SetMuted(muted);
    return true;
}

AudioStatisticsSnapshot PlayerEngine::Statistics() const {
    return mStatistics.Snapshot();
}

void PlayerEngine::Run() {
    while (!mStopRequested.load(std::memory_order_acquire)) {
        const auto sessions = mSessions->Snapshot();
        mMixer->Mix(sessions, mConfig.mixBlockFrames, mMixSamples);
        mEffectChain.Process(mMixSamples,
                             static_cast<std::uint32_t>(mConfig.mixBlockFrames),
                             mConfig.pcm.format.channels);
        std::string error;
        if (!mOutput->Write(mMixSamples, mConfig.mixBlockFrames, error)) {
            std::lock_guard<std::mutex> lock(mStateMutex);
            mRuntimeError = error;
            mState = PlayerState::Error;
            mStopRequested.store(true, std::memory_order_release);
            break;
        }
        mStatistics.AddOutputFrames(mConfig.mixBlockFrames);
        if (sessions.empty()) {
            std::unique_lock<std::mutex> lock(mWakeMutex);
            mWakeCondition.wait_for(lock, std::chrono::milliseconds(5));
        }
    }
}

bool PlayerEngine::ValidateInputFormat(const AudioFormat& inputFormat, std::string& error) const {
    if (!inputFormat.IsValid()) {
        error = "输入音频格式无效";
        return false;
    }
    if (inputFormat.sampleRate != mConfig.pcm.format.sampleRate) {
        error = "输入采样率与输出采样率不一致";
        return false;
    }
    return true;
}

void PlayerEngine::BuildEffectChain() {
    mEffectChain = AudioEffectChain{};
    mEffectChain.AddEffect(std::make_unique<EqualizerEffect>());
    mEffectChain.AddEffect(std::make_unique<DrcEffect>());
    mEffectChain.AddEffect(std::make_unique<LimiterEffect>());
}

}  // namespace alsa_playback
