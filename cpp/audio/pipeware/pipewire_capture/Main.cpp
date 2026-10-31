// pipewire_capture - 使用 PipeWire 采集 PCM 音频并保存为 WAV。

#include <pipewire/pipewire.h>
#include <spa/param/audio/raw-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/pod/builder.h>
#include <spa/utils/result.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

constexpr uint32_t kDefaultRate = 48000;
constexpr uint32_t kDefaultChannels = 2;
constexpr uint64_t kMaximumWavDataBytes = UINT32_MAX - 36ULL;

volatile std::sig_atomic_t gStopRequested = 0;

struct Options {
    uint32_t rate = kDefaultRate;
    uint32_t channels = kDefaultChannels;
    double duration = 0.0;
    std::string outputPath = "capture.wav";
    std::string sourceName;
};

struct CaptureState {
    pw_main_loop* mainLoop = nullptr;
    pw_stream* stream = nullptr;
    FILE* output = nullptr;
    Options options;
    uint64_t dataBytes = 0;
    uint64_t frameCount = 0;
    uint16_t peakSample = 0;
    uint16_t totalPeakSample = 0;
    std::chrono::steady_clock::time_point lastLevelReport = std::chrono::steady_clock::now();
    bool streamStarted = false;
    bool streamFailed = false;
};

void HandleSignal(int) {
    gStopRequested = 1;
}

bool ParseUnsigned(const std::string& value, uint32_t& output) {
    try {
        std::size_t parsed = 0;
        const unsigned long parsedValue = std::stoul(value, &parsed);
        if (parsed != value.size() || parsedValue == 0 || parsedValue > UINT32_MAX) {
            return false;
        }
        output = static_cast<uint32_t>(parsedValue);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool ParseDuration(const std::string& value, double& output) {
    try {
        std::size_t parsed = 0;
        output = std::stod(value, &parsed);
        return parsed == value.size() && output >= 0.0 && output <= 86400.0;
    } catch (const std::exception&) {
        return false;
    }
}

void PrintUsage(const char* programName) {
    std::cout << "用法: " << programName << " [选项]\n\n"
              << "通过 PipeWire 默认输入设备采集 PCM 并保存为 WAV。\n"
              << "  --rate <hz>          采样率，默认 48000\n"
              << "  --channels <count>   声道数，默认 2\n"
              << "  --duration <sec>     采集时长，0 表示持续到 Ctrl+C\n"
              << "  --out <path>         WAV 输出路径，默认 capture.wav\n"
              << "  --source <name>      指定 PipeWire source 的 node.name，默认系统选择\n"
              << "  -h, --help           显示帮助\n";
}

bool ParseOptions(int argc, char** argv, Options& options, std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string flag = argv[index];
        if (flag == "-h" || flag == "--help") {
            PrintUsage(argv[0]);
            std::exit(0);
        }
        if (index + 1 >= argc) {
            error = "参数缺少值: " + flag;
            return false;
        }
        const std::string value = argv[++index];
        if (flag == "--rate") {
            if (!ParseUnsigned(value, options.rate)) {
                error = "--rate 需要正整数";
                return false;
            }
        } else if (flag == "--channels") {
            if (!ParseUnsigned(value, options.channels) || options.channels > UINT16_MAX) {
                error = "--channels 需要 1 到 65535 之间的整数";
                return false;
            }
        } else if (flag == "--duration") {
            if (!ParseDuration(value, options.duration)) {
                error = "--duration 需要 0 到 86400 之间的数值";
                return false;
            }
        } else if (flag == "--out") {
            options.outputPath = value;
        } else if (flag == "--source") {
            options.sourceName = value;
        } else {
            error = "无法识别的参数: " + flag;
            return false;
        }
    }
    return true;
}

void WriteLittleEndian16(FILE* output, uint16_t value) {
    const uint8_t bytes[2] = {static_cast<uint8_t>(value & 0xff),
                              static_cast<uint8_t>((value >> 8) & 0xff)};
    std::fwrite(bytes, sizeof(bytes), 1, output);
}

void WriteLittleEndian32(FILE* output, uint32_t value) {
    const uint8_t bytes[4] = {static_cast<uint8_t>(value & 0xff),
                              static_cast<uint8_t>((value >> 8) & 0xff),
                              static_cast<uint8_t>((value >> 16) & 0xff),
                              static_cast<uint8_t>((value >> 24) & 0xff)};
    std::fwrite(bytes, sizeof(bytes), 1, output);
}

bool WriteWavHeader(FILE* output, const Options& options, uint32_t dataSize) {
    const uint16_t channels = static_cast<uint16_t>(options.channels);
    constexpr uint16_t bitsPerSample = 16;
    const uint32_t blockAlign = channels * bitsPerSample / 8;
    const uint32_t byteRate = options.rate * blockAlign;
    std::fwrite("RIFF", 4, 1, output);
    WriteLittleEndian32(output, 36U + dataSize);
    std::fwrite("WAVEfmt ", 8, 1, output);
    WriteLittleEndian32(output, 16);
    WriteLittleEndian16(output, 1);
    WriteLittleEndian16(output, channels);
    WriteLittleEndian32(output, options.rate);
    WriteLittleEndian32(output, byteRate);
    WriteLittleEndian16(output, static_cast<uint16_t>(blockAlign));
    WriteLittleEndian16(output, bitsPerSample);
    std::fwrite("data", 4, 1, output);
    WriteLittleEndian32(output, dataSize);
    return std::ferror(output) == 0;
}

bool UpdateWavHeader(CaptureState& state) {
    if (std::fseek(state.output, 0, SEEK_SET) != 0 ||
        !WriteWavHeader(state.output, state.options, static_cast<uint32_t>(state.dataBytes))) {
        return false;
    }
    return std::fflush(state.output) == 0;
}

void UpdatePeak(const uint8_t* bytes, uint32_t size, CaptureState& state) {
    const uint32_t sampleCount = size / sizeof(int16_t);
    for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        int16_t sample = 0;
        std::memcpy(&sample, bytes + sampleIndex * sizeof(sample), sizeof(sample));
        const uint16_t magnitude = sample == INT16_MIN
                                       ? static_cast<uint16_t>(INT16_MAX) + 1U
                                       : static_cast<uint16_t>(std::abs(sample));
        state.peakSample = std::max(state.peakSample, magnitude);
        state.totalPeakSample = std::max(state.totalPeakSample, magnitude);
    }
}

bool WritePcmData(FILE* output, const uint8_t* bytes, uint32_t size, uint64_t& dataBytes) {
    if (dataBytes > kMaximumWavDataBytes || size > kMaximumWavDataBytes - dataBytes ||
        std::fwrite(bytes, 1, size, output) != size) {
        return false;
    }
    dataBytes += size;
    return true;
}

void ProcessCapture(void* userData) {
    auto* state = static_cast<CaptureState*>(userData);
    pw_buffer* buffer = pw_stream_dequeue_buffer(state->stream);
    if (buffer == nullptr) {
        return;
    }
    spa_data* data = buffer->buffer->datas;
    if (buffer->buffer->n_datas == 0 || data == nullptr || data[0].data == nullptr ||
        data[0].chunk == nullptr) {
        state->streamFailed = true;
        pw_main_loop_quit(state->mainLoop);
    } else {
        const uint32_t offset = data[0].chunk->offset;
        const uint32_t size = data[0].chunk->size;
        const uint32_t capacity = data[0].maxsize;
        const auto* bytes = static_cast<const uint8_t*>(data[0].data);
        if (offset > capacity || size > capacity - offset) {
            state->streamFailed = true;
            pw_main_loop_quit(state->mainLoop);
        } else {
            const uint32_t firstSize = std::min(size, capacity - offset);
            const uint32_t secondSize = size - firstSize;
            const bool writeOk = WritePcmData(state->output, bytes + offset, firstSize,
                                               state->dataBytes) &&
                                 (secondSize == 0 || WritePcmData(state->output, bytes, secondSize,
                                                                  state->dataBytes));
            if (!writeOk) {
                state->streamFailed = true;
                pw_main_loop_quit(state->mainLoop);
            } else {
                UpdatePeak(bytes + offset, firstSize, *state);
                UpdatePeak(bytes, secondSize, *state);
                state->frameCount += size / (state->options.channels * sizeof(int16_t));
                const auto now = std::chrono::steady_clock::now();
                if (now - state->lastLevelReport >= std::chrono::seconds(1)) {
                    std::cout << "采集电平峰值=" << state->peakSample << " / 32768"
                              << "，累计 " << state->frameCount / state->options.rate << " 秒\n";
                    state->peakSample = 0;
                    state->lastLevelReport = now;
                }
            }
        }
    }
    pw_stream_queue_buffer(state->stream, buffer);
    if (gStopRequested ||
        (state->options.duration > 0.0 &&
         static_cast<double>(state->frameCount) / state->options.rate >= state->options.duration)) {
        pw_main_loop_quit(state->mainLoop);
    }
}

void OnStreamStateChanged(void* userData, pw_stream_state, pw_stream_state state,
                          const char* error) {
    auto* captureState = static_cast<CaptureState*>(userData);
    if (state == PW_STREAM_STATE_STREAMING) {
        captureState->streamStarted = true;
        std::cout << "采集流已连接并开始接收音频数据。\n";
    }
    if (state == PW_STREAM_STATE_ERROR || error != nullptr) {
        std::cerr << "采集流错误: " << (error == nullptr ? "未知错误" : error) << "\n";
        captureState->streamFailed = true;
        pw_main_loop_quit(captureState->mainLoop);
    }
}

int RunCapture(const Options& options) {
    const std::filesystem::path outputPath(options.outputPath);
    if (outputPath.has_parent_path()) {
        std::error_code error;
        std::filesystem::create_directories(outputPath.parent_path(), error);
        if (error) {
            std::cerr << "创建输出目录失败: " << error.message() << "\n";
            return 1;
        }
    }

    CaptureState state;
    state.options = options;
    state.output = std::fopen(options.outputPath.c_str(), "wb+");
    if (state.output == nullptr) {
        std::perror("打开 WAV 输出文件失败");
        return 1;
    }
    if (!WriteWavHeader(state.output, options, 0)) {
        std::cerr << "写入 WAV 头失败\n";
        std::fclose(state.output);
        return 1;
    }

    pw_init(nullptr, nullptr);
    state.mainLoop = pw_main_loop_new(nullptr);
    pw_loop* loop = state.mainLoop == nullptr ? nullptr : pw_main_loop_get_loop(state.mainLoop);
    pw_context* context = loop == nullptr ? nullptr : pw_context_new(loop, nullptr, 0);
    pw_core* core = context == nullptr ? nullptr : pw_context_connect(context, nullptr, 0);
    if (core == nullptr) {
        std::cerr << "无法连接 PipeWire 服务\n";
        if (context != nullptr) {
            pw_context_destroy(context);
        }
        if (state.mainLoop != nullptr) {
            pw_main_loop_destroy(state.mainLoop);
        }
        std::fclose(state.output);
        pw_deinit();
        return 1;
    }

    pw_stream_events streamEvents{};
    streamEvents.version = PW_VERSION_STREAM_EVENTS;
    streamEvents.state_changed = OnStreamStateChanged;
    streamEvents.process = ProcessCapture;
    pw_properties* properties = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                                                   "Capture", PW_KEY_MEDIA_ROLE, "Record", nullptr);
    if (properties == nullptr) {
        std::cerr << "无法创建 PipeWire 流属性\n";
        pw_core_disconnect(core);
        pw_context_destroy(context);
        pw_main_loop_destroy(state.mainLoop);
        std::fclose(state.output);
        pw_deinit();
        return 1;
    }
    if (!options.sourceName.empty() &&
        pw_properties_set(properties, PW_KEY_TARGET_OBJECT, options.sourceName.c_str()) < 0) {
        std::cerr << "无法设置 PipeWire 输入 source\n";
        pw_properties_free(properties);
        pw_core_disconnect(core);
        pw_context_destroy(context);
        pw_main_loop_destroy(state.mainLoop);
        std::fclose(state.output);
        pw_deinit();
        return 1;
    }
    state.stream = pw_stream_new_simple(loop, "pipewire-capture", properties, &streamEvents, &state);
    if (state.stream == nullptr) {
        std::cerr << "无法创建 PipeWire 采集流\n";
        pw_core_disconnect(core);
        pw_context_destroy(context);
        pw_main_loop_destroy(state.mainLoop);
        std::fclose(state.output);
        pw_deinit();
        return 1;
    }

    uint8_t podBuffer[1024];
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(podBuffer, sizeof(podBuffer));
    spa_audio_info_raw audioInfo{};
    audioInfo.format = SPA_AUDIO_FORMAT_S16;
    audioInfo.rate = options.rate;
    audioInfo.channels = options.channels;
    const spa_pod* params[1] = {spa_format_audio_raw_build(
        &builder, SPA_PARAM_EnumFormat, &audioInfo)};
    const int connectResult = pw_stream_connect(
        state.stream, PW_DIRECTION_INPUT, PW_ID_ANY,
        static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
        params, 1);
    if (connectResult < 0) {
        std::cerr << "无法连接采集流: " << spa_strerror(connectResult) << "\n";
        pw_stream_destroy(state.stream);
        pw_core_disconnect(core);
        pw_context_destroy(context);
        pw_main_loop_destroy(state.mainLoop);
        std::fclose(state.output);
        pw_deinit();
        return 1;
    }

    std::cout << "PipeWire 采集已启动，输入 source: "
              << (options.sourceName.empty() ? "系统默认" : options.sourceName)
              << "。\n输出文件: " << options.outputPath << "\n按 Ctrl+C 结束。\n";
    pw_main_loop_run(state.mainLoop);

    pw_stream_destroy(state.stream);
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_main_loop_destroy(state.mainLoop);
    const bool headerUpdated = UpdateWavHeader(state);
    std::fclose(state.output);
    pw_deinit();
    if (!headerUpdated) {
        std::cerr << "更新 WAV 文件头失败\n";
        return 1;
    }
    if (state.streamFailed || !state.streamStarted || state.dataBytes == 0) {
        std::cerr << "没有采集到有效音频数据，请用 wpctl status 检查输入设备和 PipeWire 连接。\n";
        return 1;
    }
    std::cout << "采集完成，帧数: " << state.frameCount
              << "，有效 PCM 数据: " << state.dataBytes << " 字节\n";
    if (state.totalPeakSample == 0) {
        std::cerr << "警告：采集到的样本峰值为 0。请检查麦克风硬件增益、Capture 音量和输入路由。\n";
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
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
