#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

/** Fixed 20-byte big endian wire format; sequence encodes session and packet index. */
class PacketCodec {
public:
    static std::uint64_t Read(const std::uint8_t* bytes, unsigned count) {
        std::uint64_t value = 0;
        for (unsigned index = 0; index < count; ++index) value = (value << 8) | bytes[index];
        return value;
    }
    static void Write(std::uint8_t* bytes, std::uint64_t value, unsigned count) {
        for (unsigned index = count; index > 0; --index) { bytes[index - 1] = value; value >>= 8; }
    }
    static std::array<std::uint8_t, 20> Request(std::uint32_t sequence, std::uint64_t sent) {
        std::array<std::uint8_t, 20> frame{};
        Write(frame.data(), sequence, 4);
        Write(frame.data() + 4, sent, 8);
        Write(frame.data() + 12, 0x4741545452545431ULL, 8);
        return frame;
    }
};
