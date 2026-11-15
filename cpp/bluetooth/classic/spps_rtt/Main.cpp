#include "LatencyBenchmark.h"
#include "SppServer.h"

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        BenchmarkOptions options;
        for (int index = 1; index < argc; ++index) {
            const std::string key = argv[index];
            if (key == "--help") {
                std::cout << "spps [--size 64] [--count 1000] [--warmup 20] [--interval-ms 20]\n"
                             "     [--timeout-ms 1000] [--mode baseline|fixed] [--window 1] [--csv file]\n";
                return 0;
            }
            if (++index >= argc) throw std::runtime_error("Missing value for " + key);
            const std::string value = argv[index];
            if (key == "--csv") options.Csv = value;
            else if (key == "--mode") {
                if (value != "baseline" && value != "fixed") throw std::runtime_error("Unknown mode");
                options.FixedRate = value == "fixed";
            } else {
                std::size_t consumed = 0;
                const auto number = std::stoull(value, &consumed);
                if (consumed != value.size() || value[0] == '-' || number > 1000000)
                    throw std::runtime_error("Invalid numeric value: " + value);
                if (key == "--size") options.FrameSize = number;
                else if (key == "--count") options.Count = number;
                else if (key == "--warmup") options.Warmup = number;
                else if (key == "--interval-ms") options.IntervalMs = number;
                else if (key == "--timeout-ms") options.TimeoutMs = number;
                else if (key == "--window") options.Window = number;
                else throw std::runtime_error("Unknown option: " + key);
            }
        }
        if (!options.Count || options.Count + options.Warmup > 1000000 ||
            !options.IntervalMs || !options.TimeoutMs || !options.Window || options.Window > 1024 ||
            options.FrameSize < 40 || options.FrameSize > 65536)
            throw std::runtime_error("Invalid limits: total <= 1000000, size 40..65536, window 1..1024, times > 0");
        if (!options.FixedRate) options.Window = 1;
        SppServer server;
        std::cout << "SPP service RPi5_SPP_RTT registered. Connect the Android app.\n" << std::flush;
        return LatencyBenchmark::Run(server.WaitForConnection(), options);
    } catch (const std::exception& error) {
        std::cerr << "spps: " << error.what() << '\n';
        return 1;
    }
}
