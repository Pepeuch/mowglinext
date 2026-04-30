#include "hal/hal_time.h"

#include "stm32f_board_hal.h"

uint32_t hal_millis(void)
{
    return HAL_GetTick();
}

void hal_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}
