#include "LatencyBenchmark.h"
#include "PacketCodec.h"
#include "TimingEstimate.h"
#include <limits>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
std::uint64_t Now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}
struct Sample {
    std::uint64_t Sent = 0;
    std::uint64_t Received = 0;
    std::uint64_t AndroidReceived = 0;
    std::uint64_t AndroidSent = 0;
    bool Expired = false;
    bool CompleteWrite = false;
};
void CheckIo(bool failed) {
    if (failed) throw std::runtime_error(std::strerror(errno));
}
void Summarize(const std::vector<Sample>& samples, const BenchmarkOptions& options,
               std::uint64_t skipped, std::uint64_t late, std::uint64_t invalid,
               std::uint64_t duration) {
    std::vector<double> values;
    unsigned timeouts = 0;
    for (std::size_t index = options.Warmup; index < samples.size(); ++index) {
        const auto& sample = samples[index];
        if (sample.Expired) ++timeouts;
        if (sample.Received) values.push_back((sample.Received - sample.Sent) / 1e6);
    }
    std::sort(values.begin(), values.end());
    std::cout << "Measured=" << options.Count << " successful=" << values.size()
              << " timeout=" << timeouts << " timeout_percent=" << (100.0 * timeouts / options.Count)
              << " skipped_slots=" << skipped << " late=" << late << " invalid=" << invalid
              << " send_rate_hz=" << samples.size() * 1e9 / duration << '\n';
    if (!values.empty()) {
        const auto percentile = [&](double fraction) {
            return values[static_cast<std::size_t>(std::ceil(fraction * values.size())) - 1];
        };
        std::cout << std::fixed << std::setprecision(3) << "RTT ms: min=" << values.front()
                  << " mean=" << std::accumulate(values.begin(), values.end(), 0.0) / values.size()
                  << " p50=" << percentile(.50) << " p95=" << percentile(.95)
                  << " p99=" << percentile(.99) << " max=" << values.back() << '\n';
    }
    std::size_t calibration = samples.size();
    double best = std::numeric_limits<double>::infinity();
    const auto calibrationEnd = options.Warmup ? options.Warmup : samples.size();
    for (std::size_t index = 0; index < calibrationEnd; ++index) {
        const auto& sample = samples[index];
        if (!sample.Received) continue;
        const double network = TimingEstimate::Difference(sample.Received, sample.Sent) -
            TimingEstimate::Processing(sample.AndroidReceived, sample.AndroidSent);
        if (network >= 0 && network < best) { best = network; calibration = index; }
    }
    const bool calibrated = calibration < samples.size();
    double offset = 0;
    if (calibrated) {
        const auto& reference = samples[calibration];
        offset = TimingEstimate::Offset(reference.Sent, reference.AndroidReceived, reference.AndroidSent, reference.Received);
        std::cout << "Calibration: sequence=" << calibration << " source=" << (options.Warmup ? "warmup" : "all_samples")
                  << " android_minus_pi_ms=" << offset / 1e6
                  << " offset_bound_at_calibration_ms=+/-" << best / 2e6
                  << " (equal clock rates, nonnegative paths; excludes timestamp error/drift)\n";
    } else std::cout << "One-way estimates unavailable: no valid calibration sample\n";
    std::vector<double> forward, reverse, processing, network;
    for (std::size_t index = options.Warmup; index < samples.size(); ++index) {
        const auto& sample = samples[index];
        if (!sample.Received) continue;
        const double residence = TimingEstimate::Processing(sample.AndroidReceived, sample.AndroidSent);
        processing.push_back(residence / 1e6);
        network.push_back((TimingEstimate::Difference(sample.Received, sample.Sent) - residence) / 1e6);
        if (calibrated) {
            forward.push_back((TimingEstimate::Difference(sample.AndroidReceived, sample.Sent) - offset) / 1e6);
            reverse.push_back((TimingEstimate::Difference(sample.Received, sample.AndroidSent) + offset) / 1e6);
        }
    }
    const auto printDistribution = [](const char* label, std::vector<double>& data) {
        if (data.empty()) return;
        std::sort(data.begin(), data.end());
        const auto quantile = [&](double fraction) { return data[static_cast<std::size_t>(std::ceil(fraction * data.size())) - 1]; };
        std::cout << label << " ms: min=" << data.front() << " mean="
                  << std::accumulate(data.begin(), data.end(), 0.0) / data.size()
                  << " p50=" << quantile(.5) << " p95=" << quantile(.95)
                  << " p99=" << quantile(.99) << " max=" << data.back() << '\n';
    };
    printDistribution("Android processing (measured)", processing);
    printDistribution("RTT minus Android processing (measured)", network);
    printDistribution("Pi->Android (ESTIMATE)", forward);
    printDistribution("Android->Pi (ESTIMATE)", reverse);
    if (calibrated) std::cout << "One-way values assume symmetric calibration paths; negative estimates indicate asymmetry/drift. No drift correction.\n";
    if (!options.Csv.empty()) {
        std::ofstream output(options.Csv);
        if (!output) throw std::runtime_error("Cannot open CSV: " + options.Csv);
        output << "sequence,warmup,sent_monotonic_ns,received_monotonic_ns,status,rtt_ms,android_receive_ns,android_send_ns,android_processing_ms,rtt_minus_processing_ms,forward_estimate_ms,reverse_estimate_ms,calibration_offset_ns,calibration_bound_ms,calibration_age_s\n";
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const auto& sample = samples[index];
            output << index << ',' << (index < options.Warmup) << ',' << sample.Sent << ','
                   << sample.Received << ',' << (sample.Expired ? "timeout" : "ok") << ',';
            if (sample.Received) output << std::fixed << std::setprecision(6) << (sample.Received - sample.Sent) / 1e6;
            output << ',';
            if (sample.Received) {
                const double residence = TimingEstimate::Processing(sample.AndroidReceived, sample.AndroidSent);
                output << sample.AndroidReceived << ',' << sample.AndroidSent << ',' << residence / 1e6 << ','
                       << (TimingEstimate::Difference(sample.Received, sample.Sent) - residence) / 1e6 << ',';
                if (calibrated) output << (TimingEstimate::Difference(sample.AndroidReceived, sample.Sent) - offset) / 1e6
                    << ',' << (TimingEstimate::Difference(sample.Received, sample.AndroidSent) + offset) / 1e6;
                else output << ',';
            } else output << ",,,,,";
            output << ',';
            if (calibrated) output << offset << ',' << best / 2e6 << ','
                << TimingEstimate::Difference(sample.Sent, samples[calibration].Sent) / 1e9;
            else output << ",,";
            output << '\n';
        }
        output.close();
        if (!output) throw std::runtime_error("Failed writing CSV");
    }
}
}

int LatencyBenchmark::Run(int socket, const BenchmarkOptions& options) {
    const int flags = fcntl(socket, F_GETFL);
    CheckIo(flags < 0 || fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0);
    const std::size_t total = options.Count + options.Warmup;
    std::vector<Sample> samples(total);
    std::vector<std::size_t> pending;
    pending.reserve(options.Window);
    auto outgoing = PacketCodec::Encode(0, 0, options.FrameSize);
    std::vector<std::uint8_t> incoming;
    incoming.reserve(options.FrameSize);
    std::array<std::uint8_t, 8192> buffer{};
    PacketCodec parser;
    std::size_t sentCount = 0, completed = 0, inFlight = 0, writeOffset = outgoing.size();
    std::uint64_t skipped = 0, late = 0, invalid = 0;
    const std::uint64_t interval = options.IntervalMs * 1000000ULL;
    const std::uint64_t timeout = options.TimeoutMs * 1000000ULL;
    const auto started = Now();
    auto nextSend = started;
    while (completed < total) {
        auto now = Now();
        for (const auto index : pending) {
            auto& sample = samples[index];
            if (!sample.Received && !sample.Expired && now - sample.Sent >= timeout) {
                sample.Expired = true;
                --inFlight;
                ++completed;
                if (!sample.CompleteWrite && writeOffset != 0)
                    throw std::runtime_error("Partial frame write timed out; stream cannot be recovered");
                if (!sample.CompleteWrite) {
                    writeOffset = outgoing.size();
                    sample.CompleteWrite = true;
                }
            }
        }
        pending.erase(std::remove_if(pending.begin(), pending.end(), [&](std::size_t index) {
            return samples[index].Expired || samples[index].Received;
        }), pending.end());
        if (options.FixedRate && now >= nextSend && sentCount < total) {
            const auto missed = (now - nextSend) / interval;
            skipped += missed;
            nextSend += missed * interval;
            if (inFlight >= options.Window || writeOffset < outgoing.size()) {
                ++skipped;
                nextSend += interval;
            }
        }
        if (sentCount < total && now >= nextSend && inFlight < options.Window && writeOffset == outgoing.size()) {
            const auto sequence = static_cast<std::uint32_t>(sentCount);
            PacketCodec::Fill(outgoing, sequence, 0);
            now = Now();
            for (std::size_t byte = 0; byte < 8; ++byte)
                outgoing[16 + byte] = static_cast<std::uint8_t>(now >> ((7 - byte) * 8));
            samples[sentCount].Sent = now;
            pending.erase(std::remove_if(pending.begin(), pending.end(), [&](std::size_t index) {
                return samples[index].Expired || samples[index].Received;
            }), pending.end());
            pending.push_back(sentCount);
            ++sentCount;
            ++inFlight;
            writeOffset = 0;
            nextSend = options.FixedRate ? nextSend + interval : now + interval;
        }
        if (writeOffset < outgoing.size()) {
            const auto written = send(socket, outgoing.data() + writeOffset, outgoing.size() - writeOffset, MSG_NOSIGNAL);
            if (written > 0) {
                writeOffset += static_cast<std::size_t>(written);
                if (writeOffset == outgoing.size()) samples[sentCount - 1].CompleteWrite = true;
            } else if (written < 0) CheckIo(errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR);
        }
        const auto received = recv(socket, buffer.data(), buffer.size(), 0);
        if (received == 0) throw std::runtime_error("Peer disconnected before benchmark completed");
        if (received > 0) {
            const auto receivedAt = Now();
            parser.Append(buffer.data(), static_cast<std::size_t>(received));
            while (parser.Next(incoming)) {
                const auto sequence = PacketCodec::Read(incoming.data() + 12, 4);
                if (sequence >= sentCount) { ++invalid; continue; }
                auto& sample = samples[sequence];
                bool valid = incoming.size() == options.FrameSize &&
                             PacketCodec::Read(incoming.data() + 16, 8) == sample.Sent;
                for (std::size_t offset = PacketCodec::HeaderSize; valid && offset < incoming.size(); ++offset)
                    valid = incoming[offset] == static_cast<std::uint8_t>(sequence + offset);
                if (!valid || sample.Received) { ++invalid; continue; }
                if (sample.Expired) { ++late; continue; }
                sample.AndroidReceived = PacketCodec::Read(incoming.data() + 24, 8);
                sample.AndroidSent = PacketCodec::Read(incoming.data() + 32, 8);
                TimingEstimate::Processing(sample.AndroidReceived, sample.AndroidSent);
                sample.Received = receivedAt;
                --inFlight;
                ++completed;
            }
        } else if (received < 0) CheckIo(errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR);
        now = Now();
        std::uint64_t wake = now + 100000000ULL;
        if (sentCount < total && (options.FixedRate || (inFlight < options.Window && writeOffset == outgoing.size())))
            wake = std::min(wake, nextSend);
        for (const auto index : pending)
            if (!samples[index].Received && !samples[index].Expired) wake = std::min(wake, samples[index].Sent + timeout);
        if (completed == total) break;
        pollfd descriptor{socket, static_cast<short>(POLLIN | (writeOffset < outgoing.size() ? POLLOUT : 0)), 0};
        const int waitMs = wake <= now ? 0 : static_cast<int>((wake - now + 999999) / 1000000);
        const int result = poll(&descriptor, 1, waitMs);
        CheckIo(result < 0 && errno != EINTR);
        if (descriptor.revents & POLLNVAL) throw std::runtime_error("Invalid socket descriptor");
    }
    Summarize(samples, options, skipped, late, invalid, Now() - started);
    return std::any_of(samples.begin() + options.Warmup, samples.end(), [](const Sample& sample) {
        return sample.Received != 0;
    }) ? 0 : 2;
}
