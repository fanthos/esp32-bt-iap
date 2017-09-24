#include "play_ctrl.h"
#include "bt_app_av.h"
#include "uart_rc.h"

static uint8_t m_play_status = PC_STOP;
static uint32_t m_play_ticks = 0;

void play_control(uint8_t ctrl) {
    bt_app_rc_ctrl(ctrl);
}

void play_set_status(uint8_t status) {
    m_play_status = status;
    if(status == PC_STOP || status == PC_PAUSE) {
        m_play_ticks = 0;
    } else if(status == PC_PLAY) {
        m_play_ticks = xTaskGetTickCount();
    }
    uart_update_status(status);
}

uint8_t play_status() {
    return m_play_status;
}

uint32_t play_get_time() {
    if(m_play_ticks == 0) {
        return 0;
    }
    portTickType new_ticks = xTaskGetTickCount();
    return (new_ticks - m_play_ticks) * portTICK_PERIOD_MS;
}
