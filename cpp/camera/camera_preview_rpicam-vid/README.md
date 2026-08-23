# Camera Preview (rpicam-vid)

Camera preview driven by the `rpicam-vid` subprocess. YUV420 frames are piped to OpenCV for display.

## Layout

```
camera_preview_rpicam-vid/
├── CMakeLists.txt
├── build_run.sh
├── Main.cpp              ← entry
├── CameraPreview.cpp/.h  ← preview class
└── README.md
```

## Build

System packages:

```bash
sudo apt install libcamera-apps libopencv-dev cmake build-essential pkg-config
```

Build & run:

```bash
chmod +x build_run.sh
./build_run.sh          # configure + build + run
./build_run.sh build    # only build
./build_run.sh clean    # remove build/
./build_run.sh rebuild  # rebuild from scratch
NORUN=1 ./build_run.sh  # build without run
```

Press `q` in the preview window to exit.

## Parameters

| Location | Default | Meaning |
| :--- | :--- | :--- |
| `Main.cpp` | `Init(1920, 1080, 30)` | Width / height / fps |

## Notes

- `rpicam-vid` is provided by `libcamera-apps`; install it before building.
- YUV420 (I420) frames are converted to BGR via OpenCV. Higher resolutions increase CPU usage; consider MMAL/V4L2 for hardware acceleration.
