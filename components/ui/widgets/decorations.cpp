#include "decorations.h"

#include "base.h"
#include "images.h"
#include "theme.h"

namespace ui {

void corner(lv_obj_t *parent, int32_t x, int32_t y, bool flip_x, bool flip_y)
{
    constexpr int32_t SIZE = 24;
    constexpr int32_t THICKNESS = 2;
    lv_obj_t *bracket = box(parent, x, y, SIZE, SIZE);
    fill(bracket, 0, flip_y ? SIZE - THICKNESS : 0, SIZE, THICKNESS, theme::palette().label);
    fill(bracket, flip_x ? SIZE - THICKNESS : 0, 0, THICKNESS, SIZE, theme::palette().label);
    outline(bracket, flip_x ? 5 : 10, flip_y ? 5 : 10, 9, 9, theme::palette().border);
}

namespace {

// The hazard stripes stick out of the frame by this much, sideways and up or down.
constexpr int32_t STRIPES_OUT_X = 1;
constexpr int32_t STRIPES_OUT_Y = 5;
constexpr int32_t STRIPES_SIZE = 14;

} // namespace

AlertFrame::AlertFrame(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, const char *label)
{
    // LVGL clips children to their parent: the frame and the stripes sit side by side in a slightly bigger group.
    group_ = box(parent, x - STRIPES_OUT_X, y - STRIPES_OUT_Y, width + 2 * STRIPES_OUT_X, height + 2 * STRIPES_OUT_Y);
    lv_obj_t *frame = outline(group_, STRIPES_OUT_X, STRIPES_OUT_Y, width, height, theme::palette().fault);
    image(group_, &img_hazard, theme::palette().fault, 0, 0);
    image(group_, &img_hazard, theme::palette().fault, width + 2 * STRIPES_OUT_X - STRIPES_SIZE, height + 2 * STRIPES_OUT_Y - STRIPES_SIZE);
    if (label) {
        // A black backing keeps the label readable over a bar that is also red, and hides what is under it.
        lv_obj_t *tag = spaced(text(frame, theme::TEXT_LABEL_BOLD, theme::palette().fault, 0, 0, label), theme::SPACING_LABEL);
        lv_obj_set_style_bg_opa(tag, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(tag, theme::palette().background, 0);
        lv_obj_set_style_pad_left(tag, 3, 0);
        lv_obj_set_style_pad_right(tag, 1, 0);
        lv_obj_align(tag, LV_ALIGN_BOTTOM_RIGHT, -3, -2);
    }
    lv_obj_add_flag(group_, LV_OBJ_FLAG_HIDDEN);
}

void AlertFrame::show(bool shown)
{
    if (shown && lv_obj_has_flag(group_, LV_OBJ_FLAG_HIDDEN)) lv_obj_move_foreground(group_);
    set_visible(group_, shown);
}

} // namespace ui
