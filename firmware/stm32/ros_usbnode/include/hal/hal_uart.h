#ifndef MOWGLI_HAL_UART_H
#define MOWGLI_HAL_UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *instance;   // pointeur vers UART handle STM32
} hal_uart_t;

int hal_uart_tx(hal_uart_t *uart, const uint8_t *data, uint16_t len, uint32_t timeout);
int hal_uart_rx(hal_uart_t *uart, uint8_t *data, uint16_t len, uint32_t timeout);
int hal_uart_tx_dma(hal_uart_t *uart, const uint8_t *data, uint16_t len);
#ifdef __cplusplus
}
#endif

#endif