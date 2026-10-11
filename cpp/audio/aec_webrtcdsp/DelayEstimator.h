#pragma once

#include <vector>

struct AecMetrics {
    double delayMs = 0.0;    // 互相关峰值对应的算法延迟
    double rmsBefore = 0.0;  // 处理前麦克风 RMS
    double rmsAfter = 0.0;   // 处理后麦克风 RMS
    double erleDb = 0.0;     // 10*log10(rmsBefore^2 / rmsAfter^2)
};

class DelayEstimator {
public:
    // 在 [0, maxLagSamples] 里搜索互相关峰值，返回算法延迟（样本数）
    static int EstimateLagSamples(const std::vector<float>& reference,
                                  const std::vector<float>& processed, int maxLagSamples);

    static void ComputeMetrics(const std::vector<float>& micBefore,
                               const std::vector<float>& micAfter, int sampleRate,
                               AecMetrics& out);
};