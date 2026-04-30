#include "hal/hal_adc.h"
#include "stm32f_board_hal.h"

extern volatile uint16_t adc_u16BatteryVoltage;
extern volatile uint16_t adc_u16Current;
extern volatile uint16_t adc_u16ChargerVoltage;
extern volatile uint16_t adc_u16ChargerInputVoltage;
extern volatile uint16_t adc_u16Input_NTC;

hal_adc_charging_snapshot_t hal_adc_get_charging_snapshot(void)
{
    hal_adc_charging_snapshot_t snapshot;

    __disable_irq();
    snapshot.battery_voltage_raw = adc_u16BatteryVoltage;
    snapshot.charge_current_raw = adc_u16Current;
    snapshot.charge_voltage_raw = adc_u16ChargerVoltage;
    snapshot.charger_input_voltage_raw = adc_u16ChargerInputVoltage;
    snapshot.ntc_raw = adc_u16Input_NTC;
    __enable_irq();

    return snapshot;
}