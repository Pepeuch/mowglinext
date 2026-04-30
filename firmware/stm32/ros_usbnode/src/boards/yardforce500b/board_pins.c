#if BOARD_YARDFORCE500_VARIANT_B

#include "stm32f_board_hal.h"
#include "board_config.h"
#include "board.h"

extern TIM_HandleTypeDef TIM1_Handle;
extern TIM_HandleTypeDef TIM3_Handle;
extern TIM_HandleTypeDef TIM4_Handle;

const hal_pin_t BOARD_STATUS_LED = {LED_GPIO_PORT, LED_PIN};
const hal_pin_t BOARD_RAIN_SENSOR = {RAIN_SENSOR_PORT, RAIN_SENSOR_PIN};
const hal_pin_t BOARD_TF4_SWITCH = {TF4_GPIO_PORT, TF4_PIN};
const hal_pwm_t BOARD_PWM_CHARGE = {&TIM3_Handle, TIM_CHANNEL_1};
const hal_pwm_t BOARD_PWM_BEEPER = {&TIM4_Handle, TIM_CHANNEL_1};
const hal_pwm_t BOARD_PWM_BUZZER = {&TIM1_Handle, TIM_CHANNEL_1};
const hal_pin_t BOARD_SOFT_I2C_SCL = {SOFT_I2C_SCL_PORT, SOFT_I2C_SCL_PIN};
const hal_pin_t BOARD_SOFT_I2C_SDA = {SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN};
const hal_pin_t BOARD_TILT = {TILT_PORT, TILT_PIN};
const hal_pin_t BOARD_STOP_BUTTON_YELLOW = {STOP_BUTTON_YELLOW_PORT, STOP_BUTTON_YELLOW_PIN};
const hal_pin_t BOARD_STOP_BUTTON_WHITE = {STOP_BUTTON_WHITE_PORT, STOP_BUTTON_WHITE_PIN};
const hal_pin_t BOARD_WHEEL_LIFT_BLUE = {WHEEL_LIFT_BLUE_PORT, WHEEL_LIFT_BLUE_PIN};
const hal_pin_t BOARD_WHEEL_LIFT_RED = {WHEEL_LIFT_RED_PORT, WHEEL_LIFT_RED_PIN};
const hal_pin_t BOARD_PLAY_BUTTON = {PLAY_BUTTON_PORT, PLAY_BUTTON_PIN};

#endif 