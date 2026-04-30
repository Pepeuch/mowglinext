#ifndef MOWGLI_HAL_I2C_H
#define MOWGLI_HAL_I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *hi2c;
} hal_i2c_t;

int hal_i2c_mem_write(hal_i2c_t *bus, uint16_t dev_addr, uint16_t reg,
                      uint16_t mem_addr_size, const uint8_t *data,
                      uint16_t len, uint32_t timeout);

int hal_i2c_mem_read(hal_i2c_t *bus, uint16_t dev_addr, uint16_t reg,
                     uint16_t mem_addr_size, uint8_t *data,
                     uint16_t len, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif