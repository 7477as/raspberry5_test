#include "VideoHal.h"

#include <algorithm>
#include <cstring>
#include <thread>
#include <vector>

#include <curl/curl.h>

VideoHal::~VideoHal() {
    if (mPacket)      av_packet_free(&mPacket);
    if (mFrame)       av_frame_free(&mFrame);
    if (mCodecCtx)    avcodec_free_context(&mCodecCtx);
    if (mTexture)     SDL_DestroyTexture(mTexture);
    if (mRenderer)    SDL_DestroyRenderer(mRenderer);
    if (mWindow)      SDL_DestroyWindow(mWindow);
}

bool VideoHal::Init(const std::vector<uint8_t>& spsPps) {
    mSpsPpsCache = spsPps;

    mCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!mCodec) return false;

    mCodecCtx = avcodec_alloc_context3(mCodec);
    if (avcodec_open2(mCodecCtx, mCodec, nullptr) < 0) return false;

    mPacket = av_packet_alloc();
    mFrame  = av_frame_alloc();

    mWindow   = SDL_CreateWindow(
        "RTOS Player Simulator (A/V Sync)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN);
    mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED);
    return true;
}

bool VideoHal::DecodeAndRender(std::vector<uint8_t>& naluData) {
    if (naluData.empty()) return false;

    size_t offset = 0;
    while (offset + 4 <= naluData.size()) {
        uint32_t naluLen =
            (uint32_t(naluData[offset]) << 24) |
            (uint32_t(naluData[offset + 1]) << 16) |
            (uint32_t(naluData[offset + 2]) << 8) |
             uint32_t(naluData[offset + 3]);
        if (offset + 4 + naluLen > naluData.size()) break;
        naluData[offset]     = 0x00;
        naluData[offset + 1] = 0x00;
        naluData[offset + 2] = 0x00;
        naluData[offset + 3] = 0x01;
        offset += 4 + naluLen;
    }

    if (!mSpsPpsCache.empty()) {
        naluData.insert(naluData.begin(), mSpsPpsCache.begin(), mSpsPpsCache.end());
    }

    mPacket->data = naluData.data();
    mPacket->size = naluData.size();

    bool gotPicture = false;
    if (avcodec_send_packet(mCodecCtx, mPacket) == 0) {
        while (avcodec_receive_frame(mCodecCtx, mFrame) == 0) {
            if (!mTextureInited) {
                mTexture = SDL_CreateTexture(
                    mRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
                    mFrame->width, mFrame->height);
                SDL_SetWindowSize(mWindow, mFrame->width, mFrame->height);
                mTextureInited = true;
            }
            SDL_UpdateYUVTexture(mTexture, nullptr,
                                 mFrame->data[0], mFrame->linesize[0],
                                 mFrame->data[1], mFrame->linesize[1],
                                 mFrame->data[2], mFrame->linesize[2]);
            SDL_RenderClear(mRenderer);
            SDL_RenderCopy(mRenderer, mTexture, nullptr, nullptr);
            SDL_RenderPresent(mRenderer);
            gotPicture = true;
        }
    }
    return gotPicture;
}

void VideoHal::Flush() {
    if (mCodecCtx) avcodec_flush_buffers(mCodecCtx);
}

AudioHal::~AudioHal() {
    if (mAudioDev) {
        SDL_PauseAudioDevice(mAudioDev, 1);
        SDL_CloseAudioDevice(mAudioDev);
    }
    if (mSwrCtx)    swr_free(&mSwrCtx);
    if (mPacket)    av_packet_free(&mPacket);
    if (mFrame)     av_frame_free(&mFrame);
    if (mCodecCtx)  avcodec_free_context(&mCodecCtx);
}

bool AudioHal::Init(const uint8_t* extraData, unsigned int extraDataSize) {
    mCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!mCodec) return false;

    mCodecCtx = avcodec_alloc_context3(mCodec);
    if (extraData && extraDataSize > 0) {
        mCodecCtx->extradata =
            static_cast<uint8_t*>(av_mallocz(extraDataSize + AV_INPUT_BUFFER_PADDING_SIZE));
        std::memcpy(mCodecCtx->extradata, extraData, extraDataSize);
        mCodecCtx->extradata_size = extraDataSize;
    }
    if (avcodec_open2(mCodecCtx, mCodec, nullptr) < 0) return false;

    mPacket = av_packet_alloc();
    mFrame  = av_frame_alloc();

    SDL_AudioSpec wantedSpec{};
    wantedSpec.freq     = mTargetSampleRate;
    wantedSpec.format   = AUDIO_S16SYS;
    wantedSpec.channels = mTargetChannels;
    wantedSpec.samples  = 1024;
    wantedSpec.callback = nullptr;

    SDL_AudioSpec obtainedSpec;
    mAudioDev = SDL_OpenAudioDevice(nullptr, 0, &wantedSpec, &obtainedSpec, 0);
    if (mAudioDev == 0) return false;

    SDL_PauseAudioDevice(mAudioDev, 1);
    return true;
}

void AudioHal::DecodeAndQueue(const std::vector<uint8_t>& aacData, double ptsSec) {
    if (aacData.empty()) return;

    mPacket->data = const_cast<uint8_t*>(aacData.data());
    mPacket->size = aacData.size();

    if (avcodec_send_packet(mCodecCtx, mPacket) == 0) {
        while (avcodec_receive_frame(mCodecCtx, mFrame) == 0) {
            if (!mSwrCtx) {
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
                AVChannelLayout outChLayout;
                av_channel_layout_default(&outChLayout, mTargetChannels);
                swr_alloc_set_opts2(&mSwrCtx,
                                    &outChLayout, AV_SAMPLE_FMT_S16, mTargetSampleRate,
                                    &mFrame->ch_layout,
                                    static_cast<AVSampleFormat>(mFrame->format),
                                    mFrame->sample_rate, 0, nullptr);
#else
                uint64_t inChLayout = mFrame->channel_layout
                    ? mFrame->channel_layout
                    : av_get_default_channel_layout(mFrame->channels);
                mSwrCtx = swr_alloc_set_opts(
                    nullptr,
                    av_get_default_channel_layout(mTargetChannels),
                    AV_SAMPLE_FMT_S16, mTargetSampleRate,
                    inChLayout,
                    static_cast<AVSampleFormat>(mFrame->format),
                    mFrame->sample_rate, 0, nullptr);
#endif
                swr_init(mSwrCtx);
            }

            const int dstSamples = av_rescale_rnd(
                swr_get_delay(mSwrCtx, mFrame->sample_rate) + mFrame->nb_samples,
                mTargetSampleRate, mFrame->sample_rate, AV_ROUND_UP);

            std::vector<uint8_t> pcmBuffer(dstSamples * mTargetChannels * 2);
            uint8_t* outPtr[1] = { pcmBuffer.data() };

            const int retSamples = swr_convert(
                mSwrCtx, outPtr, dstSamples,
                const_cast<const uint8_t**>(mFrame->data), mFrame->nb_samples);

            if (retSamples > 0) {
                const int pcmBytes = retSamples * mTargetChannels * 2;
                SDL_QueueAudio(mAudioDev, pcmBuffer.data(), pcmBytes);

                if (SDL_GetQueuedAudioSize(mAudioDev) == static_cast<Uint32>(pcmBytes)) {
                    mCurrentAudioPts = ptsSec + static_cast<double>(retSamples) / mTargetSampleRate;
                } else {
                    mCurrentAudioPts += static_cast<double>(retSamples) / mTargetSampleRate;
                }
            }
        }
    }
}

double AudioHal::GetMasterClock() {
    if (mAudioDev == 0) return 0.0;
    const int queuedBytes = SDL_GetQueuedAudioSize(mAudioDev);
    const double queuedSeconds =
        static_cast<double>(queuedBytes) / (mTargetSampleRate * mTargetChannels * 2);
    return mCurrentAudioPts - queuedSeconds;
}

void AudioHal::Flush() {
    if (mCodecCtx) avcodec_flush_buffers(mCodecCtx);
    if (mAudioDev)  SDL_ClearQueuedAudio(mAudioDev);
}
