/* std_mvc_ui_sdl - SDL2 图形 UI（温度条 + 数值显示） */

#include "std_mvc_ui_sdl.h"
#include "app/std_mvc_apps.h"
#include "util/std_mvc_log.h"
#include "stdf_mvc_api.h"
#include "subsystem/dm/std_mvc_data_dm.h"
#include <SDL2/SDL.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define SDL_WIN_W       800
#define SDL_WIN_H       480
#define TEMP_MAX_C      90.0f
#define TEMP_BAR_X      50
#define TEMP_BAR_Y      140
#define TEMP_BAR_W      700
#define TEMP_BAR_H      60
#define TEXT_SCALE      5
#define CHAR_W          6
#define CHAR_H          7

static SDL_Window   *s_win = NULL;
static SDL_Renderer *s_ren = NULL;
static atomic_int    s_dirty = 0;
static float         s_last_temp_c = -273.15f;

/* 5x7 字体（每列 1 字节，bit0 = 顶行） */
static const uint8_t s_font_5x7[][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* . */
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* space */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* C */
    {0x08, 0x14, 0x14, 0x22, 0x22}, /* - */
    {0x63, 0x13, 0x08, 0x64, 0x63}, /* % */
};

static int font_index(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c == '.') return 10;
    if (c == ' ') return 11;
    if (c == 'C') return 12;
    if (c == '-') return 13;
    if (c == '%') return 14;
    return 11;
}

static void draw_char(int x, int y, char c, uint8_t r, uint8_t g, uint8_t b)
{
    const uint8_t *bits = s_font_5x7[font_index(c)];
    SDL_SetRenderDrawColor(s_ren, r, g, b, 255);
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            if (bits[col] & (1u << row)) {
                SDL_Rect px = {
                    x + col * TEXT_SCALE,
                    y + row * TEXT_SCALE,
                    TEXT_SCALE,
                    TEXT_SCALE,
                };
                SDL_RenderFillRect(s_ren, &px);
            }
        }
    }
}

static void draw_text(int x, int y, const char *text, uint8_t r, uint8_t g, uint8_t b)
{
    int cx = x;
    while (*text) {
        draw_char(cx, y, *text, r, g, b);
        cx += CHAR_W * TEXT_SCALE;
        text++;
    }
}

static int text_pixel_width(const char *text)
{
    int n = 0;
    while (text[n]) n++;
    return n * CHAR_W * TEXT_SCALE;
}

static void on_temperature(stdf_mvc_subject_id_t  subject_id,
                           std_mvc_publisher_t    publisher,
                           const void           *payload)
{
    (void)subject_id;
    (void)publisher;
    if (!payload) return;

    std_mvc_dm_temperature_t *s = (std_mvc_dm_temperature_t *)payload;
    s_last_temp_c = s->value_c;
    atomic_store(&s_dirty, 1);
}

static uint8_t temp_color_r(float c)
{
    if (c < 30.0f) return 50u;
    if (c < 50.0f) return 50u;
    if (c < 70.0f) return 230u;
    return 220u;
}

static uint8_t temp_color_g(float c)
{
    if (c < 30.0f) return 100u;
    if (c < 50.0f) return 200u;
    if (c < 70.0f) return 200u;
    return 50u;
}

static uint8_t temp_color_b(float c)
{
    if (c < 30.0f) return 200u;
    if (c < 50.0f) return 50u;
    if (c < 70.0f) return 50u;
    return 50u;
}

int std_mvc_ui_sdl_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        STD_MVC_LOG_E_TAG("[UI-SDL]", "SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    s_win = SDL_CreateWindow("stdf_mvc temperature",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SDL_WIN_W, SDL_WIN_H,
                              SDL_WINDOW_SHOWN);
    if (!s_win) {
        STD_MVC_LOG_E_TAG("[UI-SDL]", "SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    s_ren = SDL_CreateRenderer(s_win, -1,
                                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!s_ren) {
        STD_MVC_LOG_E_TAG("[UI-SDL]", "SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(s_win);
        SDL_Quit();
        return -1;
    }

    SDL_SetRenderDrawColor(s_ren, 20, 20, 30, 255);
    SDL_RenderClear(s_ren);

    int rc = stdf_mvc_api_subscribe(STD_MVC_LOCAL,
                                     STDF_MVC_SUBJECT_DM_TEMPERATURE,
                                     on_temperature);
    if (rc != 0) {
        STD_MVC_LOG_W_TAG("[UI-SDL]", "subscribe failed: %d", rc);
    }

    STD_MVC_LOG_I_TAG("[UI-SDL]", "%s", "init ok");
    return 0;
}

void std_mvc_ui_sdl_tick(void)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            g_running = 0;
            return;
        }
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
            g_running = 0;
            return;
        }
    }

    if (!atomic_load(&s_dirty)) return;
    atomic_store(&s_dirty, 0);

    SDL_SetRenderDrawColor(s_ren, 20, 20, 30, 255);
    SDL_RenderClear(s_ren);

    float render_temp = s_last_temp_c;
    if (render_temp < -200.0f) render_temp = 0.0f;
    if (render_temp < 0.0f) render_temp = 0.0f;
    if (render_temp > TEMP_MAX_C) render_temp = TEMP_MAX_C;

    char temp_text[16];
    snprintf(temp_text, sizeof(temp_text), "%4.1f C", (double)render_temp);
    int text_w = text_pixel_width(temp_text);
    int text_x = (SDL_WIN_W - text_w) / 2;
    int text_y = 40;
    draw_text(text_x, text_y, temp_text, 240, 240, 240);

    char min_max_text[32];
    snprintf(min_max_text, sizeof(min_max_text), "0.0 C  -  %.0f C", (double)TEMP_MAX_C);
    int label_y = text_y + CHAR_H * TEXT_SCALE + 12;
    draw_text(TEMP_BAR_X, label_y, min_max_text, 140, 140, 160);

    SDL_SetRenderDrawColor(s_ren, 60, 60, 80, 255);
    SDL_Rect bg = { TEMP_BAR_X - 2, TEMP_BAR_Y - 2, TEMP_BAR_W + 4, TEMP_BAR_H + 4 };
    SDL_RenderFillRect(s_ren, &bg);

    SDL_SetRenderDrawColor(s_ren, 40, 40, 50, 255);
    SDL_Rect bar_bg = { TEMP_BAR_X, TEMP_BAR_Y, TEMP_BAR_W, TEMP_BAR_H };
    SDL_RenderFillRect(s_ren, &bar_bg);

    float ratio = render_temp / TEMP_MAX_C;
    uint16_t fill_w = (uint16_t)(ratio * TEMP_BAR_W);
    if (fill_w > 0) {
        SDL_SetRenderDrawColor(s_ren,
                               temp_color_r(render_temp),
                               temp_color_g(render_temp),
                               temp_color_b(render_temp),
                               255);
        SDL_Rect fill = { TEMP_BAR_X, TEMP_BAR_Y, fill_w, TEMP_BAR_H };
        SDL_RenderFillRect(s_ren, &fill);
    }

    SDL_SetRenderDrawColor(s_ren, 80, 80, 100, 255);
    SDL_Rect footer = { 0, SDL_WIN_H - 40, SDL_WIN_W, 40 };
    SDL_RenderFillRect(s_ren, &footer);
    draw_text(20, SDL_WIN_H - 30, "ESC quit", 200, 200, 200);

    SDL_RenderPresent(s_ren);
}

void std_mvc_ui_sdl_deinit(void)
{
    if (s_ren) { SDL_DestroyRenderer(s_ren); s_ren = NULL; }
    if (s_win) { SDL_DestroyWindow(s_win); s_win = NULL; }
    SDL_Quit();
    STD_MVC_LOG_I_TAG("[UI-SDL]", "%s", "deinit ok");
}
