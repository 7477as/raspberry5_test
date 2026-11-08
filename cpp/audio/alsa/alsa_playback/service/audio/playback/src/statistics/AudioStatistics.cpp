#include "statistics/AudioStatistics.h"

#include <algorithm>

namespace alsa_playback {

void AudioStatistics::AddPushedFrames(std::uint64_t frames) {
    mPushedFrames.fetch_add(frames, std::memory_order_relaxed);
}

void AudioStatistics::AddAcceptedFrames(std::uint64_t frames) {
    mAcceptedFrames.fetch_add(frames, std::memory_order_relaxed);
}

void AudioStatistics::AddDroppedFrames(std::uint64_t frames) {
    mDroppedFrames.fetch_add(frames, std::memory_order_relaxed);
}

void AudioStatistics::AddOutputFrames(std::uint64_t frames) {
    mOutputFrames.fetch_add(frames, std::memory_order_relaxed);
}

void AudioStatistics::AddUnderrun() {
    mUnderrunCount.fetch_add(1, std::memory_order_relaxed);
}

void AudioStatistics::AddRecovery() {
    mRecoveryCount.fetch_add(1, std::memory_order_relaxed);
}

void AudioStatistics::UpdatePeakQueuedFrames(std::uint64_t frames) {
    auto peak = mPeakQueuedFrames.load(std::memory_order_relaxed);
    while (peak < frames &&
           !mPeakQueuedFrames.compare_exchange_weak(peak,
                                                    frames,
                                                    std::memory_order_relaxed,
                                                    std::memory_order_relaxed)) {
    }
}

AudioStatisticsSnapshot AudioStatistics::Snapshot() const {
    return {
        mPushedFrames.load(std::memory_order_relaxed),
        mAcceptedFrames.load(std::memory_order_relaxed),
        mDroppedFrames.load(std::memory_order_relaxed),
        mOutputFrames.load(std::memory_order_relaxed),
        mUnderrunCount.load(std::memory_order_relaxed),
        mRecoveryCount.load(std::memory_order_relaxed),
        mPeakQueuedFrames.load(std::memory_order_relaxed),
    };
}

}  // namespace alsa_playback
