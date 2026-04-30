#ifndef MOWGLI_MOTOR_BACKEND_H
#define MOWGLI_MOTOR_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOTOR_BACKEND_PAC_UART,
    MOTOR_BACKEND_DRONECAN,
    MOTOR_BACKEND_PWM,
} motor_backend_type_t;

typedef enum {
    MOTOR_FEEDBACK_NONE,
    MOTOR_FEEDBACK_GPIO_ONLY,
    MOTOR_FEEDBACK_TELEMETRY,
} motor_feedback_type_t;

typedef struct {
    int16_t left_pwm;
    int16_t right_pwm;
    int16_t blade_pwm;
    int16_t left_rpm;
    int16_t right_rpm;
    int16_t blade_rpm;
    uint16_t fault_flags;
} motor_feedback_t;

bool motor_backend_init(void);
void motor_backend_set_type(motor_backend_type_t type);

bool motor_set_drive_pwm(int16_t left_pwm, int16_t right_pwm);
bool motor_set_blade_pwm(int16_t blade_pwm);

motor_feedback_type_t motor_feedback_capability(void);
bool motor_get_feedback(motor_feedback_t *feedback);
bool motor_backend_is_healthy(void);

#ifdef __cplusplus
}
#endif

#endif /* MOWGLI_MOTOR_BACKEND_H */