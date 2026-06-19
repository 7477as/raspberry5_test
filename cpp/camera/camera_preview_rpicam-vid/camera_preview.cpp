#include <iostream>
#include <opencv2/opencv.hpp>
#include <cstdio>

int main() {
    int width = 1920;
    int height = 1080;
    int fps = 30;
    
    // 构建命令
    std::string cmd = "rpicam-vid --nopreview -t 0 "
                      "--autofocus-mode continuous "
                      "--width " + std::to_string(width) + " "
                      "--height " + std::to_string(height) + " "
                      "--framerate " + std::to_string(fps) + " "
                      "--codec yuv420 -o -";
    
    // 打开管道
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to open camera" << std::endl;
        return -1;
    }
    
    // YUV420 格式大小
    size_t frame_size = width * height * 3 / 2;
    std::vector<uint8_t> buffer(frame_size);
    
    cv::Mat frame;
    
    while (true) {
        // 读取一帧 YUV 数据
        size_t bytes = fread(buffer.data(), 1, frame_size, pipe);
        if (bytes != frame_size) break;
        
        // 转换为 OpenCV 格式
        cv::Mat yuv(height + height/2, width, CV_8UC1, buffer.data());
        cv::cvtColor(yuv, frame, cv::COLOR_YUV2BGR_I420);
        
        // 显示
        cv::imshow("Camera Test", frame);
        if (cv::waitKey(1) == 'q') break;
    }
    
    pclose(pipe);
    return 0;
}