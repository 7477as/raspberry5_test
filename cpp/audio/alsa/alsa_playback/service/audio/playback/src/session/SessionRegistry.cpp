#include "session/SessionRegistry.h"

#include <algorithm>
#include <utility>

namespace alsa_playback {

SessionRegistry::SessionRegistry(std::size_t maxSessions, std::uint32_t queueChannels)
    : mMaxSessions(maxSessions), mQueueChannels(queueChannels) {
}

std::shared_ptr<AudioSession> SessionRegistry::Create(AudioSessionConfig config, std::string& error) {
    if (!config.inputFormat.IsValid() || config.maxQueueFrames == 0) {
        error = "业务 session 配置无效";
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSessions.size() >= mMaxSessions) {
        error = "已达到最大业务 session 数量";
        return nullptr;
    }
    if (mNextId == kInvalidSessionId) {
        error = "session ID 已耗尽";
        return nullptr;
    }
    auto session = std::make_shared<AudioSession>(mNextId++, std::move(config), mQueueChannels);
    mSessions.push_back(session);
    return session;
}

std::shared_ptr<AudioSession> SessionRegistry::Find(SessionId id) const {
    std::lock_guard<std::mutex> lock(mMutex);
    const auto iterator = std::find_if(mSessions.cbegin(), mSessions.cend(),
                                       [id](const auto& session) { return session->Id() == id; });
    return iterator == mSessions.cend() ? nullptr : *iterator;
}

bool SessionRegistry::Remove(SessionId id) {
    std::lock_guard<std::mutex> lock(mMutex);
    const auto iterator = std::find_if(mSessions.begin(), mSessions.end(),
                                       [id](const auto& session) { return session->Id() == id; });
    if (iterator == mSessions.end()) {
        return false;
    }
    (*iterator)->Close();
    mSessions.erase(iterator);
    return true;
}

std::vector<std::shared_ptr<AudioSession>> SessionRegistry::Snapshot() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mSessions;
}

void SessionRegistry::CloseAll() {
    std::lock_guard<std::mutex> lock(mMutex);
    for (const auto& session : mSessions) {
        session->Close();
    }
}

void SessionRegistry::ReopenAll() {
    std::lock_guard<std::mutex> lock(mMutex);
    for (const auto& session : mSessions) {
        session->Reopen();
    }
}

void SessionRegistry::Clear() {
    std::lock_guard<std::mutex> lock(mMutex);
    for (const auto& session : mSessions) {
        session->Close();
    }
    mSessions.clear();
}

}  // namespace alsa_playback
