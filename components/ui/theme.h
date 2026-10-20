#pragma once

// Design tokens shared by every widget: colors, type scale, scales of the gauges and the screen layout. The
// measurements follow the Rust/Slint version (ui/theme.slint), which follows the design (1920x480).

#include "fonts.h"
#include "lvgl.h"

namespace theme {

// Colors, by role. Few levels on purpose: on the real screen, and in sunlight, close grays look the same. Opaque
// colors only: filling with an opaque color is much cheaper than blending.
struct Palette {
    lv_color_t background;
    lv_color_t value;  // numbers, values, filled bars
    lv_color_t label;  // labels, units, scales, header and footer, corners
    lv_color_t border; // outlines, rules, lamps and arrows off
    // Accents
    lv_color_t fault;     // warning / fault (red)
    lv_color_t high_beam; // high beam (blue)
    lv_color_t signal;    // turn signal / fog light (green)
    lv_color_t service;   // service / airbag (yellow)
};

// At night (low or high beam on): light on black, the design's colors.
inline const Palette DARK{
    .background = lv_color_hex(0x000000),
    .value = lv_color_hex(0xffffff),
    .label = lv_color_hex(0x9a9a9a),
    .border = lv_color_hex(0x5a5a5a),
    .fault = lv_color_hex(0xfb2c36),
    .high_beam = lv_color_hex(0x00a6f4),
    .signal = lv_color_hex(0x00bc7d),
    .service = lv_color_hex(0xfd9a00),
};

// By day: dark on light gray, readable in sunlight at full backlight. The accents are the same hues, fully saturated
// (one channel full, one off) so they stay apart from each other on the panel. Darker shades score better on paper
// but blur into the same brown-black there; the contrast against the background is noted.
inline const Palette LIGHT{
    .background = lv_color_hex(0xe6e6e6), // a light gray: the accents look more saturated than on white
    .value = lv_color_hex(0x000000),
    .label = lv_color_hex(0x5f5f5f),
    .border = lv_color_hex(0x969696),
    .fault = lv_color_hex(0xff0000),     // 3.2:1
    .high_beam = lv_color_hex(0x0020ff), // 6.3:1
    .signal = lv_color_hex(0x00a800),    // 2.5:1
    .service = lv_color_hex(0xff6a00),   // 2.3:1, further from the red
};

// The palette in use. Dashboard switches it, and rebuilds the screen with it, when day turns to night.
inline const Palette *current = &DARK;

inline const Palette &palette()
{
    return *current;
}

// A font and its size in px. The size places texts the way the design does (see ui::text in base.h).
struct Font {
    const lv_font_t *lv;
    int32_t size;
};

// Type scale (Nordwand Mono; the _BOLD sizes are its SemiBold). To add a size, see fonts.h.
inline const Font TEXT_SMALL{&nordwand_regular_12, 12};      // captions, scales, footer
inline const Font TEXT_LABEL{&nordwand_regular_14, 14};      // column titles, trip, odometer
inline const Font TEXT_LABEL_BOLD{&nordwand_semibold_14, 14};    // status, warning lights, alert labels
inline const Font TEXT_VALUE{&nordwand_regular_18, 18};      // units
inline const Font TEXT_VALUE_BOLD{&nordwand_semibold_18, 18};    // bar values
inline const Font TEXT_READOUT{&nordwand_semibold_24, 24};       // values in the readout boxes
inline const Font TEXT_GEAR{&nordwand_semibold_40, 40};
inline const Font TEXT_RPM{&nordwand_semibold_80, 80};
inline const Font TEXT_SPEED{&nordwand_semibold_190, 190};

// How the design places text, in em of the font size: a text box is a line 1.3 em tall, with the baseline 1.025 em
// from its top; digits start 0.315 em from the top. (Measured on the design's first font, IBM Plex Mono; Nordwand
// Mono has the same advance and ascender, so the layout holds.)
constexpr float LINE_HEIGHT = 1.3f;
constexpr float BASELINE = 1.025f;
constexpr float DIGIT_TOP = 0.315f;

// Letter spacing in px, as the design spaces captions, labels and units (ui::spaced).
constexpr int32_t SPACING_CAPTION = 1; // captions and the footer (1.5 px in the design)
constexpr int32_t SPACING_LABEL = 2;   // column titles, header, trip and odometer, alert labels
constexpr int32_t SPACING_UNIT = 6;    // "KM/H"

// Scales of the gauges. Warnings (engine temperature, battery, fuel reserve, oil level) come from the car's own
// alert signals, not from thresholds here.
constexpr int RPM_MAX = 7000;
constexpr int RPM_REDLINE = 6000;
constexpr int FUEL_CAPACITY = 59; // liters

// Screen frame: the corner brackets, the header and footer rules; the three columns fill what is left.
constexpr int32_t SCREEN_WIDTH = 1920;
constexpr int32_t SCREEN_HEIGHT = 480;
constexpr int32_t CORNER_INSET = 8; // corner brackets from the screen edges
constexpr int32_t MARGIN = 40;      // content from the left and right edges
constexpr int32_t HEADER_RULE = 44;
constexpr int32_t FOOTER_RULE = 436;
constexpr int32_t RULE_GAP = 18; // between the rules and the columns
constexpr int32_t CONTENT_TOP = HEADER_RULE + RULE_GAP;
constexpr int32_t CONTENT_HEIGHT = FOOTER_RULE - RULE_GAP - CONTENT_TOP;
constexpr int32_t CENTER_WIDTH = 800; // the speed, in the middle
constexpr int32_t SIDE_WIDTH = (SCREEN_WIDTH - 2 * MARGIN - CENTER_WIDTH) / 2;

// Blinking lamps and values switch every this many milliseconds.
constexpr uint32_t BLINK_MS = 500;

} // namespace theme
