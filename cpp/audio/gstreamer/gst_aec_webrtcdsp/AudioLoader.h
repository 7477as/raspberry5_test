#pragma once

#include <string>
#include <vector>

struct MonoAudio {
    std::vector<float> samples;
    int sampleRate = 0;

    double DurationSeconds() const {
        return sampleRate > 0 ? static_cast<double>(samples.size()) / sampleRate : 0.0;
    }
};

// 用 GStreamer 把任意格式/采样率/声道数的音频归一化为单声道 float，
// 避免手写 24bit / 32bit float / 多声道 WAV 解析。
class AudioLoader {
public:
    // maxSeconds <= 0 表示读完整文件
    static bool LoadAsMono(const std::string& path, int targetRate,
                           double maxSeconds, MonoAudio& out, std::string& error);
};