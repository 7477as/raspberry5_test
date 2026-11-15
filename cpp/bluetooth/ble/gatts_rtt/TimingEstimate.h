#pragma once
#include <cstdint>

/** Signed clock arithmetic avoids unsigned underflow between unrelated clocks. */
struct TimingEstimate {
    static double Difference(std::uint64_t first, std::uint64_t second) {
        return first >= second ? double(first - second) : -double(second - first);
    }
    static double Offset(std::uint64_t t1, std::uint64_t t2, std::uint64_t t3, std::uint64_t t4) {
        return (Difference(t2, t1) + Difference(t3, t4)) / 2;
    }
};
