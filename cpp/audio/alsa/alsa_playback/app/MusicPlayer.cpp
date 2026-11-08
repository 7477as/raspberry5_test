#include "app/MusicPlayer.h"

#include "service/audio/playback/src/format/ChannelMapper.h"
#include "service/audio/playback/src/format/PcmConverter.h"

#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

namespace alsa_playback {

MusicPlayer::~MusicPlayer() {
    std::string ignored;
    Close(ignored);
}

bool MusicPlayer::Open(const PlaybackSessionConfig& sessionConfig,
                       const std::string& filePath,
                       std::chrono::milliseconds pushTimeout,
                       std::string& error) {
    std::string ignored;
    Close(ignored);
    if (!sessionConfig.outputFormat.IsValid()) {
        error = "音乐输出格式无效";
        return false;
    }
    if (!mReader.Open(filePath, error)) {
        return false;
    }
    mInputFormat = AudioFormat{mReader.Format().sampleRate, mReader.Format().channels};
    mOutputFormat = sessionConfig.outputFormat;
    mPushTimeout = pushTimeout;
    mStartTime = std::chrono::steady_clock::now();
    mResampler = std::make_unique<AudioResampler>(mReader.Format().sampleRate,
                                                  sessionConfig.outputFormat.sampleRate,
                                                  mReader.Format().channels);
    PlaybackSessionConfig config = sessionConfig;
    config.session.name = "music";
    config.session.volume = 0.45F;
    config.session.priority = 0;
    if (!mSession.Open(config, error)) {
        mResampler.reset();
        return false;
    }
    mPendingSamples.clear();
    mEndOfFile = false;
    mSucceeded = true;
    mError.clear();
    return true;
}

void MusicPlayer::Play(const std::atomic<bool>& externalStop, std::uint32_t durationSeconds) {
    StopThread();
    mExternalStop = &externalStop;
    mDurationSeconds = durationSeconds;
    mStopRequested.store(false, std::memory_order_release);
    mDone.store(false, std::memory_order_release);
    mSucceeded = true;
    mError.clear();
    mThread = std::thread(&MusicPlayer::Run, this);
}

bool MusicPlayer::Close(std::string& error) {
    JoinThread();
    error = mError;
    const bool succeeded = mSucceeded;
    std::string ignored;
    mSession.Close(ignored);
    mResampler.reset();
    mPendingSamples.clear();
    mEndOfFile = false;
    mExternalStop = nullptr;
    return succeeded;
}

bool MusicPlayer::FillPendingSamples(std::string& error) {
    std::vector<std::int16_t> inputSamples;
    std::size_t framesRead = 0;
    if (!mReader.ReadFrames(kWavReadFrames, inputSamples, framesRead, error)) {
        return false;
    }
    if (framesRead == 0) {
        mEndOfFile = true;
        return true;
    }
    std::vector<float> floatSamples;
    if (!PcmConverter::ConvertToFloat(inputSamples.data(), framesRead, mReader.Format(),
                                      floatSamples, error)) {
        return false;
    }
    if (mReader.IsEndOfFile()) {
        mEndOfFile = true;
    }
    std::vector<float> outputSamples;
    mResampler->Process(floatSamples, outputSamples);
    return ChannelMapper::MapInterleaved(outputSamples, mReader.Format().channels,
                                         mOutputFormat.channels, mPendingSamples, error);
}

bool MusicPlayer::SubmitNextBlock(std::string& error) {
    if (mEndOfFile) {
        return true;
    }
    if (mPendingSamples.empty() && !FillPendingSamples(error)) {
        return false;
    }
    if (mPendingSamples.empty()) {
        return true;
    }
    const std::size_t pendingFrames = mPendingSamples.size() / mOutputFormat.channels;
    const std::size_t blockFrames = std::min(kBlockFrames, pendingFrames);
    const PushResult result =
        mSession.Write(mPendingSamples.data(), blockFrames, mPushTimeout);
    if (!result.IsSuccess()) {
        error = result.message;
        return false;
    }
    mPendingSamples.erase(mPendingSamples.begin(),
                          mPendingSamples.begin() +
                              static_cast<std::ptrdiff_t>(result.acceptedFrames *
                                                          mOutputFormat.channels));
    return true;
}

void MusicPlayer::Run() {
    std::string error;
    bool succeeded = true;
    while (!mStopRequested.load(std::memory_order_acquire) &&
           (mExternalStop == nullptr ||
            !mExternalStop->load(std::memory_order_acquire)) &&
           !mEndOfFile) {
        if (mDurationSeconds != 0) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - mStartTime);
            if (elapsed.count() >= mDurationSeconds) {
                break;
            }
        }
        if (!SubmitNextBlock(error)) {
            succeeded = false;
            break;
        }
    }
    if (succeeded && mDurationSeconds == 0 &&
        (mExternalStop == nullptr || !mExternalStop->load(std::memory_order_acquire))) {
        WaitUntilDrained();
    }
    mError = std::move(error);
    mSucceeded = succeeded;
    mDone.store(true, std::memory_order_release);
}

void MusicPlayer::WaitUntilDrained() {
    while (!mStopRequested.load(std::memory_order_acquire) &&
           (mExternalStop == nullptr || !mExternalStop->load(std::memory_order_acquire)) &&
           mSession.QueuedFrames() != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void MusicPlayer::StopThread() {
    mStopRequested.store(true, std::memory_order_release);
    JoinThread();
}

void MusicPlayer::JoinThread() {
    if (mThread.joinable()) {
        mThread.join();
    }
}

}
