#pragma once

#include "PlaybackSession.h"
#include "service/audio/playback/src/format/AudioResampler.h"
#include "service/audio/playback/src/format/WavFileReader.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace alsa_playback {

class MusicPlayer {
public:
    MusicPlayer() = default;
    ~MusicPlayer();

    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer& operator=(const MusicPlayer&) = delete;

    bool Open(const PlaybackSessionConfig& sessionConfig,
              const std::string& filePath,
              std::chrono::milliseconds pushTimeout,
              std::string& error);
    void Play(const std::atomic<bool>& externalStop, std::uint32_t durationSeconds);
    bool Close(std::string& error);

private:
    bool FillPendingSamples(std::string& error);
    bool SubmitNextBlock(std::string& error);
    void Run();
    void WaitUntilDrained();
    void StopThread();
    void JoinThread();

    static constexpr std::size_t kBlockFrames = 1024;
    static constexpr std::size_t kWavReadFrames = 2048;
    PlaybackSession mSession;
    AudioFormat mInputFormat{};
    AudioFormat mOutputFormat{};
    std::chrono::milliseconds mPushTimeout{100};
    std::chrono::steady_clock::time_point mStartTime{};
    WavFileReader mReader;
    std::unique_ptr<AudioResampler> mResampler;
    std::vector<float> mPendingSamples;
    bool mEndOfFile = false;
    const std::atomic<bool>* mExternalStop = nullptr;
    std::uint32_t mDurationSeconds = 0;
    std::atomic<bool> mStopRequested{true};
    std::atomic<bool> mDone{true};
    bool mSucceeded = true;
    std::string mError;
    std::thread mThread;
};

}  // namespace alsa_playback
