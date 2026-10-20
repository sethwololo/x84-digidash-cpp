#pragma once

// Everything around the three columns: corner brackets, the header (brand, message, system status), the footer
// (CAN bus, firmware) and the rules between them.

#include "lvgl.h"
#include "screen.h"

namespace ui {

class Frame {
public:
    explicit Frame(lv_obj_t *screen);
    // `alert`: some value or lamp is in alarm ("ALERTA" in the header).
    void show(const Screen &screen, bool alert);

private:
    lv_obj_t *notice_;
    lv_obj_t *status_;
    lv_obj_t *can_;
    lv_obj_t *firmware_;
};

} // namespace ui
