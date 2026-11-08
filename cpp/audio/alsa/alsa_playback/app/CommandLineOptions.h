#pragma once

#include "PlaybackSession.h"

#include <chrono>
#include <cstdint>
#include <string>

namespace alsa_playback {

struct CommandLineOptions {
    PlaybackSessionConfig sessionConfig{};
    std::string musicFile = "/home/moyuping/work/audio/testAudio/test.wav";
    std::string promptFile = "/home/moyuping/work/audio/testAudio/sms_alert_hello.wav";
    std::chrono::seconds promptDelay{5};
    std::chrono::milliseconds pushTimeout{100};
    std::uint32_t durationSeconds = 0;
    bool listDevices = false;
};

bool ParseCommandLine(int argc, char** argv, CommandLineOptions& options, std::string& error);
void PrintUsage(const char* programName);

}  // namespace alsa_playback
