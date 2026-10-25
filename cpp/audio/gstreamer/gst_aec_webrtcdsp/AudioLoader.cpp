#include "AudioLoader.h"

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <sys/stat.h>

namespace {

bool FileExists(const std::string& path) {
    struct stat info {};
    return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

// 在管线运行期间持续抽取样本，拿到 maxFrames 就提前返回；返回 true 表示出错
bool DrainAppSink(GstElement* appSink, GstBus* bus, int targetRate, int maxFrames,
                  MonoAudio& out, std::string& error) {
    while (true) {
        if (maxFrames > 0 && static_cast<int>(out.samples.size()) >= maxFrames) {
            break;
        }

        GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appSink), 5 * GST_SECOND);
        if (sample == nullptr) {
            // 抽不到样本时确认是 EOS 还是 ERROR
            GstMessage* message = gst_bus_pop_filtered(bus, static_cast<GstMessageType>(
                                                            GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
            if (message == nullptr) {
                // 部分 wavparse 流不会把 EOS 传到 appsink：已经拿到数据就按读完处理
                if (!out.samples.empty()) {
                    break;
                }
                error = "解码超时（5s 内无数据）";
                return true;
            }
            if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
                GError* gstError = nullptr;
                gchar* debugInfo = nullptr;
                gst_message_parse_error(message, &gstError, &debugInfo);
                error = "解码失败: " +
                        std::string(gstError != nullptr ? gstError->message : "未知错误");
                if (gstError != nullptr) {
                    g_error_free(gstError);
                }
                g_free(debugInfo);
                gst_message_unref(message);
                return true;
            }
            gst_message_unref(message);
            break;
        }

        GstBuffer* buffer = gst_sample_get_buffer(sample);
        GstMapInfo mapInfo {};
        if (buffer != nullptr && gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
            const int frameCount = static_cast<int>(mapInfo.size) / sizeof(float);
            const int usable = maxFrames > 0
                                   ? std::min(maxFrames - static_cast<int>(out.samples.size()), frameCount)
                                   : frameCount;
            if (usable > 0) {
                const float* begin = reinterpret_cast<const float*>(mapInfo.data);
                out.samples.insert(out.samples.end(), begin, begin + usable);
            }
            gst_buffer_unmap(buffer, &mapInfo);
        }

        gst_sample_unref(sample);
    }

    out.sampleRate = targetRate;
    return false;
}

std::string BuildCapsDescription(int targetRate) {
    std::ostringstream caps;
    caps << "audio/x-raw,format=F32LE,layout=interleaved,rate=" << targetRate
         << ",channels=1";
    return caps.str();
}

}  // namespace

bool AudioLoader::LoadAsMono(const std::string& path, int targetRate, double maxSeconds,
                             MonoAudio& out, std::string& error) {
    out.samples.clear();
    out.sampleRate = 0;

    if (!FileExists(path)) {
        error = "找不到音频文件: " + path;
        return false;
    }
    if (targetRate != 8000 && targetRate != 16000 && targetRate != 32000 && targetRate != 48000) {
        error = "webrtcdsp 仅支持 8/16/32/48 kHz，当前请求: " + std::to_string(targetRate);
        return false;
    }

    GstElement* pipeline = gst_pipeline_new(nullptr);
    GstElement* fileSrc = gst_element_factory_make("filesrc", "filesrc");
    GstElement* wavParse = gst_element_factory_make("wavparse", "wavparse");
    GstElement* audioConvert = gst_element_factory_make("audioconvert", "audioconvert");
    GstElement* audioResample = gst_element_factory_make("audioresample", "audioresample");
    GstElement* capsFilter = gst_element_factory_make("capsfilter", "capsfilter");
    GstElement* appSink = gst_element_factory_make("appsink", "appsink");

    if (pipeline == nullptr || fileSrc == nullptr || wavParse == nullptr ||
        audioConvert == nullptr || audioResample == nullptr || capsFilter == nullptr ||
        appSink == nullptr) {
        error = "创建解码管线元素失败，请确认已安装 gstreamer1.0-plugins-good";
        if (pipeline != nullptr) {
            gst_object_unref(pipeline);
        }
        return false;
    }

    // 直接用文件系统路径而非 file:// URI：本机 filesrc 会把 URI 当字面文件名而找不到
    g_object_set(fileSrc, "location", path.c_str(), nullptr);

    GstCaps* caps = gst_caps_from_string(BuildCapsDescription(targetRate).c_str());
    g_object_set(capsFilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    g_object_set(appSink, "sync", FALSE, nullptr);
    g_object_set(appSink, "max-buffers", 0, nullptr);
    g_object_set(appSink, "drop", FALSE, nullptr);
    gst_app_sink_set_emit_signals(GST_APP_SINK(appSink), FALSE);

    gst_bin_add_many(GST_BIN(pipeline), fileSrc, wavParse, audioConvert, audioResample,
                     capsFilter, appSink, nullptr);
    if (!gst_element_link_many(fileSrc, wavParse, audioConvert, audioResample, capsFilter,
                               appSink, nullptr)) {
        error = "链接解码管线失败: filesrc ! wavparse ! audioconvert ! audioresample ! caps ! appsink";
        gst_object_unref(pipeline);
        return false;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // 边跑边抽：截取场景下拿够 maxFrames 就停，否则会白解完整个长文件
    GstBus* bus = gst_element_get_bus(pipeline);
    const int maxFrames = maxSeconds > 0 ? static_cast<int>(maxSeconds * targetRate) : 0;
    bool hasError = DrainAppSink(appSink, bus, targetRate, maxFrames, out, error);
    gst_object_unref(bus);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    if (!hasError && out.samples.empty()) {
        error = "解码后没有音频样本";
        return false;
    }
    return !hasError;
}