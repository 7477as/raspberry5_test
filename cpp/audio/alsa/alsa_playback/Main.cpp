#include <alsa/asoundlib.h>

#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr unsigned int kDefaultSampleRate = 48000;
constexpr unsigned int kDefaultChannels = 2;
constexpr unsigned int kDefaultToneHz = 440;
constexpr unsigned int kDefaultDurationSeconds = 5;
constexpr snd_pcm_uframes_t kPeriodFrames = 1024;
constexpr double kPi = 3.14159265358979323846;

volatile std::sig_atomic_t gStopRequested = 0;

struct Options {
    std::string device = "hw:0,0";
    unsigned int sampleRate = kDefaultSampleRate;
    unsigned int channels = kDefaultChannels;
    unsigned int toneHz = kDefaultToneHz;
    unsigned int durationSeconds = kDefaultDurationSeconds;
};

void HandleSignal(int) {
    gStopRequested = 1;
}

void PrintUsage(const char* programName) {
    std::cout << "用法: " << programName << " [选项]\n\n"
              << "播放正弦测试音到 ALSA 设备。\n"
              << "  --device <dev>       ALSA 播放设备，默认 hw:0,0\n"
              << "  --rate <hz>          采样率，默认 48000\n"
              << "  --channels <count>   声道数，默认 2\n"
              << "  --tone <hz>          正弦音频率，默认 440\n"
              << "  --duration <sec>     播放时长，默认 5\n"
              << "  -h, --help           显示帮助\n";
}

bool ParseUnsigned(const std::string& value, unsigned int& result) {
    try {
        std::size_t parsedLength = 0;
        const unsigned long parsedValue = std::stoul(value, &parsedLength);
        if (parsedLength != value.size() || parsedValue == 0 || parsedValue > 192000) {
            return false;
        }
        result = static_cast<unsigned int>(parsedValue);
        return true;
    } catch (const std::exception&) {
        return false;
    }
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
        unsigned int value = 0;
        if (flag == "--device") {
            options.device = argv[++index];
        } else if (flag == "--rate" && ParseUnsigned(argv[++index], value)) {
            options.sampleRate = value;
        } else if (flag == "--channels" && ParseUnsigned(argv[++index], value) && value <= 8) {
            options.channels = value;
        } else if (flag == "--tone" && ParseUnsigned(argv[++index], value) && value < options.sampleRate / 2) {
            options.toneHz = value;
        } else if (flag == "--duration" && ParseUnsigned(argv[++index], value)) {
            options.durationSeconds = value;
        } else {
            error = "无法识别或参数无效: " + flag;
            return false;
        }
    }
    return true;
}

int RunPlayback(const Options& options) {
    snd_pcm_t* pcm = nullptr;
    const int openResult = snd_pcm_open(&pcm, options.device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (openResult < 0) {
        std::cerr << "打开播放设备失败: " << snd_strerror(openResult) << "\n";
        return 1;
    }

    const int formatResult = snd_pcm_set_params(
        pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, options.channels,
        options.sampleRate, 1, 500000);
    if (formatResult < 0) {
        std::cerr << "配置播放设备失败: " << snd_strerror(formatResult) << "\n";
        snd_pcm_close(pcm);
        return 1;
    }

    const std::size_t sampleCount = static_cast<std::size_t>(kPeriodFrames) * options.channels;
    std::vector<int16_t> samples(sampleCount);
    const double amplitude = 0.25 * 32767.0;
    const double phaseStep = 2.0 * kPi * options.toneHz / options.sampleRate;
    std::size_t frameCount = 0;
    const std::size_t targetFrames = static_cast<std::size_t>(options.sampleRate) * options.durationSeconds;

    std::cout << "播放设备: " << options.device << "\n"
              << "格式: S16_LE, " << options.sampleRate << " Hz, " << options.channels << " "
              << "声道, " << options.toneHz << " Hz, " << options.durationSeconds << " s\n";

    while (!gStopRequested && frameCount < targetFrames) {
        const std::size_t framesToWrite = std::min<std::size_t>(kPeriodFrames, targetFrames - frameCount);
        for (std::size_t frame = 0; frame < framesToWrite; ++frame) {
            const double sample = std::sin(phaseStep * (frameCount + frame));
            for (unsigned int channel = 0; channel < options.channels; ++channel) {
                samples[frame * options.channels + channel] = static_cast<int16_t>(sample * amplitude);
            }
        }

        snd_pcm_sframes_t writtenFrames = snd_pcm_writei(pcm, samples.data(), framesToWrite);
        if (writtenFrames == -EPIPE) {
            snd_pcm_prepare(pcm);
            continue;
        }
        if (writtenFrames < 0) {
            std::cerr << "播放音频失败: " << snd_strerror(static_cast<int>(writtenFrames)) << "\n";
            snd_pcm_close(pcm);
            return 1;
        }
        frameCount += static_cast<std::size_t>(writtenFrames);
    }

    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
    std::cout << "播放结束。\n";
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
        PrintUsage(argv[0]);
        return 1;
    }
    return RunPlayback(options);
}
