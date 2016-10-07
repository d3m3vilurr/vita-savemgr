#include <string.h>

#include <psp2/types.h>
#include <psp2/system_param.h>
#include <psp2/apputil.h>
#include <psp2/ctrl.h>

#include <vita2d.h>

#include "common.h"
#include "button.h"
#include "font.h"

#define SCREEN_ROW      27
#define ROW_HEIGHT      20

static vita2d_pgf* debug_font;

int SCE_CTRL_ENTER;
int SCE_CTRL_CANCEL;
char ICON_ENTER[4];
char ICON_CANCEL[4];


void init_console() {
    vita2d_set_clear_color(black);

    debug_font = load_system_fonts();

    SceAppUtilInitParam init_param = {0};
    SceAppUtilBootParam boot_param = {0};
    sceAppUtilInit(&init_param, &boot_param);

    int enter_button;

    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter_button);
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

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
}

void draw_text(uint32_t y, char* text, uint32_t color) {
    int i;
    for (i = 0; i < 3; i++){
        vita2d_start_drawing();
        vita2d_pgf_draw_text(debug_font, 2, (y + 1) * ROW_HEIGHT, color, 1.0, text);
        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}

void draw_loop_text(uint32_t y, char *text, uint32_t color) {
    vita2d_pgf_draw_text(debug_font, 2, (y + 1) * ROW_HEIGHT, color, 1.0, text);
}

void clear_screen() {
    int i;
    for (i = 0; i < 3; i++){
        vita2d_start_drawing();
        vita2d_clear_screen();
        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}


