#include "format/WavFileReader.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace alsa_playback::tests {

void RunWavFileReaderTest() {
    const std::string path = "/tmp/alsa_playback_test.wav";
    const std::array<unsigned char, 48> wav{
        'R','I','F','F',40,0,0,0,'W','A','V','E','f','m','t',' ',
        16,0,0,0,1,0,1,0,0x44,0xac,0,0,0x88,0x58,1,0,2,0,16,0,
        'd','a','t','a',4,0,0,0,0x10,0,0xf0,0xff};
    {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(wav.data()), wav.size());
    }
    WavFileReader reader;
    std::string error;
    if (!reader.Open(path, error) || reader.Format().sampleRate != 44100 || reader.TotalFrames() != 2) {
        std::remove(path.c_str());
        throw std::runtime_error("WAV header parse test failed: " + error);
    }
    std::vector<std::int16_t> samples;
    std::size_t framesRead = 0;
    if (!reader.ReadFrames(4, samples, framesRead, error) || framesRead != 2 ||
        samples.size() != 2 || samples[0] != 16 || samples[1] != -16 || !reader.IsEndOfFile()) {
        std::remove(path.c_str());
        throw std::runtime_error("WAV block read test failed: " + error);
    }
    std::remove(path.c_str());
}

}
