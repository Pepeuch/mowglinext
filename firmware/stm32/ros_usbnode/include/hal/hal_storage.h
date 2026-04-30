#ifndef MOWGLI_HAL_STORAGE_H
#define MOWGLI_HAL_STORAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t hal_storage_read_u32(uint32_t key);
void hal_storage_write_u32(uint32_t key, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif