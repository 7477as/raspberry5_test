#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <opencv2/opencv.hpp>

// 0: H.264  1: H.265
#ifndef USE_H265
#define USE_H265 0
#endif

// 应用层 Jitter Buffer 容量：缓冲到该帧数才开始播放。
//   0  : 极限模式   (收到立刻播，抗卡顿极弱)
//   15 : 视频通话模式 (15 帧 / 30fps = 0.5 秒延迟)
//   30 : 电视直播模式 (30 帧 / 30fps = 1.0 秒延迟，绝对不卡顿)
static constexpr size_t BUFFER_FRAMES = 5;

class PacketQueue {
public:
    void Push(GstBuffer* buf) {
        std::unique_lock<std::mutex> lock(mMtx);
        mCv.wait(lock, [&]{ return mQueue.size() < 300 || mStop; });
        if (mStop) {
            if (buf) gst_buffer_unref(buf);
            return;
        }
        mQueue.push(buf);
        mCv.notify_all();
    }

    GstBuffer* PopOrWait(int timeoutMs) {
        std::unique_lock<std::mutex> lock(mMtx);
        mCv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                     [&]{ return !mQueue.empty() || mStop; });
        if (mQueue.empty()) {
            return nullptr;
        }
        GstBuffer* buf = mQueue.front();
        mQueue.pop();
        mCv.notify_all();
        return buf;
    }

    bool DrainStopped() const {
        std::unique_lock<std::mutex> lock(mMtx);
        return mStop && mQueue.empty();
    }

    void Stop() {
        {
            std::unique_lock<std::mutex> lock(mMtx);
            mStop = true;
        }
        mCv.notify_all();
    }

    void DrainAndUnref() {
        std::unique_lock<std::mutex> lock(mMtx);
        while (!mQueue.empty()) {
            gst_buffer_unref(mQueue.front());
            mQueue.pop();
        }
    }

private:
    std::queue<GstBuffer*> mQueue;
    mutable std::mutex             mMtx;
    std::condition_variable mCv;
    bool                   mStop = false;
};

class EncoderThread {
public:
    EncoderThread(int width, int height, PacketQueue* queue)
        : mWidth(width), mHeight(height), mQueue(queue) {}

    int Run() {
        GError* error = nullptr;

        std::string pipeStr =
            "libcamerasrc af-mode=continuous ! "
            "video/x-raw,width="  + std::to_string(mWidth) +
            ",height="            + std::to_string(mHeight) +
            ",framerate=30/1,format=NV12 ! videoflip method=horizontal-flip ! "
            "queue max-size-buffers=0 ! ";

#if USE_H265
        pipeStr += "videoconvert ! video/x-raw,format=I420 ! "
                   "x265enc tune=zerolatency speed-preset=ultrafast bitrate=800 key-int-max=30 ! "
                   "video/x-h265,stream-format=byte-stream ! ";
#else
        pipeStr += "x264enc tune=zerolatency speed-preset=superfast bframes=0 "
                   "key-int-max=30 bitrate=800 threads=4 ! "
                   "video/x-h264,profile=high,stream-format=byte-stream ! ";
#endif

        // 严禁底层擅自丢帧；保证 C++ 层拿到完整帧数
        pipeStr += "appsink name=enc_sink max-buffers=0 drop=false sync=false emit-signals=false";

        GstElement* pipeline = gst_parse_launch(pipeStr.c_str(), &error);
        if (error) {
            std::cerr << "[Encoder] 管道创建失败: " << error->message << std::endl;
            g_clear_error(&error);
            mQueue->Stop();
            return -1;
        }

        GstElement* appsink = gst_bin_get_by_name(GST_BIN(pipeline), "enc_sink");
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        std::cout << "[Encoder] 启动成功: " << (USE_H265 ? "H.265" : "H.264") << std::endl;

        while (true) {
            GstSample* sample = gst_app_sink_try_pull_sample(
                GST_APP_SINK(appsink), 100 * GST_MSECOND);
            if (sample) {
                GstBuffer* buffer = gst_sample_get_buffer(sample);
                GstBuffer* copy   = gst_buffer_copy_deep(buffer);
                gst_sample_unref(sample);
                mQueue->Push(copy);
            } else {
                if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) break;
            }
        }

        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(appsink);
        gst_object_unref(pipeline);
        return 0;
    }

private:
    int          mWidth;
    int          mHeight;
    PacketQueue* mQueue;
};

class DecoderAndUi {
public:
    int Run(PacketQueue* queue) {
        GError* error = nullptr;

        std::string pipeStr;
#if USE_H265
        pipeStr =
            "appsrc name=dec_src caps=\"video/x-h265,stream-format=byte-stream\" "
            "format=time is-live=true do-timestamp=true ! "
            "h265parse config-interval=-1 ! avdec_h265 max-threads=4 ! ";
#else
        pipeStr =
            "appsrc name=dec_src caps=\"video/x-h264,stream-format=byte-stream\" "
            "format=time is-live=true do-timestamp=true ! "
            "h264parse ! avdec_h264 max-threads=4 ! ";
#endif

        pipeStr += "videoconvert ! video/x-raw,format=BGR ! "
                   "appsink name=dec_sink max-buffers=0 drop=false sync=false emit-signals=false";

        GstElement* pipeline = gst_parse_launch(pipeStr.c_str(), &error);
        if (error) {
            std::cerr << "[Decoder] 管道创建失败: " << error->message << std::endl;
            g_clear_error(&error);
            queue->Stop();
            return -1;
        }

        GstElement* appsrc  = gst_bin_get_by_name(GST_BIN(pipeline), "dec_src");
        GstElement* appsink = gst_bin_get_by_name(GST_BIN(pipeline), "dec_sink");
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        std::thread feeder([&]() {
            while (true) {
                GstBuffer* buf = queue->PopOrWait(100);
                if (!buf) {
                    if (queue->DrainStopped()) break;
                    continue;
                }
                mBytesReceived += gst_buffer_get_size(buf);
                gst_app_src_push_buffer(GST_APP_SRC(appsrc), buf);
            }
            gst_app_src_end_of_stream(GST_APP_SRC(appsrc));
        });

        double   currentBitrate = 0.0;
        double   currentFps     = 0.0;
        int      frameCount     = 0;
        int      frameW         = 564;
        int      frameH         = 318;
        auto     lastStatTime   = std::chrono::steady_clock::now();

        const std::string winName = "App-Level Jitter Buffer";
        cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

        std::deque<cv::Mat> displayBuffer;
        bool                isBuffering = (BUFFER_FRAMES > 0);

        auto       nextPlayTime = std::chrono::steady_clock::now();
        const auto frameDuration = std::chrono::milliseconds(33);

        while (!mStop) {
            GstSample* sample = gst_app_sink_try_pull_sample(
                GST_APP_SINK(appsink), 5 * GST_MSECOND);
            if (sample) {
                frameCount++;
                auto now = std::chrono::steady_clock::now();
                std::chrono::duration<double> diff = now - lastStatTime;
                if (diff.count() >= 1.0) {
                    currentBitrate = (mBytesReceived.exchange(0) * 8.0) / 1024.0 / diff.count();
                    currentFps     = frameCount / diff.count();
                    frameCount     = 0;
                    lastStatTime   = now;
                }

                GstCaps* caps = gst_sample_get_caps(sample);
                gst_structure_get_int(gst_caps_get_structure(caps, 0), "width",  &frameW);
                gst_structure_get_int(gst_caps_get_structure(caps, 0), "height", &frameH);

                GstBuffer* buffer = gst_sample_get_buffer(sample);
                GstMapInfo map;
                gst_buffer_map(buffer, &map, GST_MAP_READ);
                cv::Mat frame(frameH, frameW, CV_8UC3, (void*)map.data, cv::Mat::AUTO_STEP);

                displayBuffer.push_back(frame.clone());
                gst_buffer_unmap(buffer, &map);
                gst_sample_unref(sample);

                // 防积压：缓冲远超目标时丢老帧追回延迟
                if (displayBuffer.size() > BUFFER_FRAMES + 10) {
                    while (displayBuffer.size() > BUFFER_FRAMES + 5) {
                        displayBuffer.pop_front();
                    }
                }
            }

            if (isBuffering) {
                if (displayBuffer.size() >= BUFFER_FRAMES) {
                    isBuffering = false;
                    nextPlayTime = std::chrono::steady_clock::now();
                }
            } else {
                if (displayBuffer.empty() && BUFFER_FRAMES > 0) {
                    isBuffering = true;
                }
            }

            auto now = std::chrono::steady_clock::now();
            if (isBuffering && now >= nextPlayTime) {
                cv::Mat loading = cv::Mat::zeros(frameH, frameW, CV_8UC3);
                char text[128];
                std::snprintf(text, sizeof(text),
                              "Buffering... %zu / %zu", displayBuffer.size(), BUFFER_FRAMES);
                cv::putText(loading, text,
                            cv::Point(frameW / 2 - 120, frameH / 2),
                            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
                cv::imshow(winName, loading);
                nextPlayTime = now + frameDuration;
            } else if (!isBuffering) {
                bool     showedFrame = false;
                cv::Mat  frameToShow;

                while (now >= nextPlayTime && !displayBuffer.empty()) {
                    frameToShow = displayBuffer.front();
                    displayBuffer.pop_front();
                    nextPlayTime += frameDuration;
                    showedFrame   = true;
                }

                if (showedFrame) {
                    char text[128];
                    std::snprintf(text, sizeof(text),
                                  "FPS: %.1f | Bitrate: %.1f kbps", currentFps, currentBitrate);
                    cv::putText(frameToShow, text, cv::Point(15, 30),
                                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 3);
                    cv::putText(frameToShow, text, cv::Point(15, 30),
                                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 1);

                    std::snprintf(text, sizeof(text),
                                  "Delay: %zu Frames (%.1f sec)", BUFFER_FRAMES, BUFFER_FRAMES / 30.0);
                    cv::putText(frameToShow, text, cv::Point(15, 60),
                                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 3);
                    cv::putText(frameToShow, text, cv::Point(15, 60),
                                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 1);

                    cv::imshow(winName, frameToShow);
                }
            }

            int key = cv::waitKey(1);
            if (key == 'q' || key == 27 ||
                cv::getWindowProperty(winName, cv::WND_PROP_AUTOSIZE) < 0) {
                mStop = true;
                queue->Stop();
            }
        }

        feeder.join();
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(appsrc);
        gst_object_unref(appsink);
        gst_object_unref(pipeline);
        cv::destroyAllWindows();
        return 0;
    }

private:
    std::atomic<bool>   mStop = false;
    std::atomic<size_t> mBytesReceived{0};
};

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);
    std::cout << "正在启动，目前应用层强制延迟设定为: "
              << BUFFER_FRAMES << " 帧" << std::endl;

    constexpr int StreamWidth  = 564;
    constexpr int StreamHeight = 318;

    PacketQueue queue;
    EncoderThread encoder(StreamWidth, StreamHeight, &queue);
    DecoderAndUi  decoder;

    std::thread encThread([&]() { encoder.Run(); });
    decoder.Run(&queue);
    encThread.join();

    queue.DrainAndUnref();
    std::cout << "测试完全结束。" << std::endl;
    return 0;
}
