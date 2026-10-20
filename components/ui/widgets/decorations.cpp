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

constexpr int32_t BORDER = 1;
constexpr int32_t STRIPES_SIZE = 10;
// The label's text ends this far from the frame's right edge, clear of the stripes in the corner. Its backing still
// reaches the border, hiding what is under it; the stripes are drawn over it.
constexpr int32_t LABEL_INSET = BORDER + STRIPES_SIZE + 6;

} // namespace

AlertFrame::AlertFrame(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, const char *label)
{
    frame_ = outline(parent, x, y, width, height, theme::palette().fault);
    if (label) {
        // A backing in the background color keeps the label readable over a bar that is also red, and hides what is under it.
        lv_obj_t *tag = spaced(text(frame_, theme::TEXT_LABEL_BOLD, theme::palette().fault, 0, 0, label), theme::SPACING_LABEL);
        lv_obj_set_style_bg_opa(tag, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(tag, theme::palette().background, 0);
        lv_obj_set_style_pad_left(tag, 3, 0);
        lv_obj_set_style_pad_right(tag, LABEL_INSET - BORDER, 0);
        lv_obj_align(tag, LV_ALIGN_BOTTOM_RIGHT, 0, -2);
    }
    // Inside the border, in the top left and bottom right corners (the frame's children start inside its border).
    image(frame_, &img_hazard, theme::palette().fault, 0, 0);
    image(frame_, &img_hazard, theme::palette().fault, width - 2 * BORDER - STRIPES_SIZE, height - 2 * BORDER - STRIPES_SIZE);
    lv_obj_add_flag(frame_, LV_OBJ_FLAG_HIDDEN);
}

void AlertFrame::show(bool shown)
{
    if (shown && lv_obj_has_flag(frame_, LV_OBJ_FLAG_HIDDEN)) lv_obj_move_foreground(frame_);
    set_visible(frame_, shown);
}

} // namespace ui
