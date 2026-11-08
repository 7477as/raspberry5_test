#include "PlaybackSession.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace alsa_playback::tests {

namespace {

PlaybackSessionConfig MakeConfig() {
    PlaybackSessionConfig config{};
    config.deviceHint = "null";
    config.outputFormat.sampleRate = 48000;
    config.outputFormat.channels = 2;
    return config;
}

void Check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

}

void RunDeviceOpenOnDemandTest() {
    PlaybackSession first;
    PlaybackSession second;
    std::string error;
    Check(first.Open(MakeConfig(), error), "首个会话打开失败");
    Check(first.IsDeviceOpen(), "首个会话应打开声卡");
    PlaybackSessionConfig config = MakeConfig();
    config.session.name = "prompt";
    Check(second.Open(config, error), "第二个会话打开失败");
    Check(first.IsDeviceOpen(), "双会话期间声卡应保持打开");
    Check(first.Close(error), "销毁首个会话失败");
    Check(second.IsDeviceOpen(), "仍有会话时不应关闭声卡");
    Check(second.Close(error), "销毁末个会话失败");
    Check(!first.IsDeviceOpen() && !second.IsDeviceOpen(), "末会话关闭后声卡应释放");
    std::cout << "device open on demand passed\n";
}

}
