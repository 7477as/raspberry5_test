#pragma once

#include "AudioLoader.h"

#include <string>

struct EchoParams {
    double delayMs = 120.0;   // 回声路径延迟
    double echoGain = 0.35;   // 回声幅度
    double noiseDb = -30.0;   // 底噪电平
};

class EchoScenario {
public:
    // 模式 A：把参考信号当作扬声器播放内容，按 delayMs/echoGain 延迟叠加出回声，
    // 再叠加 noiseDb 底噪与可选近端人声，合成麦克风信号。
    static bool SynthesizeMicSignal(const MonoAudio& reference, const EchoParams& params,
                                    const MonoAudio* nearEndSpeech, MonoAudio& micOut,
                                    std::string& error);

    // 模式 B：真实参考 + 真实麦克风，长度自动补齐到一致（短的一方补零）
    static bool MergeRealPair(const MonoAudio& reference, const MonoAudio& mic,
                              MonoAudio& referenceOut, MonoAudio& micOut, std::string& error);
};