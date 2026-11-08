#include "app/CommandLineOptions.h"
#include "app/MusicPlayer.h"
#include "app/PromptPlayer.h"
#include "AudioPlaybackService.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

namespace {

std::atomic<bool> gStopRequested{false};

void HandleSignal(int) {
    gStopRequested.store(true, std::memory_order_release);
}

void PrintPlaybackStatistics(const audio_service::PlaybackStatistics& statistics) {
    std::cout << "播放结束，accepted: " << statistics.acceptedFrames
              << ", dropped: " << statistics.droppedFrames
              << ", output: " << statistics.outputFrames << '\n';
}

int PrintPlaybackDevices() {
    std::vector<audio_service::PlaybackDeviceInfo> devices;
    std::string error;
    if (!audio_service::AudioPlaybackService::EnumeratePlaybackDevices(devices, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    for (const auto& device : devices) {
        std::cout << device.id << "\n" << device.description << "\n\n";
    }
    return 0;
}

int RunPlayback(const alsa_playback::CommandLineOptions& options) {
    std::string error;
    auto playbackService = audio_service::CreateAudioPlaybackService();
    if (!playbackService->Open(options.serviceConfig, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    alsa_playback::MusicPlayer musicPlayer;
    if (!musicPlayer.Open(*playbackService, options.serviceConfig.outputFormat, options.musicFile,
                           options.pushTimeout, error)) {
        std::cerr << error << '\n';
        playbackService->Close();
        return 1;
    }
    alsa_playback::PromptPlayer promptPlayer;
    if (!promptPlayer.Open(*playbackService, options.serviceConfig.outputFormat, options.promptFile,
                            options.pushTimeout, error)) {
        std::cerr << error << '\n';
        std::string ignored;
        musicPlayer.Close(ignored);
        playbackService->Close();
        return 1;
    }
    if (!playbackService->Start(error)) {
        std::cerr << error << '\n';
        std::string ignored;
        musicPlayer.Close(ignored);
        promptPlayer.Close(ignored);
        playbackService->Close();
        return 1;
    }
    promptPlayer.Play(std::chrono::steady_clock::now() + options.promptDelay);
    musicPlayer.Play(gStopRequested, options.durationSeconds);
    std::string promptError;
    const bool played = musicPlayer.Close(error);
    promptPlayer.Close(promptError);
    if (!promptError.empty()) {
        std::cerr << promptError << '\n';
    }
    playbackService->Stop();
    playbackService->Close();
    const auto statistics = playbackService->Statistics();
    if (!played) {
        std::cerr << error << '\n';
        return 1;
    }
    PrintPlaybackStatistics(statistics);
    return 0;
}

}

int main(int argc, char** argv) {
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    alsa_playback::CommandLineOptions options;
    std::string error;
    if (!alsa_playback::ParseCommandLine(argc, argv, options, error)) {
        std::cerr << error << '\n';
        alsa_playback::PrintUsage(argv[0]);
        return 1;
    }
    if (options.listDevices) {
        return PrintPlaybackDevices();
    }
    return RunPlayback(options);
}
