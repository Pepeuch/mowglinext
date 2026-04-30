#if BOARD_YARDFORCE500_VARIANT_ORIG


#include "stm32f_board_hal.h"
#include "board_config.h"
#include "board.h"

extern TIM_HandleTypeDef TIM1_Handle;
extern TIM_HandleTypeDef TIM3_Handle;
extern TIM_HandleTypeDef TIM4_Handle;

const hal_pin_t BOARD_STATUS_LED = {LED_GPIO_PORT, LED_PIN};
const hal_pin_t BOARD_RAIN_SENSOR = {RAIN_SENSOR_PORT, RAIN_SENSOR_PIN};
const hal_pin_t BOARD_TF4_SWITCH = {TF4_GPIO_PORT, TF4_PIN};
const hal_pwm_t BOARD_PWM_CHARGE = { &TIM1_Handle, TIM_CHANNEL_1 };
const hal_pwm_t BOARD_PWM_BEEPER = { &TIM3_Handle, TIM_CHANNEL_4 };
const hal_pwm_t BOARD_PWM_BUZZER = { &TIM4_Handle, TIM_CHANNEL_3 };

#endif 