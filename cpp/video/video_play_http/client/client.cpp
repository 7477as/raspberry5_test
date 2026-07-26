#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>
#include <curl/curl.h>

#define MINIMP4_IMPLEMENTATION
#include "minimp4.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h> // [新增] 音频重采样库
#include <SDL2/SDL.h>
}

// =========================================================================
// [网络底层代码] 网络句柄复用优化
// =========================================================================
static std::vector<uint8_t> g_cache;
static int64_t g_cache_offset = -1;

size_t my_curl_write_callback(char* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto* mem = static_cast<std::vector<uint8_t>*>(userp);
    mem->insert(mem->end(), (uint8_t*)contents, ((uint8_t*)contents) + realsize);
    return realsize;
}

// 优化：传入已存在的 CURL 句柄，避免每帧发起沉重的 TCP 三次握手
std::vector<uint8_t> fetch_http_range(CURL* curl, const std::string& url, size_t start, size_t end, bool silent = false) {
    std::vector<uint8_t> buffer;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        std::string range = std::to_string(start) + "-" + std::to_string(end);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); 
        curl_easy_perform(curl);
    }
    return buffer;
}

int64_t get_file_size(const std::string& url) {
    int64_t file_size = 0;
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); 
        if (curl_easy_perform(curl) == CURLE_OK) {
            curl_off_t cl;
            if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl) == CURLE_OK) { file_size = cl; }
        }
        curl_easy_cleanup(curl);
    }
    return file_size;
}

static int my_read_callback(int64_t offset, void *buffer, size_t size, void *token) {
    if (size == 0) return 0;
    std::string* url = static_cast<std::string*>(token);
    
    if (g_cache_offset != -1 && offset >= g_cache_offset && (offset + size) <= (g_cache_offset + g_cache.size())) {
        if (buffer != nullptr) memcpy(buffer, g_cache.data() + (offset - g_cache_offset), size);
        return 0;
    }

    CURL* curl = curl_easy_init();
    size_t fetch_size = std::max(size, (size_t)(256 * 1024));
    auto data = fetch_http_range(curl, *url, offset, offset + fetch_size - 1, true);
    curl_easy_cleanup(curl);
    
    if (data.size() < size) return 1; 

    g_cache = data; g_cache_offset = offset;
    if (buffer != nullptr) memcpy(buffer, g_cache.data(), size);
    return 0;
}

// =========================================================================
// [Video HAL] 视频硬件抽象层 (已去除 SDL_Init，统一由 main 管理)
// =========================================================================
class VideoHAL {
private:
    const AVCodec* codec;
    AVCodecContext* codec_ctx;
    AVPacket* pkt;
    AVFrame* frame;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    bool texture_inited;
    std::vector<uint8_t> sps_pps_cache;

public:
    VideoHAL() : codec(nullptr), codec_ctx(nullptr), pkt(nullptr), frame(nullptr),
                 window(nullptr), renderer(nullptr), texture(nullptr), texture_inited(false) {}

    bool init(const std::vector<uint8_t>& sps_pps) {
        sps_pps_cache = sps_pps;
        
        codec = avcodec_find_decoder(AV_CODEC_ID_H264); 
        if (!codec) return false;
        
        codec_ctx = avcodec_alloc_context3(codec);
        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) return false;
        
        pkt = av_packet_alloc();
        frame = av_frame_alloc();
        
        window = SDL_CreateWindow("RTOS Player Simulator (A/V Sync)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        return true;
    }

    bool decode_and_render(std::vector<uint8_t>& nalu_data) {
        if (nalu_data.empty()) return false;
        
        size_t offset = 0;
        while (offset + 4 <= nalu_data.size()) {
            uint32_t nalu_len = (nalu_data[offset] << 24) | (nalu_data[offset+1] << 16) | (nalu_data[offset+2] << 8) | nalu_data[offset+3];
            if (offset + 4 + nalu_len > nalu_data.size()) break;
            nalu_data[offset] = 0x00; nalu_data[offset+1] = 0x00;
            nalu_data[offset+2] = 0x00; nalu_data[offset+3] = 0x01;
            offset += 4 + nalu_len;
        }

        if (!sps_pps_cache.empty()) nalu_data.insert(nalu_data.begin(), sps_pps_cache.begin(), sps_pps_cache.end());

        pkt->data = nalu_data.data();
        pkt->size = nalu_data.size();
        
        bool got_picture = false;
        if (avcodec_send_packet(codec_ctx, pkt) == 0) {
            while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                if (!texture_inited) {
                    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, frame->width, frame->height);
                    SDL_SetWindowSize(window, frame->width, frame->height);
                    texture_inited = true;
                }
                SDL_UpdateYUVTexture(texture, nullptr, frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, nullptr, nullptr);
                SDL_RenderPresent(renderer);
                got_picture = true; 
            }
        }
        return got_picture;
    }

    void flush() { if (codec_ctx) avcodec_flush_buffers(codec_ctx); }

    ~VideoHAL() {
        if (pkt) av_packet_free(&pkt);
        if (frame) av_frame_free(&frame);
        if (codec_ctx) avcodec_free_context(&codec_ctx);
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
    }
};

// =========================================================================
// [Audio HAL]【新增】音频解码与重采样抽象层
// =========================================================================
class AudioHAL {
private:
    const AVCodec* codec;
    AVCodecContext* codec_ctx;
    SwrContext* swr_ctx;
    AVPacket* pkt;
    AVFrame* frame;
    SDL_AudioDeviceID audio_dev;
    
    double current_audio_pts;
    int target_sample_rate;
    int target_channels;

public:
    AudioHAL() : codec(nullptr), codec_ctx(nullptr), swr_ctx(nullptr), pkt(nullptr), frame(nullptr), 
                 audio_dev(0), current_audio_pts(0.0), target_sample_rate(44100), target_channels(2) {}

    bool init(const uint8_t* extradata, unsigned int extradata_size) {
        codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
        if (!codec) return false;
        
        codec_ctx = avcodec_alloc_context3(codec);
        // AAC 解码非常依赖 MP4 的配置数据 (AudioSpecificConfig)，必须要喂给它
        if (extradata && extradata_size > 0) {
            codec_ctx->extradata = (uint8_t*)av_mallocz(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(codec_ctx->extradata, extradata, extradata_size);
            codec_ctx->extradata_size = extradata_size;
        }
        
        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) return false;
        
        pkt = av_packet_alloc();
        frame = av_frame_alloc();

        // 强行将输出转为 S16 (16位整数)，这非常契合 RTOS 系统的 I2S 硬件要求
        SDL_AudioSpec wanted_spec, obtained_spec;
        SDL_zero(wanted_spec);
        wanted_spec.freq = target_sample_rate;
        wanted_spec.format = AUDIO_S16SYS;
        wanted_spec.channels = target_channels;
        wanted_spec.samples = 1024;
        wanted_spec.callback = nullptr; // 不用独立线程回调，采用“推流队列”模式

        audio_dev = SDL_OpenAudioDevice(nullptr, 0, &wanted_spec, &obtained_spec, 0);
        if (audio_dev == 0) return false;

        SDL_PauseAudioDevice(audio_dev, 1); // 默认初始状态为暂停
        return true;
    }

    void decode_and_queue(const std::vector<uint8_t>& aac_data, double pts_sec) {
        if (aac_data.empty()) return;
        
        pkt->data = (uint8_t*)aac_data.data();
        pkt->size = aac_data.size();
        
        if (avcodec_send_packet(codec_ctx, pkt) == 0) {
            while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                // 初始化重采样器 (FFmpeg 吐出的是 FLTP，外设声卡只要 S16 PCM)
                if (!swr_ctx) {
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
                    AVChannelLayout out_ch_layout;
                    av_channel_layout_default(&out_ch_layout, target_channels);
                    swr_alloc_set_opts2(&swr_ctx, &out_ch_layout, AV_SAMPLE_FMT_S16, target_sample_rate,
                                        &frame->ch_layout, (AVSampleFormat)frame->format, frame->sample_rate, 0, nullptr);
#else
                    uint64_t in_ch_layout = frame->channel_layout ? frame->channel_layout : av_get_default_channel_layout(frame->channels);
                    swr_ctx = swr_alloc_set_opts(nullptr, av_get_default_channel_layout(target_channels), AV_SAMPLE_FMT_S16, target_sample_rate,
                                                 in_ch_layout, (AVSampleFormat)frame->format, frame->sample_rate, 0, nullptr);
#endif
                    swr_init(swr_ctx);
                }

                int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples, target_sample_rate, frame->sample_rate, AV_ROUND_UP);
                
                std::vector<uint8_t> pcm_buffer(dst_nb_samples * target_channels * 2);
                uint8_t* out_ptr[1] = { pcm_buffer.data() };
                
                int ret_samples = swr_convert(swr_ctx, out_ptr, dst_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
                
                if (ret_samples > 0) {
                    int pcm_bytes = ret_samples * target_channels * 2;
                    SDL_QueueAudio(audio_dev, pcm_buffer.data(), pcm_bytes);
                    
                    // 动态对齐队列游标，规避累加时间戳的浮点数漂移
                    if (SDL_GetQueuedAudioSize(audio_dev) == (Uint32)pcm_bytes) {
                        current_audio_pts = pts_sec + (double)ret_samples / target_sample_rate;
                    } else {
                        current_audio_pts += (double)ret_samples / target_sample_rate;
                    }
                }
            }
        }
    }

    // 【核心同步机制：向外部提供精准的喇叭发声时间戳】
    double get_master_clock() {
        if (audio_dev == 0) return 0.0;
        int queued_bytes = SDL_GetQueuedAudioSize(audio_dev);
        double queued_seconds = (double)queued_bytes / (target_sample_rate * target_channels * 2);
        // 当前播放时间 = 最新推入的帧的结束PTS - 积压在声卡里还没播出去的时间
        return current_audio_pts - queued_seconds;
    }

    int get_queued_bytes() { return audio_dev ? SDL_GetQueuedAudioSize(audio_dev) : 0; }
    void set_current_pts(double pts) { current_audio_pts = pts; }
    void pause(bool p) { if (audio_dev) SDL_PauseAudioDevice(audio_dev, p ? 1 : 0); }
    
    void flush() {
        if (codec_ctx) avcodec_flush_buffers(codec_ctx);
        if (audio_dev) SDL_ClearQueuedAudio(audio_dev);
    }

    ~AudioHAL() {
        if (audio_dev) { SDL_PauseAudioDevice(audio_dev, 1); SDL_CloseAudioDevice(audio_dev); }
        if (swr_ctx) swr_free(&swr_ctx);
        if (pkt) av_packet_free(&pkt);
        if (frame) av_frame_free(&frame);
        if (codec_ctx) avcodec_free_context(&codec_ctx);
    }
};

// =========================================================================
// [主逻辑] 
// =========================================================================
int main() {
    // 全局初始化视频与音频子系统
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) return -1;

    std::string url = "http://127.0.0.1:8080/test.mp4"; 
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl_handle = curl_easy_init(); // 复用同一句柄，提升十几倍网络响应

    int64_t file_size = get_file_size(url);
    if (file_size <= 0) return -1;
    
    MP4D_demux_t mp4;
    std::cout << ">>> 正在连接网络，解析 MP4 索引...\n";
    if (MP4D_open(&mp4, my_read_callback, &url, file_size) == 0) return -1;

    int video_track = -1, audio_track = -1;
    for (unsigned int i = 0; i < mp4.track_count; i++) {
        if (mp4.track[i].handler_type == MP4D_HANDLER_TYPE_VIDE) video_track = i;
        if (mp4.track[i].handler_type == MP4D_HANDLER_TYPE_SOUN) audio_track = i;
    }
    if (video_track == -1) return -1;

    // --- 提取视频头及初始化 VideoHAL ---
    std::vector<uint8_t> sps_pps_cache;
    int bytes = 0, idx = 0; const void* data = nullptr;
    while ((data = MP4D_read_sps(&mp4, video_track, idx, &bytes)) != nullptr) {
        sps_pps_cache.insert(sps_pps_cache.end(), {0x00, 0x00, 0x00, 0x01});
        sps_pps_cache.insert(sps_pps_cache.end(), (const uint8_t*)data, ((const uint8_t*)data) + bytes);
        idx++;
    }
    idx = 0;
    while ((data = MP4D_read_pps(&mp4, video_track, idx, &bytes)) != nullptr) {
        sps_pps_cache.insert(sps_pps_cache.end(), {0x00, 0x00, 0x00, 0x01});
        sps_pps_cache.insert(sps_pps_cache.end(), (const uint8_t*)data, ((const uint8_t*)data) + bytes);
        idx++;
    }

    VideoHAL hw_video;
    if (!hw_video.init(sps_pps_cache)) return -1;

    // --- 提取音频配置及初始化 AudioHAL ---
    AudioHAL hw_audio;
    bool has_audio = false;
    if (audio_track != -1) {
        // dsi 即 MP4 结构下的 AudioSpecificConfig，对 AAC 极为重要
        has_audio = hw_audio.init(mp4.track[audio_track].dsi, mp4.track[audio_track].dsi_bytes);
    }

    uint32_t current_v_frame = 0, current_a_frame = 0;
    uint32_t total_v_frames = mp4.track[video_track].sample_count;
    uint32_t total_a_frames = has_audio ? mp4.track[audio_track].sample_count : 0;
    
    bool running = true, is_paused = true; 
    SDL_Event event;
    unsigned int fb, ts, dur; uint64_t f_off;

    // --- 智能系统时钟（用于平滑断声或纯视频回放） ---
    auto sys_clock_start = std::chrono::steady_clock::now();
    double sys_clock_pts_offset = 0.0;
    bool clock_needs_reset = true;

    // 【首图唤醒机制】
    bool frame_rendered = false;
    while (!frame_rendered && current_v_frame < total_v_frames) {
        f_off = MP4D_frame_offset(&mp4, video_track, current_v_frame, &fb, &ts, &dur);
        auto nalu = fetch_http_range(curl_handle, url, f_off, f_off + fb - 1, true);
        if (hw_video.decode_and_render(nalu)) {
            frame_rendered = true;
            std::cout << "[系统] 首图已点亮！支持 A/V Sync\n";
        } else {
            current_v_frame++; 
        }
    }
    
    if (has_audio && current_v_frame < total_v_frames) {
        MP4D_frame_offset(&mp4, video_track, current_v_frame, &fb, &ts, &dur);
        hw_audio.set_current_pts((double)ts / mp4.track[video_track].timescale);
    }

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    is_paused = !is_paused; 
                    if (has_audio) hw_audio.pause(is_paused);
                    if (!is_paused) clock_needs_reset = true;
                } 
                else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT) {
                    is_paused = true;
                    if (has_audio) hw_audio.pause(true); 

                    if (event.key.keysym.sym == SDLK_LEFT) current_v_frame = (current_v_frame > 60) ? current_v_frame - 60 : 0; 
                    else current_v_frame = (current_v_frame + 60 < total_v_frames) ? current_v_frame + 60 : total_v_frames - 1;
                    
                    hw_video.flush();
                    if (has_audio) hw_audio.flush();
                    
                    // 下塞新封面
                    frame_rendered = false;
                    while (!frame_rendered && current_v_frame < total_v_frames) {
                        f_off = MP4D_frame_offset(&mp4, video_track, current_v_frame, &fb, &ts, &dur);
                        auto nalu = fetch_http_range(curl_handle, url, f_off, f_off + fb - 1, true);
                        if (hw_video.decode_and_render(nalu)) frame_rendered = true; 
                        else current_v_frame++;
                    }
                    
                    // 重新对齐音轨游标
                    if (current_v_frame < total_v_frames) {
                        MP4D_frame_offset(&mp4, video_track, current_v_frame, &fb, &ts, &dur);
                        double actual_target_pts = (double)ts / mp4.track[video_track].timescale;

                        if (has_audio) {
                            current_a_frame = 0;
                            for (uint32_t i = 0; i < total_a_frames; i++) {
                                unsigned int a_ts;
                                MP4D_frame_offset(&mp4, audio_track, i, &fb, &a_ts, &dur);
                                if ((double)a_ts / mp4.track[audio_track].timescale >= actual_target_pts) {
                                    current_a_frame = i; break;
                                }
                            }
                            hw_audio.set_current_pts(actual_target_pts);
                        }
                    }
                    clock_needs_reset = true;
                }
            }
        }

        if (!is_paused) {
            double v_pts = 999999.0, a_pts = 999999.0;
            unsigned int v_fb=0, a_fb=0, v_ts=0, a_ts=0;
            uint64_t v_off = 0, a_off = 0;

            if (current_v_frame < total_v_frames) {
                v_off = MP4D_frame_offset(&mp4, video_track, current_v_frame, &v_fb, &v_ts, &dur);
                v_pts = (double)v_ts / mp4.track[video_track].timescale;
            }
            if (has_audio && current_a_frame < total_a_frames) {
                a_off = MP4D_frame_offset(&mp4, audio_track, current_a_frame, &a_fb, &a_ts, &dur);
                a_pts = (double)a_ts / mp4.track[audio_track].timescale;
            }

            if (current_v_frame >= total_v_frames && current_a_frame >= total_a_frames) {
                is_paused = true; 
                if (has_audio) hw_audio.pause(true); 
                continue; 
            }

            // 【交织拉取引擎】确保底层 HTTP 按物理顺序读取 MP4，极其省资源
            if (has_audio && (current_v_frame >= total_v_frames || a_pts <= v_pts) && current_a_frame < total_a_frames) {
                // 音频数据很小，若缓冲区内已有大于 0.5s 数据，需微挂起防内存暴涨
                if (hw_audio.get_queued_bytes() > 44100 * 2 * 2 * 0.5) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                } else {
                    auto aac = fetch_http_range(curl_handle, url, a_off, a_off + a_fb - 1, true);
                    hw_audio.decode_and_queue(aac, a_pts);
                    current_a_frame++;
                }
            } 
            else if (current_v_frame < total_v_frames) {
                // 【A/V Sync 音视频同步】
                double master_clock = 0.0;
                bool use_sys_clock = false;
                
                if (has_audio) {
                    master_clock = hw_audio.get_master_clock();
                    // 假如音频文件先于视频播完结束，则抛弃音频时钟，平滑切入系统时钟
                    if (current_a_frame >= total_a_frames && hw_audio.get_queued_bytes() == 0) {
                        use_sys_clock = true;
                    }
                } else {
                    use_sys_clock = true;
                }

                if (use_sys_clock) {
                    if (clock_needs_reset) {
                        sys_clock_start = std::chrono::steady_clock::now();
                        sys_clock_pts_offset = v_pts; // 以视频当前帧作为锚点
                        clock_needs_reset = false;
                    }
                    auto now = std::chrono::steady_clock::now();
                    master_clock = sys_clock_pts_offset + std::chrono::duration<double>(now - sys_clock_start).count();
                } else {
                    // 当有音频正在播放时，默默让系统时钟绑定着音频走
                    sys_clock_start = std::chrono::steady_clock::now();
                    sys_clock_pts_offset = master_clock;
                    clock_needs_reset = false;
                }

                double delay = v_pts - master_clock;
                
                // 视频超前了，去一边睡一会（让出 CPU），不跳过任何事件监听
                if (delay > 0.015 && (!has_audio || hw_audio.get_queued_bytes() > 0)) { 
                    int sleep_ms = std::min((int)(delay * 1000), 10); // 分段式睡眠避免卡死 UI
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                    continue; // 这一帧延后再画
                } 

                auto nalu = fetch_http_range(curl_handle, url, v_off, v_off + v_fb - 1, true);
                hw_video.decode_and_render(nalu);
                current_v_frame++;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    curl_easy_cleanup(curl_handle);
    MP4D_close(&mp4);
    curl_global_cleanup();
    SDL_Quit();
    return 0;
}