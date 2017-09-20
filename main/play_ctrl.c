#include "play_ctrl.h"
#include "bt_app_av.h"
#include "uart_rc.h"

static uint8_t m_play_status = PC_STOP;

void play_control(uint8_t ctrl) {
    bt_app_rc_ctrl(ctrl);
}

void play_set_status(uint8_t status) {
    m_play_status = status;
    uart_update_status(status);
}

uint8_t play_status() {
    return m_play_status == PC_PLAY;
}
