// gst_loopback - 麦克风回声回路示例：双麦采集的声音直接送回扬声器
//
// 双麦管线：alsasrc ! audioconvert ! audioresample ! alsasink
// 单麦管线：alsasrc ! audioconvert ! deinterleave，选定声道后 interleave 回立体声输出

#include <gst/gst.h>

#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

constexpr const char* kDefaultDevice = "hw:0,0";
constexpr int kBusPollTimeoutMs = 200;

volatile std::sig_atomic_t gStopRequested = 0;

enum class RouteMode {
    Both,
    Left,
    Right,
};

struct Options {
    RouteMode mode = RouteMode::Both;
    std::string device = kDefaultDevice;
    double durationSeconds = 0.0;
};

void HandleSignal(int) {
    gStopRequested = 1;
}

void PrintUsage(const char* programName) {
    std::cout
        << "用法: " << programName << " [选项]\n\n"
        << "麦克风回声回路：采集双麦声音并直接送到扬声器。\n"
        << "按 Ctrl+C 结束。\n\n"
        << "路由:\n"
        << "  --mode both|left|right  送回哪一路麦克风，默认 both\n"
        << "                          both = MIC0+MIC1，left = 只 MIC0，right = 只 MIC1\n"
        << "\n输入与设备:\n"
        << "  --device <dev>          ALSA 设备（采集与播放共用），默认 " << kDefaultDevice << "\n"
        << "  --duration <sec>        运行时长秒数，0 或省略表示一直运行到 Ctrl+C\n"
        << "  -h, --help              显示本帮助\n";
}

bool ParseNonNegativeDouble(const std::string& value, double& out) {
    try {
        std::size_t parsedLength = 0;
        out = std::stod(value, &parsedLength);
        return parsedLength == value.size() && out >= 0.0;
    } catch (const std::exception&) {
        return false;
    }
}

bool ParseOptions(int argc, char** argv, Options& options, std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string flag = argv[index];
        const bool hasValue = index + 1 < argc;

        if (flag == "-h" || flag == "--help") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (flag == "--mode" && hasValue) {
            const std::string mode = argv[++index];
            if (mode == "both") {
                options.mode = RouteMode::Both;
            } else if (mode == "left") {
                options.mode = RouteMode::Left;
            } else if (mode == "right") {
                options.mode = RouteMode::Right;
            } else {
                error = "--mode 只能是 both、left 或 right";
                return false;
            }
        } else if (flag == "--device" && hasValue) {
            options.device = argv[++index];
        } else if (flag == "--duration" && hasValue) {
            if (!ParseNonNegativeDouble(argv[++index], options.durationSeconds)) {
                error = "--duration 需要非负数";
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

std::string ModeName(RouteMode mode) {
    switch (mode) {
        case RouteMode::Both:
            return "both（MIC0 + MIC1）";
        case RouteMode::Left:
            return "left（仅 MIC0）";
        case RouteMode::Right:
            return "right（仅 MIC1）";
    }
    return "unknown";
}

std::string BuildPipelineDescription(const Options& options) {
    const std::string device = EscapeForParseLaunch(options.device);
    std::ostringstream description;
    description << "alsasrc name=src device=\"" << device << "\" ! audioconvert ! ";
    if (options.mode == RouteMode::Both) {
        description << "audioresample ! alsasink device=\"" << device << "\" sync=true";
        return description.str();
    }

    const int channel = options.mode == RouteMode::Left ? 0 : 1;
    description << "deinterleave name=d interleave name=i ! audioconvert ! "
                << "alsasink device=\"" << device << "\" sync=true "
                << "d.src_" << channel << " ! queue ! i.sink_0";
    return description.str();
}

std::string FormatBusError(GstMessage* message) {
    GError* gstError = nullptr;
    gchar* debugInfo = nullptr;
    gst_message_parse_error(message, &gstError, &debugInfo);
    std::ostringstream text;
    text << (gstError != nullptr ? gstError->message : "未知错误");
    GstObject* source = GST_MESSAGE_SRC(message);
    if (source != nullptr) {
        text << "\n出错元素: " << GST_OBJECT_NAME(source);
    }
    if (debugInfo != nullptr && debugInfo[0] != '\0') {
        text << "\nGStreamer 调试信息: " << debugInfo;
    }
    if (gstError != nullptr) {
        g_error_free(gstError);
    }
    g_free(debugInfo);
    return text.str();
}

int RunLoopback(const Options& options) {
    const std::string description = BuildPipelineDescription(options);

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

    std::cout << "ALSA 设备      : " << options.device << "（采集与播放共用）\n"
              << "采样格式      : ALSA 自动协商\n"
              << "回声路由      : " << ModeName(options.mode) << "\n"
              << "运行时长      : "
              << (options.durationSeconds > 0.0 ? std::to_string(options.durationSeconds) + " s"
                                                : std::string("手动 Ctrl+C 结束"))
              << "\n\n注意：扬声器声音会被麦克风重新采集，注意啸叫。\n"
              << "开始回环（Ctrl+C 结束）...\n"
              << std::flush;

    GstBus* bus = gst_element_get_bus(pipeline);
    const gint64 startUs = g_get_monotonic_time();
    const gint64 deadlineUs =
        options.durationSeconds > 0.0
            ? startUs + static_cast<gint64>(options.durationSeconds * 1'000'000.0)
            : 0;

    const GstStateChangeReturn stateResult = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (stateResult == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "启动管线失败：无法进入 PLAYING 状态\n";
    }

    bool failed = stateResult == GST_STATE_CHANGE_FAILURE;
    while (true) {
        GstMessage* message = gst_bus_timed_pop_filtered(
            bus, static_cast<guint64>(kBusPollTimeoutMs) * GST_MSECOND,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (message != nullptr) {
            const GstMessageType messageType = GST_MESSAGE_TYPE(message);
            if (messageType == GST_MESSAGE_ERROR) {
                std::cerr << "管线报错: " << FormatBusError(message) << "\n";
                failed = true;
            }
            gst_message_unref(message);
            break;
        }

        if (gStopRequested) {
            std::cout << "\n收到 Ctrl+C，停止回环。\n";
            break;
        }
        if (deadlineUs > 0 && g_get_monotonic_time() >= deadlineUs) {
            std::cout << "\n达到设定时长，停止回环。\n";
            break;
        }
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);

    if (failed) {
        return 1;
    }

    const double elapsedSeconds =
        static_cast<double>(g_get_monotonic_time() - startUs) / 1'000'000.0;
    std::cout << "回环结束，运行时长约 " << std::fixed << std::setprecision(1) << elapsedSeconds
              << " s\n";
    return 0;
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

    return RunLoopback(options);
}
