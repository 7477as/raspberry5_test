#include "LatencyBenchmark.h"
#include "PacketCodec.h"
#include "TimingEstimate.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
std::uint64_t Now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
struct Sample {
    std::uint64_t T1 = 0, T2 = 0, T3 = 0, T4 = 0;
    bool Timeout = false;
};
void Distribution(const char* label, std::vector<double> values) {
    if (values.empty()) return;
    std::sort(values.begin(), values.end());
    const auto quantile = [&](double fraction) { return values[std::size_t(std::ceil(values.size() * fraction)) - 1]; };
    std::cout << label << " ms: min=" << values.front() << " mean="
        << std::accumulate(values.begin(), values.end(), 0.0) / values.size()
        << " p50=" << quantile(.5) << " p95=" << quantile(.95) << " p99=" << quantile(.99)
        << " max=" << values.back() << '\n';
}
}

int RunBenchmark(GattServer& server, const BenchmarkOptions& options) {
    const auto waitStarted = Now();
    bool hello = false;
    std::uint32_t session = 0;
    server.Receive = [&](const std::uint8_t* data, std::size_t length) {
        if (length == 20 && PacketCodec::Read(data, 4) == 0xffffffff &&
            PacketCodec::Read(data + 12, 8) == 0x4741545452545431ULL) {
            session = static_cast<std::uint32_t>(PacketCodec::Read(data + 4, 8));
            hello = true;
        }
    };
    struct ClearReceiver { GattServer& Server; ~ClearReceiver() { Server.Receive = {}; } } clear{server};
    while (!server.Ready() || !hello) {
        server.Pump();
        if (Now() - waitStarted > options.WaitSeconds * 1000000000ULL) throw std::runtime_error("Timed out waiting for subscription and HELLO");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const std::size_t total = options.Count + options.Warmup;
    const std::uint32_t base = session & 0xfff00000U;
    std::vector<Sample> samples(total);
    unsigned sent = 0, completed = 0, inFlight = 0, invalid = 0, late = 0;
    std::uint64_t skipped = 0;
    server.Receive = [&](const std::uint8_t* bytes, std::size_t length) {
        const auto received = Now();
        if (length != 20) { ++invalid; return; }
        const auto sequence = static_cast<std::uint32_t>(PacketCodec::Read(bytes, 4));
        const auto index = sequence - base;
        if (index >= sent) { ++invalid; return; }
        auto& sample = samples[index];
        const auto t2 = PacketCodec::Read(bytes + 4, 8), t3 = PacketCodec::Read(bytes + 12, 8);
        if (!t2 || t3 < t2 || double(t3 - t2) > double(received - sample.T1)) { ++invalid; return; }
        if (sample.Timeout) { ++late; return; }
        if (sample.T4) { ++invalid; return; }
        sample.T2 = t2; sample.T3 = t3; sample.T4 = received;
        --inFlight; ++completed;
    };
    auto next = Now();
    const auto started = next;
    const std::uint64_t interval = options.IntervalMs * 1000000ULL;
    while (completed < total) {
        server.Pump();
        if (!server.Ready()) throw std::runtime_error("Peer disconnected or unsubscribed during test");
        const auto now = Now();
        for (unsigned index = 0; index < sent; ++index) {
            auto& sample = samples[index];
            if (!sample.T4 && !sample.Timeout && now - sample.T1 >= options.TimeoutMs * 1000000ULL) {
                sample.Timeout = true; --inFlight; ++completed;
            }
        }
        if (sent < total && now >= next) {
            if (options.FixedRate) {
                const auto missed = (now - next) / interval;
                skipped += missed; next += missed * interval;
            }
            if (inFlight < options.Window) {
                samples[sent].T1 = Now();
                const auto request = PacketCodec::Request(base + sent, samples[sent].T1);
                ++sent; ++inFlight;
                server.Send(request);
                next = options.FixedRate ? next + interval : now + interval;
            } else if (options.FixedRate) { ++skipped; next += interval; }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    server.Receive = {};
    double best = std::numeric_limits<double>::infinity(), offset = 0;
    std::size_t calibration = total;
    for (std::size_t index = 0; index < (options.Warmup ? options.Warmup : total); ++index) {
        const auto& sample = samples[index];
        if (!sample.T4) continue;
        const double net = double(sample.T4 - sample.T1) - double(sample.T3 - sample.T2);
        if (net < best) {
            best = net; calibration = index;
            offset = TimingEstimate::Offset(sample.T1, sample.T2, sample.T3, sample.T4);
        }
    }
    std::vector<double> rtt, processing, network, forward, reverse;
    unsigned timeouts = 0;
    for (unsigned index = options.Warmup; index < total; ++index) {
        const auto& sample = samples[index];
        if (!sample.T4) { ++timeouts; continue; }
        rtt.push_back(double(sample.T4 - sample.T1) / 1e6);
        processing.push_back(double(sample.T3 - sample.T2) / 1e6);
        network.push_back(rtt.back() - processing.back());
        if (calibration < total) {
            forward.push_back((TimingEstimate::Difference(sample.T2, sample.T1) - offset) / 1e6);
            reverse.push_back((TimingEstimate::Difference(sample.T4, sample.T3) + offset) / 1e6);
        }
    }
    std::cout << std::fixed << std::setprecision(6) << "Measured=" << options.Count << " successful=" << rtt.size()
        << " timeout=" << timeouts << " timeout_percent=" << 100.0 * timeouts / options.Count
        << " invalid=" << invalid << " late=" << late << " skipped_slots=" << skipped
        << " send_rate_hz=" << total * 1e9 / (Now() - started) << '\n';
    Distribution("RTT", rtt); Distribution("Android processing (measured)", processing);
    Distribution("RTT minus processing (measured)", network);
    Distribution("Pi->Android (ESTIMATE)", forward); Distribution("Android->Pi (ESTIMATE)", reverse);
    if (calibration < total) std::cout << "Calibration sequence=" << calibration << " android_minus_pi_ns=" << offset
        << " offset_bound_at_calibration_ms=+/-" << best / 2e6
        << " assumes symmetric calibration and equal clock rates; excludes drift/timestamp error; negatives retained\n";
    else std::cout << "No successful calibration sample: one-way estimates unavailable\n";
    if (!options.Csv.empty()) {
        std::ofstream file(options.Csv);
        if (!file) throw std::runtime_error("Cannot open CSV: " + options.Csv);
        file << "sequence,warmup,t1_ns,t2_ns,t3_ns,t4_ns,status,rtt_ms,android_processing_ms,net_rtt_ms,forward_estimate_ms,reverse_estimate_ms,offset_ns,calibration_bound_ms,calibration_age_s\n";
        file << std::fixed << std::setprecision(6);
        for (std::size_t index = 0; index < total; ++index) {
            const auto& sample = samples[index];
            file << base + index << ',' << (index < options.Warmup) << ',' << sample.T1 << ',' << sample.T2
                << ',' << sample.T3 << ',' << sample.T4 << ',' << (sample.Timeout ? "timeout" : "ok") << ',';
            if (sample.T4) {
                const double duration = double(sample.T4 - sample.T1) / 1e6, residence = double(sample.T3 - sample.T2) / 1e6;
                file << duration << ',' << residence << ',' << duration - residence << ',';
                if (calibration < total) file << (TimingEstimate::Difference(sample.T2, sample.T1) - offset) / 1e6
                    << ',' << (TimingEstimate::Difference(sample.T4, sample.T3) + offset) / 1e6;
                else file << ',';
            } else file << ",,,,";
            file << ',';
            if (calibration < total) file << offset << ',' << best / 2e6 << ',' << TimingEstimate::Difference(sample.T1, samples[calibration].T1) / 1e9;
            else file << ",,";
            file << '\n';
        }
        file.close();
        if (!file) throw std::runtime_error("CSV write failed");
    }
    return rtt.empty() ? 2 : 0;
}
