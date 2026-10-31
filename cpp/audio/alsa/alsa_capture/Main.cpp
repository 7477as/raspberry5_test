#include <alsa/asoundlib.h>

#include <cstdint>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr unsigned int kDefaultSampleRate = 48000;
constexpr unsigned int kDefaultChannels = 2;
constexpr unsigned int kDefaultDurationSeconds = 5;
constexpr snd_pcm_uframes_t kPeriodFrames = 1024;

volatile std::sig_atomic_t gStopRequested = 0;

struct Options {
    std::string device = "plughw:CARD=wm8960soundcard,DEV=0";
    std::string outputPath = "capture.wav";
    unsigned int sampleRate = kDefaultSampleRate;
    unsigned int channels = kDefaultChannels;
    unsigned int durationSeconds = kDefaultDurationSeconds;
};

void HandleSignal(int) {
    gStopRequested = 1;
}

void PrintUsage(const char* programName) {
    std::cout << "用法: " << programName << " [选项]\n\n"
              << "从 ALSA 设备采集 PCM 音频并保存为 WAV 文件。\n"
              << "  --device <dev>       ALSA 采集设备，默认 plughw:CARD=wm8960soundcard,DEV=0\n"
              << "  --output <path>      WAV 输出路径，默认 capture.wav\n"
              << "  --rate <hz>          采样率，默认 48000\n"
              << "  --channels <count>   声道数，默认 2\n"
              << "  --duration <sec>     采集时长，默认 5\n"
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
        } else if (flag == "--output") {
            options.outputPath = argv[++index];
        } else if (flag == "--rate" && ParseUnsigned(argv[++index], value)) {
            options.sampleRate = value;
        } else if (flag == "--channels" && ParseUnsigned(argv[++index], value) && value <= 8) {
            options.channels = value;
        } else if (flag == "--duration" && ParseUnsigned(argv[++index], value)) {
            options.durationSeconds = value;
        } else {
            error = "无法识别或参数无效: " + flag;
            return false;
        }
    }
    return true;
}

void WriteLittleEndian16(std::ofstream& output, std::uint16_t value) {
    const char bytes[2] = {
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
    };
    output.write(bytes, sizeof(bytes));
}

void WriteLittleEndian32(std::ofstream& output, std::uint32_t value) {
    const char bytes[4] = {
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
        static_cast<char>((value >> 16) & 0xff),
        static_cast<char>((value >> 24) & 0xff),
    };
    output.write(bytes, sizeof(bytes));
}

void WriteWavHeader(std::ofstream& output, const Options& options, std::uint32_t dataSize) {
    const std::uint32_t byteRate = options.sampleRate * options.channels * sizeof(std::int16_t);
    const std::uint16_t blockAlign = options.channels * sizeof(std::int16_t);
    output.write("RIFF", 4);
    WriteLittleEndian32(output, 36 + dataSize);
    output.write("WAVEfmt ", 8);
    WriteLittleEndian32(output, 16);
    WriteLittleEndian16(output, 1);
    WriteLittleEndian16(output, options.channels);
    WriteLittleEndian32(output, options.sampleRate);
    WriteLittleEndian32(output, byteRate);
    WriteLittleEndian16(output, blockAlign);
    WriteLittleEndian16(output, 16);
    output.write("data", 4);
    WriteLittleEndian32(output, dataSize);
}

int RunCapture(const Options& options) {
    snd_pcm_t* pcm = nullptr;
    const int openResult = snd_pcm_open(&pcm, options.device.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if (openResult < 0) {
        std::cerr << "打开采集设备失败: " << snd_strerror(openResult) << "\n";
        return 1;
    }

    const int formatResult = snd_pcm_set_params(
        pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, options.channels,
        options.sampleRate, 1, 500000);
    if (formatResult < 0) {
        std::cerr << "配置采集设备失败: " << snd_strerror(formatResult) << "\n";
        snd_pcm_close(pcm);
        return 1;
    }
    const int startResult = snd_pcm_start(pcm);
    if (startResult < 0) {
        std::cerr << "启动采集设备失败: " << snd_strerror(startResult)
                  << " (PCM 状态: " << snd_pcm_state_name(snd_pcm_state(pcm)) << ")\n";
        snd_pcm_close(pcm);
        return 1;
    }

    std::ofstream output(options.outputPath, std::ios::binary);
    if (!output) {
        std::cerr << "创建 WAV 文件失败: " << options.outputPath << "\n";
        snd_pcm_close(pcm);
        return 1;
    }

    WriteWavHeader(output, options, 0);
    const std::size_t sampleCount = static_cast<std::size_t>(kPeriodFrames) * options.channels;
    std::vector<std::int16_t> samples(sampleCount);
    std::size_t frameCount = 0;
    std::uint32_t dataSize = 0;
    const std::size_t targetFrames = static_cast<std::size_t>(options.sampleRate) * options.durationSeconds;

    std::cout << "采集设备: " << options.device << "\n"
              << "格式: S16_LE, " << options.sampleRate << " Hz, " << options.channels << " 声道\n"
              << "输出: " << options.outputPath << "（" << options.durationSeconds << " s）\n";

    unsigned int recoveryCount = 0;
    while (!gStopRequested && frameCount < targetFrames) {
        const std::size_t framesToRead = std::min<std::size_t>(kPeriodFrames, targetFrames - frameCount);
        snd_pcm_sframes_t readFrames = snd_pcm_readi(pcm, samples.data(), framesToRead);
        if (readFrames < 0) {
            const int recoveryResult = snd_pcm_recover(pcm, static_cast<int>(readFrames), 1);
            if (recoveryResult >= 0 && recoveryCount++ < 3) {
                continue;
            }
            std::cerr << "采集音频失败: " << snd_strerror(static_cast<int>(readFrames))
                      << " (PCM 状态: " << snd_pcm_state_name(snd_pcm_state(pcm)) << ")\n"
                      << "恢复失败: " << snd_strerror(recoveryResult) << "\n"
                      << "请确认设备支持当前格式，或尝试 --device plughw:0,0；"
                      << "也可以先运行 arecord -D <设备> -f S16_LE -r 48000 -c 2 -d 5 test.wav\n";
            snd_pcm_close(pcm);
            return 1;
        }
        recoveryCount = 0;
        const std::size_t bytesRead = static_cast<std::size_t>(readFrames) * options.channels * sizeof(std::int16_t);
        output.write(reinterpret_cast<const char*>(samples.data()), static_cast<std::streamsize>(bytesRead));
        if (!output) {
            std::cerr << "写入 WAV 文件失败。\n";
            snd_pcm_close(pcm);
            return 1;
        }
        frameCount += static_cast<std::size_t>(readFrames);
        dataSize += static_cast<std::uint32_t>(bytesRead);
    }

    snd_pcm_drop(pcm);
    snd_pcm_close(pcm);
    output.seekp(0);
    WriteWavHeader(output, options, dataSize);
    output.close();
    std::cout << "采集结束，已写入 " << dataSize << " 字节 PCM 数据。\n";
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
    return RunCapture(options);
}
