#include <gst/gst.h>
#include <iostream>
#include <string>
#include <thread>

// GStreamer 消息循环
GMainLoop *loop;

void bus_thread_func() {
    g_main_loop_run(loop);
}

int main(int argc, char *argv[]) {
    // 1. 初始化 GStreamer
    gst_init(&argc, &argv);

    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <mp4文件绝对/相对路径>\n";
        return -1;
    }

    // 2. 创建 playbin，它会自动处理音视频分离、解码、同步和渲染输出
    GstElement *pipeline = gst_element_factory_make("playbin", "playbin");
    if (!pipeline) {
        std::cerr << "创建 playbin 失败！\n";
        return -1;
    }

    // 将本地文件路径转为 GStreamer 需要的 URI 格式
    gchar *uri = gst_filename_to_uri(argv[1], NULL);
    g_object_set(pipeline, "uri", uri, NULL);
    g_free(uri);

    // 3. 启动 GLib 消息循环 (放在独立线程，保证底层事件和音视频时钟正常运转)
    loop = g_main_loop_new(NULL, FALSE);
    std::thread bus_thread(bus_thread_func);

    // 4. 开始播放
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "正在启动播放器...\n";

    // 5. 终端交互状态机 (模拟 UI 操作)
    std::string cmd;
    while (true) {
        std::cout << "\n[操作菜单] p: 暂停 | r: 恢复播放 | s <秒数>: Seek | q: 退出\n> ";
        std::cin >> cmd;

        if (cmd == "q") {
            break;
        } 
        else if (cmd == "p") {
            // 切入 PAUSED 状态，画面定格
            gst_element_set_state(pipeline, GST_STATE_PAUSED);
            std::cout << "状态: 已暂停 (此时执行 Seek 即可观察到封面帧刷新)。\n";
        } 
        else if (cmd == "r") {
            // 恢复 PLAYING，音视频继续同步播放
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
            std::cout << "状态: 正在播放...\n";
        } 
        else if (cmd == "s") {
            double target_sec;
            std::cin >> target_sec;
            
            // 将秒换算为纳秒
            gint64 target_ns = (gint64)(target_sec * GST_SECOND);
            
            // 【核心机制】：FLUSH (清空旧数据) + KEY_UNIT (精准定位到关键帧)
            bool ret = gst_element_seek_simple(pipeline, GST_FORMAT_TIME,
                        (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | 
                                    GST_SEEK_FLAG_KEY_UNIT | 
                                    GST_SEEK_FLAG_SNAP_BEFORE), // 强制向前对齐到最近的完整关键帧
                        target_ns);
                                    
            if (ret) {
                std::cout << "=> 成功 Seek 到 " << target_sec << " 秒处。\n";
            } else {
                std::cerr << "=> Seek 失败！可能是文件无索引(未做FastStart)或尚未初始化完成。\n";
            }
        }
    }

    // 6. 退出前的安全清理
    std::cout << "正在清理资源...\n";
    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_main_loop_quit(loop);
    bus_thread.join();
    g_main_loop_unref(loop);
    gst_object_unref(pipeline);

    return 0;
}