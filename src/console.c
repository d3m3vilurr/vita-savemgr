#include <stdio.h>
#include <string.h>

#include <psp2/types.h>
#include <psp2/system_param.h>
#include <psp2/apputil.h>
#include <psp2/ctrl.h>

#include <vita2d.h>

#include "common.h"
#include "button.h"
#include "console.h"
#include "font.h"

#define SCREEN_WIDTH            960
#define SCREEN_HEIGHT           544
#define SCREEN_STRIDE_IN_PIXELS 1024
#define SCREEN_HALF_WIDTH       (SCREEN_WIDTH / 2)
#define SCREEN_HALF_HEIGHT      (SCREEN_HEIGHT / 2)
#define SCREEN_ROW              27
#define ROW_HEIGHT              20
#define BOX_MIN_WIDTH           350
#define BOX_BORDER              2
#define BOX_PADDING             20

#define CENTER(a, b) (((a) - (b)) / 2)

static vita2d_pgf* font;

int SCE_CTRL_ENTER;
int SCE_CTRL_CANCEL;
char ICON_ENTER[4];
char ICON_CANCEL[4];

char confirm_msg[256];
int confirm_msg_width;

void init_console() {
    vita2d_set_clear_color(black);

    font = load_system_fonts();

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

    snprintf(confirm_msg, 256, "%s Cancel    %s Confirm", ICON_CANCEL, ICON_ENTER);
    confirm_msg_width = vita2d_pgf_text_width(font, 1.0, confirm_msg);
}

void draw_start() {
    vita2d_start_drawing();
    vita2d_clear_screen();
}

void draw_end() {
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();
}

void draw_text(uint32_t y, const char* text, uint32_t color) {
    for (int i = 0; i < 3; i++){
        vita2d_start_drawing();
        vita2d_pgf_draw_text(font, 2, (y + 1) * ROW_HEIGHT, color, 1.0, text);
        draw_end();
    }
}

void draw_loop_text(uint32_t y, const char *text, uint32_t color) {
    vita2d_pgf_draw_text(font, 2, (y + 1) * ROW_HEIGHT, color, 1.0, text);
}

void clear_screen() {
    for (int i = 0; i < 3; i++){
        draw_start();
        draw_end();
    }
}

typedef struct point {
    int x;
    int y;
} point;

point draw_rectangle(int width, int height, int border, int padding, int color) {
    point p;
    p.x = SCREEN_HALF_WIDTH - (width / 2);
    p.y = SCREEN_HALF_HEIGHT - (height / 2);

    vita2d_draw_rectangle(p.x - border - padding,
                          p.y - border - padding,
                          width + (border * 2) + (padding * 2),
                          height + (border * 2) + (padding * 2),
                          color);

    vita2d_draw_rectangle(p.x - padding,
                          p.y - padding,
                          width + (padding * 2),
                          height + (padding * 2),
                          black);

    return p;
}

void draw_popup(popup_type type, const char *lines[]) {
    int max_width = 0;
    const char *max_line = NULL;
    int cnt = 0;
    while (lines[cnt]) {
        const char *line = lines[cnt];
        cnt += 1;
        int width = vita2d_pgf_text_width(font, 1.0, line);
        if (width <= max_width) {
            continue;
        }
        max_line = line;
        max_width = width;
    }
    if (!max_line) {
        return;
    }

    int height;
    int line;
    switch (type) {
        case CONFIRM_AND_CANCEL:
            if (max_width < confirm_msg_width) {
                max_width = confirm_msg_width;
            }
            height = (cnt + 2) * ROW_HEIGHT;
            line = white;
            break;
        case WARNING:
            line = orange;
            height = cnt * ROW_HEIGHT;
            break;
        default:
            line = white;
            height = cnt * ROW_HEIGHT;
    }
    if (max_width < BOX_MIN_WIDTH) {
        max_width = BOX_MIN_WIDTH;
    }

    point p = draw_rectangle(max_width, height, BOX_BORDER, BOX_PADDING, line);

    for (int i = 0; i < cnt; i += 1) {
        vita2d_pgf_draw_text(font, p.x, p.y + ((i + 1) * ROW_HEIGHT), white, 1.0, lines[i]);
    }

    switch (type) {
        case CONFIRM_AND_CANCEL:
            vita2d_pgf_draw_text(font,
                                 SCREEN_HALF_WIDTH - (confirm_msg_width / 2),
                                 p.y + ((cnt + 2) * ROW_HEIGHT),
                                 white,
                                 1.0,
                                 confirm_msg);
            break;
        default:
            break;
    }
}

uint32_t backup_fb[SCREEN_STRIDE_IN_PIXELS * SCREEN_HEIGHT];

void open_popup(popup_type type, const char *lines[]) {
    uint32_t *fb = vita2d_get_current_fb();
    memcpy(backup_fb, fb, sizeof(backup_fb));

    for (int i = 0; i < 3; i += 1) {
        vita2d_start_drawing();
        draw_popup(type, lines);
        draw_end();
    }
}

void close_popup() {
    for (int i = 0; i < 3; i += 1) {
        vita2d_start_drawing();

        uint32_t *fb = vita2d_get_current_fb();
        memcpy(fb, backup_fb, sizeof(backup_fb));

        draw_end();
    }
}
