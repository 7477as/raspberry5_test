# Video Encode & Decode

Camera → H.264/H.265 encoder → application-level Jitter Buffer → software decoder → OpenCV display. Demonstrates precise playback-delay control.

## Layout

```
video_encode_decode/
├── CMakeLists.txt
├── build_run.sh
├── Main.cpp          ← entry
└── README.md
```

## Pipeline

```
libcamerasrc → x264enc/x265enc → appsink
                                       ↓
                              PacketQueue (MT-safe)
                                       ↓
                              appsrc → avdec → appsink
                                                ↓
                                            cv::imshow
```

## Build

```bash
sudo apt install \
    libopencv-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav

chmod +x build_run.sh
./build_run.sh
./build_run.sh build | clean | rebuild
NORUN=1 ./build_run.sh
BUILD_TYPE=Debug ./build_run.sh
```

To switch to H.265:

```bash
./build_run.sh rebuild   # then edit Main.cpp USE_H265 = 1
```

## Parameters

| Location | Default | Meaning |
| :--- | :--- | :--- |
| `Main.cpp` `USE_H265` | `0` | `0`=H.264, `1`=H.265 |
| `Main.cpp` `BUFFER_FRAMES` | `5` | App-level Jitter Buffer size |
| `Main.cpp` `StreamWidth/Height` | `564×318` | Encode resolution |
| Encoder pipeline `bitrate` | `800` kbps | |
| `key-int-max` | `30` | IDR interval (frames) |

## Notes

- BUFFER_FRAMES=0 ⇒ minimal latency, weak against jitter.
- BUFFER_FRAMES=30 ≈ 1 second latency, suitable for stable live viewing.
- Higher resolutions stress the software decoder; keep `564×318` on Pi 5 for smooth playback.
