1.启动http server
python http_server.py

2.测试 HTTP 上传 (使用 curl)
curl -X POST -F "file=@/你要上传的本地文件路径.txt" http://localhost:8080/upload
curl -X POST -F "file=@test.mp4" http://127.0.0.1:8080/upload

3.测试 HTTP Range 下载 (断点续传测试)
curl -r 0-99 -o partial_file.txt http://localhost:8080/download/你刚才上传的文件名.txt 
curl -O http://127.0.0.1:8080/download/test.mp4