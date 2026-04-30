#ifndef MOWGLI_HAL_TIME_H
#define MOWGLI_HAL_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t hal_millis(void);
void hal_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* MOWGLI_HAL_TIME_H */
