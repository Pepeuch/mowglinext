#include "hal/hal_pwm.h"
#include "stm32f_board_hal.h"

void hal_pwm_set(hal_pwm_t pwm, uint32_t value)
{
    __HAL_TIM_SET_COMPARE((TIM_HandleTypeDef *)pwm.htim, pwm.channel, value);
}