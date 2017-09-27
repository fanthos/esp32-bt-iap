#ifndef PLAY_CTRL_H
#define PLAY_CTRL_H

#include <stdint.h>

enum PLAY_CONTROL{
    PC_NONE = 0,
    PC_STOP = 1,
    PC_PLAY,
    PC_PAUSE,
    PC_NEXT,
    PC_PREV,
    PC_TOGGLE,
    PC_SEEK = 0x40,
    PC_TRACK
};

void play_control(uint8_t ctrl);
void play_set_status(uint8_t status);

uint8_t play_status();
uint32_t play_get_time();

#endif
