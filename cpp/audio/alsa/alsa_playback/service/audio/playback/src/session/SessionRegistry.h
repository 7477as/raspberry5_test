#pragma once

#include "session/AudioSession.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace alsa_playback {

class SessionRegistry {
public:
    SessionRegistry(std::size_t maxSessions, std::uint32_t queueChannels);

    std::shared_ptr<AudioSession> Create(AudioSessionConfig config, std::string& error);
    std::shared_ptr<AudioSession> Find(SessionId id) const;
    bool Remove(SessionId id);
    std::vector<std::shared_ptr<AudioSession>> Snapshot() const;
    void Clear();
    void CloseAll();
    void ReopenAll();

private:
    std::size_t mMaxSessions;
    std::uint32_t mQueueChannels;
    mutable std::mutex mMutex;
    SessionId mNextId = 1;
    std::vector<std::shared_ptr<AudioSession>> mSessions;
};

}  // namespace alsa_playback
