# HTTP Video Player

HTTP Range based MP4 streaming: a small Python server feeds a C++ client that decodes via FFmpeg and renders via SDL2. A/V sync is driven by the SDL audio queue, with a steady_clock fallback when audio is absent or exhausted.

## Layout

```
video_play_http/
├── README.md             ← this file
├── server/
│   ├── server.py         ← Python HTTP Range server
│   ├── input.mp4         ← source (gitignored)
│   └── test.mp4          ← faststart version (gitignored)
└── client/
    ├── README.md         ← client build / run
    ├── CMakeLists.txt
    ├── build_run.sh
    ├── Main.cpp          ← entry
    ├── VideoHal.cpp/.h   ← FFmpeg + SDL2 video/audio HAL
    └── minimp4.h         ← public-domain third-party header
```

## Run

Server side:

```bash
cd server
ffmpeg -i input.mp4 -c copy -movflags +faststart test.mp4
python3 server.py        # listens on 0.0.0.0:8080
```

Client side (see `client/README.md`):

```bash
cd ../client
chmod +x build_run.sh
./build_run.sh           # builds and runs, fetching http://127.0.0.1:8080/test.mp4
```

## Parameters

| Location | Default | Meaning |
| :--- | :--- | :--- |
| `server/server.py` port | `8080` | HTTP listen port |
| `client/Main.cpp` URL | `http://127.0.0.1:8080/test.mp4` | Source MP4 |
| `client/Main.cpp` seek step | `60` frames | ← / → key step |

## Notes

- The MP4 must be faststart (`-movflags +faststart`); non-faststart files won't be servable via Range.
- AAC `AudioSpecificConfig` comes from the MP4 `dsi` box; out-of-spec sample rates are not auto-adapted.
- The client uses CPU-only FFmpeg decoding; throughput is much lower than GStreamer but more controllable.
- `test.mp4` and `input.mp4` are gitignored — bring your own files.
