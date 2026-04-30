#ifndef MOWGLI_HAL_ADC_H
#define MOWGLI_HAL_ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t battery_voltage_raw;
    uint16_t charge_voltage_raw;
    uint16_t charge_current_raw;
    uint16_t charger_input_voltage_raw;
    uint16_t ntc_raw;
} hal_adc_charging_snapshot_t;

hal_adc_charging_snapshot_t hal_adc_get_charging_snapshot(void);

#ifdef __cplusplus
}
#endif

#endif