#include <exception>
#include <iostream>

namespace alsa_playback::tests {
void RunBoundedPcmQueueTest();
void RunBlockingQueueTest();
void RunAudioResamplerTest();
void RunWavFileReaderTest();
void RunAudioMixerTest();
void RunBypassEffectTest();
void RunPcmConverterTest();
void RunAlsaDeviceCatalogTest();
void RunDeviceOpenOnDemandTest();
}

int main() {
    try {
        alsa_playback::tests::RunBoundedPcmQueueTest();
        alsa_playback::tests::RunBlockingQueueTest();
        alsa_playback::tests::RunAudioResamplerTest();
        alsa_playback::tests::RunWavFileReaderTest();
        alsa_playback::tests::RunAudioMixerTest();
        alsa_playback::tests::RunBypassEffectTest();
        alsa_playback::tests::RunPcmConverterTest();
        alsa_playback::tests::RunAlsaDeviceCatalogTest();
        alsa_playback::tests::RunDeviceOpenOnDemandTest();
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
    std::cout << "alsa_playback_tests passed\n";
    return 0;
}
