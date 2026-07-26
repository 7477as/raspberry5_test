import os
import http.server
import socketserver

PORT = 8080

class RangeRequestHandler(http.server.SimpleHTTPRequestHandler):
    def send_head(self):
        # 拦截请求，解析 HTTP Range
        if 'Range' not in self.headers:
            return super().send_head()

        path = self.translate_path(self.path)
        if not os.path.isfile(path):
            self.send_error(404, "File not found")
            return None

        file_size = os.path.getsize(path)
        range_header = self.headers['Range']
        range_match = range_header.replace('bytes=', '').split('-')
        start = int(range_match[0]) if range_match[0] else 0
        end = int(range_match[1]) if len(range_match) > 1 and range_match[1] else file_size - 1
        
        # 边界校验
        if start >= file_size or start > end:
            self.send_error(416, "Requested Range Not Satisfiable")
            return None

        length = end - start + 1

        # 构造 HTTP Range 响应头 (206)
        self.send_response(206) 
        self.send_header('Content-Type', self.guess_type(path))
        self.send_header('Accept-Ranges', 'bytes')
        self.send_header('Content-Range', f'bytes {start}-{end}/{file_size}')
        self.send_header('Content-Length', str(length))
        self.end_headers()

        f = open(path, 'rb')
        f.seek(start)
        self.range_length = length
        return f

    def copyfile(self, source, outputfile):
        if not hasattr(self, 'range_length'):
            return super().copyfile(source, outputfile)
        
        # 精确按需发送客户端请求的字节量，绝不浪费带宽
        bytes_to_send = self.range_length
        while bytes_to_send > 0:
            chunk = source.read(min(bytes_to_send, 64 * 1024))
            if not chunk: break
            outputfile.write(chunk)
            bytes_to_send -= len(chunk)

if __name__ == '__main__':
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("0.0.0.0", PORT), RangeRequestHandler) as httpd:
        print(f"HTTP Range Server 已启动，端口: {PORT} ...")
        httpd.serve_forever()