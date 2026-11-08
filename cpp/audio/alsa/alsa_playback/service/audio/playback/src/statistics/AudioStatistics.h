#pragma once

#include <atomic>
#include <cstdint>

namespace alsa_playback {

struct AudioStatisticsSnapshot {
    std::uint64_t pushedFrames = 0;
    std::uint64_t acceptedFrames = 0;
    std::uint64_t droppedFrames = 0;
    std::uint64_t outputFrames = 0;
    std::uint64_t underrunCount = 0;
    std::uint64_t recoveryCount = 0;
    std::uint64_t peakQueuedFrames = 0;
};

class AudioStatistics {
public:
    void AddPushedFrames(std::uint64_t frames);
    void AddAcceptedFrames(std::uint64_t frames);
    void AddDroppedFrames(std::uint64_t frames);
    void AddOutputFrames(std::uint64_t frames);
    void AddUnderrun();
    void AddRecovery();
    void UpdatePeakQueuedFrames(std::uint64_t frames);
    AudioStatisticsSnapshot Snapshot() const;

private:
    std::atomic<std::uint64_t> mPushedFrames{0};
    std::atomic<std::uint64_t> mAcceptedFrames{0};
    std::atomic<std::uint64_t> mDroppedFrames{0};
    std::atomic<std::uint64_t> mOutputFrames{0};
    std::atomic<std::uint64_t> mUnderrunCount{0};
    std::atomic<std::uint64_t> mRecoveryCount{0};
    std::atomic<std::uint64_t> mPeakQueuedFrames{0};
};

}  // namespace alsa_playback
