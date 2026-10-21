#!/usr/bin/env bash
# Converts the Nordwand Mono TTFs (assets/fonts, release 1.3.1 of github.com/tywr/Nordwand-Mono) into LVGL bitmap fonts (components/ui/fonts), one C file per
# weight and size. Run it after changing the list below; the C files are committed, so the build does not need it.
#
# Each font only embeds the characters the dashboard can show: printable ASCII plus the Portuguese capitals and the
# degree sign for text, just the digits for the big numbers (a full 190 px font would cost
# about 1.5 MB of flash).
set -e
cd "$(dirname "$0")/.."

TEXT="0x20-0x7E,0xB0,0xC0-0xC3,0xC7,0xC9,0xCA,0xCD,0xD3-0xD5,0xDA"
DIGITS="0x2D,0x30-0x39"

font() { # name ttf size range
  echo "$1"
  npx --yes lv_font_conv@1.5.3 --font "assets/fonts/$2" --size "$3" --range "$4" \
    --bpp 4 --no-compress --format lvgl --lv-include lvgl.h --lv-font-name "$1" -o "components/ui/fonts/$1.c"
}

font nordwand_regular_12 NordwandMono-Regular.ttf 12 "$TEXT"
font nordwand_regular_14 NordwandMono-Regular.ttf 14 "$TEXT"
font nordwand_regular_18 NordwandMono-Regular.ttf 18 "$TEXT"
font nordwand_semibold_14 NordwandMono-SemiBold.ttf 14 "$TEXT"
font nordwand_semibold_18 NordwandMono-SemiBold.ttf 18 "$TEXT"
font nordwand_semibold_24 NordwandMono-SemiBold.ttf 24 "$TEXT"
font nordwand_semibold_80 NordwandMono-SemiBold.ttf 80 "$DIGITS,0x4E,0x52" # RPM and gear: digits, "-", N, R
font nordwand_semibold_190 NordwandMono-SemiBold.ttf 190 "$DIGITS"
