# GStreamer Local Player

Local MP4 player built on GStreamer's `playbin`. Demonstrates pause / seek / resume from the terminal — the same control surface that a future UI would call.

## Layout

```
video_play_gstreamer/
├── CMakeLists.txt
├── build_run.sh
├── Main.cpp       ← entry
└── README.md
```

## Build

```bash
sudo apt install \
    cmake build-essential pkg-config \
    libgstreamer1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good

chmod +x build_run.sh
./build_run.sh /path/to/your.mp4
./build_run.sh build | clean | rebuild
NORUN=1 ./build_run.sh
```

If no path is given, the script tries `$HOME/work/tmp/output_fixed.mp4` and `/tmp/output_fixed.mp4` as fallbacks.

## Controls

```
p          pause
r          resume
s <sec>    seek to seconds (FLUSH + KEY_UNIT + SNAP_BEFORE)
q          quit
```

## Notes

- Sparse I-frames reduce Seek accuracy. Re-encode with denser GOP if precise Seek is required:

  ```bash
  ffmpeg -i input.mp4 -c:v libx264 \
      -g 30 -keyint_min 30 -flags +cgop \
      -c:a copy output_fixed.mp4
  ```

- `playbin` pauses on the last decoded B-frame, not the latest; this is by design.
- Local files only (`gst_filename_to_uri`); no network playback in this example.
