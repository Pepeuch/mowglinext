#include "hal/hal_i2c.h"
#include "stm32f_board_hal.h"

int hal_i2c_mem_write(hal_i2c_t *bus, uint16_t dev_addr, uint16_t reg,
                      uint16_t mem_addr_size, const uint8_t *data,
                      uint16_t len, uint32_t timeout)
{
    return HAL_I2C_Mem_Write((I2C_HandleTypeDef *)bus->hi2c,
                             dev_addr,
                             reg,
                             mem_addr_size,
                             (uint8_t *)data,
                             len,
                             timeout);
}

int hal_i2c_mem_read(hal_i2c_t *bus, uint16_t dev_addr, uint16_t reg,
                     uint16_t mem_addr_size, uint8_t *data,
                     uint16_t len, uint32_t timeout)
{
    return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)bus->hi2c,
                            dev_addr,
                            reg,
                            mem_addr_size,
                            data,
                            len,
                            timeout);
}