#include "motor_backend.h"

#include "drivemotor.h"
#include "blademotor.h"

#define MOTOR_PWM_MAX 1000

static motor_backend_type_t current_backend = MOTOR_BACKEND_PAC_UART;

bool motor_backend_init(void)
{
    DRIVEMOTOR_Init();

#ifdef BLADEMOTOR_USART_ENABLED
    BLADEMOTOR_Init();
#endif

    return true;
}

void motor_backend_set_type(motor_backend_type_t type)
{
    current_backend = type;
}

bool motor_set_drive_pwm(int16_t left_pwm, int16_t right_pwm)
{
    // sécurité saturation
    if (left_pwm > MOTOR_PWM_MAX)
        left_pwm = MOTOR_PWM_MAX;
    if (left_pwm < -MOTOR_PWM_MAX)
        left_pwm = -MOTOR_PWM_MAX;

    if (right_pwm > MOTOR_PWM_MAX)
        right_pwm = MOTOR_PWM_MAX;
    if (right_pwm < -MOTOR_PWM_MAX)
        right_pwm = -MOTOR_PWM_MAX;

    switch (current_backend)
    {
    case MOTOR_BACKEND_PAC_UART:
        DRIVEMOTOR_SetSpeedSigned(left_pwm, right_pwm);
        return true;

    case MOTOR_BACKEND_PWM:
    case MOTOR_BACKEND_DRONECAN:
        return false;
    }

    return false; // sécurité fallback
}

bool motor_set_blade_pwm(int16_t blade_pwm)
{
    if (blade_pwm > MOTOR_PWM_MAX) blade_pwm = MOTOR_PWM_MAX;
    if (blade_pwm < -MOTOR_PWM_MAX) blade_pwm = -MOTOR_PWM_MAX;

    switch (current_backend)
    {
    case MOTOR_BACKEND_PAC_UART:
#ifdef BLADEMOTOR_USART_ENABLED
        if (blade_pwm == 0) {
            BLADEMOTOR_Set(0, 0);
        } else {
            BLADEMOTOR_Set(1, blade_pwm < 0);
        }
        return true;
#else
        return false;
#endif

    case MOTOR_BACKEND_PWM:
    case MOTOR_BACKEND_DRONECAN:
        return false;
    }

    return false;
}

motor_feedback_type_t motor_feedback_capability(void)
{
    return MOTOR_FEEDBACK_GPIO_ONLY;
}

bool motor_get_feedback(motor_feedback_t *feedback)
{
    if (!feedback)
    {
        return false;
    }

    feedback->left_pwm = 0;
    feedback->right_pwm = 0;
    feedback->blade_pwm = 0;
    feedback->left_rpm = 0;
    feedback->right_rpm = 0;

#ifdef BLADEMOTOR_USART_ENABLED
    feedback->blade_rpm = (int16_t)BLADEMOTOR_u16RPM;
    feedback->fault_flags = (uint16_t)BLADEMOTOR_u32Error;
#else
    feedback->blade_rpm = 0;
    feedback->fault_flags = 0;
#endif

    return true;
}

bool motor_backend_is_healthy(void)
{
    return true;
}
