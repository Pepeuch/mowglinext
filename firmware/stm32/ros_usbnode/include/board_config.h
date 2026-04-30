#ifndef MOWGLI_BOARD_CONFIG_H
#define MOWGLI_BOARD_CONFIG_H

#include "hal/hal_gpio.h"
#include "hal/hal_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const hal_pin_t BOARD_STATUS_LED;
extern const hal_pin_t BOARD_RAIN_SENSOR;
extern const hal_pin_t BOARD_TF4_SWITCH;
extern const hal_pwm_t BOARD_PWM_CHARGE;
extern const hal_pwm_t BOARD_PWM_BEEPER;
extern const hal_pwm_t BOARD_PWM_BUZZER;
extern const hal_pin_t BOARD_SOFT_I2C_SCL;
extern const hal_pin_t BOARD_SOFT_I2C_SDA;
extern const hal_pin_t BOARD_TILT;
extern const hal_pin_t BOARD_STOP_BUTTON_YELLOW;
extern const hal_pin_t BOARD_STOP_BUTTON_WHITE;
extern const hal_pin_t BOARD_WHEEL_LIFT_BLUE;
extern const hal_pin_t BOARD_WHEEL_LIFT_RED;
extern const hal_pin_t BOARD_PLAY_BUTTON;
#ifdef __cplusplus
}
#endif

#endif /* MOWGLI_BOARD_CONFIG_H */