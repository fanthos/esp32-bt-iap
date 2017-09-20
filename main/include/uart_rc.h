#ifndef UART_RC_H
#define UART_RC_H

#include "driver/uart.h"

void uart_app_start();

void uart_app_stop();

void uart_app_write(const char *buf, uint32_t len);
void uart_update_status(uint8_t status);

enum {
    UART_APP_UPDATE = UART_EVENT_MAX + 1
};

#endif