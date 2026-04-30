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

void hal_gpio_configure(hal_pin_t pin, hal_gpio_mode_t mode, hal_gpio_pull_t pull)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin.pin;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;

    switch (mode) {
    case HAL_GPIO_MODE_INPUT:
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        break;
    case HAL_GPIO_MODE_OUTPUT_PP:
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        break;
    case HAL_GPIO_MODE_OUTPUT_OD:
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        break;
    }

    switch (pull) {
    case HAL_GPIO_PULL_NONE:
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        break;
    case HAL_GPIO_PULL_UP:
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
    case HAL_GPIO_PULL_DOWN:
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
    }

    HAL_GPIO_Init((GPIO_TypeDef *)pin.port, &GPIO_InitStruct);
}

void hal_gpio_deinit(hal_pin_t pin)
{
    HAL_GPIO_DeInit((GPIO_TypeDef *)pin.port, pin.pin);
}