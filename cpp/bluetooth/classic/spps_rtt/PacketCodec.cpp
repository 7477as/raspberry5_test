#include "PacketCodec.h"
#include <stdexcept>

namespace {
void Write(std::uint8_t* bytes, std::uint64_t value, std::size_t count) {
    for (std::size_t offset = count; offset > 0; --offset) {
        bytes[offset - 1] = static_cast<std::uint8_t>(value);
        value >>= 8;
    }
}
}

std::uint64_t PacketCodec::Read(const std::uint8_t* bytes, std::size_t count) {
    std::uint64_t value = 0;
    for (std::size_t offset = 0; offset < count; ++offset) value = (value << 8) | bytes[offset];
    return value;
}

std::vector<std::uint8_t> PacketCodec::Encode(std::uint32_t sequence, std::uint64_t timestamp,
                                             std::size_t size) {
    if (size < HeaderSize || size > MaxSize) throw std::runtime_error("Frame size must be 40..65536");
    std::vector<std::uint8_t> frame(size);
    Fill(frame, sequence, timestamp);
    return frame;
}

void PacketCodec::Fill(std::vector<std::uint8_t>& frame, std::uint32_t sequence, std::uint64_t timestamp) {
    const auto size = frame.size();
    Write(frame.data(), 0x53505031, 4);
    frame[4] = 2;
    frame[5] = 1;
    Write(frame.data() + 6, HeaderSize, 2);
    Write(frame.data() + 8, size, 4);
    Write(frame.data() + 12, sequence, 4);
    Write(frame.data() + 16, timestamp, 8);
    Write(frame.data() + 24, 0, 8);
    Write(frame.data() + 32, 0, 8);
    for (std::size_t offset = HeaderSize; offset < size; ++offset)
        frame[offset] = static_cast<std::uint8_t>(sequence + offset);
}

void PacketCodec::Append(const std::uint8_t* bytes, std::size_t count) {
    if (mBuffer.size() + count > MaxSize * 2) throw std::runtime_error("Receive buffer limit exceeded");
    mBuffer.insert(mBuffer.end(), bytes, bytes + count);
}

bool PacketCodec::Next(std::vector<std::uint8_t>& frame) {
    if (mBuffer.size() < HeaderSize) return false;
    if (Read(mBuffer.data(), 4) != 0x53505031 || mBuffer[4] != 2 || mBuffer[5] != 1 ||
        Read(mBuffer.data() + 6, 2) != HeaderSize) throw std::runtime_error("Invalid frame header");
    const auto size = Read(mBuffer.data() + 8, 4);
    if (size < HeaderSize || size > MaxSize) throw std::runtime_error("Invalid frame length");
    if (mBuffer.size() < size) return false;
    frame.assign(mBuffer.begin(), mBuffer.begin() + size);
    mBuffer.erase(mBuffer.begin(), mBuffer.begin() + size);
    return true;
}
