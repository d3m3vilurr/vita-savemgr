#include <psp2/ctrl.h>

#include "common.h"
#include "button.h"

int read_btn() {
    SceCtrlData pad = {0};
    static int old;
    static int hold_times;
    int curr, btn;

    sceCtrlPeekBufferPositive(0, &pad, 1);

    if (pad.ly < 0x10) {
        pad.buttons |= SCE_CTRL_UP;
    } else if (pad.ly > 0xef) {
        pad.buttons |= SCE_CTRL_DOWN;
    }
    curr = pad.buttons;
    btn = pad.buttons & ~old;
    if (curr && old == curr) {
        hold_times += 1;
        if (hold_times >= 10) {
            btn = curr;
            hold_times = 10;
            btn |= SCE_CTRL_HOLD;
        }
    } else {
        hold_times = 0;
        old = curr;
    }
    return btn;
}

