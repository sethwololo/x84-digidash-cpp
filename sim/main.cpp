// The dashboard on the computer: a window with the simulation, or one frame of a scenario saved as PNG.
//
//   dashboard-sim                          a 1920x480 window (--zoom 0.75 to make it smaller)
//   dashboard-sim --screenshot out.png 1   scenario 1 (0 normal, 1 warning, 2 low fuel) to out.png
//   dashboard-sim --screenshot out.png 1 light   the same by day, in the light theme

#include <SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "dashboard.h"
#include "lvgl.h"
#include "simulation.h"
#include "theme.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace {

constexpr int W = theme::SCREEN_WIDTH;
constexpr int H = theme::SCREEN_HEIGHT;

uint32_t ticks()
{
    return SDL_GetTicks();
}

// One frame rendered off screen, like on the board, then written as PNG.
int screenshot(const char *path, int scenario, bool night)
{
    static std::vector<uint16_t> pixels(W * H);
    lv_display_t *display = lv_display_create(W, H);
    lv_display_set_buffers(display, pixels.data(), nullptr, pixels.size() * sizeof(uint16_t), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, [](lv_display_t *d, const lv_area_t *, uint8_t *) { lv_display_flush_ready(d); });

    ui::Dashboard dashboard;
    ui::Simulation simulation;
    simulation.pin(scenario, night);
    dashboard.show(simulation.screen(0));
    lv_refr_now(display);

    std::vector<uint8_t> rgb(W * H * 3);
    for (int i = 0; i < W * H; i++) {
        const uint16_t p = pixels[i];
        const int r = p >> 11, g = (p >> 5) & 0x3f, b = p & 0x1f;
        rgb[i * 3] = static_cast<uint8_t>(r << 3 | r >> 2);
        rgb[i * 3 + 1] = static_cast<uint8_t>(g << 2 | g >> 4);
        rgb[i * 3 + 2] = static_cast<uint8_t>(b << 3 | b >> 2);
    }
    if (!stbi_write_png(path, W, H, 3, rgb.data(), W * 3)) {
        std::fprintf(stderr, "could not write %s\n", path);
        return 1;
    }
    std::printf("%s: %dx%d, scenario %d, %s\n", path, W, H, scenario, night ? "night" : "day");
    return 0;
}

int window(float zoom)
{
    lv_display_t *display = lv_sdl_window_create(W, H);
    lv_sdl_window_set_zoom(display, zoom);
    lv_sdl_window_set_title(display, "x84-digidash");

    ui::Dashboard dashboard;
    ui::Simulation simulation;
    for (;;) {
        dashboard.show(simulation.screen(lv_tick_get()));
        SDL_Delay(lv_timer_handler());
    }
}

} // namespace

int main(int argc, char **argv)
{
    lv_init();
    lv_tick_set_cb(ticks);

    if (argc >= 3 && std::strcmp(argv[1], "--screenshot") == 0) {
        return screenshot(argv[2], argc >= 4 ? std::atoi(argv[3]) : 0, !(argc >= 5 && std::strcmp(argv[4], "light") == 0));
    }
    float zoom = 1.0f;
    if (argc >= 3 && std::strcmp(argv[1], "--zoom") == 0) zoom = std::strtof(argv[2], nullptr);
    return window(zoom);
}
