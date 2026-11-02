// El host no duerme: usbPresent() es siempre true y lightSleepAllowed() nunca deja llegar aqui.
#pragma once
#include <stdint.h>

inline void esp_sleep_enable_timer_wakeup(uint64_t) {}
inline void esp_sleep_enable_gpio_wakeup() {}
inline void esp_light_sleep_start() {}
