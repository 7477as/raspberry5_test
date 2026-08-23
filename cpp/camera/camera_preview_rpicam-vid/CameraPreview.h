#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

class CameraPreview {
public:
    CameraPreview();
    ~CameraPreview();

    bool Init(int width, int height, int fps);
    int  Run();

private:
    bool OpenPipe();
    void ClosePipe();

    int  mWidth;
    int  mHeight;
    int  mFps;

    std::string           mPipeCmd;
    std::vector<uint8_t>  mBuffer;
    FILE*                 mPipe;
};
