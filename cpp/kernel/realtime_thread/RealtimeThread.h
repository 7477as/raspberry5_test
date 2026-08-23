#pragma once

#include <cstdint>

class RealtimeThread {
public:
    struct Config {
        int targetCore     = 3;
        int intervalNs     = 125 * 1000;
        int testIterations = 100 * 8000;
    };

    int Run(const Config& cfg);

private:
    void SetupEnvironment(int targetCore);
    void LoopAndMeasure(int intervalNs, int iterations, int64_t* jitterArray);
    void PrintStats(const int64_t* jitterArray, int iterations) const;

    static uint64_t GetTimeNs();
};
