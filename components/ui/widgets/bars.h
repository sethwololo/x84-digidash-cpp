#pragma once

// Bars and what goes around them: a level bar, a segmented bar, a scale under a bar, and a header over it.

#include <initializer_list>

#include "lvgl.h"

namespace ui {

// Outlined bar filled from the left.
class LevelBar {
public:
    LevelBar(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height);
    // `fraction` from 0 to 1; `alert` fills in the fault color.
    void set(float fraction, bool alert);

private:
    lv_obj_t *fill_;
    int32_t inner_width_;
};

// Outlined bar split into equal segments by thin rules, the first `lit` of them filled.
class SegmentBar {
public:
    SegmentBar(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, int32_t height, int segments);
    void set(int lit);

private:
    lv_obj_t *fill_;
    int32_t inner_width_;
    int segments_;
};

// Labels spread evenly under a bar: the first flush left, the last flush right.
void scale(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, std::initializer_list<const char *> marks);

// Caption on the left, value on the right, sharing a baseline.
class BarHeader {
public:
    BarHeader(lv_obj_t *parent, int32_t x, int32_t y, int32_t width, const char *name);
    void set(const char *value, bool alert);

private:
    lv_obj_t *value_;
};

} // namespace ui
