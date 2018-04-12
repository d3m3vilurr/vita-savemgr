#include <string.h>
#include <stdlib.h>
#include <psp2/apputil.h>
#include <psp2/system_param.h>
#include <psp2/ctrl.h>

#include "common.h"
#include "display.h"
#include "input.h"
#include "util.h"

void init_console() {
    int enter_button;

    SceAppUtilInitParam init_param = {0};
    SceAppUtilBootParam boot_param = {0};
    sceAppUtilInit(&init_param, &boot_param);

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

    confirm_msg = calloc(sizeof(char), 1);
    aprintf(&confirm_msg, "%s CANCEL    %s CONFIRM", ICON_CANCEL, ICON_ENTER);
    confirm_msg_width = vita2d_pgf_text_width(font, 1.0, confirm_msg);

    close_msg = calloc(sizeof(char), 1);
    aprintf(&close_msg, "%s CLOSE", ICON_ENTER);
    close_msg_width = vita2d_pgf_text_width(font, 1.0, close_msg);
}

// TODO: support multi line
int confirm(const char *msg, float zoom) {
    vita2d_start_drawing();

    int text_width = vita2d_pgf_text_width(font, zoom, msg);
    int text_height = vita2d_pgf_text_height(font, zoom, msg);

    int padding = 50;
    int width = text_width + (padding * 2);
    int height = text_height + (padding * 2);

    int left = (SCREEN_WIDTH - width) / 2;
    int top = (SCREEN_HEIGHT - height) / 2;

    vita2d_draw_rectangle(left, top, width, height, LIGHT_GRAY);

    vita2d_pgf_draw_text(font, left + padding, top + padding, BLACK, zoom, msg);
    vita2d_pgf_draw_text(font,
                         left + ((width - confirm_msg_width) / 2),
                         top + height - 25, BLACK, zoom, confirm_msg);
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();

    while (1) {
        int btn = read_buttons();
        if (btn & SCE_CTRL_HOLD) {
            continue;
        }
        if (btn & SCE_CTRL_ENTER) {
            return CONFIRM;
        }
        if (btn & SCE_CTRL_CANCEL) {
            return CANCEL;
        }
    }
}

// TODO: support multi line
int alert(const char *msg, float zoom) {
    vita2d_start_drawing();
    int text_width = vita2d_pgf_text_width(font, zoom, msg);
    int text_height = vita2d_pgf_text_height(font, zoom, msg);

    int padding = 50;
    int width = text_width + (padding * 2);
    int height = text_height + (padding * 2);

    int left = (SCREEN_WIDTH - width) / 2;
    int top = (SCREEN_HEIGHT - height) / 2;

    vita2d_draw_rectangle(left, top, width, height, LIGHT_GRAY);

    vita2d_pgf_draw_text(font, left + padding, top + padding, BLACK, zoom, msg);
    vita2d_pgf_draw_text(font,
                         left + ((width - close_msg_width) / 2),
                         top + height - 25, BLACK, zoom, close_msg);

    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();

    while (1) {
        int btn = read_buttons();
        if (btn & SCE_CTRL_HOLD) {
            continue;
        }
        if (btn & SCE_CTRL_ENTER) {
            return 0;
        }
    }
}

static int progress_max = 0;
static int progress_curr = 0;

void draw_progress() {
    vita2d_start_drawing();

    int width = 500;
    int height = 140;
    int left = (SCREEN_WIDTH - width) / 2;
    int top = (SCREEN_HEIGHT - height) / 2;

    int gauge_width = 400;
    int gauge_height = 40;
    int gauge_left = left + 50;
    int gauge_top = top + 50;

    vita2d_draw_rectangle(left, top, width, height, LIGHT_GRAY);
    vita2d_draw_rectangle(gauge_left, gauge_top, gauge_width, gauge_height, BLACK);

    int progress_width = progress_curr * gauge_width / progress_max;

    vita2d_draw_rectangle(gauge_left, gauge_top, progress_width, gauge_height, LIGHT_SKY_BLUE);

    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();
}

void init_progress(int max) {
    progress_max = max;
    progress_curr = 0;
    draw_progress();
}

void incr_progress() {
    progress_curr += 1;
    draw_progress();
}
