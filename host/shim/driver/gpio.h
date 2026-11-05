#pragma once

typedef int gpio_num_t;
enum { GPIO_INTR_NEGEDGE, GPIO_INTR_LOW_LEVEL };

inline void gpio_wakeup_enable(gpio_num_t, int) {}
inline void gpio_wakeup_disable(gpio_num_t) {}
inline void gpio_set_intr_type(gpio_num_t, int) {}
inline void gpio_hold_en(gpio_num_t) {}
inline void gpio_hold_dis(gpio_num_t) {}
