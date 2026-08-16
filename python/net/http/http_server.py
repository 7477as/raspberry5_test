import os
from flask import Flask, request, send_from_directory, jsonify

app = Flask(__name__)

# 设定存储文件的目录
UPLOAD_FOLDER = "uploads"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route('/upload', methods=['POST'])
def upload_file():
    """处理文件上传"""
    if 'file' not in request.files:
        return jsonify({"error": "请求中没有文件字段 (file)"}), 400
        
    file = request.files['file']
    if file.filename == '':
        return jsonify({"error": "未选择文件"}), 400

    # 保存文件到指定目录
    filepath = os.path.join(UPLOAD_FOLDER, file.filename)
    file.save(filepath)
    print(f"[文件上传] 成功保存: {filepath}")
    
    return jsonify({"message": f"文件 {file.filename} 上传成功"}), 200

@app.route('/download/<filename>', methods=['GET'])
def download_file(filename):
    """处理文件下载 (支持 Range 断点续传)"""
    print(f"[文件下载] 请求获取: {filename}")
    
    # conditional=True 会自动解析 HTTP Range 标头，实现断点续传/分块传输
    return send_from_directory(
        os.path.abspath(UPLOAD_FOLDER), 
        filename, 
        conditional=True
    )

if __name__ == "__main__":
    print(f"HTTP 服务器已启动 -> http://0.0.0.0:8080")
    print(f"上传地址: POST http://0.0.0.0:8080/upload (form-data 键名为 'file')")
    print(f"下载地址: GET  http://0.0.0.0:8080/download/<文件名>")
    
    # 启动 Flask 服务
    app.run(host="0.0.0.0", port=8080, threaded=True)