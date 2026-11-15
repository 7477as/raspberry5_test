#pragma once
#include <cstdint>
#include <stdexcept>

/** Four application timestamps; offset assumes equal clock rates and symmetric calibration paths. */
struct TimingEstimate {
    static double Difference(std::uint64_t first, std::uint64_t second) {
        return first >= second ? static_cast<double>(first - second) : -static_cast<double>(second - first);
    }
    static double Processing(std::uint64_t received, std::uint64_t sent) {
        if (!received || sent < received) throw std::runtime_error("Invalid Android timestamps; install protocol v2 APK");
        return Difference(sent, received);
    }
    static double Offset(std::uint64_t t1, std::uint64_t t2, std::uint64_t t3, std::uint64_t t4) {
        return (Difference(t2, t1) + Difference(t3, t4)) / 2.0;
    }
};
