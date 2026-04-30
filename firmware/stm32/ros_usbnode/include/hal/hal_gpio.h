#ifndef MOWGLI_HAL_GPIO_H
#define MOWGLI_HAL_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *port;
    uint16_t pin;
} hal_pin_t;

void hal_gpio_write(hal_pin_t pin, bool value);
bool hal_gpio_read(hal_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* MOWGLI_HAL_GPIO_H */
