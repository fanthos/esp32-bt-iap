#ifndef UART_RC_H
#define UART_RC_H

void uart_app_start();

void uart_app_stop();

void uart_app_write(const uint8_t *buf, uint32_t len);

#endif