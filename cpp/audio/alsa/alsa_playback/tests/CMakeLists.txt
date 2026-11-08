cmake_minimum_required(VERSION 3.16)

add_executable(alsa_playback_tests
    Main.cpp
    session/BoundedPcmQueueTest.cpp
    session/BlockingQueueTest.cpp
    format/AudioResamplerTest.cpp
    format/WavFileReaderTest.cpp
    mixer/AudioMixerTest.cpp
    effect/BypassEffectTest.cpp
    format/PcmConverterTest.cpp
    device/AlsaDeviceCatalogTest.cpp
    player/DeviceOpenOnDemandTest.cpp
)

target_include_directories(alsa_playback_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../service/audio/playback/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../service/audio/playback/src
)
target_compile_options(alsa_playback_tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(alsa_playback_tests PRIVATE alsa_playback_service)
add_test(NAME alsa_playback_tests COMMAND alsa_playback_tests)
