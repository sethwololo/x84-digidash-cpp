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

Door::Door(lv_obj_t *parent, lv_point_precise_t closed_from, lv_point_precise_t closed_to, lv_point_precise_t open_from,
           lv_point_precise_t open_to)
    : closed_{closed_from, closed_to}, open_{open_from, open_to}
{
    line_ = lv_line_create(parent);
    lv_line_set_points(line_, closed_, 2);
    lv_obj_set_style_line_width(line_, 3, 0);
    lv_obj_set_style_line_rounded(line_, true, 0);
    lv_obj_set_style_line_color(line_, theme::palette().value, 0);
}

void Door::set(bool open)
{
    if (open == is_open_) return;
    is_open_ = open;
    lv_line_set_points(line_, open ? open_ : closed_, 2);
    lv_obj_set_style_line_color(line_, open ? theme::palette().fault : theme::palette().value, 0);
}

namespace {

// Where things are in the car drawing (176x176 inside the box's 1 px outline): the sides of the body at the doors,
// and the doors' extent along them. Doors hinge at their front end and swing out by about 35 degrees.
constexpr float LEFT = 44;
constexpr float RIGHT = 132;
constexpr float FRONT_DOOR = 72;
constexpr float REAR_DOOR = 100;
constexpr float REAR_DOOR_END = 124;
constexpr float SWING_OUT = 0.57f;  // sin 35
constexpr float SWING_BACK = 0.82f; // cos 35

lv_point_precise_t point(float x, float y)
{
    return {static_cast<lv_value_precise_t>(x), static_cast<lv_value_precise_t>(y)};
}

// A door on the left (side -1) or right (side 1) side, from `from` to `to` along it.
Door side_door(lv_obj_t *box, int side, float from, float to)
{
    const float x = side < 0 ? LEFT : RIGHT;
    const float length = to - from - 2;
    return Door(box, point(x, from + 1), point(x, to - 1), point(x, from + 1),
                point(x + side * length * SWING_OUT, from + 1 + length * SWING_BACK));
}

lv_obj_t *diagram_box(lv_obj_t *parent, int32_t x, int32_t y)
{
    lv_obj_t *box = outline(parent, x, y, 178, 178, theme::palette().border);
    image(box, &img_car_top, theme::palette().label, 0, 0);
    return box;
}

} // namespace

DoorDiagram::DoorDiagram(lv_obj_t *parent, int32_t x, int32_t y)
    : box_(diagram_box(parent, x, y)),
      front_left_(side_door(box_, -1, FRONT_DOOR, REAR_DOOR)),
      front_right_(side_door(box_, 1, FRONT_DOOR, REAR_DOOR)),
      rear_left_(side_door(box_, -1, REAR_DOOR, REAR_DOOR_END)),
      rear_right_(side_door(box_, 1, REAR_DOOR, REAR_DOOR_END)),
      // The trunk lid's rear edge; open, it shows lifted, a little further back.
      trunk_(box_, point(66, 152), point(110, 152), point(66, 160), point(110, 160))
{
}

void DoorDiagram::set(const x84::Doors &doors)
{
    front_left_.set(doors.front_left);
    front_right_.set(doors.front_right);
    rear_left_.set(doors.rear_left);
    rear_right_.set(doors.rear_right);
    trunk_.set(doors.trunk);
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
