// gst_capture - 双麦录音示例：把两路麦克风录成双通道 WAV
//
// 管线：alsasrc ! audioconvert ! audioresample ! capsfilter(S16LE/2ch) ! wavenc ! filesink
// --split 时额外用 tee + deinterleave 拆出两个单通道 WAV。
// 停止时必须发 EOS 让 wavenc 回填 WAV 头，否则文件头不完整。

#include <gst/gst.h>

#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kChannels = 2;
constexpr int kDefaultRate = 16000;
constexpr const char* kDefaultDevice = "hw:0,0";
constexpr int kBusPollTimeoutMs = 200;
constexpr int kLevelPrintIntervalMs = 500;
constexpr int kEosWaitTimeoutMs = 3000;
constexpr double kMinDb = -100.0;

volatile std::sig_atomic_t gStopRequested = 0;

struct Options {
    std::string device = kDefaultDevice;
    int rate = kDefaultRate;
    double durationSeconds = 0.0;
    std::string outputPath = "output/capture.wav";
    bool splitChannels = false;
};

struct LevelStats {
    std::mutex mutex;
    std::vector<double> peak;
    std::vector<double> squareSum;
    std::vector<uint64_t> sampleCount;
    std::vector<uint64_t> fullScaleCount;
};

void HandleSignal(int) {
    gStopRequested = 1;
}

void PrintUsage(const char* programName) {
    std::cout
        << "用法: " << programName << " [选项]\n\n"
        << "双麦录音：两路麦克风录成双通道 WAV（MIC0/MIC1 交织）。\n"
        << "按 Ctrl+C 结束，程序会补发 EOS 保证 WAV 头完整。\n\n"
        << "输入与设备:\n"
        << "  --device <dev>      ALSA 采集设备，默认 " << kDefaultDevice << "\n"
        << "  --rate <hz>         采样率，默认 " << kDefaultRate
        << "（hw: 设备只支持固定格式，换采样率请用 plughw:）\n"
        << "  --duration <sec>    录制时长秒数，0 或省略表示一直录到 Ctrl+C\n"
        << "\n输出:\n"
        << "  --out <path>         双通道 WAV 路径，默认 output/capture.wav\n"
        << "  --split              额外写出 mic0.wav / mic1.wav 两个单通道文件\n"
        << "  -h, --help           显示本帮助\n";
}

bool ParsePositiveInt(const std::string& value, int& out) {
    try {
        out = std::stoi(value);
    } catch (const std::exception&) {
        return false;
    }
    return out > 0;
}

bool ParseNonNegativeDouble(const std::string& value, double& out) {
    try {
        out = std::stod(value);
    } catch (const std::exception&) {
        return false;
    }
    return out >= 0.0;
}

bool ParseOptions(int argc, char** argv, Options& options, std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string flag = argv[index];
        const bool hasValue = index + 1 < argc;

        if (flag == "-h" || flag == "--help") {
            PrintUsage(argv[0]);
            std::exit(0);
        } else if (flag == "--split") {
            options.splitChannels = true;
        } else if (flag == "--device" && hasValue) {
            options.device = argv[++index];
        } else if (flag == "--out" && hasValue) {
            options.outputPath = argv[++index];
        } else if (flag == "--rate" && hasValue) {
            if (!ParsePositiveInt(argv[++index], options.rate)) {
                error = "--rate 需要正整数";
                return false;
            }
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

std::string FormatDb(double amplitude) {
    if (amplitude <= 0.0) {
        return std::to_string(static_cast<int>(kMinDb)) + " dBFS";
    }
    const double db = 20.0 * std::log10(amplitude);
    std::ostringstream text;
    text << std::fixed << std::setprecision(1) << std::max(db, kMinDb) << " dBFS";
    return text.str();
}

std::string BuildPipelineDescription(const Options& options, const std::string& mic0Path,
                                     const std::string& mic1Path) {
    const std::string caps =
        "audio/x-raw,format=S16LE,rate=" + std::to_string(options.rate) +
        ",channels=" + std::to_string(kChannels) + ",layout=interleaved";

    std::ostringstream description;
    description << "alsasrc name=src device=\"" << EscapeForParseLaunch(options.device) << "\" ! "
                << "audioconvert ! audioresample ! capsfilter name=cap caps=\"" << caps << "\" ! ";

    if (options.splitChannels) {
        description << "tee name=t t. ! queue ! wavenc ! filesink location=\""
                    << EscapeForParseLaunch(options.outputPath) << "\" "
                    << "t. ! queue ! deinterleave name=d "
                    << "d.src_0 ! queue ! audioconvert ! wavenc ! filesink location=\""
                    << EscapeForParseLaunch(mic0Path) << "\" "
                    << "d.src_1 ! queue ! audioconvert ! wavenc ! filesink location=\""
                    << EscapeForParseLaunch(mic1Path) << "\"";
    } else {
        description << "wavenc ! filesink location=\"" << EscapeForParseLaunch(options.outputPath)
                    << "\"";
    }
    return description.str();
}

GstPadProbeReturn OnLevelProbe(GstPad*, GstPadProbeInfo* info, gpointer userData) {
    auto* stats = static_cast<LevelStats*>(userData);
    GstBuffer* buffer = gst_pad_probe_info_get_buffer(info);
    if (stats == nullptr || buffer == nullptr) {
        return GST_PAD_PROBE_OK;
    }

    GstMapInfo mapInfo;
    if (!gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
        return GST_PAD_PROBE_OK;
    }

    const int16_t* rawSamples = reinterpret_cast<const int16_t*>(mapInfo.data);
    const size_t sampleCount = mapInfo.size / sizeof(int16_t);
    const size_t frameCount = sampleCount / kChannels;

    {
        std::lock_guard<std::mutex> guard(stats->mutex);
        for (size_t frame = 0; frame < frameCount; ++frame) {
            for (int channel = 0; channel < kChannels; ++channel) {
                const int16_t sample = rawSamples[frame * kChannels + channel];
                const double amplitude =
                    std::fabs(static_cast<double>(sample) / 32768.0);
                stats->peak[static_cast<size_t>(channel)] =
                    std::max(stats->peak[static_cast<size_t>(channel)], amplitude);
                stats->squareSum[static_cast<size_t>(channel)] += amplitude * amplitude;
                stats->sampleCount[static_cast<size_t>(channel)] += 1;
                if (sample == INT16_MIN || sample == INT16_MAX) {
                    stats->fullScaleCount[static_cast<size_t>(channel)] += 1;
                }
            }
        }
    }

    gst_buffer_unmap(buffer, &mapInfo);
    return GST_PAD_PROBE_OK;
}

void PrintLevels(LevelStats& stats) {
    std::vector<double> peak;
    std::vector<double> rms;
    std::vector<uint64_t> fullScaleCount;
    {
        std::lock_guard<std::mutex> guard(stats.mutex);
        peak = stats.peak;
        fullScaleCount = stats.fullScaleCount;
        rms.resize(kChannels);
        for (int channel = 0; channel < kChannels; ++channel) {
            const uint64_t count = stats.sampleCount[static_cast<size_t>(channel)];
            rms[static_cast<size_t>(channel)] =
                count > 0 ? std::sqrt(stats.squareSum[static_cast<size_t>(channel)] /
                                      static_cast<double>(count))
                           : 0.0;
        }
        std::fill(stats.peak.begin(), stats.peak.end(), 0.0);
        std::fill(stats.squareSum.begin(), stats.squareSum.end(), 0.0);
        std::fill(stats.sampleCount.begin(), stats.sampleCount.end(), 0);
        std::fill(stats.fullScaleCount.begin(), stats.fullScaleCount.end(), 0);
    }

    std::cout << "  MIC0 peak " << FormatDb(peak[0]) << " rms " << FormatDb(rms[0])
              << " full-scale " << fullScaleCount[0] << " samples"
              << " | MIC1 peak " << FormatDb(peak[1]) << " rms " << FormatDb(rms[1])
              << " full-scale " << fullScaleCount[1] << " samples\n"
              << std::flush;
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

int RunCapture(const Options& options) {
    const std::filesystem::path outputPath(options.outputPath);
    const std::filesystem::path outputDir =
        outputPath.has_parent_path() ? outputPath.parent_path() : std::filesystem::path(".");

    std::error_code filesystemError;
    std::filesystem::create_directories(outputDir, filesystemError);
    if (filesystemError) {
        std::cerr << "创建输出目录失败: " << filesystemError.message() << "\n";
        return 1;
    }

    const std::string mic0Path = (outputDir / "mic0.wav").string();
    const std::string mic1Path = (outputDir / "mic1.wav").string();

    const std::string description = BuildPipelineDescription(options, mic0Path, mic1Path);

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

    LevelStats stats;
    stats.peak.assign(kChannels, 0.0);
    stats.squareSum.assign(kChannels, 0.0);
    stats.sampleCount.assign(kChannels, 0);
    stats.fullScaleCount.assign(kChannels, 0);

    GstElement* capsFilter = gst_bin_get_by_name(GST_BIN(pipeline), "cap");
    if (capsFilter == nullptr) {
        std::cerr << "管线缺少 capsfilter 元素\n";
        gst_object_unref(pipeline);
        return 1;
    }

    GstPad* levelPad = gst_element_get_static_pad(capsFilter, "src");
    gst_pad_add_probe(levelPad, GST_PAD_PROBE_TYPE_BUFFER, &OnLevelProbe, &stats, nullptr);
    gst_object_unref(levelPad);
    gst_object_unref(capsFilter);

    std::cout << "采集设备      : " << options.device << "\n"
              << "采样率        : " << options.rate << " Hz（双通道）\n"
              << "输出文件      : " << outputPath.string() << "\n";
    if (options.splitChannels) {
        std::cout << "拆分文件      : " << mic0Path << " / " << mic1Path << "\n";
    }
    std::cout << "录制时长      : "
              << (options.durationSeconds > 0.0 ? std::to_string(options.durationSeconds) + " s"
                                                : std::string("手动 Ctrl+C 结束"))
              << "\n\n开始录音...\n"
              << std::flush;

    GstBus* bus = gst_element_get_bus(pipeline);
    const gint64 startUs = g_get_monotonic_time();
    const gint64 deadlineUs =
        options.durationSeconds > 0.0
            ? startUs + static_cast<gint64>(options.durationSeconds * 1'000'000.0)
            : 0;

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    bool failed = false;
    bool reachedEnd = false;
    gint64 nextLevelUs = startUs + kLevelPrintIntervalMs * 1000;

    while (true) {
        GstMessage* message = gst_bus_timed_pop_filtered(
            bus, static_cast<guint64>(kBusPollTimeoutMs) * GST_MSECOND,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (message != nullptr) {
            const GstMessageType messageType = GST_MESSAGE_TYPE(message);
            if (messageType == GST_MESSAGE_ERROR) {
                std::cerr << "管线报错: " << FormatBusError(message) << "\n";
                failed = true;
            } else {
                reachedEnd = true;
            }
            gst_message_unref(message);
            if (failed || reachedEnd) {
                break;
            }
        }

        if (gStopRequested) {
            std::cout << "\n收到 Ctrl+C，正在收尾...\n";
            break;
        }
        if (deadlineUs > 0 && g_get_monotonic_time() >= deadlineUs) {
            break;
        }
        if (g_get_monotonic_time() >= nextLevelUs) {
            PrintLevels(stats);
            nextLevelUs += kLevelPrintIntervalMs * 1000;
        }
    }

    gint64 captureEndUs = g_get_monotonic_time();
    if (!failed) {
        gst_element_send_event(pipeline, gst_event_new_eos());
        const gint64 eosDeadlineUs = g_get_monotonic_time() + kEosWaitTimeoutMs * 1000;
        while (g_get_monotonic_time() < eosDeadlineUs) {
            GstMessage* message = gst_bus_timed_pop_filtered(
                bus, static_cast<guint64>(kBusPollTimeoutMs) * GST_MSECOND,
                static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
            if (message == nullptr) {
                continue;
            }
            const GstMessageType messageType = GST_MESSAGE_TYPE(message);
            if (messageType == GST_MESSAGE_ERROR) {
                std::cerr << "收尾时管线报错: " << FormatBusError(message) << "\n";
                failed = true;
            }
            gst_message_unref(message);
            break;
        }
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);

    if (failed) {
        return 1;
    }

    const double recordedSeconds =
        static_cast<double>(captureEndUs - startUs) / 1'000'000.0;
    std::cout << "\n录音完成，时长约 " << std::fixed << std::setprecision(1) << recordedSeconds
              << " s\n"
              << "输出文件      : " << outputPath.string() << "\n";
    if (options.splitChannels) {
        std::cout << "拆分文件      : " << mic0Path << "\n"
                  << "                " << mic1Path << "\n";
    }
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

    return RunCapture(options);
}
