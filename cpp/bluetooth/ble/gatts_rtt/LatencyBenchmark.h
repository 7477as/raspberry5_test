#pragma once
#include "GattServer.h"
#include <cstdint>
#include <string>

struct BenchmarkOptions {
    unsigned Count = 1000;
    unsigned Warmup = 100;
    unsigned IntervalMs = 20;
    unsigned TimeoutMs = 1000;
    unsigned Window = 1;
    unsigned WaitSeconds = 120;
    bool FixedRate = false;
    std::string Csv;
};
/** Notification/write-command application RTT benchmark. */
int RunBenchmark(GattServer& server, const BenchmarkOptions& options);
