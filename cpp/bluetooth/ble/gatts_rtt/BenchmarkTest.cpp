#include "LatencyBenchmark.h"
#include "PacketCodec.h"
#include "TimingEstimate.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
std::vector<std::array<std::uint8_t, 20>> gFrames;
unsigned gMode = 0;
std::uint64_t Now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
void Require(bool condition) { if (!condition) throw std::runtime_error("Test failed"); }
}
GattServer::GattServer(const std::string&) { mReady = true; }
GattServer::~GattServer() = default;
void GattServer::Pump() {
    if (!mPeer.empty()) return;
    mPeer = "simulated";
    auto hello = PacketCodec::Request(0xffffffff, 0x12300000);
    Receive(hello.data(), hello.size());
}
void GattServer::Send(const std::array<std::uint8_t, 20>& frame) {
    gFrames.push_back(frame);
    if (gMode == 1 && (PacketCodec::Read(frame.data(), 4) & 1)) return;
    if (gMode == 2) return;
    auto reply = frame;
    const auto now = Now();
    PacketCodec::Write(reply.data() + 4, now + 1000000000, 8);
    PacketCodec::Write(reply.data() + 12, now + 1000000000, 8);
    std::array<std::uint8_t, 1> invalid{};
    Receive(invalid.data(), invalid.size());
    Receive(reply.data(), reply.size());
    Receive(reply.data(), reply.size());
}
int main() {
    try {
        const auto frame = PacketCodec::Request(0x12345678, 0xfedcba9876543210ULL);
        Require(PacketCodec::Read(frame.data(), 4) == 0x12345678);
        Require(PacketCodec::Read(frame.data() + 4, 8) == 0xfedcba9876543210ULL);
        Require(TimingEstimate::Offset(100, 1110, 1115, 125) == 1000);
        Require(TimingEstimate::Offset(1110, 100, 125, 1115) == -1000);
        for (gMode = 0; gMode < 3; ++gMode) {
            BenchmarkOptions options;
            options.Count = 8; options.Warmup = 2; options.IntervalMs = 1;
            options.TimeoutMs = 3; options.Window = 2; options.FixedRate = true;
            options.Csv = "test-" + std::to_string(gMode) + ".csv";
            GattServer server("mock");
            Require(RunBenchmark(server, options) == (gMode == 2 ? 2 : 0));
            std::ifstream file(options.Csv);
            std::string line;
            unsigned count = 0;
            while (std::getline(file, line)) {
                unsigned commas = 0;
                for (char value : line) if (value == ',') ++commas;
                Require(commas == 14); ++count;
            }
            Require(count == 11);
        }
        std::cout << "Protocol, signed clock offset, loopback, duplicate/invalid, mixed/all timeout and CSV verified\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
