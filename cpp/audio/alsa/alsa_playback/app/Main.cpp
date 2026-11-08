#include "app/CommandLineOptions.h"
#include "app/MusicPlayer.h"
#include "app/PromptPlayer.h"
#include "PlaybackSession.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

namespace {

std::atomic<bool> gStopRequested{false};

void HandleSignal(int) {
    gStopRequested.store(true, std::memory_order_release);
}

int PrintPlaybackDevices() {
    std::vector<alsa_playback::AlsaDeviceInfo> devices;
    std::string error;
    if (!alsa_playback::PlaybackSession::EnumeratePlaybackDevices(devices, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    for (const auto& device : devices) {
        std::cout << device.name << "\n" << device.description << "\n\n";
    }
    return 0;
}

int RunPlayback(const alsa_playback::CommandLineOptions& options) {
    alsa_playback::MusicPlayer musicPlayer;
    std::string error;
    if (!musicPlayer.Open(options.sessionConfig, options.musicFile, options.pushTimeout, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    alsa_playback::PromptPlayer promptPlayer;
    if (!promptPlayer.Open(options.sessionConfig, options.promptFile, options.pushTimeout, error)) {
        std::cerr << error << '\n';
        std::string ignored;
        musicPlayer.Close(ignored);
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
    if (!played) {
        std::cerr << error << '\n';
        return 1;
    }
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
