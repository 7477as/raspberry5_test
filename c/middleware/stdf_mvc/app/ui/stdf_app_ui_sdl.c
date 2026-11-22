/* SDL system dashboard; all rendering and callbacks run in the main loop. */
#include "stdf_app_ui_sdl.h"
#include "stdf_app_ui_status.h"
#include "app/stdf_app.h"
#include "app/stdf_app_sampler.h"
#include <SDL2/SDL.h>
#include <ctype.h>
#include <string.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 620
#define ROW_HEIGHT 20
#define PAGE_ROWS 18u
#define BAR_HEIGHT 6
#define BAR_GAP 10

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static stdf_app_ui_status_t g_status[STDF_UI_GROUPS];
static bool g_subscribed[STDF_UI_GROUPS];
static unsigned g_page;
static unsigned g_scroll;
static uint64_t g_last_render;
static bool g_dirty;

static const uint8_t g_digits[10][5] = {
    {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
    {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
    {0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},
    {0x36,0x49,0x49,0x49,0x36},{6,0x49,0x49,0x29,0x1e}
};
static const uint8_t g_letters[26][5] = {
    {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},
    {0x3e,0x41,0x41,0x41,0x22},{0x7f,0x41,0x41,0x22,0x1c},
    {0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},
    {0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},
    {0,0x41,0x7f,0x41,0},{0x20,0x40,0x41,0x3f,1},
    {0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
    {0x7f,2,0x0c,2,0x7f},{0x7f,4,8,0x10,0x7f},
    {0x3e,0x41,0x41,0x41,0x3e},{0x7f,9,9,9,6},
    {0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{1,1,0x7f,1,1},
    {0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},
    {0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,8,0x14,0x63},
    {7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}
};

static void glyph_bits(unsigned char character, uint8_t bits[5])
{
    memset(bits, 0, 5);
    unsigned char upper = (unsigned char)toupper(character);
    if (upper >= 'A' && upper <= 'Z') { memcpy(bits, g_letters[upper - 'A'], 5); return; }
    if (upper >= '0' && upper <= '9') { memcpy(bits, g_digits[upper - '0'], 5); return; }
    switch (character) {
    case ' ': break;
    case '.': bits[2] = 0x60; break;
    case ':': bits[2] = 0x36; break;
    case '-': memset(bits, 8, 5); break;
    case '_': memset(bits, 0x40, 5); break;
    case '/': bits[0]=0x40; bits[1]=0x20; bits[2]=0x10; bits[3]=8; bits[4]=4; break;
    case '|': bits[2]=0x7f; break;
    case '%': bits[0]=0x63; bits[1]=0x13; bits[2]=8; bits[3]=0x64; bits[4]=0x63; break;
    case '(': bits[2]=0x3e; bits[3]=0x41; break;
    case ')': bits[1]=0x41; bits[2]=0x3e; break;
    case '+': bits[1]=8; bits[2]=0x3e; bits[3]=8; break;
    case '=': memset(bits, 0x14, 5); break;
    default: bits[0]=2; bits[1]=1; bits[2]=0x51; bits[3]=9; bits[4]=6; break;
    }
}

static void draw_text(int x, int y, const char *text, SDL_Color color)
{
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, 255);
    for (unsigned offset = 0; text[offset] && x + 10 < WINDOW_WIDTH - 20; offset++, x += 12) {
        uint8_t bits[5];
        glyph_bits((unsigned char)text[offset], bits);
        for (unsigned column = 0; column < 5; column++) {
            for (unsigned row = 0; row < 7; row++) {
                if (!(bits[column] & (1u << row))) continue;
                SDL_Rect pixel = { x + (int)column * 2, y + (int)row * 2, 2, 2 };
                SDL_RenderFillRect(g_renderer, &pixel);
            }
        }
    }
}

static void on_status(stdf_mvc_publisher_t publisher, stdf_mvc_subject_id_t subject,
                      const void *payload)
{
    (void)publisher;
    if (!payload) return;
    for (unsigned group = 0; group < STDF_UI_GROUPS; group++) {
        if (subject != g_ui_subjects[group]) continue;
        stdf_app_ui_status_format(group, payload, &g_status[group]);
        g_dirty = true;
    }
}

static int draw_group(unsigned group, int y, unsigned first, unsigned count, uint64_t now)
{
    SDL_Color heading = { 90, 190, 240, 255 };
    SDL_Color body = { 220, 225, 230, 255 };
    char title[80];
    snprintf(title, sizeof(title), "%s%s", g_ui_titles[group],
             stdf_app_ui_status_stale(group, g_status[group].timestamp_ms, now) ? " [STALE]" : "");
    draw_text(24, y, title, heading);
    y += ROW_HEIGHT + 4;
    if (!g_status[group].count) {
        draw_text(36, y, "Waiting for sample...", body);
        return y + ROW_HEIGHT + 12;
    }
    for (unsigned row = first; row < g_status[group].count && row < first + count; row++) {
        draw_text(36, y, g_status[group].lines[row], body);
        y += ROW_HEIGHT;
    }
    if ((group == 0 || group == 1 || group == 5) && g_status[group].bar_percent >= 0) {
        SDL_Rect background = {36, y + BAR_GAP, WINDOW_WIDTH - 72, BAR_HEIGHT};
        SDL_SetRenderDrawColor(g_renderer, 50, 60, 70, 255);
        SDL_RenderFillRect(g_renderer, &background);
        float percent = g_status[group].bar_percent;
        if (percent > 100) percent = 100;
        SDL_Rect fill = {36, y + BAR_GAP, (int)((WINDOW_WIDTH - 72) * percent / 100), BAR_HEIGHT};
        SDL_SetRenderDrawColor(g_renderer, percent > 80 ? 235 : 70, 165, 90, 255);
        SDL_RenderFillRect(g_renderer, &fill);
        y += BAR_GAP + BAR_HEIGHT;
    }
    return y + 14;
}

static void render_dashboard(uint64_t now)
{
    SDL_SetRenderDrawColor(g_renderer, 19, 25, 34, 255);
    SDL_RenderClear(g_renderer);
    SDL_Color white = {235, 240, 245, 255};
    draw_text(24, 16, "RASPBERRY PI SYSTEM STATUS", white);
    draw_text(24, 42, "1 OVERVIEW | 2 NETWORK | 3 POWER | 4 STORAGE | UP/DOWN SCROLL | ESC QUIT", white);
    if (g_page == 0) {
        int y = draw_group(0, 78, 0, 1, now);
        y = draw_group(1, y, 0, 1, now);
        y = draw_group(5, y, 0, 1, now);
        y = draw_group(4, y, 0, 2, now);
        y = draw_group(3, y, 0, 1, now);
        draw_group(2, y, 0, 3, now);
    } else {
        unsigned group = g_page == 1 ? 4 : (g_page == 2 ? 3 : 5);
        unsigned maximum = g_status[group].count > PAGE_ROWS ? g_status[group].count - PAGE_ROWS : 0;
        if (g_scroll > maximum) g_scroll = maximum;
        draw_group(group, 78, g_scroll, PAGE_ROWS, now);
        char position[80];
        snprintf(position, sizeof(position), "ROW %u / %u", g_scroll + 1, g_status[group].count);
        draw_text(24, WINDOW_HEIGHT - 30, position, white);
    }
    SDL_RenderPresent(g_renderer);
}

int stdf_app_ui_sdl_init(void)
{
    memset(g_status, 0, sizeof(g_status));
    g_page = 0; g_scroll = 0; g_last_render = 0; g_dirty = true;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
    g_window = SDL_CreateWindow("stdf_mvc system status", SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!g_window) { stdf_app_ui_sdl_deinit(); return -1; }
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    if (!g_renderer) { stdf_app_ui_sdl_deinit(); return -1; }
    SDL_RenderSetLogicalSize(g_renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    for (unsigned group = 0; group < STDF_UI_GROUPS; group++) {
        int result = stdf_mvc_api_subscribe(STDF_MVC_LOCAL, g_ui_subjects[group], on_status);
        if (result != 0) { stdf_app_ui_sdl_deinit(); return result; }
        g_subscribed[group] = true;
    }
    return 0;
}

void stdf_app_ui_sdl_poll(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) g_running = 0;
        if (event.type == SDL_KEYDOWN) {
            SDL_Keycode key = event.key.keysym.sym;
            if (key == SDLK_ESCAPE) g_running = 0;
            if (key >= SDLK_1 && key <= SDLK_4) { g_page = (unsigned)(key - SDLK_1); g_scroll = 0; }
            if (key == SDLK_DOWN) g_scroll++;
            if (key == SDLK_UP && g_scroll) g_scroll--;
        }
        if (event.type == SDL_MOUSEWHEEL) {
            if (event.wheel.y < 0) g_scroll++;
            else if (event.wheel.y > 0 && g_scroll) g_scroll--;
        }
        g_dirty = true;
    }
    uint64_t now = stdf_app_now_ms();
    if (g_dirty || now - g_last_render >= 1000u) {
        render_dashboard(now);
        g_last_render = now;
        g_dirty = false;
    }
}

void stdf_app_ui_sdl_deinit(void)
{
    for (unsigned group = 0; group < STDF_UI_GROUPS; group++) {
        if (g_subscribed[group])
            stdf_mvc_api_unsubscribe(STDF_MVC_LOCAL, g_ui_subjects[group], on_status);
        g_subscribed[group] = false;
    }
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);
    g_renderer = NULL; g_window = NULL;
    SDL_Quit();
}
