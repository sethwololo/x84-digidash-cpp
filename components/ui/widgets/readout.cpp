#include "readout.h"

#include <cmath>

#include "base.h"
#include "theme.h"

namespace ui {

namespace {

constexpr int32_t SIZE = 90;
constexpr int32_t LABEL_GAP = 2;

} // namespace

Readout::Readout(lv_obj_t *parent, int32_t x, int32_t y, const theme::Font &font)
{
    const int32_t label_height = static_cast<int32_t>(std::lround(theme::LINE_HEIGHT * theme::TEXT_LABEL.size));
    label_ = text(parent, theme::TEXT_LABEL, theme::palette().label, x, y - label_height - LABEL_GAP);
    lv_obj_t *frame = outline(parent, x, y, SIZE, SIZE, theme::palette().border);
    value_ = text(frame, font, theme::palette().value, 0, 0, "--");
    center_text(value_, font);
}

void Readout::set_label(const char *label)
{
    set_text(label_, label);
}

void Readout::space_label(int32_t px)
{
    spaced(label_, px);
}

void Readout::set(const char *value, State state, bool hidden)
{
    set_text(value_, value);
    set_text_color(value_, state == State::fault ? theme::palette().fault : state == State::inactive ? theme::palette().label : theme::palette().value);
    set_visible(value_, !hidden);
}

} // namespace ui
