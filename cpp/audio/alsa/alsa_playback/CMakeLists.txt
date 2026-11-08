cmake_minimum_required(VERSION 3.16)
project(AlsaPlayback VERSION 2.0.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)
enable_testing()

set(ALSA_PLAYBACK_SERVICE_SOURCES
    service/audio/playback/src/PlaybackSession.cpp
    service/audio/playback/src/format/AudioFormat.cpp
    service/audio/playback/src/format/AudioResampler.cpp
    service/audio/playback/src/format/WavFileReader.cpp
    service/audio/playback/src/format/PcmBlock.cpp
    service/audio/playback/src/format/PcmConverter.cpp
    service/audio/playback/src/format/ChannelMapper.cpp
    service/audio/playback/src/statistics/AudioStatistics.cpp
    service/audio/playback/src/session/BoundedPcmQueue.cpp
    service/audio/playback/src/session/AudioSession.cpp
    service/audio/playback/src/session/SessionRegistry.cpp
    service/audio/playback/src/mixer/AudioMixer.cpp
    service/audio/playback/src/effect/AudioEffectChain.cpp
    service/audio/playback/src/effect/BypassEffect.cpp
    service/audio/playback/src/effect/EqualizerEffect.cpp
    service/audio/playback/src/effect/DrcEffect.cpp
    service/audio/playback/src/effect/LimiterEffect.cpp
    service/audio/playback/src/device/AlsaError.cpp
    service/audio/playback/src/device/AlsaDeviceCatalog.cpp
    service/audio/playback/src/device/AlsaPcmParameters.cpp
    service/audio/playback/src/device/AlsaPcmOutput.cpp
    service/audio/playback/src/player/AlsaAudioPlayer.cpp
    service/audio/playback/src/player/PlayerEngine.cpp
)

add_library(alsa_playback_service STATIC ${ALSA_PLAYBACK_SERVICE_SOURCES})
target_include_directories(alsa_playback_service
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/service/audio/playback/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/service/audio/playback/src
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ALSA_INCLUDE_DIRS}
)
target_compile_options(alsa_playback_service PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(alsa_playback_service PUBLIC Threads::Threads ${ALSA_LIBRARIES})
target_link_directories(alsa_playback_service PUBLIC ${ALSA_LIBRARY_DIRS})

add_executable(alsa_playback_app
    app/Main.cpp
    app/CommandLineOptions.cpp
    app/MusicPlayer.cpp
    app/PromptPlayer.cpp
)
target_include_directories(alsa_playback_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/service/audio/playback/include
        ${CMAKE_CURRENT_SOURCE_DIR}/service/audio/playback/src
)
target_compile_options(alsa_playback_app PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(alsa_playback_app PRIVATE alsa_playback_service)

add_subdirectory(tests)
