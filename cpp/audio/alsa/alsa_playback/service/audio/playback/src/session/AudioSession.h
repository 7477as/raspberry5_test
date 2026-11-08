#pragma once

#include "session/AudioSessionConfig.h"
#include "session/BoundedPcmQueue.h"
#include "session/SessionTypes.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace alsa_playback {

class AudioSession {
public:
    AudioSession(SessionId id, AudioSessionConfig config, std::uint32_t queueChannels);

    SessionId Id() const;
    const AudioSessionConfig& Config() const;
    PushResult Push(PcmBlock block);
    PushResult PushBlocking(PcmBlock block, std::chrono::milliseconds timeout);
    std::size_t ReadFrames(std::size_t frameCount, std::vector<float>& outputSamples);
    void Flush();
    void Close();
    void Reopen();
    std::size_t QueuedFrames() const;
    void SetVolume(float volume);
    float Volume() const;
    void SetMuted(bool muted);
    bool IsMuted() const;
    std::int32_t Priority() const;
    std::uint32_t QueueChannels() const;

private:
    SessionId mId;
    AudioSessionConfig mConfig;
    std::uint32_t mQueueChannels;
    BoundedPcmQueue mQueue;
    std::atomic<float> mVolume;
    std::atomic<bool> mMuted;
};

}  // namespace alsa_playback
