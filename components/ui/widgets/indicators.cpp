#include "indicators.h"

#include <cmath>

#include "base.h"
#include "images.h"
#include "theme.h"

namespace ui {

IconChip::IconChip(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, const char *label)
{
    frame_ = outline(parent, x, y, width, height, theme::palette().border);
    label_ = text(frame_, theme::TEXT_LABEL_BOLD, theme::palette().border, 0, 0, label);
    center_text(label_, theme::TEXT_LABEL_BOLD);
}

void IconChip::set(x84::Lamp lamp, lv_color_t color, bool blink_on)
{
    const bool shown = lamp == x84::Lamp::on || (lamp == x84::Lamp::blinking && blink_on);
    set_border_color(frame_, shown ? color : theme::palette().border);
    set_text_color(label_, shown ? color : theme::palette().border);
}

namespace {

// The diagram is drawn for 154 px and scaled to the 178 px box.
constexpr float K = 178.0f / 154.0f;

int32_t scaled(float px)
{
    return static_cast<int32_t>(std::lround(px * K));
}

// Where the car's outline starts in the box, as drawn (children of an outlined box start inside its 1 px border).
constexpr float CAR_X = 35;
constexpr float CAR_Y = 7;

// A door or the trunk at (x, y) from the car's outline: outlined, filled in the fault color when open. Doors stick
// out of the car, so they are drawn over it rather than inside it (LVGL clips children to their parent).
lv_obj_t *opening(lv_obj_t *box, float x, float y, float width, float height)
{
    lv_obj_t *door = outline(box, scaled(CAR_X + x) - 1, scaled(CAR_Y + y) - 1, scaled(width), scaled(height), theme::palette().label);
    lv_obj_set_style_bg_opa(door, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(door, theme::palette().background, 0);
    return door;
}

void show_open(lv_obj_t *door, bool open)
{
    set_border_color(door, open ? theme::palette().fault : theme::palette().label);
    set_fill_color(door, open ? theme::palette().fault : theme::palette().background);
}

} // namespace

DoorDiagram::DoorDiagram(lv_obj_t *parent, int32_t x, int32_t y)
{
    lv_obj_t *frame = outline(parent, x, y, 178, 178, theme::palette().border);
    lv_obj_t *car = outline(frame, scaled(CAR_X) - 1, scaled(CAR_Y) - 1, scaled(84), scaled(140), theme::palette().label);
    lv_obj_set_style_radius(car, scaled(14), 0);
    fill(car, scaled(9) - 1, scaled(37) - 1, scaled(66), 1, theme::palette().border); // windshield
    fill(car, scaled(9) - 1, scaled(101) - 1, scaled(66), 1, theme::palette().border); // rear window
    front_left_ = opening(frame, -3, 41, 6, 26);
    front_right_ = opening(frame, 81, 41, 6, 26);
    rear_left_ = opening(frame, -3, 71, 6, 26);
    rear_right_ = opening(frame, 81, 71, 6, 26);
    trunk_ = opening(frame, 25, 137, 34, 6);
}

void DoorDiagram::set(const x84::Doors &doors)
{
    show_open(front_left_, doors.front_left);
    show_open(front_right_, doors.front_right);
    show_open(rear_left_, doors.rear_left);
    show_open(rear_right_, doors.rear_right);
    show_open(trunk_, doors.trunk);
}

TurnSignal::TurnSignal(lv_obj_t *parent, int32_t x, int32_t y, bool right)
{
    // Drawn 1:1 from images rasterized at 36x36: scaled on the device, the diagonal edges would get jagged.
    arrow_ = image(parent, right ? &img_arrow_right : &img_arrow_left, theme::palette().border, x, y);
}

void TurnSignal::set(bool lit)
{
    set_image_color(arrow_, lit ? theme::palette().signal : theme::palette().border);
}

} // namespace ui
