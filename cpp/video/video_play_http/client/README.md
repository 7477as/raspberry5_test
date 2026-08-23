# HTTP Video Player — Client

C++ client that pulls an MP4 over HTTP Range and plays it via FFmpeg + SDL2. A/V sync is audio-master (SDL audio queue clock) with a steady_clock fallback.

## Layout

```
client/
├── CMakeLists.txt
├── build_run.sh
├── Main.cpp          ← entry: HTTP, MP4, sync
├── VideoHal.cpp/.h   ← video / audio HAL
├── minimp4.h         ← public-domain third-party header
└── README.md
```

## Build

```bash
sudo apt install \
    cmake build-essential pkg-config \
    libavcodec-dev libavutil-dev libswresample-dev \
    libsdl2-dev \
    libcurl4-openssl-dev

chmod +x build_run.sh
./build_run.sh          # build + run
./build_run.sh build
./build_run.sh clean
./build_run.sh rebuild
NORUN=1 ./build_run.sh
BUILD_TYPE=Debug ./build_run.sh
```

Default URL: `http://127.0.0.1:8080/test.mp4`. Edit `Main.cpp` to change.

## Controls

| Key | Action |
| :--- | :--- |
| `Space` | pause / resume |
| `←` | back 60 frames |
| `→` | forward 60 frames |
| `ESC` | quit |

## Parameters

| Location | Default | Meaning |
| :--- | :--- | :--- |
| `Main.cpp` URL | `http://127.0.0.1:8080/test.mp4` | Source |
| `VideoHal.cpp` window | `800×600` | Resized to actual frame size after first decode |
| `VideoHal.h` `mTargetSampleRate` | `44100` | SDL output rate |
| `VideoHal.h` `mTargetChannels` | `2` | SDL output channels |
| `Main.cpp` seek step | `60` frames | ← / → step |

## Notes

- First-frame load depends on network RTT for the moov / SPS / PPS range request.
- `minimp4.h` is a public-domain third-party header — keep the original name to match upstream.
- FFmpeg version compatibility: code supports both the old `channel_layout` API and the new `ch_layout` (FFmpeg 5.0+).
