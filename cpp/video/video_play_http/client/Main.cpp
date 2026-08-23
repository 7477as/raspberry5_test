#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <curl/curl.h>
#include <SDL2/SDL.h>

#include "VideoHal.h"

#define MINIMP4_IMPLEMENTATION
#include "minimp4.h"

namespace {
std::vector<uint8_t> gCache;
int64_t              gCacheOffset = -1;

size_t CurlWriteCallback(char* contents, size_t size, size_t nmemb, void* userp) {
    const size_t realSize = size * nmemb;
    auto* mem = static_cast<std::vector<uint8_t>*>(userp);
    mem->insert(mem->end(),
                reinterpret_cast<uint8_t*>(contents),
                reinterpret_cast<uint8_t*>(contents) + realSize);
    return realSize;
}

std::vector<uint8_t> FetchHttpRange(CURL* curl, const std::string& url,
                                    size_t start, size_t end, bool /*silent*/ = false) {
    std::vector<uint8_t> buffer;
    if (!curl) return buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    const std::string range = std::to_string(start) + "-" + std::to_string(end);
    curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_perform(curl);
    return buffer;
}

int64_t GetFileSize(const std::string& url) {
    int64_t fileSize = 0;
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        if (curl_easy_perform(curl) == CURLE_OK) {
            curl_off_t cl;
            if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl) == CURLE_OK) {
                fileSize = cl;
            }
        }
        curl_easy_cleanup(curl);
    }
    return fileSize;
}

int ReadCallback(int64_t offset, void* buffer, size_t size, void* token) {
    if (size == 0) return 0;
    const auto* url = static_cast<std::string*>(token);

    if (gCacheOffset != -1 &&
        offset >= gCacheOffset &&
        (offset + size) <= static_cast<int64_t>(gCacheOffset + gCache.size())) {
        if (buffer != nullptr) std::memcpy(buffer, gCache.data() + (offset - gCacheOffset), size);
        return 0;
    }

    CURL* curl = curl_easy_init();
    const size_t fetchSize = std::max(size, static_cast<size_t>(256 * 1024));
    auto data = FetchHttpRange(curl, *url, offset, offset + fetchSize - 1, true);
    curl_easy_cleanup(curl);

    if (data.size() < size) return 1;

    gCache       = std::move(data);
    gCacheOffset = offset;
    if (buffer != nullptr) std::memcpy(buffer, gCache.data(), size);
    return 0;
}
}  // namespace

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) return -1;

    const std::string url = "http://127.0.0.1:8080/test.mp4";
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curlHandle = curl_easy_init();

    const int64_t fileSize = GetFileSize(url);
    if (fileSize <= 0) return -1;

    MP4D_demux_t mp4;
    std::cout << ">>> 正在连接网络，解析 MP4 索引...\n";
    if (MP4D_open(&mp4, ReadCallback, const_cast<std::string*>(&url), fileSize) == 0) return -1;

    int videoTrack = -1;
    int audioTrack = -1;
    for (unsigned int i = 0; i < mp4.track_count; i++) {
        if (mp4.track[i].handler_type == MP4D_HANDLER_TYPE_VIDE) videoTrack = i;
        if (mp4.track[i].handler_type == MP4D_HANDLER_TYPE_SOUN) audioTrack = i;
    }
    if (videoTrack == -1) return -1;

    std::vector<uint8_t> spsPpsCache;
    int idx = 0;
    int bytes = 0;
    const void* data = nullptr;
    while ((data = MP4D_read_sps(&mp4, videoTrack, idx, &bytes)) != nullptr) {
        spsPpsCache.insert(spsPpsCache.end(), {0x00, 0x00, 0x00, 0x01});
        spsPpsCache.insert(spsPpsCache.end(),
                           static_cast<const uint8_t*>(data),
                           static_cast<const uint8_t*>(data) + bytes);
        idx++;
    }
    idx = 0;
    while ((data = MP4D_read_pps(&mp4, videoTrack, idx, &bytes)) != nullptr) {
        spsPpsCache.insert(spsPpsCache.end(), {0x00, 0x00, 0x00, 0x01});
        spsPpsCache.insert(spsPpsCache.end(),
                           static_cast<const uint8_t*>(data),
                           static_cast<const uint8_t*>(data) + bytes);
        idx++;
    }

    VideoHal hwVideo;
    if (!hwVideo.Init(spsPpsCache)) return -1;

    AudioHal hwAudio;
    bool hasAudio = false;
    if (audioTrack != -1) {
        hasAudio = hwAudio.Init(mp4.track[audioTrack].dsi, mp4.track[audioTrack].dsi_bytes);
    }

    uint32_t currentVFrame  = 0;
    uint32_t currentAFrame  = 0;
    const uint32_t totalVFrames = mp4.track[videoTrack].sample_count;
    const uint32_t totalAFrames = hasAudio ? mp4.track[audioTrack].sample_count : 0;

    bool        running     = true;
    bool        isPaused    = true;
    SDL_Event   event;
    unsigned int fb = 0, ts = 0, dur = 0;
    uint64_t    fOff = 0;

    auto                 sysClockStart     = std::chrono::steady_clock::now();
    double               sysClockPtsOffset = 0.0;
    bool                 clockNeedsReset   = true;

    // 首图唤醒：解码直到成功一帧，让封面立即显示
    bool frameRendered = false;
    while (!frameRendered && currentVFrame < totalVFrames) {
        fOff = MP4D_frame_offset(&mp4, videoTrack, currentVFrame, &fb, &ts, &dur);
        auto nalu = FetchHttpRange(curlHandle, url, fOff, fOff + fb - 1, true);
        if (hwVideo.DecodeAndRender(nalu)) {
            frameRendered = true;
            std::cout << "[System] 首图已点亮！支持 A/V Sync\n";
        } else {
            currentVFrame++;
        }
    }

    if (hasAudio && currentVFrame < totalVFrames) {
        MP4D_frame_offset(&mp4, videoTrack, currentVFrame, &fb, &ts, &dur);
        hwAudio.SetCurrentPts(static_cast<double>(ts) / mp4.track[videoTrack].timescale);
    }

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    isPaused = !isPaused;
                    if (hasAudio) hwAudio.Pause(isPaused);
                    if (!isPaused) clockNeedsReset = true;
                }
                else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT) {
                    isPaused = true;
                    if (hasAudio) hwAudio.Pause(true);

                    if (event.key.keysym.sym == SDLK_LEFT)
                        currentVFrame = (currentVFrame > 60) ? currentVFrame - 60 : 0;
                    else
                        currentVFrame = (currentVFrame + 60 < totalVFrames) ? currentVFrame + 60 : totalVFrames - 1;

                    hwVideo.Flush();
                    if (hasAudio) hwAudio.Flush();

                    frameRendered = false;
                    while (!frameRendered && currentVFrame < totalVFrames) {
                        fOff = MP4D_frame_offset(&mp4, videoTrack, currentVFrame, &fb, &ts, &dur);
                        auto nalu = FetchHttpRange(curlHandle, url, fOff, fOff + fb - 1, true);
                        if (hwVideo.DecodeAndRender(nalu)) frameRendered = true;
                        else currentVFrame++;
                    }

                    if (currentVFrame < totalVFrames) {
                        MP4D_frame_offset(&mp4, videoTrack, currentVFrame, &fb, &ts, &dur);
                        const double actualTargetPts =
                            static_cast<double>(ts) / mp4.track[videoTrack].timescale;
                        if (hasAudio) {
                            currentAFrame = 0;
                            for (uint32_t i = 0; i < totalAFrames; i++) {
                                unsigned int aTs;
                                MP4D_frame_offset(&mp4, audioTrack, i, &fb, &aTs, &dur);
                                if (static_cast<double>(aTs) /
                                    mp4.track[audioTrack].timescale >= actualTargetPts) {
                                    currentAFrame = i;
                                    break;
                                }
                            }
                            hwAudio.SetCurrentPts(actualTargetPts);
                        }
                    }
                    clockNeedsReset = true;
                }
            }
        }

        if (!isPaused) {
            double vPts = 999999.0, aPts = 999999.0;
            unsigned int vFb = 0, aFb = 0, vTs = 0, aTs = 0;
            uint64_t vOff = 0, aOff = 0;

            if (currentVFrame < totalVFrames) {
                vOff = MP4D_frame_offset(&mp4, videoTrack, currentVFrame, &vFb, &vTs, &dur);
                vPts = static_cast<double>(vTs) / mp4.track[videoTrack].timescale;
            }
            if (hasAudio && currentAFrame < totalAFrames) {
                aOff = MP4D_frame_offset(&mp4, audioTrack, currentAFrame, &aFb, &aTs, &dur);
                aPts = static_cast<double>(aTs) / mp4.track[audioTrack].timescale;
            }

            if (currentVFrame >= totalVFrames && currentAFrame >= totalAFrames) {
                isPaused = true;
                if (hasAudio) hwAudio.Pause(true);
                continue;
            }

            if (hasAudio &&
                (currentVFrame >= totalVFrames || aPts <= vPts) &&
                currentAFrame < totalAFrames) {
                if (hwAudio.GetQueuedBytes() > 44100 * 2 * 2 * 0.5) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                } else {
                    auto aac = FetchHttpRange(curlHandle, url, aOff, aOff + aFb - 1, true);
                    hwAudio.DecodeAndQueue(aac, aPts);
                    currentAFrame++;
                }
            }
            else if (currentVFrame < totalVFrames) {
                double masterClock = 0.0;
                bool   useSysClock = false;

                if (hasAudio) {
                    masterClock = hwAudio.GetMasterClock();
                    if (currentAFrame >= totalAFrames && hwAudio.GetQueuedBytes() == 0) {
                        useSysClock = true;
                    }
                } else {
                    useSysClock = true;
                }

                if (useSysClock) {
                    if (clockNeedsReset) {
                        sysClockStart     = std::chrono::steady_clock::now();
                        sysClockPtsOffset = vPts;
                        clockNeedsReset   = false;
                    }
                    auto now = std::chrono::steady_clock::now();
                    masterClock = sysClockPtsOffset +
                                  std::chrono::duration<double>(now - sysClockStart).count();
                } else {
                    sysClockStart     = std::chrono::steady_clock::now();
                    sysClockPtsOffset = masterClock;
                    clockNeedsReset   = false;
                }

                const double delay = vPts - masterClock;
                if (delay > 0.015 && (!hasAudio || hwAudio.GetQueuedBytes() > 0)) {
                    const int sleepMs = std::min(static_cast<int>(delay * 1000), 10);
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
                    continue;
                }

                auto nalu = FetchHttpRange(curlHandle, url, vOff, vOff + vFb - 1, true);
                hwVideo.DecodeAndRender(nalu);
                currentVFrame++;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    curl_easy_cleanup(curlHandle);
    MP4D_close(&mp4);
    curl_global_cleanup();
    SDL_Quit();
    return 0;
}
