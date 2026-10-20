#include "bars.h"

#include <algorithm>
#include <cmath>

#include "base.h"
#include "theme.h"

namespace ui {

// Children are placed inside the 1 px outline: (0, 0) is the first pixel within it.

LevelBar::LevelBar(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height)
    : inner_width_(width - 2)
{
    lv_obj_t *frame = outline(parent, x, y, width, height, theme::palette().border);
    fill_ = fill(frame, 0, 0, 0, height - 2, theme::palette().value);
}

void LevelBar::set(float fraction, bool alert)
{
    set_width(fill_, static_cast<int32_t>(std::lround(inner_width_ * std::clamp(fraction, 0.0f, 1.0f))));
    set_fill_color(fill_, alert ? theme::palette().fault : theme::palette().value);
}

SegmentBar::SegmentBar(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, int segments)
    : inner_width_(width - 2), segments_(segments)
{
    lv_obj_t *frame = outline(parent, x, y, width, height, theme::palette().border);
    fill_ = fill(frame, 0, 0, 0, height - 2, theme::palette().value);
    // The rules between segments, over the fill.
    for (int i = 1; i < segments; i++) {
        fill(frame, static_cast<int32_t>(std::lround(static_cast<float>(inner_width_) * i / segments)), 0, 1, height - 2,
             theme::palette().border);
    }
}

void SegmentBar::set(int lit)
{
    set_width(fill_, inner_width_ * std::clamp(lit, 0, segments_) / segments_);
}

void scale(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, std::initializer_list<const char *> marks)
{
    lv_obj_t *row = box(parent, x, y, width, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    for (const char *mark : marks) text(row, theme::TEXT_SMALL, theme::palette().label, 0, 0, mark);
}

BarHeader::BarHeader(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, const char *name)
{
    lv_obj_t *header = box(parent, x, y, width, 32);
    value_ = text(header, theme::TEXT_VALUE_BOLD, theme::palette().value, 0, 0, "--");
    lv_obj_align(value_, LV_ALIGN_TOP_RIGHT, 0, text_y(theme::TEXT_VALUE_BOLD, 0));
    // Lowered by the difference of the font sizes times the baseline, to share it with the value.
    const int32_t name_top = static_cast<int32_t>(theme::BASELINE * (theme::TEXT_VALUE_BOLD.size - theme::TEXT_LABEL.size));
    spaced(text(header, theme::TEXT_LABEL, theme::palette().label, 0, name_top, name), theme::SPACING_LABEL);
}

void BarHeader::set(const char *value, bool alert)
{
    set_text(value_, value);
    set_text_color(value_, alert ? theme::palette().fault : theme::palette().value);
}

} // namespace ui
