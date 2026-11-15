#include "LatencyBenchmark.h"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        BenchmarkOptions options;
        std::string adapter = "hci0";
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--help") {
                std::cout << "gatts [--adapter hci0] [--count 1000] [--warmup 100] [--interval-ms 20] [--timeout-ms 1000] [--window 1] [--fixed-rate] [--wait-seconds 120] [--csv build/rtt.csv]\n";
                return 0;
            }
            if (argument == "--fixed-rate") { options.FixedRate = true; continue; }
            if (++index >= argc) throw std::runtime_error("Missing argument for " + argument);
            const std::string value = argv[index];
            if (argument == "--csv") options.Csv = value;
            else if (argument == "--adapter") {
                if (value.size() < 4 || value.substr(0, 3) != "hci" || value.find_first_not_of("0123456789", 3) != std::string::npos)
                    throw std::runtime_error("Invalid adapter");
                adapter = value;
            } else {
                std::size_t consumed = 0;
                if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) throw std::runtime_error("Expected unsigned integer");
                const auto number = std::stoul(value, &consumed);
                if (consumed != value.size() || number > 1000000) throw std::runtime_error("Value outside 0..1000000");
                if (argument == "--count") options.Count = number;
                else if (argument == "--warmup") options.Warmup = number;
                else if (argument == "--interval-ms") options.IntervalMs = number;
                else if (argument == "--timeout-ms") options.TimeoutMs = number;
                else if (argument == "--window") options.Window = number;
                else if (argument == "--wait-seconds") options.WaitSeconds = number;
                else throw std::runtime_error("Unknown option: " + argument);
            }
        }
        if (!options.Count || !options.IntervalMs || !options.TimeoutMs || !options.Window || !options.WaitSeconds ||
            options.Count + options.Warmup >= 1048575 || options.Window > 32 || (!options.FixedRate && options.Window != 1))
            throw std::runtime_error("Invalid options: positive count/interval/timeout/wait, window 1..32 (window>1 requires --fixed-rate), total<1048575");
        GattServer server(adapter);
        std::cout << "RPi5_GATT_RTT advertising. Connect GATT RTT Client; waiting for Notify subscription + HELLO.\n";
        return RunBenchmark(server, options);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\nCheck bluetooth.service, powered adapter and D-Bus permission; see README.\n";
        return 1;
    }
}
