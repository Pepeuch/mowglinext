#if BOARD_YARDFORCE500_VARIANT_ORIG


#include "stm32f_board_hal.h"
#include "board_config.h"
#include "board.h"

const hal_pin_t BOARD_STATUS_LED = {LED_GPIO_PORT, LED_PIN};
const hal_pin_t BOARD_RAIN_SENSOR = {RAIN_SENSOR_PORT, RAIN_SENSOR_PIN};
const hal_pin_t BOARD_TF4_SWITCH = {TF4_GPIO_PORT, TF4_PIN};

#endif 