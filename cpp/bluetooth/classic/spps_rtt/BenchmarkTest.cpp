#include "LatencyBenchmark.h"
#include "PacketCodec.h"
#include "TimingEstimate.h"
#include <cmath>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace {
void Require(bool condition) {
    if (!condition) throw std::runtime_error("Test condition failed");
}
void Benchmark(bool fixed, bool drop, bool dropAll = false) {
    int sockets[2];
    Require(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    std::thread echo([&] {
        PacketCodec parser;
        std::uint8_t bytes[31];
        std::vector<std::uint8_t> frame;
        ssize_t count;
        while ((count = read(sockets[1], bytes, sizeof(bytes))) > 0) {
            parser.Append(bytes, count);
            while (parser.Next(frame)) {
                if (dropAll || (drop && PacketCodec::Read(frame.data() + 12, 4) % 3 == 0)) continue;
                const auto t1 = PacketCodec::Read(frame.data() + 16, 8);
                for (const auto field : {24U, 32U}) {
                    const auto stamp = t1 + 1000000000ULL + 1000ULL + (field == 32 ? 100ULL : 0ULL);
                    for (unsigned byte = 0; byte < 8; ++byte)
                        frame[field + byte] = static_cast<std::uint8_t>(stamp >> ((7 - byte) * 8));
                }
                for (std::size_t offset = 0; offset < frame.size();) {
                    const auto written = send(sockets[1], frame.data() + offset,
                        std::min<std::size_t>(7, frame.size() - offset), MSG_NOSIGNAL);
                    if (written <= 0) return;
                    offset += written;
                }
            }
        }
    });
    BenchmarkOptions options;
    options.Count = 12;
    options.Warmup = 2;
    options.IntervalMs = 2;
    options.TimeoutMs = 20;
    options.Window = fixed ? 4 : 1;
    options.FixedRate = fixed;
    options.Csv = fixed ? "fixed.csv" : "baseline.csv";
    int result = -1;
    try { result = LatencyBenchmark::Run(sockets[0], options); }
    catch (...) { shutdown(sockets[0], SHUT_RDWR); echo.join(); close(sockets[0]); close(sockets[1]); throw; }
    shutdown(sockets[0], SHUT_RDWR);
    echo.join();
    close(sockets[0]);
    close(sockets[1]);
    Require(result == (dropAll ? 2 : 0));
    std::ifstream csv(options.Csv);
    std::string line;
    std::getline(csv, line);
    unsigned rows = 0, timeouts = 0;
    while (std::getline(csv, line)) {
        if (line.find(",timeout,") != std::string::npos) ++timeouts;
        else {
            std::istringstream columns(line);
            std::string field;
            std::getline(columns, field, ',');
            Require(std::stoul(field) == rows);
            std::getline(columns, field, ',');
            std::getline(columns, field, ',');
            const auto sent = std::stoull(field);
            std::getline(columns, field, ',');
            Require(std::stoull(field) >= sent);
        }
        ++rows;
    }
    Require(rows == 14 && timeouts == (dropAll ? 14U : (drop ? 5U : 0U)));
}
}
int main() {
    try {
        auto first = PacketCodec::Encode(5, 123456, 64);
        const auto second = PacketCodec::Encode(6, 987654, 48);
        PacketCodec parser;
        std::vector<std::uint8_t> frame;
        parser.Append(first.data(), 13);
        Require(!parser.Next(frame));
        parser.Append(first.data() + 13, first.size() - 13);
        parser.Append(second.data(), second.size());
        Require(parser.Next(frame) && frame == first);
        Require(parser.Next(frame) && frame == second);
        Require(!parser.Next(frame));
        first[8] = 127;
        parser.Append(first.data(), first.size());
        bool rejected = false;
        try { parser.Next(frame); } catch (const std::runtime_error&) { rejected = true; }
        Require(rejected);
        Require(TimingEstimate::Processing(10020, 10025) == 5);
        Require(TimingEstimate::Offset(100, 1110, 1115, 125) == 1000);
        const double clockOffset = TimingEstimate::Offset(100, 1110, 1115, 125);
        Require(TimingEstimate::Difference(1110, 100) - clockOffset == 10);
        Require(TimingEstimate::Difference(125, 1115) + clockOffset == 10);
        Require(TimingEstimate::Offset(1110, 100, 125, 1115) == -1000);
        Benchmark(false, false);
        Benchmark(true, true);
        Benchmark(false, false, true);
        std::cout << "Protocol fragmentation/coalescing and RTT timeout tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
