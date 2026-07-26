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
#include <SDL2/SDL.h>
}

// =========================================================================
// [HAL 层] 自动转换 AVCC 到 Annex-B，无缝对接任何硬件解码器
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

        if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
        
        codec = avcodec_find_decoder(AV_CODEC_ID_H264); 
        if (!codec) return false;
        
        codec_ctx = avcodec_alloc_context3(codec);
        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) return false;
        
        pkt = av_packet_alloc();
        frame = av_frame_alloc();
        
        window = SDL_CreateWindow("RTOS Player Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        
        return true;
    }

    bool decode_and_render(std::vector<uint8_t>& nalu_data) {
        if (nalu_data.empty()) return false;
        
        // 替换 4 字节长度头为 Annex-B 起始码
        size_t offset = 0;
        while (offset + 4 <= nalu_data.size()) {
            uint32_t nalu_len = (nalu_data[offset] << 24) | (nalu_data[offset+1] << 16) | (nalu_data[offset+2] << 8) | nalu_data[offset+3];
            if (offset + 4 + nalu_len > nalu_data.size()) {
                break; // 防越界
            }
            nalu_data[offset] = 0x00; nalu_data[offset+1] = 0x00;
            nalu_data[offset+2] = 0x00; nalu_data[offset+3] = 0x01;
            offset += 4 + nalu_len;
        }

        // 把说明书 (SPS/PPS) 强制塞在每一帧前面
        if (!sps_pps_cache.empty()) {
            nalu_data.insert(nalu_data.begin(), sps_pps_cache.begin(), sps_pps_cache.end());
        }

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
                
                SDL_UpdateYUVTexture(texture, nullptr,
                                     frame->data[0], frame->linesize[0],
                                     frame->data[1], frame->linesize[1],
                                     frame->data[2], frame->linesize[2]);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, nullptr, nullptr);
                SDL_RenderPresent(renderer);
                
                got_picture = true; // 终于吐出画面了！
            }
        }
        return got_picture; // 只有真实画出画面，才返回 true
    }

    void flush() {
        if (codec_ctx) avcodec_flush_buffers(codec_ctx);
    }

    ~VideoHAL() {
        if (pkt) av_packet_free(&pkt);
        if (frame) av_frame_free(&frame);
        if (codec_ctx) avcodec_free_context(&codec_ctx);
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

// =========================================================================
// [网络底层代码] 极简缓存滑动窗口 (保持不变)
// =========================================================================
static std::vector<uint8_t> g_cache;
static int64_t g_cache_offset = -1;

size_t my_curl_write_callback(char* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto* mem = static_cast<std::vector<uint8_t>*>(userp);
    mem->insert(mem->end(), (uint8_t*)contents, ((uint8_t*)contents) + realsize);
    return realsize;
}

std::vector<uint8_t> fetch_http_range(const std::string& url, size_t start, size_t end, bool silent = false) {
    std::vector<uint8_t> buffer;
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        std::string range = std::to_string(start) + "-" + std::to_string(end);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        if (!silent) std::cout << "[网络拉取] Range: bytes=" << range << " ...\n";
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
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

    size_t fetch_size = std::max(size, (size_t)(256 * 1024));
    auto data = fetch_http_range(*url, offset, offset + fetch_size - 1, true);
    if (data.size() < size) return 1; 

    g_cache = data; g_cache_offset = offset;
    if (buffer != nullptr) memcpy(buffer, g_cache.data(), size);
    return 0;
}

// =========================================================================
// [主逻辑] 
// =========================================================================
int main() {
    std::string url = "http://127.0.0.1:8080/test.mp4"; 
    curl_global_init(CURL_GLOBAL_ALL);

    int64_t file_size = get_file_size(url);
    if (file_size <= 0) return -1;
    
    MP4D_demux_t mp4;
    std::cout << ">>> 正在连接网络，解析 MP4 索引...\n";
    if (MP4D_open(&mp4, my_read_callback, &url, file_size) == 0) return -1;

    int video_track = -1;
    for (unsigned int i = 0; i < mp4.track_count; i++) {
        if (mp4.track[i].handler_type == MP4D_HANDLER_TYPE_VIDE) {
            video_track = i; break;
        }
    }
    if (video_track == -1) return -1;

    std::vector<uint8_t> sps_pps_cache;
    int bytes = 0;
    const void* data = nullptr;
    int idx = 0;
    
    while ((data = MP4D_read_sps(&mp4, video_track, idx, &bytes)) != nullptr) {
        sps_pps_cache.push_back(0x00); sps_pps_cache.push_back(0x00);
        sps_pps_cache.push_back(0x00); sps_pps_cache.push_back(0x01);
        sps_pps_cache.insert(sps_pps_cache.end(), (const uint8_t*)data, ((const uint8_t*)data) + bytes);
        idx++;
    }
    idx = 0;
    while ((data = MP4D_read_pps(&mp4, video_track, idx, &bytes)) != nullptr) {
        sps_pps_cache.push_back(0x00); sps_pps_cache.push_back(0x00);
        sps_pps_cache.push_back(0x00); sps_pps_cache.push_back(0x01);
        sps_pps_cache.insert(sps_pps_cache.end(), (const uint8_t*)data, ((const uint8_t*)data) + bytes);
        idx++;
    }

    VideoHAL hw;
    if (!hw.init(sps_pps_cache)) {
        std::cerr << "硬件抽象层初始化失败！\n";
        return -1;
    }

    uint32_t total_frames = mp4.track[video_track].sample_count;
    uint32_t current_frame = 0;
    bool running = true;
    bool is_paused = true; 
    SDL_Event event;

    std::cout << "\n=========================================\n";
    std::cout << " 播放器引擎已启动！(请点击弹出的黑色视频窗口获取焦点)\n";
    std::cout << " [空格键]: 播放 / 暂停\n";
    std::cout << " [左/右方向键]: 快进/快退 (极速刷新关键封面)\n";
    std::cout << " [ESC键]: 退出播放器\n";
    std::cout << "=========================================\n";

    // ==================================================================
    // 【首图唤醒机制】
    // 解决黑屏死锁：采用"填鸭式"喂帧，没出画就立刻狂塞下一帧，直到吐出首帧上屏！
    // ==================================================================
    std::cout << "[系统] 解码器需要缓冲，正在强塞数据以点亮首图...\n";
    unsigned int fb, ts, dur;
    uint64_t f_off;
    bool frame_rendered = false;
    
    while (!frame_rendered && current_frame < total_frames) {
        f_off = MP4D_frame_offset(&mp4, video_track, current_frame, &fb, &ts, &dur);
        auto nalu = fetch_http_range(url, f_off, f_off + fb - 1, true);
        
        if (hw.decode_and_render(nalu)) {
            frame_rendered = true;
            std::cout << "[系统] 画面成功点亮！已定格在封面。\n";
        } else {
            current_frame++; // 解码器嫌不够，继续强行塞下一帧！
        }
    }

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    is_paused = !is_paused; 
                } 
                else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT) {
                    is_paused = true;
                    if (event.key.keysym.sym == SDLK_LEFT) {
                        current_frame = (current_frame > 30) ? current_frame - 30 : 0; 
                    } else {
                        current_frame = (current_frame + 30 < total_frames) ? current_frame + 30 : total_frames - 1;
                    }
                    
                    hw.flush(); // 清空解码器肚子里的旧数据残影，防止花屏
                    
                    // 【Seek 封面唤醒机制】：同理，跳转后极其野蛮地疯狂塞帧，直到刷新封面画面！
                    frame_rendered = false;
                    while (!frame_rendered && current_frame < total_frames) {
                        f_off = MP4D_frame_offset(&mp4, video_track, current_frame, &fb, &ts, &dur);
                        auto nalu = fetch_http_range(url, f_off, f_off + fb - 1, true);
                        if (hw.decode_and_render(nalu)) {
                            frame_rendered = true; 
                        } else {
                            current_frame++;
                        }
                    }
                }
            }
        }

        if (!is_paused && current_frame < total_frames) {
            f_off = MP4D_frame_offset(&mp4, video_track, current_frame, &fb, &ts, &dur);
            auto nalu = fetch_http_range(url, f_off, f_off + fb - 1, true);
            hw.decode_and_render(nalu);
            
            current_frame++;
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); 
            
        } else if (is_paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    MP4D_close(&mp4);
    curl_global_cleanup();
    return 0;
}