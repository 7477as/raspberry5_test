# 将原视频转封装为 faststart 格式，并重命名为 test.mp4
ffmpeg -i input.mp4 -c copy -movflags +faststart test.mp4

cd server
python server.py

cd client
./build_run.sh