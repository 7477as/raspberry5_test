#include "DelayEstimator.h"

#include <algorithm>
#include <cmath>

namespace {

// 互相关只在中间一段窗口上算，避开 AEC 收敛的起始段和尾部的静音
constexpr size_t kWindowFrames = 16000;

double CrossCorrelationAt(const std::vector<float>& before, const std::vector<float>& after,
                          int lag, size_t windowStart, size_t windowLength) {
    double correlation = 0.0;
    for (size_t offset = 0; offset < windowLength; ++offset) {
        const size_t beforeIndex = windowStart + offset;
        const size_t afterIndex = windowStart + offset + static_cast<size_t>(lag);
        if (afterIndex >= after.size()) {
            break;
        }
        correlation += static_cast<double>(before[beforeIndex]) * static_cast<double>(after[afterIndex]);
    }
    return correlation;
}

double ComputeRms(const std::vector<float>& samples) {
    if (samples.empty()) {
        return 0.0;
    }
    double sumSquares = 0.0;
    for (float sample : samples) {
        sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
    }
    return std::sqrt(sumSquares / static_cast<double>(samples.size()));
}

}  // namespace

int DelayEstimator::EstimateLagSamples(const std::vector<float>& before,
                                       const std::vector<float>& after, int maxLagSamples) {
    if (before.empty() || after.empty() || maxLagSamples <= 0) {
        return 0;
    }

    const size_t commonFrames = std::min(before.size(), after.size());
    if (commonFrames <= static_cast<size_t>(maxLagSamples)) {
        return 0;
    }

    size_t windowLength = std::min(kWindowFrames, commonFrames / 2);
    if (windowLength == 0) {
        return 0;
    }
    size_t windowStart = (commonFrames - windowLength) / 2;

    const int searchLimit =
        std::min(maxLagSamples, static_cast<int>(commonFrames - windowLength) - 1);

    int bestLag = 0;
    double bestCorrelation = -1.0;
    for (int lag = 0; lag <= searchLimit; ++lag) {
        const double correlation =
            CrossCorrelationAt(before, after, lag, windowStart, windowLength);
        if (correlation > bestCorrelation) {
            bestCorrelation = correlation;
            bestLag = lag;
        }
    }

    return bestLag;
}

void DelayEstimator::ComputeMetrics(const std::vector<float>& micBefore,
                                    const std::vector<float>& micAfter, int sampleRate,
                                    AecMetrics& out) {
    out.rmsBefore = ComputeRms(micBefore);
    out.rmsAfter = ComputeRms(micAfter);

    if (out.rmsAfter > 1e-12 && out.rmsBefore > 1e-12) {
        out.erleDb = 10.0 * std::log10((out.rmsBefore * out.rmsBefore) /
                                       (out.rmsAfter * out.rmsAfter));
    } else {
        out.erleDb = 0.0;
    }

    if (sampleRate > 0) {
        out.delayMs = static_cast<double>(out.delayMs) * 1000.0 / sampleRate;
    }
}