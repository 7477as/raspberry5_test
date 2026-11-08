#include "session/BoundedPcmQueue.h"

#include <algorithm>
#include <limits>

namespace alsa_playback {

BoundedPcmQueue::BoundedPcmQueue(std::size_t capacityFrames)
    : mCapacityFrames(capacityFrames) {
}

bool BoundedPcmQueue::Push(PcmBlock block) {
    if (!block.IsValid()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mMutex);
    if (mClosed || block.frames > mCapacityFrames - std::min(mQueuedFrames, mCapacityFrames)) {
        return false;
    }
    mQueuedFrames += block.frames;
    mBlocks.emplace_back(std::move(block));
    return true;
}

bool BoundedPcmQueue::PushBlocking(PcmBlock block, std::chrono::milliseconds timeout) {
    if (!block.IsValid() || block.frames > mCapacityFrames) {
        return false;
    }
    std::unique_lock<std::mutex> lock(mMutex);
    if (!mSpaceAvailable.wait_for(lock, timeout, [this, &block] {
            return mClosed || block.frames <= mCapacityFrames - mQueuedFrames;
        })) {
        return false;
    }
    if (mClosed) {
        return false;
    }
    mQueuedFrames += block.frames;
    mBlocks.emplace_back(std::move(block));
    return true;
}

std::size_t BoundedPcmQueue::PopFrames(std::size_t frameCount,
                                       std::uint32_t channels,
                                       std::vector<float>& outputSamples) {
    if (frameCount == 0 || channels == 0 ||
        frameCount > std::numeric_limits<std::size_t>::max() / channels) {
        outputSamples.clear();
        return 0;
    }
    outputSamples.assign(frameCount * channels, 0.0F);
    std::unique_lock<std::mutex> lock(mMutex);
    std::size_t copiedFrames = 0;
    while (copiedFrames < frameCount && !mBlocks.empty()) {
        PcmBlock& block = mBlocks.front();
        if (block.channels != channels || mFrontOffsetFrames >= block.frames) {
            mQueuedFrames -= block.frames - mFrontOffsetFrames;
            mFrontOffsetFrames = 0;
            mBlocks.pop_front();
            continue;
        }
        const std::size_t availableFrames = block.frames - mFrontOffsetFrames;
        const std::size_t framesToCopy = std::min(frameCount - copiedFrames, availableFrames);
        const float* source = block.samples.data() + mFrontOffsetFrames * channels;
        float* destination = outputSamples.data() + copiedFrames * channels;
        std::copy_n(source, framesToCopy * channels, destination);
        copiedFrames += framesToCopy;
        mFrontOffsetFrames += framesToCopy;
        mQueuedFrames -= framesToCopy;
        if (mFrontOffsetFrames == block.frames) {
            mFrontOffsetFrames = 0;
            mBlocks.pop_front();
        }
    }
    lock.unlock();
    mSpaceAvailable.notify_all();
    return copiedFrames;
}

void BoundedPcmQueue::Clear() {
    std::lock_guard<std::mutex> lock(mMutex);
    mBlocks.clear();
    mQueuedFrames = 0;
    mFrontOffsetFrames = 0;
    mSpaceAvailable.notify_all();
}

void BoundedPcmQueue::Close() {
    std::lock_guard<std::mutex> lock(mMutex);
    mClosed = true;
    mSpaceAvailable.notify_all();
}

void BoundedPcmQueue::Reopen() {
    std::lock_guard<std::mutex> lock(mMutex);
    mClosed = false;
    mSpaceAvailable.notify_all();
}

std::size_t BoundedPcmQueue::QueuedFrames() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mQueuedFrames;
}

bool BoundedPcmQueue::IsClosed() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mClosed;
}

}  // namespace alsa_playback
