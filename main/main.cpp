#include "benchmark.h"
#include "display.h"

extern "C" void app_main()
{
    lv_display_t *screen = display::start();
    {
        display::Lock lock;
        benchmark::start(screen);
    }
    display::set_brightness(85);
}
