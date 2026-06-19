#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <queue>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

// ==========================================
// 【配置区 1】：宏定义区分 H.264 和 H.265
// ==========================================
#define USE_H265 0  // 0: 使用 H.264 | 1: 使用 H.265

// ==========================================
// 【配置区 2】：精准帧控制参数 (核心)
// ==========================================
// 控制接收到多少帧之后才开始播放，实现精确的物理延迟。
// 0  : 极限穿梭机模式 (收到立刻播，毫无库存，抗卡顿极弱)
// 15 : 视频通话模式 (15帧 / 30fps = 0.5秒延迟，画面平滑，抗一般抖动)
// 30 : 电视直播模式 (30帧 / 30fps = 1.0秒延迟，绝对不卡顿)
const size_t BUFFER_FRAMES = 5; 
// ==========================================

std::queue<GstBuffer*> packet_queue;
std::mutex q_mutex;
std::condition_variable q_cv;
std::atomic<bool> stop_flag(false);
std::atomic<size_t> bytes_received(0);

// --- 1. 编码线程 ---
void encoder_thread(int target_width, int target_height) {
    GError *error = nullptr;

    // GStreamer 管道不再负责控制延迟，所有队列无限制，让数据极速流向 C++ 代码
    std::string pipe_str = "libcamerasrc af-mode=continuous ! "
                           "video/x-raw,width=" + std::to_string(target_width) + 
                           ",height=" + std::to_string(target_height) + 
                           ",framerate=30/1,format=NV12 ! videoflip method=horizontal-flip ! "
                           "queue max-size-buffers=0 ! "; // 0 = 无限制

#if USE_H265
    pipe_str += "videoconvert ! video/x-raw,format=I420 ! ";
    pipe_str += "x265enc tune=zerolatency speed-preset=ultrafast bitrate=800 key-int-max=30 ! ";
    pipe_str += "video/x-h265,stream-format=byte-stream ! ";
#else
    pipe_str += "x264enc tune=zerolatency speed-preset=superfast bframes=0 key-int-max=30 bitrate=800 threads=4 ! ";
    pipe_str += "video/x-h264,profile=high,stream-format=byte-stream ! ";
#endif

    // 【重要】max-buffers=0 和 drop=false：严禁底层擅自丢帧，保证我们在 C++ 层拿到完整帧数
    pipe_str += "appsink name=enc_sink max-buffers=0 drop=false sync=false emit-signals=false";

    GstElement *pipeline = gst_parse_launch(pipe_str.c_str(), &error);
    if (error) {
        std::cerr << "[编码器] 管道创建失败: " << error->message << std::endl;
        g_clear_error(&error); stop_flag = true; q_cv.notify_all(); return;
    }

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "enc_sink");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "[编码器] 启动成功: " << (USE_H265 ? "H.265" : "H.264") << std::endl;

    while (!stop_flag) {
        GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), 100 * GST_MSECOND);
        if (sample) {
            GstBuffer *buffer = gst_sample_get_buffer(sample);
            GstBuffer *buf_copy = gst_buffer_copy_deep(buffer);
            gst_sample_unref(sample);

            std::unique_lock<std::mutex> lock(q_mutex);
            // 放宽中转队列上限，防止缓冲时把编码器卡死
            q_cv.wait(lock, [&]{ return packet_queue.size() < 300 || stop_flag; });
            if (stop_flag) { gst_buffer_unref(buf_copy); break; }
            
            packet_queue.push(buf_copy);
            q_cv.notify_all();
        } else {
            if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) break;
        }
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink); gst_object_unref(pipeline);
}

// --- 2. 解码与界面显示 (主线程) ---
void run_decoder_and_ui() {
    GError *error = nullptr;

    std::string pipe_str;
#if USE_H265
    pipe_str = "appsrc name=dec_src caps=\"video/x-h265,stream-format=byte-stream\" format=time is-live=true do-timestamp=true ! "
               "h265parse config-interval=-1 ! avdec_h265 max-threads=4 ! ";
#else
    pipe_str = "appsrc name=dec_src caps=\"video/x-h264,stream-format=byte-stream\" format=time is-live=true do-timestamp=true ! "
               "h264parse ! avdec_h264 max-threads=4 ! ";
#endif

    pipe_str += "videoconvert ! video/x-raw,format=BGR ! ";
    pipe_str += "appsink name=dec_sink max-buffers=0 drop=false sync=false emit-signals=false";

    GstElement *pipeline = gst_parse_launch(pipe_str.c_str(), &error);
    if (error) {
        std::cerr << "[解码器] 管道创建失败: " << error->message << std::endl;
        g_clear_error(&error); stop_flag = true; q_cv.notify_all(); return;
    }

    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "dec_src");
    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "dec_sink");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    std::thread feeder_thread([&]() {
        while (!stop_flag) {
            GstBuffer *buf = nullptr;
            {
                std::unique_lock<std::mutex> lock(q_mutex);
                q_cv.wait_for(lock, std::chrono::milliseconds(100), []{ return !packet_queue.empty() || stop_flag; });
                if (stop_flag && packet_queue.empty()) break;
                if (!packet_queue.empty()) {
                    buf = packet_queue.front();
                    packet_queue.pop();
                    q_cv.notify_all();
                }
            }
            if (buf) {
                bytes_received += gst_buffer_get_size(buf);
                gst_app_src_push_buffer(GST_APP_SRC(appsrc), buf);
            }
        }
        gst_app_src_end_of_stream(GST_APP_SRC(appsrc));
    });

    double current_bitrate = 0.0, current_fps = 0.0;
    int frame_count = 0;
    auto last_stat_time = std::chrono::steady_clock::now();
    int frame_w = 564, frame_h = 318;
    
    std::string win_name = "App-Level Jitter Buffer";
    cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE);

    // =======================================================
    // 【核心架构】：Jitter Buffer 蓄水池及匀速播放独立时间轴
    // =======================================================
    std::deque<cv::Mat> display_buffer;
    bool is_buffering = (BUFFER_FRAMES > 0);
    
    auto next_play_time = std::chrono::steady_clock::now();
    const auto frame_duration = std::chrono::milliseconds(33); // 强制控制按 30FPS 匀速播放

    while (!stop_flag) {
        // 1. 尝试收取解码后的最新画面 (超时极短，5ms不阻塞主循环)
        GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), 5 * GST_MSECOND);
        if (sample) {
            frame_count++;
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = now - last_stat_time;
            if (diff.count() >= 1.0) {
                current_bitrate = (bytes_received.exchange(0) * 8.0) / 1024.0 / diff.count(); 
                current_fps = frame_count / diff.count();
                frame_count = 0; last_stat_time = now;
            }

            GstCaps *caps = gst_sample_get_caps(sample);
            gst_structure_get_int(gst_caps_get_structure(caps, 0), "width", &frame_w);
            gst_structure_get_int(gst_caps_get_structure(caps, 0), "height", &frame_h);

            GstBuffer *buffer = gst_sample_get_buffer(sample);
            GstMapInfo map;
            gst_buffer_map(buffer, &map, GST_MAP_READ);
            cv::Mat frame(frame_h, frame_w, CV_8UC3, (void*)map.data, cv::Mat::AUTO_STEP);
            
            // 【进水管】无脑将最新画面推入队列
            display_buffer.push_back(frame.clone());
            gst_buffer_unmap(buffer, &map);
            gst_sample_unref(sample);

            // 【防积压安全阀】防止网络恢复时瞬时涌入过量画面导致延迟无限增加
            if (display_buffer.size() > BUFFER_FRAMES + 10) {
                while (display_buffer.size() > BUFFER_FRAMES + 5) {
                    display_buffer.pop_front(); // 强制丢弃太老的帧追回延迟
                }
            }
        }

        // 2. 状态机：判断今天该存货，还是该放货
        if (is_buffering) {
            if (display_buffer.size() >= BUFFER_FRAMES) {
                is_buffering = false; // 水位达标，停止缓冲黑屏
                next_play_time = std::chrono::steady_clock::now(); // 重置播放沙漏，准备发车
            }
        } else {
            if (display_buffer.empty() && BUFFER_FRAMES > 0) {
                is_buffering = true; // 发生严重卡顿，水池被抽干了，强行退回缓冲状态
            }
        }

        // 3. 【出水管】不依赖解码速度，严格按真实时间(30帧/秒) 匀速向屏幕输出历史积压画面
        auto now = std::chrono::steady_clock::now();
        if (is_buffering && now >= next_play_time) {
            // 画缓冲进度条
            cv::Mat loading_frame = cv::Mat::zeros(frame_h, frame_w, CV_8UC3);
            char text[128];
            snprintf(text, sizeof(text), "Buffering... %zu / %zu", display_buffer.size(), BUFFER_FRAMES);
            cv::putText(loading_frame, text, cv::Point(frame_w/2 - 120, frame_h/2), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
            cv::imshow(win_name, loading_frame);
            next_play_time = now + frame_duration;
            
        } else if (!is_buffering) {
            bool showed_frame = false;
            cv::Mat frame_to_show;
            
            // 用 while 是为了如果 OpenCV 处理变慢了，能自动连抽多帧快进补齐时间戳
            while (now >= next_play_time && !display_buffer.empty()) {
                frame_to_show = display_buffer.front();
                display_buffer.pop_front();
                next_play_time += frame_duration;
                showed_frame = true;
            }
            
            if (showed_frame) {
                char text[128];
                snprintf(text, sizeof(text), "FPS: %.1f | Bitrate: %.1f kbps", current_fps, current_bitrate);
                cv::putText(frame_to_show, text, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 3);
                cv::putText(frame_to_show, text, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 1);
                
                snprintf(text, sizeof(text), "Delay: %zu Frames (%.1f sec)", BUFFER_FRAMES, BUFFER_FRAMES / 30.0);
                cv::putText(frame_to_show, text, cv::Point(15, 60), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 3);
                cv::putText(frame_to_show, text, cv::Point(15, 60), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 1);

                cv::imshow(win_name, frame_to_show);
            }
        }
        
        int key = cv::waitKey(1);
        if (key == 'q' || key == 27 || cv::getWindowProperty(win_name, cv::WND_PROP_AUTOSIZE) < 0) {
            stop_flag = true; q_cv.notify_all();
        }
    }

    feeder_thread.join();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsrc); gst_object_unref(appsink); gst_object_unref(pipeline);
    cv::destroyAllWindows();
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    std::cout << "正在启动，目前应用层强制延迟设定为: " << BUFFER_FRAMES << " 帧" << std::endl;

    int STREAM_WIDTH = 564;
    int STREAM_HEIGHT = 318;
    
    std::thread enc_thread(encoder_thread, STREAM_WIDTH, STREAM_HEIGHT);
    run_decoder_and_ui(); 
    enc_thread.join();

    {
        std::unique_lock<std::mutex> lock(q_mutex);
        while (!packet_queue.empty()) {
            GstBuffer* b = packet_queue.front();
            gst_buffer_unref(b);
            packet_queue.pop();
        }
    }
    std::cout << "测试完全结束。" << std::endl;
    return 0;
}