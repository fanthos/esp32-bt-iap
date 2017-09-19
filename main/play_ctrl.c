#include "play_ctrl.h"
#include "bt_app_av.h"
#include "uart_rc.h"

void play_control(uint8_t ctrl) {
    bt_app_rc_ctrl(ctrl);
}

uint8_t play_status() {
    return 0;
}