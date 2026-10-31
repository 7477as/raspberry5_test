// pipewire_playback - 使用 PipeWire 播放正弦测试音。

#include <pipewire/pipewire.h>
#include <spa/param/audio/raw-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/pod/builder.h>
#include <spa/utils/result.h>

#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cstdint>
#include <string>

namespace {

constexpr uint32_t kDefaultRate = 48000;
constexpr uint32_t kDefaultChannels = 2;
constexpr double kDefaultTone = 440.0;
constexpr double kDefaultVolume = 0.2;
constexpr double kPi = 3.14159265358979323846;

volatile std::sig_atomic_t gStopRequested = 0;

struct Options {
    uint32_t rate = kDefaultRate;
    uint32_t channels = kDefaultChannels;
    double tone = kDefaultTone;
    double volume = kDefaultVolume;
    double duration = 0.0;
};

struct PlaybackState {
    pw_main_loop* mainLoop = nullptr;
    pw_stream* stream = nullptr;
    Options options;
    uint64_t framePosition = 0;
    std::chrono::steady_clock::time_point startTime;
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

bool ParseDouble(const std::string& value, double minimum, double maximum, double& output) {
    try {
        std::size_t parsed = 0;
        output = std::stod(value, &parsed);
        return parsed == value.size() && output >= minimum && output <= maximum;
    } catch (const std::exception&) {
        return false;
    }
}

void PrintUsage(const char* programName) {
    std::cout << "用法: " << programName << " [选项]\n\n"
              << "通过 PipeWire 默认播放设备播放正弦测试音。\n"
              << "  --rate <hz>          采样率，默认 48000\n"
              << "  --channels <count>   声道数，默认 2\n"
              << "  --tone <hz>          测试音频率，默认 440\n"
              << "  --volume <0.0-1.0>   音量，默认 0.2\n"
              << "  --duration <sec>     播放时长，0 表示持续到 Ctrl+C\n"
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
        if (flag == "--rate" && !ParseUnsigned(value, options.rate)) {
            error = "--rate 需要正整数";
            return false;
        }
        if (flag == "--channels" && !ParseUnsigned(value, options.channels)) {
            error = "--channels 需要正整数";
            return false;
        }
        if (flag == "--tone" && !ParseDouble(value, 0.0, 20000.0, options.tone)) {
            error = "--tone 需要 0 到 20000 之间的数值";
            return false;
        }
        if (flag == "--volume" && !ParseDouble(value, 0.0, 1.0, options.volume)) {
            error = "--volume 需要 0.0 到 1.0 之间的数值";
            return false;
        }
        if (flag == "--duration" && !ParseDouble(value, 0.0, 86400.0, options.duration)) {
            error = "--duration 需要 0 到 86400 之间的数值";
            return false;
        }
        if (flag != "--rate" && flag != "--channels" && flag != "--tone" &&
            flag != "--volume" && flag != "--duration") {
            error = "无法识别的参数: " + flag;
            return false;
        }
    }
    return true;
}

void ProcessPlayback(void* userData) {
    auto* state = static_cast<PlaybackState*>(userData);
    pw_buffer* buffer = pw_stream_dequeue_buffer(state->stream);
    if (buffer == nullptr) {
        return;
    }

    spa_data* data = buffer->buffer->datas;
    if (data[0].data == nullptr) {
        pw_stream_queue_buffer(state->stream, buffer);
        return;
    }

    const uint32_t frameBytes = state->options.channels * sizeof(int16_t);
    const uint32_t frameCount = buffer->buffer->datas[0].maxsize / frameBytes;
    auto* samples = static_cast<int16_t*>(data[0].data);
    for (uint32_t frame = 0; frame < frameCount; ++frame) {
        const double phase = 2.0 * kPi * state->options.tone *
                             static_cast<double>(state->framePosition++) /
                             static_cast<double>(state->options.rate);
        const auto sample = static_cast<int16_t>(std::sin(phase) * state->options.volume * 32767.0);
        for (uint32_t channel = 0; channel < state->options.channels; ++channel) {
            samples[frame * state->options.channels + channel] = sample;
        }
    }
    data[0].chunk->offset = 0;
    data[0].chunk->stride = static_cast<int32_t>(frameBytes);
    data[0].chunk->size = frameCount * frameBytes;
    pw_stream_queue_buffer(state->stream, buffer);
    if (gStopRequested) {
        pw_main_loop_quit(state->mainLoop);
    }
    if (state->options.duration > 0.0) {
        const double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - state->startTime).count();
        if (elapsed >= state->options.duration) {
            pw_main_loop_quit(state->mainLoop);
        }
    }
}

void OnStreamStateChanged(void* userData, pw_stream_state, pw_stream_state, const char* error) {
    auto* state = static_cast<PlaybackState*>(userData);
    if (error != nullptr) {
        std::cerr << "播放流错误: " << error << "\n";
        pw_main_loop_quit(state->mainLoop);
    }
}

void OnStreamProcess(void* userData) {
    ProcessPlayback(userData);
}

int RunPlayback(const Options& options) {
    PlaybackState state;
    state.options = options;
    state.startTime = std::chrono::steady_clock::now();
    pw_init(nullptr, nullptr);
    state.mainLoop = pw_main_loop_new(nullptr);
    if (state.mainLoop == nullptr) {
        std::cerr << "无法创建 PipeWire 主循环\n";
        pw_deinit();
        return 1;
    }

    pw_loop* loop = pw_main_loop_get_loop(state.mainLoop);
    pw_context* context = pw_context_new(loop, nullptr, 0);
    pw_core* core = context == nullptr ? nullptr : pw_context_connect(context, nullptr, 0);
    if (core == nullptr) {
        std::cerr << "无法连接 PipeWire 服务\n";
        if (context != nullptr) {
            pw_context_destroy(context);
        }
        pw_main_loop_destroy(state.mainLoop);
        pw_deinit();
        return 1;
    }

    pw_stream_events streamEvents{};
    streamEvents.version = PW_VERSION_STREAM_EVENTS;
    streamEvents.state_changed = OnStreamStateChanged;
    streamEvents.process = OnStreamProcess;
    pw_properties* properties = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                                                     "Playback", PW_KEY_MEDIA_ROLE, "Test", nullptr);
    pw_stream* stream = pw_stream_new_simple(loop, "pipewire-playback", properties, &streamEvents,
                                              &state);
    state.stream = stream;
    if (stream == nullptr) {
        std::cerr << "无法创建 PipeWire 播放流\n";
        pw_core_disconnect(core);
        pw_context_destroy(context);
        pw_main_loop_destroy(state.mainLoop);
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
    const int connectResult = pw_stream_connect(stream, PW_DIRECTION_OUTPUT,
                                                 PW_ID_ANY,
                                                 static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                                                                PW_STREAM_FLAG_MAP_BUFFERS |
                                                                                PW_STREAM_FLAG_RT_PROCESS),
                                                 params, 1);
    if (connectResult < 0) {
        std::cerr << "无法连接播放流: " << spa_strerror(connectResult) << "\n";
        pw_stream_destroy(stream);
        pw_core_disconnect(core);
        pw_context_destroy(context);
        pw_main_loop_destroy(state.mainLoop);
        pw_deinit();
        return 1;
    }

    pw_main_loop_run(state.mainLoop);

    pw_stream_destroy(stream);
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_main_loop_destroy(state.mainLoop);
    pw_deinit();
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
    return RunPlayback(options);
}
