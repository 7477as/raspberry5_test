#pragma once

#include <cstdint>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>
}

class VideoHal {
public:
    bool Init(const std::vector<uint8_t>& spsPps);
    bool DecodeAndRender(std::vector<uint8_t>& naluData);
    void Flush();

    ~VideoHal();

private:
    const AVCodec*   mCodec         = nullptr;
    AVCodecContext*  mCodecCtx      = nullptr;
    AVPacket*        mPacket        = nullptr;
    AVFrame*         mFrame         = nullptr;
    SDL_Window*      mWindow        = nullptr;
    SDL_Renderer*    mRenderer      = nullptr;
    SDL_Texture*     mTexture       = nullptr;
    bool             mTextureInited = false;
    std::vector<uint8_t> mSpsPpsCache;
};

class AudioHal {
public:
    bool Init(const uint8_t* extraData, unsigned int extraDataSize);
    void DecodeAndQueue(const std::vector<uint8_t>& aacData, double ptsSec);
    double GetMasterClock();
    int    GetQueuedBytes() { return mAudioDev ? SDL_GetQueuedAudioSize(mAudioDev) : 0; }
    void   SetCurrentPts(double pts) { mCurrentAudioPts = pts; }
    void   Pause(bool p) { if (mAudioDev) SDL_PauseAudioDevice(mAudioDev, p ? 1 : 0); }
    void   Flush();

    ~AudioHal();

private:
    const AVCodec*   mCodec        = nullptr;
    AVCodecContext*  mCodecCtx     = nullptr;
    SwrContext*      mSwrCtx       = nullptr;
    AVPacket*        mPacket       = nullptr;
    AVFrame*         mFrame        = nullptr;
    SDL_AudioDeviceID mAudioDev    = 0;

    double  mCurrentAudioPts   = 0.0;
    int     mTargetSampleRate = 44100;
    int     mTargetChannels   = 2;
};
