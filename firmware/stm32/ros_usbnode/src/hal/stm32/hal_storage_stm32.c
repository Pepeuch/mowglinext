#include "hal/hal_storage.h"
#include "stm32f_board_hal.h"

extern RTC_HandleTypeDef hrtc;

uint32_t hal_storage_read_u32(uint32_t key)
{
    return HAL_RTCEx_BKUPRead(&hrtc, key);
}

void hal_storage_write_u32(uint32_t key, uint32_t value)
{
    HAL_RTCEx_BKUPWrite(&hrtc, key, value);
}