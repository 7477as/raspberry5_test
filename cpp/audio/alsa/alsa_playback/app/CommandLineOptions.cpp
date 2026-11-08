#include "app/CommandLineOptions.h"

#include <iostream>

namespace alsa_playback {
namespace {

bool ParseUnsigned(const std::string& text,
                   std::uint32_t& value,
                   std::uint32_t maximum,
                   bool allowZero = false) {
    try {
        std::size_t parsed = 0;
        const unsigned long number = std::stoul(text, &parsed);
        if (parsed != text.size() || (!allowZero && number == 0) || number > maximum) {
            return false;
        }
        value = static_cast<std::uint32_t>(number);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace

void PrintUsage(const char* programName) {
    std::cout << "用法: " << programName << " [选项]\n"
              << "  --list-devices             枚举播放设备并退出\n"
              << "  --device <hint>            输出设备，默认 plughw:3,0\n"
              << "  --rate <hz>                输出采样率，默认 48000\n"
              << "  --channels <count>         输出声道数，默认 2\n"
              << "  --music-file <path>        音乐 WAV 文件\n"
              << "  --prompt-file <path>       提示音 WAV 文件\n"
              << "  --prompt-delay <seconds>   提示音延迟播放秒数，默认 5\n"
              << "  --push-timeout-ms <ms>     队列等待超时，默认 100\n"
              << "  --duration <seconds>       可选播放时长上限\n"
              << "  -h, --help                 显示帮助\n";
}

bool ParseCommandLine(int argc, char** argv, CommandLineOptions& options, std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string flag = argv[index];
        if (flag == "-h" || flag == "--help") {
            PrintUsage(argv[0]);
            std::exit(0);
        }
        if (flag == "--list-devices") {
            options.listDevices = true;
            continue;
        }
        if (index + 1 >= argc) {
            error = "参数缺少值: " + flag;
            return false;
        }
        const std::string value = argv[++index];
        std::uint32_t number = 0;
        if (flag == "--device") {
            options.serviceConfig.deviceHint = value;
        } else if (flag == "--music-file") {
            options.musicFile = value;
        } else if (flag == "--rate" && ParseUnsigned(value, number, 192000)) {
            options.serviceConfig.outputFormat.sampleRate = number;
        } else if (flag == "--channels" && ParseUnsigned(value, number, 32)) {
            options.serviceConfig.outputFormat.channels = number;
        } else if (flag == "--duration" && ParseUnsigned(value, number, 86400, true)) {
            options.durationSeconds = number;
        } else if (flag == "--prompt-delay" && ParseUnsigned(value, number, 3600, true)) {
            options.promptDelay = std::chrono::seconds(number);
        } else if (flag == "--prompt-file") {
            options.promptFile = value;
        } else if (flag == "--push-timeout-ms" && ParseUnsigned(value, number, 60000)) {
            options.pushTimeout = std::chrono::milliseconds(number);
        } else {
            error = "无法识别或参数无效: " + flag;
            return false;
        }
    }
    return true;
}

}
