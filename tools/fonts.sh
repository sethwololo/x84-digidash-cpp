#!/usr/bin/env bash
# Converts the IBM Plex Mono TTFs (assets/fonts) into LVGL bitmap fonts (components/ui/fonts), one C file per
# weight and size. Run it after changing the list below; the C files are committed, so the build does not need it.
#
# Each font only embeds the characters the dashboard can show: printable ASCII plus the Portuguese capitals, the
# degree sign and the middle dot for text, just the digits for the big numbers (a full 190 px font would cost
# about 1.5 MB of flash).
set -e
cd "$(dirname "$0")/.."

TEXT="0x20-0x7E,0xB0,0xB7,0xC0-0xC3,0xC7,0xC9,0xCA,0xCD,0xD3-0xD5,0xDA"
DIGITS="0x2D,0x30-0x39"

font() { # name ttf size range
  echo "$1"
  npx --yes lv_font_conv@1.5.3 --font "assets/fonts/$2" --size "$3" --range "$4" \
    --bpp 4 --no-compress --format lvgl --lv-include lvgl.h --lv-font-name "$1" -o "components/ui/fonts/$1.c"
}

font plex_regular_12 IBMPlexMono-Regular.ttf 12 "$TEXT"
font plex_regular_14 IBMPlexMono-Regular.ttf 14 "$TEXT"
font plex_regular_18 IBMPlexMono-Regular.ttf 18 "$TEXT"
font plex_bold_14 IBMPlexMono-Bold.ttf 14 "$TEXT"
font plex_bold_18 IBMPlexMono-Bold.ttf 18 "$TEXT"
font plex_bold_24 IBMPlexMono-Bold.ttf 24 "$TEXT"
font plex_bold_40 IBMPlexMono-Bold.ttf 40 "$DIGITS,0x4E,0x52" # gear: digits, N, R
font plex_bold_80 IBMPlexMono-Bold.ttf 80 "$DIGITS"
font plex_bold_190 IBMPlexMono-Bold.ttf 190 "$DIGITS"
