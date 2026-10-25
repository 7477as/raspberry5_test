// gst_playback - 音频播放示例：播放文件或生成测试音
//
// 文件模式：uridecodebin(uri) ! audioconvert ! audioresample ! volume ! sink
// 测试音：  audiotestsrc ! audioconvert ! audioresample ! volume ! sink
// sink 可选 ALSA 直连（alsasink device=...）或 PulseAudio（pulsesink，跟随 wpctl 默认设备）。

#include <gst/gst.h>

#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

constexpr const char* kDefaultDevice = "hw:0,0";
constexpr double kDefaultToneFrequency = 440.0;
constexpr int kBusPollTimeoutMs = 200;

volatile std::sig_atomic_t gStopRequested = 0;

enum class SinkType { Alsa, Pulse };

struct Options {
    std::string filePath;
    double toneFrequency = kDefaultToneFrequency;
    bool useTone = false;
    SinkType sinkType = SinkType::Alsa;
    std::string device = kDefaultDevice;
    double volume = 1.0;
};

void HandleSignal(int) {
    gStopRequested = 1;
}

void PrintUsage(const char* programName) {
    std::cout
        << "用法: " << programName << " [选项]\n\n"
        << "播放音频文件，或生成测试音验证扬声器通路。\n"
        << "不指定 --file 时默认播放 " << kDefaultToneFrequency << " Hz 测试音。\n"
        << "按 Ctrl+C 结束播放。\n\n"
        << "输入:\n"
        << "  --file <path>          要播放的音频文件（wav / mp3 / flac / ogg 等）\n"
        << "  --tone <hz>            播放测试音频率，默认 " << kDefaultToneFrequency << "\n"
        << "\n输出:\n"
        << "  --sink alsa|pulse      输出后端，默认 alsa\n"
        << "  --device <dev>         ALSA 设备（--sink alsa）或 PulseAudio sink 名（--sink pulse）\n"
        << "                        ALSA 默认 " << kDefaultDevice
        << "；Pulse 留空则跟随 wpctl 默认设备\n"
        << "  --volume <0.0-1.0>     音量，默认 1.0\n"
        << "  -h, --help             显示本帮助\n";
}

bool ParseVolume(const std::string& value, double& out) {
    try {
        out = std::stod(value);
    } catch (const std::exception&) {
        return false;
    }
    return out >= 0.0 && out <= 1.0;
}

bool ParseFrequency(const std::string& value, double& out) {
    try {
        out = std::stod(value);
    } catch (const std::exception&) {
        return false;
    }
    return out > 0.0 && out < 20000.0;
}

bool ParseOptions(int argc, char** argv, Options& options, std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string flag = argv[index];
        const bool hasValue = index + 1 < argc;

        if (flag == "-h" || flag == "--help") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (flag == "--file" && hasValue) {
            options.filePath = argv[++index];
            options.useTone = false;
        } else if (flag == "--tone" && hasValue) {
            if (!ParseFrequency(argv[++index], options.toneFrequency)) {
                error = "--tone 需要 0 ~ 20000 之间的频率";
                return false;
            }
            options.useTone = true;
        } else if (flag == "--sink" && hasValue) {
            const std::string sink = argv[++index];
            if (sink == "alsa") {
                options.sinkType = SinkType::Alsa;
            } else if (sink == "pulse") {
                options.sinkType = SinkType::Pulse;
            } else {
                error = "--sink 只能是 alsa 或 pulse";
                return false;
            }
        } else if (flag == "--device" && hasValue) {
            options.device = argv[++index];
        } else if (flag == "--volume" && hasValue) {
            if (!ParseVolume(argv[++index], options.volume)) {
                error = "--volume 需要 0.0 ~ 1.0 之间的数值";
                return false;
            }
        } else {
            error = "无法识别的参数: " + flag + "（用 --help 查看用法）";
            return false;
        }
    }
    return true;
}

std::string EscapeForParseLaunch(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size() + 8);
    for (const char character : text) {
        if (character == '"' || character == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(character);
    }
    return escaped;
}

std::string BuildPipelineDescription(const Options& options, const std::string& uri) {
    std::ostringstream description;
    if (options.useTone) {
        description << "audiotestsrc is-live=true freq=" << options.toneFrequency;
    } else {
        description << "uridecodebin uri=\"" << EscapeForParseLaunch(uri) << "\"";
    }
    description << " ! audioconvert ! audioresample ! volume name=vol volume="
               << options.volume << " ! ";

    if (options.sinkType == SinkType::Pulse) {
        description << "pulsesink";
        if (!options.device.empty()) {
            description << " device=\"" << EscapeForParseLaunch(options.device) << "\"";
        }
    } else {
        description << "alsasink device=\"" << EscapeForParseLaunch(options.device) << "\"";
    }
    description << " sync=true";
    return description.str();
}

std::string FormatBusError(GstMessage* message) {
    GError* gstError = nullptr;
    gchar* debugInfo = nullptr;
    gst_message_parse_error(message, &gstError, &debugInfo);
    std::string text = gstError != nullptr ? gstError->message : "未知错误";
    if (gstError != nullptr) {
        g_error_free(gstError);
    }
    g_free(debugInfo);
    return text;
}

std::string BuildErrorHint(const std::string& message) {
    if (message.find("busy") != std::string::npos ||
        message.find("Resource busy") != std::string::npos) {
        return "设备被占用：先停掉正在使用声卡的其他程序（如 gst-launch-1.0、aplay、浏览器）再试";
    }
    if (message.find("No such file or directory") != std::string::npos ||
        message.find("Could not open audio device") != std::string::npos) {
        return "设备不存在或无法打开：用 aplay -l / arecord -l 确认设备号，"
               "或改用 --device plughw:0,0";
    }
    if (message.find("not found") != std::string::npos ||
        message.find("No such file") != std::string::npos) {
        return "找不到媒体文件或解码器：用 gst-inspect-1.0 检查编解码插件是否安装";
    }
    return "排查思路：aplay -l 确认设备、wpctl status 确认 PulseAudio 状态";
}

int RunPlayback(const Options& options) {
    std::string uri;
    if (!options.useTone) {
        if (options.filePath.empty()) {
            std::cerr << "没有指定 --file，也没有启用 --tone\n";
            return 1;
        }
        const std::filesystem::path filePath(options.filePath);
        if (!std::filesystem::is_regular_file(filePath)) {
            std::cerr << "找不到文件: " << options.filePath << "\n";
            return 1;
        }
        gchar* fileUri = gst_filename_to_uri(filePath.string().c_str(), nullptr);
        if (fileUri == nullptr) {
            std::cerr << "无法构造文件 URI: " << options.filePath << "\n";
            return 1;
        }
        uri = fileUri;
        g_free(fileUri);
    }

    const std::string description = BuildPipelineDescription(options, uri);

    GError* parseError = nullptr;
    GstElement* pipeline = gst_parse_launch(description.c_str(), &parseError);
    if (pipeline == nullptr) {
        std::cerr << "解析管线失败: "
                  << (parseError != nullptr ? parseError->message : "未知错误") << "\n";
        if (parseError != nullptr) {
            g_error_free(parseError);
        }
        return 1;
    }

    std::cout << "播放内容      : "
              << (options.useTone ? std::to_string(options.toneFrequency) + " Hz 测试音"
                                  : options.filePath)
              << "\n"
              << "输出后端      : "
              << (options.sinkType == SinkType::Pulse ? "PulseAudio" : "ALSA") << "\n";
    if (options.sinkType == SinkType::Pulse) {
        std::cout << "Pulse sink     : "
                  << (options.device.empty() ? "默认设备（见 wpctl status）" : options.device) << "\n";
    } else {
        std::cout << "ALSA 设备      : " << options.device << "\n";
    }
    std::cout << "音量          : " << std::fixed << std::setprecision(2) << options.volume << "\n\n"
              << "开始播放（Ctrl+C 结束）...\n"
              << std::flush;

    GstBus* bus = gst_element_get_bus(pipeline);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    bool failed = false;
    while (true) {
        GstMessage* message = gst_bus_timed_pop_filtered(
            bus, static_cast<guint64>(kBusPollTimeoutMs) * GST_MSECOND,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (message == nullptr) {
            if (gStopRequested) {
                std::cout << "\n收到 Ctrl+C，停止播放。\n";
                break;
            }
            continue;
        }

        const GstMessageType messageType = GST_MESSAGE_TYPE(message);
        if (messageType == GST_MESSAGE_ERROR) {
            const std::string text = FormatBusError(message);
            std::cerr << "播放失败: " << text << "\n";
            std::cerr << "提示: " << BuildErrorHint(text) << "\n";
            failed = true;
        } else {
            std::cout << "\n播放结束。\n";
        }
        gst_message_unref(message);
        break;
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);

    return failed ? 1 : 0;
}

}  // namespace

int main(int argc, char** argv) {
    gst_init(&argc, &argv);
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    Options options;
    std::string error;
    if (!ParseOptions(argc, argv, options, error)) {
        std::cerr << "参数错误: " << error << "\n";
        return 1;
    }

    return RunPlayback(options);
}
