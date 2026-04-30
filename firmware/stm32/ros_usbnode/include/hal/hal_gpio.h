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

typedef enum {
    HAL_GPIO_MODE_INPUT,
    HAL_GPIO_MODE_OUTPUT_PP,
    HAL_GPIO_MODE_OUTPUT_OD,
} hal_gpio_mode_t;

typedef enum {
    HAL_GPIO_PULL_NONE,
    HAL_GPIO_PULL_UP,
    HAL_GPIO_PULL_DOWN,
} hal_gpio_pull_t;

void hal_gpio_write(hal_pin_t pin, bool value);
bool hal_gpio_read(hal_pin_t pin);
void hal_gpio_configure(hal_pin_t pin, hal_gpio_mode_t mode, hal_gpio_pull_t pull);
void hal_gpio_deinit(hal_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* MOWGLI_HAL_GPIO_H */