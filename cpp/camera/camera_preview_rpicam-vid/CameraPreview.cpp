#include "CameraPreview.h"

#include <iostream>

CameraPreview::CameraPreview()
    : mWidth(1920), mHeight(1080), mFps(30), mPipe(nullptr) {}

CameraPreview::~CameraPreview() {
    ClosePipe();
}

bool CameraPreview::Init(int width, int height, int fps) {
    mWidth  = width;
    mHeight = height;
    mFps    = fps;

    mPipeCmd = "rpicam-vid --nopreview -t 0 "
               "--autofocus-mode continuous "
               "--width "  + std::to_string(mWidth)  + " "
               "--height " + std::to_string(mHeight) + " "
               "--framerate " + std::to_string(mFps) + " "
               "--codec yuv420 -o -";

    return OpenPipe();
}

bool CameraPreview::OpenPipe() {
    mPipe = popen(mPipeCmd.c_str(), "r");
    if (!mPipe) {
        std::cerr << "Failed to open camera (popen failed)" << std::endl;
        return false;
    }

    mBuffer.resize(static_cast<size_t>(mWidth) * mHeight * 3 / 2);
    return true;
}

void CameraPreview::ClosePipe() {
    if (mPipe) {
        pclose(mPipe);
        mPipe = nullptr;
    }
}

int CameraPreview::Run() {
    if (!mPipe) {
        std::cerr << "CameraPreview not initialized" << std::endl;
        return -1;
    }

    const size_t FrameSize = static_cast<size_t>(mWidth) * mHeight * 3 / 2;
    cv::Mat      frame;
    const char*  WinName = "Camera Test";

    while (true) {
        const size_t BytesRead = fread(mBuffer.data(), 1, FrameSize, mPipe);
        if (BytesRead != FrameSize) {
            break;
        }

        cv::Mat yuv(mHeight + mHeight / 2, mWidth, CV_8UC1, mBuffer.data());
        cv::cvtColor(yuv, frame, cv::COLOR_YUV2BGR_I420);

        cv::imshow(WinName, frame);
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    ClosePipe();
    return 0;
}
