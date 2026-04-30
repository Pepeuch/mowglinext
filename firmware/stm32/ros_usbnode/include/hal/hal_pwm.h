#ifndef MOWGLI_HAL_PWM_H
#define MOWGLI_HAL_PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *htim;
    uint32_t channel;
} hal_pwm_t;

void hal_pwm_set(hal_pwm_t pwm, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif