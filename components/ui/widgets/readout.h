#pragma once

// A boxed value with a caption above it: the row of readouts under the tachometer.

#include "lvgl.h"
#include "theme.h"

namespace ui {

class Readout {
public:
    // (x, y) is the top of the box; the caption sits 2 px above it. `font` sets the size of the value.
    Readout(lv_obj_t *parent, int32_t x, int32_t y, const theme::Font &font);

    enum class State {
        normal,
        fault,    // in the fault color
        inactive, // a remembered value with nothing being applied (a suspended speed limiter), in the label color
    };

    void set_label(const char *label);
    // Spreads the caption's letters (only MARCHA is spaced in the design).
    void space_label(int32_t px);
    // `hidden` blanks the value (blinking).
    void set(const char *value, State state = State::normal, bool hidden = false);

private:
    lv_obj_t *label_;
    lv_obj_t *value_;
};

} // namespace ui
