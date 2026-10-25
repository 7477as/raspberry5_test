#pragma once

#include <gst/gst.h>

#include <mutex>
#include <string>
#include <vector>

struct AecConfig {
    bool echoCancel = true;
    bool noiseSuppression = true;
    bool gainControl = true;
    bool highPassFilter = true;
    std::string playbackDevice = "hw:0,0";
    std::string captureDevice = "hw:0,0";
};

// 四路结果：两路原始麦克风、参考信号、算法输出
struct AecCaptureResult {
    std::vector<float> mic0;
    std::vector<float> mic1;
    std::vector<float> reference;
    std::vector<float> processed;
};

// 探针跑在各自分支的流线程上，累积样本需要加锁，取走时再解交织成逐通道
class ProbeContext {
public:
    void SetChannelCount(int channels);
    void AppendInterleaved(const float* samples, size_t sampleCount);
    void Deinterleave(std::vector<std::vector<float>>& outChannels) const;
    void Reset();

private:
    mutable std::mutex mMutex;
    int mChannels = 1;
    std::vector<float> mInterleaved;
};

// 实时硬件回环：参考音频经扬声器播放制造真实回声，双麦采集后 MIC0 进 webrtcdsp。
// 因为 webrtcdsp 只处理通道 0，MIC1 仅录音留档、不参与算法。
class AecPipeline {
public:
    AecPipeline();
    ~AecPipeline();

    AecPipeline(const AecPipeline&) = delete;
    AecPipeline& operator=(const AecPipeline&) = delete;

    bool Initialize(const AecConfig& config, const std::string& referencePath, int sampleRate,
                    std::string& error);

    // durationSeconds <= 0 表示按参考音频时长自动录制；收到 EOS 也会提前收尾
    bool RunForDuration(double durationSeconds, AecCaptureResult& result, std::string& error);

    // Ctrl+C 时调用，让录制提前收尾并保存已有数据
    static void RequestStop();

private:
    static GstPadProbeReturn OnReferencePadProbe(GstPad* pad, GstPadProbeInfo* info,
                                                 gpointer userData);
    static GstPadProbeReturn OnMicPadProbe(GstPad* pad, GstPadProbeInfo* info, gpointer userData);
    static GstPadProbeReturn OnOutputPadProbe(GstPad* pad, GstPadProbeInfo* info, gpointer userData);

    double QueryReferenceDurationSeconds();
    void Teardown();

    GstElement* mPipeline = nullptr;
    GstElement* mFileSrc = nullptr;
    gulong mReferenceProbeId = 0;
    gulong mMicProbeId = 0;
    gulong mOutputProbeId = 0;

    ProbeContext mReferenceContext;
    ProbeContext mMicContext;
    ProbeContext mOutputContext;
    int mSampleRate = 16000;
    AecConfig mConfig;
};