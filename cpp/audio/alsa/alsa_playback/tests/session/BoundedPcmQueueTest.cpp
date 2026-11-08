#include "session/BoundedPcmQueue.h"

#include <cmath>
#include <stdexcept>

namespace alsa_playback::tests {

void RunBoundedPcmQueueTest() {
    BoundedPcmQueue queue(4);
    PcmBlock block(4, 2);
    for (std::size_t index = 0; index < block.samples.size(); ++index) {
        block.samples[index] = static_cast<float>(index);
    }
    if (!queue.Push(std::move(block)) || queue.Push(PcmBlock(1, 2))) {
        throw std::runtime_error("bounded queue capacity test failed");
    }
    std::vector<float> output;
    if (queue.PopFrames(2, 2, output) != 2 || output[0] != 0.0F || output[3] != 3.0F) {
        throw std::runtime_error("bounded queue pop test failed");
    }
    if (queue.QueuedFrames() != 2) {
        throw std::runtime_error("bounded queue depth test failed");
    }
}

}  // namespace alsa_playback::tests
