#include "AecPipeline.h"

#include <algorithm>
#include <csignal>
#include <iostream>
#include <sstream>
#include <string>

namespace {

constexpr int kReferenceChannels = 1;
constexpr int kMicChannels = 2;
constexpr int kBusPollIntervalMs = 200;
constexpr int kDurationQueryAttempts = 20;
constexpr double kFallbackDurationSeconds = 10.0;

volatile sig_atomic_t g_stopRequested = 0;

// S16LE -> float，按交错顺序写入探针缓冲
void CaptureS16Interleaved(GstBuffer* buffer, ProbeContext& context) {
    GstMapInfo mapInfo {};
    if (buffer == nullptr || !gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
        return;
    }

    const int16_t* rawSamples = reinterpret_cast<const int16_t*>(mapInfo.data);
    const size_t sampleCount = mapInfo.size / sizeof(int16_t);
    std::vector<float> converted(sampleCount);
    for (size_t index = 0; index < sampleCount; ++index) {
        converted[index] = static_cast<float>(rawSamples[index]) / 32768.0f;
    }
    gst_buffer_unmap(buffer, &mapInfo);

    context.AppendInterleaved(converted.data(), sampleCount);
}

std::string EscapeForParseLaunch(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size() + 8);
    for (char character : text) {
        if (character == '"' || character == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(character);
    }
    return escaped;
}

std::string BuildCapsDescription(int sampleRate, int channels) {
    std::ostringstream caps;
    caps << "audio/x-raw,format=S16LE,layout=interleaved,rate=" << sampleRate
         << ",channels=" << channels;
    return caps.str();
}

std::string FormatBusError(GstMessage* message) {
    GError* gstError = nullptr;
    gchar* debugInfo = nullptr;
    gst_message_parse_error(message, &gstError, &debugInfo);
    std::string text = gstError != nullptr ? gstError->message : "未知错误";
    if (gstError != nullptr) {
        g_error_free(gstError);
    }
    g_free(debugInfo);
    return text;
}

// 管线串成两条分支：参考 -> 扬声器（制造回声），双麦 -> webrtcdsp -> 丢弃。
// mic 侧 caps 必须是 2 通道，否则 MIC1 会在下混时丢失。
std::string BuildPipelineDescription(const AecConfig& config, const std::string& referencePath,
                                     int sampleRate) {
    const std::string referenceCaps = BuildCapsDescription(sampleRate, kReferenceChannels);
    const std::string micCaps = BuildCapsDescription(sampleRate, kMicChannels);
    const std::string escapedPath = EscapeForParseLaunch(referencePath);
    const std::string playbackDevice = EscapeForParseLaunch(config.playbackDevice);
    const std::string captureDevice = EscapeForParseLaunch(config.captureDevice);

    std::ostringstream desc;
    desc << "filesrc name=refsrc location=\"" << escapedPath << "\" ! wavparse ! audioconvert ! "
         << "audioresample ! capsfilter name=refcap caps=\"" << referenceCaps << "\" ! "
         << "queue name=refq ! webrtcechoprobe name=probe ! audioconvert ! audioresample ! "
         << "alsasink device=\"" << playbackDevice << "\" sync=true "
         << "alsasrc device=\"" << captureDevice << "\" ! audioconvert ! audioresample ! "
         << "capsfilter name=miccap caps=\"" << micCaps << "\" ! queue name=micq ! "
         << "webrtcdsp name=dsp probe=probe "
         << "echo-cancel=" << (config.echoCancel ? "true" : "false") << " "
         << "noise-suppression=" << (config.noiseSuppression ? "true" : "false") << " "
         << "gain-control=" << (config.gainControl ? "true" : "false") << " "
         << "high-pass-filter=" << (config.highPassFilter ? "true" : "false") << " ! "
         << "fakesink sync=false";
    return desc.str();
}

}  // namespace

void ProbeContext::SetChannelCount(int channels) {
    std::lock_guard<std::mutex> guard(mMutex);
    mChannels = channels > 0 ? channels : 1;
}

void ProbeContext::AppendInterleaved(const float* samples, size_t sampleCount) {
    if (samples == nullptr || sampleCount == 0) {
        return;
    }
    std::lock_guard<std::mutex> guard(mMutex);
    mInterleaved.insert(mInterleaved.end(), samples, samples + sampleCount);
}

void ProbeContext::Deinterleave(std::vector<std::vector<float>>& outChannels) const {
    std::lock_guard<std::mutex> guard(mMutex);
    outChannels.assign(static_cast<size_t>(mChannels), std::vector<float>());
    const size_t frameCount =
        static_cast<size_t>(mChannels) > 0 ? mInterleaved.size() / static_cast<size_t>(mChannels) : 0;
    for (int channel = 0; channel < mChannels; ++channel) {
        outChannels[static_cast<size_t>(channel)].resize(frameCount);
        for (size_t frame = 0; frame < frameCount; ++frame) {
            const size_t index =
                frame * static_cast<size_t>(mChannels) + static_cast<size_t>(channel);
            outChannels[static_cast<size_t>(channel)][frame] = mInterleaved[index];
        }
    }
}

void ProbeContext::Reset() {
    std::lock_guard<std::mutex> guard(mMutex);
    mInterleaved.clear();
}

AecPipeline::AecPipeline() = default;

AecPipeline::~AecPipeline() {
    Teardown();
}

void AecPipeline::RequestStop() {
    g_stopRequested = 1;
}

bool AecPipeline::Initialize(const AecConfig& config, const std::string& referencePath,
                             int sampleRate, std::string& error) {
    if (sampleRate != 8000 && sampleRate != 16000 && sampleRate != 32000 && sampleRate != 48000) {
        error = "webrtcdsp 仅支持 8/16/32/48 kHz，当前请求: " + std::to_string(sampleRate);
        return false;
    }
    if (referencePath.empty()) {
        error = "参考音频路径为空";
        return false;
    }

    Teardown();
    mConfig = config;
    mSampleRate = sampleRate;

    mReferenceContext.SetChannelCount(kReferenceChannels);
    mMicContext.SetChannelCount(kMicChannels);
    mOutputContext.SetChannelCount(kMicChannels);

    GError* parseError = nullptr;
    GstElement* pipeline = gst_parse_launch(
        BuildPipelineDescription(config, referencePath, sampleRate).c_str(), &parseError);
    if (pipeline == nullptr) {
        error = std::string("解析管线失败: ") +
                (parseError != nullptr ? parseError->message : "未知错误");
        if (parseError != nullptr) {
            g_error_free(parseError);
        }
        return false;
    }

    mFileSrc = gst_bin_get_by_name(GST_BIN(pipeline), "refsrc");
    GstElement* echoprobe = gst_bin_get_by_name(GST_BIN(pipeline), "probe");
    GstElement* micCaps = gst_bin_get_by_name(GST_BIN(pipeline), "miccap");
    GstElement* dsp = gst_bin_get_by_name(GST_BIN(pipeline), "dsp");

    if (mFileSrc == nullptr || echoprobe == nullptr || micCaps == nullptr || dsp == nullptr) {
        error = "管线缺少必要元素（refsrc/probe/miccap/dsp）";
        gst_object_unref(pipeline);
        mFileSrc = nullptr;
        return false;
    }

    // 参考在送进 probe 之前抓，双麦在进 dsp 之前抓（算法看到的输入），算法输出抓 dsp.src 的 ch0
    GstPad* referenceTapPad = gst_element_get_static_pad(echoprobe, "src");
    GstPad* micTapPad = gst_element_get_static_pad(micCaps, "src");
    GstPad* outputTapPad = gst_element_get_static_pad(dsp, "src");
    mReferenceProbeId = gst_pad_add_probe(referenceTapPad, GST_PAD_PROBE_TYPE_BUFFER,
                                          &AecPipeline::OnReferencePadProbe, &mReferenceContext,
                                          nullptr);
    mMicProbeId = gst_pad_add_probe(micTapPad, GST_PAD_PROBE_TYPE_BUFFER,
                                    &AecPipeline::OnMicPadProbe, &mMicContext, nullptr);
    mOutputProbeId = gst_pad_add_probe(outputTapPad, GST_PAD_PROBE_TYPE_BUFFER,
                                       &AecPipeline::OnOutputPadProbe, &mOutputContext, nullptr);
    gst_object_unref(referenceTapPad);
    gst_object_unref(micTapPad);
    gst_object_unref(outputTapPad);
    gst_object_unref(echoprobe);
    gst_object_unref(micCaps);
    gst_object_unref(dsp);

    mPipeline = pipeline;
    return true;
}

double AecPipeline::QueryReferenceDurationSeconds() {
    // filesrc 要进入 PLAYING 打开文件头后才知道时长，这里轮询一小段时间
    for (int attempt = 0; attempt < kDurationQueryAttempts && mFileSrc != nullptr; ++attempt) {
        gint64 duration = 0;
        if (gst_element_query_duration(mFileSrc, GST_FORMAT_TIME, &duration) && duration > 0) {
            return static_cast<double>(duration) / static_cast<double>(GST_SECOND);
        }
        g_usleep(50 * 1000);
    }
    return 0.0;
}

bool AecPipeline::RunForDuration(double durationSeconds, AecCaptureResult& result,
                                 std::string& error) {
    if (mPipeline == nullptr) {
        error = "管线尚未初始化";
        return false;
    }

    mReferenceContext.Reset();
    mMicContext.Reset();
    mOutputContext.Reset();

    GstBus* bus = gst_element_get_bus(mPipeline);
    const gint64 startUs = g_get_monotonic_time();
    gst_element_set_state(mPipeline, GST_STATE_PLAYING);

    double effectiveDuration = durationSeconds;
    if (effectiveDuration <= 0.0) {
        effectiveDuration = QueryReferenceDurationSeconds();
        if (effectiveDuration <= 0.0) {
            effectiveDuration = kFallbackDurationSeconds;
        }
    }
    const gint64 deadlineUs =
        startUs + static_cast<gint64>(effectiveDuration * static_cast<double>(G_USEC_PER_SEC));

    bool interrupted = false;
    bool failed = false;
    while (!failed) {
        if (g_stopRequested) {
            interrupted = true;
            break;
        }
        if (g_get_monotonic_time() >= deadlineUs) {
            break;
        }

        GstMessage* message = gst_bus_timed_pop_filtered(
            bus, static_cast<guint64>(kBusPollIntervalMs) * GST_MSECOND,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (message == nullptr) {
            continue;
        }

        const GstMessageType messageType = GST_MESSAGE_TYPE(message);
        if (messageType == GST_MESSAGE_ERROR) {
            error = "管线报错: " + FormatBusError(message);
            failed = true;
        }
        gst_message_unref(message);
        if (messageType == GST_MESSAGE_EOS) {
            break;
        }
    }

    gst_element_set_state(mPipeline, GST_STATE_NULL);
    gst_object_unref(bus);

    if (failed) {
        return false;
    }

    std::vector<std::vector<float>> referenceChannels;
    std::vector<std::vector<float>> micChannels;
    std::vector<std::vector<float>> outputChannels;
    mReferenceContext.Deinterleave(referenceChannels);
    mMicContext.Deinterleave(micChannels);
    mOutputContext.Deinterleave(outputChannels);

    result.reference = referenceChannels.empty() ? std::vector<float>() : referenceChannels[0];
    result.mic0 = micChannels.size() > 0 ? micChannels[0] : std::vector<float>();
    result.mic1 = micChannels.size() > 1 ? micChannels[1] : std::vector<float>();
    result.processed = outputChannels.empty() ? std::vector<float>() : outputChannels[0];

    const size_t frames =
        std::min(std::min(result.mic0.size(), result.mic1.size()),
                 std::min(result.reference.size(), result.processed.size()));
    result.mic0.resize(frames);
    result.mic1.resize(frames);
    result.reference.resize(frames);
    result.processed.resize(frames);

    if (frames == 0) {
        error = "没有采集到音频数据，请检查参考文件、ALSA 设备名与扬声器/麦克风接线";
        return false;
    }

    std::cout << "采集完成" << (interrupted ? "（Ctrl+C 提前收尾）" : "") << ": 四路各 "
              << frames << " 帧\n";
    return true;
}

void AecPipeline::Teardown() {
    if (mPipeline != nullptr) {
        gst_element_set_state(mPipeline, GST_STATE_NULL);
        gst_object_unref(mPipeline);
        mPipeline = nullptr;
    }
    if (mFileSrc != nullptr) {
        gst_object_unref(mFileSrc);
        mFileSrc = nullptr;
    }
    mReferenceProbeId = 0;
    mMicProbeId = 0;
    mOutputProbeId = 0;
}

GstPadProbeReturn AecPipeline::OnReferencePadProbe(GstPad*, GstPadProbeInfo* info, gpointer userData) {
    auto* context = static_cast<ProbeContext*>(userData);
    if (context != nullptr && info != nullptr) {
        CaptureS16Interleaved(gst_pad_probe_info_get_buffer(info), *context);
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn AecPipeline::OnMicPadProbe(GstPad*, GstPadProbeInfo* info, gpointer userData) {
    auto* context = static_cast<ProbeContext*>(userData);
    if (context != nullptr && info != nullptr) {
        CaptureS16Interleaved(gst_pad_probe_info_get_buffer(info), *context);
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn AecPipeline::OnOutputPadProbe(GstPad*, GstPadProbeInfo* info, gpointer userData) {
    auto* context = static_cast<ProbeContext*>(userData);
    if (context != nullptr && info != nullptr) {
        CaptureS16Interleaved(gst_pad_probe_info_get_buffer(info), *context);
    }
    return GST_PAD_PROBE_OK;
}