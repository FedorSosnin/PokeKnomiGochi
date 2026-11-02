// Fuentes CJK del firmware. Se leen de disco en hostInit() (host/fonts/, ver extract_fonts.py);
// si faltaran son nullptr y setFont() cae a la fuente clasica.
#pragma once
#include <stdint.h>

extern const uint8_t *u8g2_font_unifont_t_japanese3;
extern const uint8_t *u8g2_font_unifont_t_korean2;
