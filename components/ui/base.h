#pragma once

// Building blocks for the widgets: plain LVGL objects without the default theme, and setters that only touch an
// object when the value really changes. Every change makes LVGL repaint the object, and the dashboard writes every
// value on every frame, so writing an unchanged value must cost nothing.

#include "lvgl.h"
#include "theme.h"

namespace ui {

// A plain rectangle: no background, border, padding or scrolling until styled. Positions are relative to `parent`.
lv_obj_t *box(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height);

// An outlined box: 1 px border in `color`, no fill.
lv_obj_t *outline(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, lv_color_t color);

// A filled rectangle (rules, bar fills).
lv_obj_t *fill(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, lv_color_t color);

// A text whose line box starts at `top`, as the design places it (1.3 em tall, baseline at 1.025 em). The fonts
// tools/fonts.sh makes have other line heights (each only fits the glyphs it holds), so this places the label by
// its baseline, wherever the font puts it.
lv_obj_t *text(lv_obj_t *parent, const theme::Font &font, lv_color_t color, int32_t x, int32_t top, const char *content = "");

// The y to give a label in `font` (for lv_obj_align) so that it sits like a design text box starting at `top`.
int32_t text_y(const theme::Font &font, int32_t top);

// Centers a label in its parent, its baseline where a centered design text box would have it.
void center_text(lv_obj_t *label, const theme::Font &font);

// Spreads the letters of a label by `px`, and returns it.
lv_obj_t *spaced(lv_obj_t *label, int32_t px);

// An A8 image (images.h), drawn in `color`.
lv_obj_t *image(lv_obj_t *parent, const lv_image_dsc_t *source, lv_color_t color, int32_t x, int32_t y);

// Setters that do nothing when the value is already the current one.
void set_text(lv_obj_t *label, const char *content);
void set_visible(lv_obj_t *object, bool visible);
void set_text_color(lv_obj_t *object, lv_color_t color);
void set_border_color(lv_obj_t *object, lv_color_t color);
void set_fill_color(lv_obj_t *object, lv_color_t color);
void set_image_color(lv_obj_t *object, lv_color_t color);
void set_width(lv_obj_t *object, int32_t width);
void set_x(lv_obj_t *object, int32_t x);

} // namespace ui
