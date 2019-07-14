#include <string.h>

#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#include <psp2/shellutil.h>
#include <psp2/system_param.h>
#include <psp2/touch.h>

#include "common.h"
#include "input.h"

void init_input() {
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,
                             SCE_TOUCH_SAMPLING_STATE_START);

    int enter_button;

    SceAppUtilInitParam init_param = {0};
    SceAppUtilBootParam boot_param = {0};
    sceAppUtilInit(&init_param, &boot_param);

    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON,
                                &enter_button);

    if (enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE) {
        SCE_CTRL_ENTER = SCE_CTRL_CIRCLE;
        SCE_CTRL_CANCEL = SCE_CTRL_CROSS;
        strcpy(ICON_ENTER, ICON_CIRCLE);
        strcpy(ICON_CANCEL, ICON_CROSS);
    } else {
        SCE_CTRL_ENTER = SCE_CTRL_CROSS;
        SCE_CTRL_CANCEL = SCE_CTRL_CIRCLE;
        strcpy(ICON_ENTER, ICON_CROSS);
        strcpy(ICON_CANCEL, ICON_CIRCLE);
    }
}

void lock_psbutton() {
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN |
                     SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU);
}

void unlock_psbutton() {
    sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN |
                       SCE_SHELL_UTIL_LOCK_TYPE_QUICK_MENU);
}

int read_buttons() {
    SceCtrlData pad = {0};
    static int old;
    static int hold_times;
    int curr, btn;

    sceCtrlPeekBufferPositive(0, &pad, 1);

    if (pad.ly < 0x10) {
        pad.buttons |= SCE_CTRL_UP;
    } else if (pad.ly > 0xef) {
        pad.buttons |= SCE_CTRL_DOWN;
    } else if (pad.lx < 0x10) {
        pad.buttons |= SCE_CTRL_LEFT;
    } else if (pad.lx > 0xef) {
        pad.buttons |= SCE_CTRL_RIGHT;
    }

    curr = pad.buttons;
    btn = pad.buttons & ~old;
    if (curr && old == curr) {
        hold_times += 1;
        if (hold_times >= 10) {
            btn = curr;
            hold_times = 8;
            btn |= SCE_CTRL_HOLD;
        }
    } else {
        hold_times = 0;
        old = curr;
    }
    return btn;
}

#define lerp(value, from_max, to_max) \
    ((((value * 10) * (to_max * 10)) / (from_max * 10)) / 10)

int read_touchscreen(point *p) {
    SceTouchData touch = {0};
    static int old_report_num = 0;
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

    // prevent hold
    if (old_report_num == touch.reportNum) {
        return 0;
    }

    old_report_num = touch.reportNum;

    if (!touch.reportNum) {
        return 0;
    }

    p->x = lerp(touch.report[0].x, 1919, SCREEN_WIDTH);
    p->y = lerp(touch.report[0].y, 1087, SCREEN_HEIGHT);

    return 1;
}

#undef lerp
