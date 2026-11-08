#pragma once

#include "format/PcmBlock.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

namespace alsa_playback {

class BoundedPcmQueue {
public:
    explicit BoundedPcmQueue(std::size_t capacityFrames);

    bool Push(PcmBlock block);
    bool PushBlocking(PcmBlock block, std::chrono::milliseconds timeout);
    std::size_t PopFrames(std::size_t frameCount,
                          std::uint32_t channels,
                          std::vector<float>& outputSamples);
    void Clear();
    void Close();
    void Reopen();
    std::size_t QueuedFrames() const;
    std::size_t CapacityFrames() const;
    bool IsClosed() const;

private:
    std::size_t mCapacityFrames;
    mutable std::mutex mMutex;
    std::condition_variable mSpaceAvailable;
    std::deque<PcmBlock> mBlocks;
    std::size_t mQueuedFrames = 0;
    std::size_t mFrontOffsetFrames = 0;
    bool mClosed = false;
};

}  // namespace alsa_playback
