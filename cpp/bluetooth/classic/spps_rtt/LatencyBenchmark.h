#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

struct BenchmarkOptions {
    std::size_t FrameSize = 64;
    std::uint32_t Count = 1000;
    std::uint32_t Warmup = 20;
    unsigned IntervalMs = 20;
    unsigned TimeoutMs = 1000;
    unsigned Window = 1;
    bool FixedRate = false;
    std::string Csv;
};

/** Measures application RTT on a nonblocking connection using one monotonic clock. */
class LatencyBenchmark {
public:
    static int Run(int socket, const BenchmarkOptions& options);
};
