#include "hal/hal_gpio.h"

#include "stm32f_board_hal.h"

void hal_gpio_write(hal_pin_t pin, bool value)
{
    HAL_GPIO_WritePin((GPIO_TypeDef *)pin.port, pin.pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool hal_gpio_read(hal_pin_t pin)
{
    return HAL_GPIO_ReadPin((GPIO_TypeDef *)pin.port, pin.pin) == GPIO_PIN_SET;
}
