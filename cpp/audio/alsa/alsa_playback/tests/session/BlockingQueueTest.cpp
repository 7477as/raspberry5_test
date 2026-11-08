#include "session/BoundedPcmQueue.h"

#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>

namespace alsa_playback::tests {

void RunBlockingQueueTest() {
    BoundedPcmQueue queue(2);
    if (!queue.Push(PcmBlock(2, 1))) {
        throw std::runtime_error("blocking queue setup failed");
    }
    auto producer = std::async(std::launch::async, [&queue] {
        return queue.PushBlocking(PcmBlock(1, 1), std::chrono::milliseconds(500));
    });
    if (producer.wait_for(std::chrono::milliseconds(20)) != std::future_status::timeout) {
        throw std::runtime_error("blocking queue producer did not wait");
    }
    std::vector<float> samples;
    queue.PopFrames(1, 1, samples);
    if (!producer.get()) {
        throw std::runtime_error("blocking queue producer was not notified");
    }
    if (queue.PushBlocking(PcmBlock(2, 1), std::chrono::milliseconds(10))) {
        throw std::runtime_error("blocking queue timeout test failed");
    }
}

}
