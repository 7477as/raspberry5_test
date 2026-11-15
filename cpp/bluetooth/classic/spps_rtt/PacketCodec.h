#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

/** Encodes bounded big-endian benchmark frames and parses RFCOMM streams. */
class PacketCodec {
public:
    PacketCodec() { mBuffer.reserve(131072); }
    static constexpr std::size_t HeaderSize = 40;
    static constexpr std::size_t MaxSize = 65536;
    static std::vector<std::uint8_t> Encode(std::uint32_t sequence, std::uint64_t timestamp,
                                           std::size_t size);
    static void Fill(std::vector<std::uint8_t>& frame, std::uint32_t sequence, std::uint64_t timestamp);
    static std::uint64_t Read(const std::uint8_t* bytes, std::size_t count);
    void Append(const std::uint8_t* bytes, std::size_t count);
    bool Next(std::vector<std::uint8_t>& frame);
private:
    std::vector<std::uint8_t> mBuffer;
};
