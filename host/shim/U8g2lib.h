// The firmware's CJK fonts. They are read from disk in hostInit() (host/fonts/, see extract_fonts.py);
// if missing they are nullptr and setFont() falls back to the classic font.
#pragma once
#include <stdint.h>

extern const uint8_t *u8g2_font_unifont_t_japanese3;
extern const uint8_t *u8g2_font_unifont_t_korean2;
