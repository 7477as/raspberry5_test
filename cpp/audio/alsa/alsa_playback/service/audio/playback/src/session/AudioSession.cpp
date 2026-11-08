#include "session/AudioSession.h"

#include <algorithm>
#include <utility>

namespace alsa_playback {

bool PushResult::IsSuccess() const {
    return error == PushError::None && acceptedFrames > 0;
}

AudioSession::AudioSession(SessionId id, AudioSessionConfig config, std::uint32_t queueChannels)
    : mId(id),
      mConfig(std::move(config)),
      mQueueChannels(queueChannels),
      mQueue(mConfig.maxQueueFrames),
      mVolume(std::clamp(mConfig.volume, 0.0F, 1.0F)),
      mMuted(mConfig.muted) {
}

SessionId AudioSession::Id() const {
    return mId;
}

const AudioSessionConfig& AudioSession::Config() const {
    return mConfig;
}

PushResult AudioSession::Push(PcmBlock block) {
    if (!block.IsValid() || block.channels != mQueueChannels) {
        return {PushError::InvalidArgument, 0, "PCM block 无效或声道数不匹配"};
    }
    const std::size_t frameCount = block.frames;
    if (!mQueue.Push(std::move(block))) {
        return {PushError::QueueFull, 0, "业务 PCM 队列已满"};
    }
    return {PushError::None, frameCount, {}};
}

PushResult AudioSession::PushBlocking(PcmBlock block, std::chrono::milliseconds timeout) {
    if (!block.IsValid() || block.channels != mQueueChannels) {
        return {PushError::InvalidArgument, 0, "PCM block 无效或声道数不匹配"};
    }
    const std::size_t frameCount = block.frames;
    if (!mQueue.PushBlocking(std::move(block), timeout)) {
        if (mQueue.IsClosed()) {
            return {PushError::NotRunning, 0, "业务 PCM 队列已关闭"};
        }
        return {PushError::QueueFull, 0, "等待业务 PCM 队列空间超时"};
    }
    return {PushError::None, frameCount, {}};
}

std::size_t AudioSession::ReadFrames(std::size_t frameCount, std::vector<float>& outputSamples) {
    return mQueue.PopFrames(frameCount, mQueueChannels, outputSamples);
}

void AudioSession::Flush() {
    mQueue.Clear();
}

void AudioSession::Close() {
    mQueue.Close();
}

void AudioSession::Reopen() {
    mQueue.Reopen();
}

std::size_t AudioSession::QueuedFrames() const {
    return mQueue.QueuedFrames();
}

void AudioSession::SetVolume(float volume) {
    mVolume.store(std::clamp(volume, 0.0F, 1.0F), std::memory_order_relaxed);
}

float AudioSession::Volume() const {
    return mVolume.load(std::memory_order_relaxed);
}

void AudioSession::SetMuted(bool muted) {
    mMuted.store(muted, std::memory_order_relaxed);
}

bool AudioSession::IsMuted() const {
    return mMuted.load(std::memory_order_relaxed);
}

std::int32_t AudioSession::Priority() const {
    return mConfig.priority;
}

std::uint32_t AudioSession::QueueChannels() const {
    return mQueueChannels;
}

}  // namespace alsa_playback
