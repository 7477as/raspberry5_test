#include "EchoScenario.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

// dBFS -> 线性幅度，-30 dBFS 约 0.0316
float DbToLinear(double db) {
    return static_cast<float>(std::pow(10.0, db / 20.0));
}

void FitToLength(MonoAudio& audio, size_t targetFrames) {
    audio.samples.resize(targetFrames, 0.0f);
}

}  // namespace

bool EchoScenario::SynthesizeMicSignal(const MonoAudio& reference, const EchoParams& params,
                                       const MonoAudio* nearEndSpeech, MonoAudio& micOut,
                                       std::string& error) {
    if (reference.samples.empty()) {
        error = "参考信号为空，无法合成麦克风信号";
        return false;
    }
    if (reference.sampleRate <= 0) {
        error = "参考信号采样率无效";
        return false;
    }
    if (params.delayMs < 0.0) {
        error = "回声延迟不能为负数";
        return false;
    }

    const size_t delaySamples =
        static_cast<size_t>(params.delayMs * reference.sampleRate / 1000.0);
    size_t totalFrames = reference.samples.size() + delaySamples;
    if (nearEndSpeech != nullptr && !nearEndSpeech->samples.empty()) {
        totalFrames = std::max(totalFrames, nearEndSpeech->samples.size());
    }

    micOut.samples.assign(totalFrames, 0.0f);
    micOut.sampleRate = reference.sampleRate;

    const float noiseAmplitude = DbToLinear(params.noiseDb);
    std::mt19937 randomEngine(20240711u);
    std::uniform_real_distribution<float> noiseDistribution(-1.0f, 1.0f);

    for (size_t frame = 0; frame < reference.samples.size(); ++frame) {
        float micSample = noiseDistribution(randomEngine) * noiseAmplitude;
        micSample += static_cast<float>(params.echoGain) * reference.samples[frame];

        const size_t echoFrame = frame + delaySamples;
        if (echoFrame < totalFrames) {
            micOut.samples[echoFrame] += micSample;
        }
    }

    if (nearEndSpeech != nullptr) {
        for (size_t frame = 0; frame < nearEndSpeech->samples.size(); ++frame) {
            if (frame < totalFrames) {
                micOut.samples[frame] += nearEndSpeech->samples[frame];
            }
        }
    }

    float peak = 0.0f;
    for (float sample : micOut.samples) {
        peak = std::max(peak, std::fabs(sample));
    }
    if (peak > 1.0f) {
        const float scale = 0.95f / peak;
        for (float& sample : micOut.samples) {
            sample *= scale;
        }
    }

    return true;
}

bool EchoScenario::MergeRealPair(const MonoAudio& reference, const MonoAudio& mic,
                                 MonoAudio& referenceOut, MonoAudio& micOut,
                                 std::string& error) {
    if (reference.samples.empty() || mic.samples.empty()) {
        error = "真实参考信号或麦克风信号为空";
        return false;
    }
    if (reference.sampleRate != mic.sampleRate) {
        error = "参考信号与麦克风信号采样率不一致（" + std::to_string(reference.sampleRate) +
                " vs " + std::to_string(mic.sampleRate) + "），请用相同 --rate 重新加载";
        return false;
    }

    const size_t commonFrames = std::max(reference.samples.size(), mic.samples.size());

    referenceOut.samples = reference.samples;
    micOut.samples = mic.samples;
    referenceOut.sampleRate = reference.sampleRate;
    micOut.sampleRate = mic.sampleRate;

    FitToLength(referenceOut, commonFrames);
    FitToLength(micOut, commonFrames);
    return true;
}