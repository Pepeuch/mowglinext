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
#ifdef __cplusplus
}
#endif

#endif /* MOWGLI_BOARD_CONFIG_H */