#include "base.h"

#include <cmath>
#include <cstring>

namespace ui {

lv_obj_t *box(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height)
{
    lv_obj_t *object = lv_obj_create(parent);
    lv_obj_remove_style_all(object);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
    return object;
}

lv_obj_t *outline(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, lv_color_t color)
{
    lv_obj_t *object = box(parent, x, y, width, height);
    lv_obj_set_style_border_width(object, 1, 0);
    lv_obj_set_style_border_color(object, color, 0);
    return object;
}

lv_obj_t *fill(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, lv_color_t color)
{
    lv_obj_t *object = box(parent, x, y, width, height);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(object, color, 0);
    return object;
}

int32_t text_y(const theme::Font &font, int32_t top)
{
    // From the top of LVGL's line box, the baseline is at line_height - base_line.
    const int32_t baseline = top + static_cast<int32_t>(std::lround(theme::BASELINE * font.size));
    return baseline - (font.lv->line_height - font.lv->base_line);
}

lv_obj_t *text(lv_obj_t *parent, const theme::Font &font, lv_color_t color, int32_t x, int32_t top, const char *content)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font.lv, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_pos(label, x, text_y(font, top));
    lv_label_set_text(label, content);
    return label;
}

void center_text(lv_obj_t *label, const theme::Font &font)
{
    // A centered design box has its baseline (1.025 - 1.3 / 2) em below the center; a centered LVGL label has it
    // line_height / 2 - base_line below.
    const float design = (theme::BASELINE - theme::LINE_HEIGHT / 2) * font.size;
    const float lvgl = font.lv->line_height / 2.0f - font.lv->base_line;
    lv_obj_align(label, LV_ALIGN_CENTER, 0, static_cast<int32_t>(std::lround(design - lvgl)));
}

lv_obj_t *spaced(lv_obj_t *label, int32_t px)
{
    lv_obj_set_style_text_letter_space(label, px, 0);
    return label;
}

lv_obj_t *image(lv_obj_t *parent, const lv_image_dsc_t *source, lv_color_t color, int32_t x, int32_t y)
{
    lv_obj_t *object = lv_image_create(parent);
    lv_image_set_src(object, source);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_style_image_recolor(object, color, 0);
    lv_obj_set_style_image_recolor_opa(object, LV_OPA_COVER, 0);
    return object;
}

void set_text(lv_obj_t *label, const char *content)
{
    if (std::strcmp(lv_label_get_text(label), content) != 0) lv_label_set_text(label, content);
}

void set_visible(lv_obj_t *object, bool visible)
{
    if (lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN) == visible) lv_obj_set_flag(object, LV_OBJ_FLAG_HIDDEN, !visible);
}

void set_text_color(lv_obj_t *object, lv_color_t color)
{
    if (!lv_color_eq(lv_obj_get_style_text_color(object, LV_PART_MAIN), color)) lv_obj_set_style_text_color(object, color, 0);
}

void set_border_color(lv_obj_t *object, lv_color_t color)
{
    if (!lv_color_eq(lv_obj_get_style_border_color(object, LV_PART_MAIN), color)) lv_obj_set_style_border_color(object, color, 0);
}

void set_fill_color(lv_obj_t *object, lv_color_t color)
{
    if (!lv_color_eq(lv_obj_get_style_bg_color(object, LV_PART_MAIN), color)) lv_obj_set_style_bg_color(object, color, 0);
}

void set_image_color(lv_obj_t *object, lv_color_t color)
{
    if (!lv_color_eq(lv_obj_get_style_image_recolor(object, LV_PART_MAIN), color)) {
        lv_obj_set_style_image_recolor(object, color, 0);
    }
}

void set_width(lv_obj_t *object, int32_t width)
{
    if (lv_obj_get_style_width(object, LV_PART_MAIN) != width) lv_obj_set_width(object, width);
}

void set_x(lv_obj_t *object, int32_t x)
{
    if (lv_obj_get_style_x(object, LV_PART_MAIN) != x) lv_obj_set_x(object, x);
}

} // namespace ui
