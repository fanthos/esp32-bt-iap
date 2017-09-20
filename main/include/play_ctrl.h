#ifndef PLAY_CTRL_H
#define PLAY_CTRL_H

#include <stdint.h>

enum PLAY_CONTROL{
    PC_STOP = 0,
    PC_PLAY = 1,
    PC_PAUSE = 2,
    PC_NEXT = 3,
    PC_PREV = 4
};

void play_control(uint8_t ctrl);
void play_set_status(uint8_t status);

uint8_t play_status();

#endif
