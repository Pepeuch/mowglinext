#include "hal/hal_uart.h"
#include "stm32f_board_hal.h"

int hal_uart_tx(hal_uart_t *uart, const uint8_t *data, uint16_t len, uint32_t timeout)
{
    return HAL_UART_Transmit((UART_HandleTypeDef *)uart->instance,
                             (uint8_t *)data,
                             len,
                             timeout);
}

int hal_uart_rx(hal_uart_t *uart, uint8_t *data, uint16_t len, uint32_t timeout)
{
    return HAL_UART_Receive((UART_HandleTypeDef *)uart->instance,
                            data,
                            len,
                            timeout);
}

int hal_uart_tx_dma(hal_uart_t *uart, const uint8_t *data, uint16_t len)
{
    return HAL_UART_Transmit_DMA((UART_HandleTypeDef *)uart->instance,
                                  (uint8_t *)data,
                                  len);
}