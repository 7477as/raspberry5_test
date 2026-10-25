#include "AecPipeline.h"
#include "DelayEstimator.h"
#include "WavWriter.h"

#include <gst/gst.h>
#include <glib-unix.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

constexpr int kOutputChannels = 4;

struct CommandLineOptions {
    std::string referencePath;
    std::string outputDir = "output";
    double durationSeconds = 0.0;
    int sampleRate = 16000;
    bool alignOutput = true;
    AecConfig aecConfig;
};

void PrintUsage(const char* programName) {
    std::cout
        << "用法: " << programName << " [选项]\n\n"
        << "实时硬件回环回声消除（webrtcdsp）：参考音频经扬声器播放制造真实回声，\n"
        << "WM8960 HAT 采集双麦，MIC0 进算法做回声消除，结果存成四通道 WAV。\n"
        << "按 Ctrl+C 可提前收尾并保存已采集的数据。\n\n"
        << "输入与设备:\n"
        << "  --ref <path>            参考音频（会被播放），默认 $HOME/work/audio/testAudio/test.wav\n"
        << "  --playback-device <dev> 播放设备，默认 hw:0,0\n"
        << "  --capture-device <dev>  采集设备，默认 hw:0,0\n"
        << "  --duration <sec>        录制时长；0 或省略表示按参考音频时长自动\n"
        << "\n算法:\n"
        << "  --rate <hz>             采样率 8000/16000/32000/48000，默认 16000\n"
        << "  --no-echo-cancel        关闭回声消除\n"
        << "  --no-noise-suppression  关闭降噪\n"
        << "  --no-gain-control       关闭自动增益\n"
        << "  --no-high-pass          关闭高通滤波\n"
        << "\n输出:\n"
        << "  --out-dir <dir>         输出目录，默认 ./output\n"
        << "  --no-align              不按算法延迟对齐 ch3\n"
        << "  -h, --help              显示本帮助\n";
}

bool ParseNonNegativeDouble(const std::string& value, double& out) {
    try {
        out = std::stod(value);
    } catch (const std::exception&) {
        return false;
    }
    return out >= 0.0;
}

bool ParsePositiveInt(const std::string& value, int& out) {
    try {
        out = std::stoi(value);
    } catch (const std::exception&) {
        return false;
    }
    return out > 0;
}

bool ParseCommandLine(int argc, char** argv, CommandLineOptions& options, std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string flag = argv[index];
        const bool hasValue = index + 1 < argc;

        if (flag == "-h" || flag == "--help") {
            PrintUsage(argv[0]);
            std::exit(0);
        }

        if (flag == "--no-echo-cancel") {
            options.aecConfig.echoCancel = false;
        } else if (flag == "--no-noise-suppression") {
            options.aecConfig.noiseSuppression = false;
        } else if (flag == "--no-gain-control") {
            options.aecConfig.gainControl = false;
        } else if (flag == "--no-high-pass") {
            options.aecConfig.highPassFilter = false;
        } else if (flag == "--no-align") {
            options.alignOutput = false;
        } else if (flag == "--ref" && hasValue) {
            options.referencePath = argv[++index];
        } else if (flag == "--out-dir" && hasValue) {
            options.outputDir = argv[++index];
        } else if (flag == "--playback-device" && hasValue) {
            options.aecConfig.playbackDevice = argv[++index];
        } else if (flag == "--capture-device" && hasValue) {
            options.aecConfig.captureDevice = argv[++index];
        } else if (flag == "--duration" && hasValue) {
            if (!ParseNonNegativeDouble(argv[++index], options.durationSeconds)) {
                error = "--duration 需要非负数";
                return false;
            }
        } else if (flag == "--rate" && hasValue) {
            if (!ParsePositiveInt(argv[++index], options.sampleRate)) {
                error = "--rate 需要正整数";
                return false;
            }
        } else {
            error = "无法识别的参数: " + flag + "（用 --help 查看用法）";
            return false;
        }
    }
    return true;
}

std::string ResolveDefaultReferencePath() {
    const std::filesystem::path defaultPath =
        std::filesystem::path(std::getenv("HOME")) / "work/audio/testAudio/test.wav";
    return defaultPath.string();
}

// 把 processed 按算法延迟右移，输出前向 mic0，实现采样级对齐
std::vector<float> ShiftByDelay(const std::vector<float>& samples, int lagSamples) {
    if (lagSamples <= 0 || samples.empty()) {
        return samples;
    }
    const size_t usable =
        samples.size() > static_cast<size_t>(lagSamples)
            ? samples.size() - static_cast<size_t>(lagSamples)
            : 0;
    std::vector<float> shifted(samples.size(), 0.0f);
    for (size_t index = 0; index < usable; ++index) {
        shifted[index] = samples[index + static_cast<size_t>(lagSamples)];
    }
    return shifted;
}

float ComputePeak(const std::vector<float>& samples) {
    float peak = 0.0f;
    for (float sample : samples) {
        peak = std::max(peak, std::fabs(sample));
    }
    return peak;
}

// 交织顺序固定为 ch0=MIC0, ch1=MIC1, ch2=参考, ch3=算法后
bool SaveQuadChannel(const std::filesystem::path& path, const AecCaptureResult& capture,
                     const std::vector<float>& processed, int sampleRate, std::string& error) {
    const size_t frames = capture.mic0.size();
    std::vector<float> interleaved(frames * kOutputChannels);
    for (size_t frame = 0; frame < frames; ++frame) {
        interleaved[frame * kOutputChannels] = capture.mic0[frame];
        interleaved[frame * kOutputChannels + 1] = capture.mic1[frame];
        interleaved[frame * kOutputChannels + 2] = capture.reference[frame];
        interleaved[frame * kOutputChannels + 3] = processed[frame];
    }

    WavWriter writer;
    if (!writer.Open(path.string(), sampleRate, kOutputChannels)) {
        error = "无法写入 " + path.string();
        return false;
    }
    if (!writer.AppendInterleaved(interleaved.data(), static_cast<int>(frames))) {
        error = "写入样本失败: " + path.string();
        return false;
    }
    if (!writer.Close()) {
        error = "关闭文件失败: " + path.string();
        return false;
    }
    return true;
}

void PrintMetrics(const AecMetrics& metrics, double durationSeconds) {
    std::cout << "\n========== 处理结果 ==========\n"
              << "时长          : " << durationSeconds << " s\n"
              << "算法延迟      : " << metrics.delayMs << " ms\n"
              << "处理前 RMS    : " << metrics.rmsBefore << "\n"
              << "处理后 RMS    : " << metrics.rmsAfter << "\n"
              << "ERLE          : " << metrics.erleDb << " dB\n"
              << "=============================\n";
}

}  // namespace

int main(int argc, char** argv) {
    gst_init(&argc, &argv);

    CommandLineOptions options;
    std::string error;
    if (!ParseCommandLine(argc, argv, options, error)) {
        std::cerr << "参数错误: " << error << "\n";
        return 1;
    }

    if (options.referencePath.empty()) {
        options.referencePath = ResolveDefaultReferencePath();
    }

    const std::filesystem::path referenceFile(options.referencePath);
    if (!std::filesystem::is_regular_file(referenceFile)) {
        std::cerr << "找不到参考音频: " << options.referencePath << "\n"
                  << "  用 --ref <path> 指定，或先确认文件存在\n";
        return 1;
    }

    std::cout << "参考音频      : " << options.referencePath << "\n"
              << "播放设备      : " << options.aecConfig.playbackDevice << "\n"
              << "采集设备      : " << options.aecConfig.captureDevice << "\n"
              << "采样率        : " << options.sampleRate << " Hz\n"
              << "录制时长      : "
              << (options.durationSeconds > 0.0 ? std::to_string(options.durationSeconds) + " s"
                                                : std::string("参考音频全长"))
              << "\n回声消除      : " << (options.aecConfig.echoCancel ? "开" : "关")
              << "  降噪: " << (options.aecConfig.noiseSuppression ? "开" : "关")
              << "  AGC: " << (options.aecConfig.gainControl ? "开" : "关")
              << "  高通: " << (options.aecConfig.highPassFilter ? "开" : "关") << "\n\n"
              << "开始实时采集，请确保扬声器与麦克风已接好（Ctrl+C 可提前结束）...\n";
    std::cout.flush();

    g_unix_signal_add(SIGINT, +[](gpointer) -> gboolean {
        AecPipeline::RequestStop();
        return G_SOURCE_CONTINUE;
    }, nullptr);
    g_unix_signal_add(SIGTERM, +[](gpointer) -> gboolean {
        AecPipeline::RequestStop();
        return G_SOURCE_CONTINUE;
    }, nullptr);

    AecPipeline pipeline;
    if (!pipeline.Initialize(options.aecConfig, options.referencePath, options.sampleRate, error)) {
        std::cerr << "初始化管线失败: " << error << "\n";
        return 1;
    }

    AecCaptureResult capture;
    if (!pipeline.RunForDuration(options.durationSeconds, capture, error)) {
        std::cerr << "采集失败: " << error << "\n";
        return 1;
    }

    // 回声消除算法延迟通常在几十毫秒内；搜索范围开太大会撞到噪声造成的伪峰，
    // 导致 ch3 被按一个错误的延迟整体错位
    const int maxLagSamples = options.sampleRate / 10;
    const int lagSamples = DelayEstimator::EstimateLagSamples(capture.mic0, capture.processed, maxLagSamples);

    AecMetrics metrics;
    metrics.delayMs = static_cast<double>(lagSamples);
    DelayEstimator::ComputeMetrics(capture.mic0, capture.processed, options.sampleRate, metrics);

    const std::vector<float> processed =
        options.alignOutput ? ShiftByDelay(capture.processed, lagSamples) : capture.processed;

    const std::filesystem::path outputDir(options.outputDir);
    std::error_code filesystemError;
    std::filesystem::create_directories(outputDir, filesystemError);
    if (filesystemError) {
        std::cerr << "创建输出目录失败: " << filesystemError.message() << "\n";
        return 1;
    }

    const std::filesystem::path outputPath = outputDir / "aec_4ch.wav";
    if (!SaveQuadChannel(outputPath, capture, processed, options.sampleRate, error)) {
        std::cerr << "保存输出失败: " << error << "\n";
        return 1;
    }

    PrintMetrics(metrics, capture.mic0.size() / static_cast<double>(options.sampleRate));

    std::cout << "\n四路峰值      : MIC0 " << ComputePeak(capture.mic0) << "  MIC1 "
              << ComputePeak(capture.mic1) << "  参考 " << ComputePeak(capture.reference)
              << "  算法后 " << ComputePeak(processed) << "\n";
    std::cout << "\n输出文件      : " << outputPath.string() << "\n"
              << "  ch0 = MIC0（原始，送入算法）\n"
              << "  ch1 = MIC1（原始，未参与算法）\n"
              << "  ch2 = 参考（扬声器播放的信号）\n"
              << "  ch3 = 算法后"
              << (options.alignOutput ? "（已按延迟对齐）" : "（未对齐）") << "\n";

    return 0;
}