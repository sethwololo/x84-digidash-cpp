// hazard.svg, converted by tools/images.mjs: 10x10, A8.
#include "lvgl.h"

static const uint8_t img_hazard_map[] = {
    0,48,240,255,159,0,0,96,255,255,
    48,0,48,240,255,159,0,0,96,255,
    239,48,0,48,240,255,159,0,0,96,
    255,239,48,0,48,240,255,159,0,0,
    160,255,239,48,0,48,240,255,159,0,
    0,160,255,239,48,0,48,240,255,159,
    0,0,160,255,239,48,0,48,240,255,
    96,0,0,160,255,239,48,0,48,240,
    255,96,0,0,160,255,239,48,0,48,
    255,255,96,0,0,160,255,239,48,0,
};

const lv_image_dsc_t img_hazard = {
    .header = {.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_A8, .w = 10, .h = 10, .stride = 10},
    .data_size = sizeof(img_hazard_map),
    .data = img_hazard_map,
};
