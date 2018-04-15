#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <psp2/ctrl.h>
#include <psp2/appmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/system_param.h>
#include <psp2/rtc.h>
#include <psp2/shellutil.h>
#include <psp2/kernel/modulemgr.h>
#include <vita2d.h>
#include <taihen.h>

#include "common.h"
#include "config.h"
#include "appdb.h"
#include "file.h"
#include "font.h"
#include "display.h"
#include "input.h"
#include "util.h"

vita2d_pgf* font;
SceUID patch_modid = -1;
SceUID kernel_modid = -1;
SceUID user_modid = -1;

int SCE_CTRL_ENTER;
int SCE_CTRL_CANCEL;
char ICON_ENTER[4];
char ICON_CANCEL[4];

char *confirm_msg;
int confirm_msg_width;
char *close_msg;
int close_msg_width;

typedef enum {
    UNKNOWN = 0,
    MAIN_SCREEN = 1,
    PRINT_APPINFO,
    BACKUP_MODE,
    BACKUP_CONFIRM,
    BACKUP_PROGRESS,
    BACKUP_FAIL,
    RESTORE_MODE,
    RESTORE_CONFIRM,
    RESTORE_PROGRESS,
    RESTORE_FAIL,
    DELETE_MODE,
    DELETE_CONFIRM,
    DELETE_PROGRESS,
    DELETE_FAIL,
    FORMAT_MODE,
    FORMAT_CONFIRM,
    FORMAT_PROGRESS,
    FORMAT_FAIL,
} ScreenState;

typedef enum {
    NO_ERROR = 0,
    ERROR_NO_SAVE_DIR = 1,
    ERROR_NO_SLOT_DIR,
    ERROR_MEMORY_ALLOC,
    ERROR_DECRYPT_DIR,
    ERROR_COPY_DIR,
} ProcessError;

typedef enum {
    USE_ICON = 1,
    USE_LIST,
} DrawListMode;

DrawListMode mainscreen_list_mode;

int select_row = 0;
int select_col = 0;
int select_appinfo_button = 0;

char *save_dir_path(const appinfo *info) {
    //if (strncmp(info->dev, "gro0", 4) == 0) {
    char *path = calloc(sizeof(char), 1);
    aprintf(&path, "grw0:savedata/%s", info->real_id);
    if (exists(path)) {
        return path;
    }
    free(path);
    //}

    path = calloc(sizeof(char), 1);
    aprintf(&path, "ux0:user/00/savedata/%s", info->real_id);
    if (exists(path)) {
        return path;
    }
    free(path);
    return NULL;
}

char *slot_dir_path(const appinfo *info, int slot) {
    char *t0 = calloc(sizeof(char), 1);
    char *t1 = calloc(sizeof(char), 1);
    char *path = calloc(sizeof(char), 1);
    aprintf(&t0, "%s/%s", config.base, config.slot_format);
    aprintf(&t1, t0, info->title_id, slot);
    aprintf(&path, "ux0:%s", t1);
    free(t1);
    free(t0);
    return path;
}

char *slot_sfo_path(const appinfo *info, int slot) {
    char *t0 = calloc(sizeof(char), 1);
    char *t1 = calloc(sizeof(char), 1);
    char *path = calloc(sizeof(char), 1);
    aprintf(&t0, "%s/%s/sce_sys/param.sfo", config.base, config.slot_format);
    aprintf(&t1, t0, info->title_id, slot);
    aprintf(&path, "ux0:%s", t1);
    free(t1);
    free(t0);
    return path;
}

void draw_icon(icon_data *icon, int row, int col) {
    if (config.use_dpad && row == select_row && col == select_col) {
        vita2d_draw_rectangle(ICON_LEFT(col) - ITEM_BOX_MARGIN,
                              ICON_TOP(row) - ITEM_BOX_MARGIN,
                              ICON_WIDTH + ITEM_BOX_MARGIN * 2,
                              ICON_HEIGHT + ITEM_BOX_MARGIN * 2,
                              WHITE);
    }
    icon->touch_area.left = ICON_LEFT(col);
    icon->touch_area.top = ICON_TOP(row);
    icon->touch_area.right = icon->touch_area.left + ICON_WIDTH;
    icon->touch_area.bottom = icon->touch_area.top + ICON_HEIGHT;

    if (!icon->texture) {
        return;
    }
    float w = vita2d_texture_get_width(icon->texture);
    float h = vita2d_texture_get_height(icon->texture);
    float z0 = ICON_WIDTH / w;
    float z1 = ICON_HEIGHT / h;
    float zoom = z0 < z1 ? z0 : z1;
    vita2d_draw_texture_scale_rotate_hotspot(icon->texture,
        ICON_LEFT(col) + (ICON_WIDTH / 2),
        ICON_TOP(row) + (ICON_HEIGHT / 2),
        zoom, zoom,
        0,
        w / 2,
        h / 2
    );
}

void draw_icons(appinfo *curr) {
    // __________tm_bat
    // |__|__|__|__|__|
    // |__|__|__|__|__|
    // |__|__|__|__|__|
    // |__|__|__|__|__|
    // ------helps-----

    vita2d_draw_rectangle(ITEMS_PANEL_LEFT, ITEMS_PANEL_TOP,
                          ITEMS_PANEL_WIDTH, ITEMS_PANEL_HEIGHT, BLACK);

    //draw icons
    //appinfo *tmp = curr;
    //i = 0;

    for (int i = 0; curr && i < (ICONS_COL * ICONS_ROW); i++, curr = curr->next) {
        load_icon(curr);
        draw_icon(&curr->icon, i / ICONS_COL, i % ICONS_COL);
    }
}

void draw_list_row(appinfo *curr, int row) {
    if (config.use_dpad && row == select_row) {
        vita2d_draw_rectangle(LIST_LEFT - ITEM_BOX_MARGIN,
                              LIST_TOP(row) - ITEM_BOX_MARGIN,
                              LIST_WIDTH + ITEM_BOX_MARGIN * 2,
                              LIST_HEIGHT + ITEM_BOX_MARGIN * 2,
                              WHITE);
        vita2d_draw_rectangle(LIST_LEFT,
                              LIST_TOP(row),
                              LIST_WIDTH,
                              LIST_HEIGHT,
                              BLACK);
    }

    load_icon(curr);
    icon_data *icon = &curr->icon;

    icon->touch_area.left = LIST_LEFT;
    icon->touch_area.top = LIST_TOP(row);
    icon->touch_area.right = icon->touch_area.left + LIST_WIDTH;
    icon->touch_area.bottom = icon->touch_area.top + LIST_HEIGHT;

    if (icon->texture) {
        float w = vita2d_texture_get_width(icon->texture);
        float h = vita2d_texture_get_height(icon->texture);
        float z0 = LIST_HEIGHT / w;
        float z1 = LIST_HEIGHT / h;
        float zoom = z0 < z1 ? z0 : z1;
        vita2d_draw_texture_scale_rotate_hotspot(icon->texture,
            LIST_LEFT + (LIST_HEIGHT / 2),
            LIST_TOP(row) + (LIST_HEIGHT / 2),
            zoom, zoom,
            0,
            w / 2,
            h / 2
        );
    }

    char *text = calloc(sizeof(char), 1);
    aprintf(&text, "%s %s", curr->title_id, curr->title);

    int text_height = vita2d_pgf_text_height(font, 1.0, text);
    int text_top_margin = (LIST_HEIGHT - text_height) / 2;

    vita2d_pgf_draw_text(font,
                         LIST_TEXT_LEFT,// + text_left_margin,
                         LIST_TOP(row) + text_top_margin + text_height,
                         WHITE, 1.0, text);
}

void draw_list(appinfo *curr) {
    // __________tm_bat
    // |__|___________|
    // |__|___________|
    // |__|___________|
    // |__|___________|
    // ------helps-----
    vita2d_draw_rectangle(ITEMS_PANEL_LEFT, ITEMS_PANEL_TOP,
                          ITEMS_PANEL_WIDTH, ITEMS_PANEL_HEIGHT, BLACK);

    for (int i = 0; curr && i < LIST_ROW; i++, curr = curr->next) {
        draw_list_row(curr, i);
    }
}

void draw_button(int left, int top, int width, int height, const char *text, float zoom,
                 int pressed) {
    // TODO render more looking button
    int text_color;
    if (pressed) {
        vita2d_draw_rectangle(left, top, width, height, BLACK);
        vita2d_draw_rectangle(left + 4, top + 4, width - 5, height - 5, LIGHT_GRAY);
        text_color = WHITE;
    } else {
        vita2d_draw_rectangle(left, top, width, height, BLACK);
        vita2d_draw_rectangle(left + 1, top + 1, width - 5, height - 5, WHITE);
        text_color = BLACK;
    }
    int text_width = vita2d_pgf_text_width(font, zoom, text);
    int text_height = vita2d_pgf_text_height(font, zoom, text);
    int text_left_margin = (width - text_width) / 2;
    int text_top_margin = (height - text_height) / 2;

    vita2d_pgf_draw_text(font,
                         left + text_left_margin,
                         top + text_top_margin + text_height,
                         text_color, zoom, text);
}

void draw_appinfo_icon(icon_data *icon) {
    if (!icon->texture) {
        return;
    }

    float w = vita2d_texture_get_width(icon->texture);
    float h = vita2d_texture_get_height(icon->texture);
    float z0 = APPINFO_ICON_WIDTH / w;
    float z1 = APPINFO_ICON_HEIGHT / h;
    float zoom = z0 < z1 ? z0 : z1;

    vita2d_draw_texture_scale_rotate_hotspot(icon->texture,
        APPINFO_ICON_LEFT + (APPINFO_ICON_WIDTH / 2),
        APPINFO_ICON_TOP + (APPINFO_ICON_HEIGHT / 2),
        zoom, zoom,
        0,
        w / 2,
        h / 2
    );
}

void draw_appinfo(ScreenState state, appinfo *info) {
    // __________tm_bat
    // |name |_|__|__|
    // |...  |_|__|__|
    // |...  |_|__|__|
    // |_____|_|__|__|
    // ------helps-----

    // .--------------------------------------
    // | icon  | backup   |
    // |       | restore  |
    // |       | format   |
    // |       | ...      |
    // |------------------|
    // | title id         |
    // | title            |
    // | cart /dl         |
    // | save position    |
    // | ...              |
    // '---------------------------------------
    vita2d_draw_rectangle(APPINFO_PANEL_LEFT, APPINFO_PANEL_TOP,
                          APPINFO_PANEL_WIDTH, APPINFO_PANEL_HEIGHT, WHITE);

    draw_appinfo_icon(&info->icon);

    draw_button(APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(0),
                APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT,
                "BACKUP", 1.0,
                (state >= BACKUP_MODE && state <= BACKUP_FAIL));

    draw_button(APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(1),
                APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT,
                "RESTORE", 1.0,
                (state >= RESTORE_MODE && state <= RESTORE_FAIL));

    draw_button(APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(2),
                APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT,
                "DELETE", 1.0,
                (state >= DELETE_MODE && state <= DELETE_FAIL));

    draw_button(APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(3),
                APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT,
                "FORMAT", 1.0,
                (state >= FORMAT_MODE && state <=FORMAT_FAIL));

    if (config.use_dpad && state == PRINT_APPINFO) {
        vita2d_draw_rectangle(APPINFO_BUTTON_LEFT,
                              APPINFO_BUTTON_TOP(select_appinfo_button),
                              APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT,
                              LIGHT_GRAY);
    }

    vita2d_draw_rectangle(APPINFO_DESC_LEFT, APPINFO_DESC_TOP,
                          APPINFO_DESC_WIDTH, APPINFO_DESC_HEIGHT,
                          LIGHT_SLATE_GRAY);

    vita2d_pgf_draw_text(font,
                         APPINFO_DESC_LEFT + APPINFO_DESC_PADDING,
                         APPINFO_DESC_TOP + APPINFO_DESC_PADDING + 20,
                         BLACK, 1.0, info->title);
    vita2d_pgf_draw_text(font,
                         APPINFO_DESC_LEFT + APPINFO_DESC_PADDING,
                         APPINFO_DESC_TOP + APPINFO_DESC_PADDING + 40,
                         BLACK, 1.0, info->title_id);

    char *savedir = save_dir_path(info);
    char *buf = calloc(sizeof(char), 1);
    aprintf(&buf, "save dir: %s", savedir ? savedir : "need to start game before action");

    vita2d_pgf_draw_text(font,
                         APPINFO_DESC_LEFT + APPINFO_DESC_PADDING,
                         APPINFO_DESC_TOP + APPINFO_DESC_PADDING + 80,
                         BLACK, 1.0, buf);
    if (savedir) {
        free(savedir);
    }
    if (buf) {
        free(buf);
    }
}

char *load_slot_string(const appinfo *info, int slot) {
    char *ret = NULL;
    char *fn = slot_sfo_path(info, slot);
    if (!exists(fn)) {
        goto exit;
    }
    SceIoStat stat = {0};
    sceIoGetstat(fn, &stat);
    SceDateTime time;
    SceRtcTick tick_utc;
    SceRtcTick tick_local;
    sceRtcGetTick(&stat.st_mtime, &tick_utc);
    sceRtcConvertUtcToLocalTime(&tick_utc, &tick_local);
    sceRtcSetTick(&time, &tick_local);

    ret = calloc(sizeof(char), 1);
    aprintf(&ret, "%04d-%02d-%02d %02d:%02d:%02d",
             time.year, time.month, time.day, time.hour, time.minute, time.second);

exit:
    free(fn);
    return ret;
}

void draw_slots(appinfo *info, int slot) {
    // __________tm_bat
    // |name |=======|
    // |...  |=======|
    // |...  |..     |
    // |_____|_______|
    // ------helps-----

    // .--------------------------------------
    // | icon  | backup   | slot0
    // |       | restore  | slot1
    // |       | format   |
    // |       | ...      |  ..
    // |------------------|
    // | title id         |
    // | title            |
    // | cart /dl         |
    // | save position    |
    // | ...              | slot9
    // '---------------------------------------
    vita2d_draw_rectangle(SLOT_PANEL_LEFT, SLOT_PANEL_TOP,
                          SLOT_PANEL_WIDTH, SLOT_PANEL_HEIGHT, WHITE);

    for (int i = 0; i < SLOT_BUTTON; i++) {
        char *slot_string = load_slot_string(info, i);

        draw_button(SLOT_BUTTON_LEFT, SLOT_BUTTON_TOP(i),
                    SLOT_BUTTON_WIDTH, SLOT_BUTTON_HEIGHT,
                    slot_string ? slot_string : "empty", 1.0,
                    (slot == i));

        if (slot_string) {
            free(slot_string);
        }
    }
}

// TODO support multi line
char* error_message(ProcessError error) {
    switch (error) {
        case NO_ERROR:
            return "NO ERROR";
        case ERROR_NO_SAVE_DIR:
            return "Could not find save directory";
        case ERROR_NO_SLOT_DIR:
            return "Could not find backup slot";
        case ERROR_MEMORY_ALLOC:
            return "Allocated memory exceeded";
        case ERROR_DECRYPT_DIR:
            return "Invalid license";
        case ERROR_COPY_DIR:
            return "Could not copy directory";
        default:
            return "UNKNOWN";
    }
}

#define IN_RANGE(start, end, value) (start < value && value < end)
#define IS_TOUCHED(rect, pt) \
    (IN_RANGE(rect.left, rect.right, pt.x) && IN_RANGE(rect.top, rect.bottom, pt.y))

ScreenState on_mainscreen_event_with_touch(int steps, int *step, appinfo **curr,
                                           appinfo **touched) {
    int moves = 0;
    switch (mainscreen_list_mode) {
        case USE_LIST:
            moves = 1;
            break;
        case USE_ICON:
        default:
            moves = ICONS_COL;
            break;
    }
    int btn = read_buttons();

    if (btn & SCE_CTRL_HOLD) {
        return UNKNOWN;
    }

    if (btn & SCE_CTRL_UP) {
        if (*step == 0) {
            return UNKNOWN;
        }
        *step -= 1;
        for (int i = 0; i < moves; i++, *curr = (*curr)->prev) {
            unload_icon(*curr);
        }
        return MAIN_SCREEN;
    }
    if (btn & SCE_CTRL_DOWN) {
        if (*step == steps) {
            return UNKNOWN;
        }
        *step += 1;
        for (int i = 0; i < moves; i++, *curr = (*curr)->next) {
            unload_icon(*curr);
        }
        return MAIN_SCREEN;
    }

    point p;
    if (!read_touchscreen(&p)) {
        return UNKNOWN;
    }

    appinfo *tmp = *curr;
    for (int i = 0; tmp && i < (ICONS_COL * ICONS_ROW); i++, tmp = tmp->next) {
        if (IS_TOUCHED(tmp->icon.touch_area, p)) {
            *touched = tmp;
            return PRINT_APPINFO;
        }
    }

    return UNKNOWN;
}

int selectable_count(appinfo *curr, int row, int col) {
    int selectable_count = 0;
    while (curr && selectable_count < (row * col)) {
        selectable_count += 1;
        curr = curr->next;
    }
    return selectable_count;
}

#define IS_OVERFLOW() ( \
        select_row * max_col + select_col >= \
        selectable_count(*curr, max_row, max_col) \
    )
ScreenState on_mainscreen_event_with_dpad(int steps, int *step, appinfo **curr,
                                          appinfo **touched) {
    int moves;
    int max_row;
    int max_col;
    switch (mainscreen_list_mode) {
        case USE_LIST:
            moves = 1;
            max_row = LIST_ROW;
            max_col = 1;
            break;
        case USE_ICON:
        default:
            moves = ICONS_COL;
            max_row = ICONS_ROW;
            max_col = ICONS_COL;
            break;
    }

    int btn = read_buttons();

    //if (btn & SCE_CTRL_HOLD) {
    //    return UNKNOWN;
    //}

    if (btn & SCE_CTRL_UP) {
        if (select_row == 0) {
            if (*step == 0) {
                return UNKNOWN;
            }
            *step -= 1;
            for (int i = 0; i < moves; i++, *curr = (*curr)->prev) {
                unload_icon(*curr);
            }
        } else {
            select_row -= 1;
        }
        return MAIN_SCREEN;
    }
    if (btn & SCE_CTRL_DOWN) {
        if (select_row + 1 == max_row) {
            if (*step == steps) {
                return UNKNOWN;
            }
            *step += 1;
            for (int i = 0; i < moves; i++, *curr = (*curr)->next) {
                unload_icon(*curr);
            }
        } else {
            select_row += 1;
        }
        if (IS_OVERFLOW()) {
            select_row -= 1;
        }
        return MAIN_SCREEN;
    }
    if (btn & SCE_CTRL_LEFT) {
        select_col -= 1;
        if (select_col < 0) {
            select_col = 0;
        }
        return MAIN_SCREEN;
    }
    if (btn & SCE_CTRL_RIGHT) {
        select_col += 1;
        if (select_col >= max_col) {
            select_col = max_col - 1;
        }
        if (IS_OVERFLOW()) {
            select_col -= 1;
        }
        return MAIN_SCREEN;
    }

    if (!(btn & SCE_CTRL_HOLD) && btn & SCE_CTRL_ENTER) {
        appinfo *tmp = *curr;
        for (int i = 0; tmp && i < (select_row * max_col) + select_col;
                i++, tmp = tmp->next);
        *touched = tmp;
        select_appinfo_button = 0;
        return PRINT_APPINFO;
    }

    return UNKNOWN;
}
#undef IS_OVERFLOW

ScreenState on_mainscreen_event(int steps, int *step, appinfo **curr,
                                appinfo **touched) {
    if (config.use_dpad) {
        return on_mainscreen_event_with_dpad(steps, step, curr, touched);
    }
    return on_mainscreen_event_with_touch(steps, step, curr, touched);
}

#define APPINFO_BUTTON_AREA(n) \
    { \
        .left = APPINFO_BUTTON_LEFT, \
        .top = APPINFO_BUTTON_TOP(n), \
        .right = APPINFO_BUTTON_LEFT + APPINFO_BUTTON_WIDTH, \
        .bottom = APPINFO_BUTTON_TOP(n) + APPINFO_BUTTON_HEIGHT, \
    }

ScreenState on_appinfo_button_event(point p) {
    static rectangle backup_button_area = APPINFO_BUTTON_AREA(0);
    static rectangle restore_button_area = APPINFO_BUTTON_AREA(1);
    static rectangle delete_button_area = APPINFO_BUTTON_AREA(2);
    static rectangle reset_button_area = APPINFO_BUTTON_AREA(3);
    if (IS_TOUCHED(backup_button_area, p)) {
        return BACKUP_MODE;
    }

    if (IS_TOUCHED(restore_button_area, p)) {
        return RESTORE_MODE;
    }

    if (IS_TOUCHED(delete_button_area, p)) {
        return DELETE_MODE;
    }

    if (IS_TOUCHED(reset_button_area, p)) {
        return FORMAT_MODE;
    }

    return UNKNOWN;
}

#undef APPINFO_BUTTON_AREA

ScreenState on_appinfo_event_with_touch() {
    static rectangle appinfo_area = {
        .left = APPINFO_PANEL_LEFT,
        .top = APPINFO_PANEL_TOP,
        .right = APPINFO_PANEL_LEFT + APPINFO_PANEL_WIDTH,
        .bottom = APPINFO_PANEL_TOP + APPINFO_PANEL_HEIGHT,
    };

    int btn = read_buttons();

    if (!(btn & SCE_CTRL_HOLD) && btn & SCE_CTRL_CANCEL) {
        return MAIN_SCREEN;
    }
    // TODO

    point p;
    if (!read_touchscreen(&p)) {
        return UNKNOWN;
    }

    if (!IS_TOUCHED(appinfo_area, p)) {
        return MAIN_SCREEN;
    }

    return on_appinfo_button_event(p);
}

ScreenState on_appinfo_event_with_dpad() {
    int btn = read_buttons();
    if (btn & SCE_CTRL_HOLD) {
        return UNKNOWN;
    }

    if (btn & SCE_CTRL_UP) {
        select_appinfo_button -= 1;
        if (select_appinfo_button < 0) {
            select_appinfo_button = 0;
        }
        return PRINT_APPINFO;
    }
    if (btn & SCE_CTRL_DOWN) {
        select_appinfo_button += 1;
        if (select_appinfo_button >= APPINFO_BUTTON) {
            select_appinfo_button = APPINFO_BUTTON - 1;
        }
        return PRINT_APPINFO;
    }
    if (btn & SCE_CTRL_CANCEL) {
        return MAIN_SCREEN;
    }
    if (btn & SCE_CTRL_ENTER) {
        switch (select_appinfo_button) {
            case 0:
                return BACKUP_MODE;
            case 1:
                return RESTORE_MODE;
            case 2:
                return DELETE_MODE;
            case 3:
                return FORMAT_MODE;
        }
    }
    return UNKNOWN;
}

ScreenState on_appinfo_event() {
    if (config.use_dpad) {
        return on_appinfo_event_with_dpad();
    }
    return on_appinfo_event_with_touch();
}


ScreenState on_slot_event(int *slot) {
    *slot = -1;
    int btn = read_buttons();

    if (!(btn & SCE_CTRL_HOLD) && btn & SCE_CTRL_CANCEL) {
        return PRINT_APPINFO;
    }

    point p;
    if (!read_touchscreen(&p)) {
        return UNKNOWN;
    }

    ScreenState move_appinfo_action = on_appinfo_button_event(p);
    if (move_appinfo_action != UNKNOWN) {
        return move_appinfo_action;
    }

    for (int i = 0; i < SLOT_BUTTON; i++) {
        rectangle slot_area = {
            .left = SLOT_BUTTON_LEFT,
            .top = SLOT_BUTTON_TOP(i),
            .right = SLOT_BUTTON_LEFT + SLOT_BUTTON_WIDTH,
            .bottom = SLOT_BUTTON_TOP(i) + SLOT_BUTTON_HEIGHT,
        };
        if (IS_TOUCHED(slot_area, p)) {
            *slot = i;
            return UNKNOWN;
        }
    }
    return UNKNOWN;
}

int copy_savedata_to_slot(appinfo *info, int slot) {
    int res = NO_ERROR;
    char *src = save_dir_path(info);
    char *dest = slot_dir_path(info, slot);

    printf("src: %s\n", src);
    printf("dest: %s\n", dest);

    if (!src) {
        // TODO: need popup; need start game
        res = ERROR_NO_SAVE_DIR;
        goto exit;
    }
    if (!dest) {
        res = ERROR_MEMORY_ALLOC;
        goto exit;
    }

    lock_psbutton();
    if (pfs_mount(src) < 0) {
        res = ERROR_DECRYPT_DIR;
        goto exit;
    }
    mkdir(dest, 0777);

    init_progress(file_count(src, 1));
    if (copydir(src, dest, incr_progress) < 0) {
        res = ERROR_COPY_DIR;
        goto exit;
    }
exit:
    pfs_unmount();
    unlock_psbutton();
    if (src) {
        free(src);
    }
    if (dest) {
        free(dest);
    }
    return res;
}

int copy_slot_to_savedata(appinfo *info, int slot) {
    int res = 0;
    char *src = slot_dir_path(info, slot);
    char *dest = save_dir_path(info);

    printf("src: %s\n", src);
    printf("dest: %s\n", dest);

    if (!dest) {
        res = ERROR_NO_SAVE_DIR;
        goto exit;
    }

    if (!src) {
        res = ERROR_MEMORY_ALLOC;
        goto exit;
    }
    if (!exists(src)) {
        res = ERROR_NO_SLOT_DIR;
        goto exit;
    }

    lock_psbutton();
    if (pfs_mount(dest) < 0) {
        res = ERROR_DECRYPT_DIR;
        goto exit;
    }

    mkdir(dest, 0777);

    init_progress(file_count(src, 1));
    if (copydir(src, dest, incr_progress) < 0) {
        res = ERROR_COPY_DIR;
        goto exit;
    }

exit:
    pfs_unmount();
    unlock_psbutton();
    if (src) {
        free(src);
    }
    if (dest) {
        free(dest);
    }
    return res;
}

int delete_slot(appinfo *info, int slot) {
    int res = NO_ERROR;
    char *target = slot_dir_path(info, slot);

    printf("target: %s\n", target);

    if (!target) {
        res = ERROR_MEMORY_ALLOC;
        goto exit;
    }

    if (!exists(target)) {
        res = ERROR_NO_SLOT_DIR;
        goto exit;
    }
    lock_psbutton();
    // TODO progress bar
    init_progress(file_count(target, 0));
    rmdir(target, incr_progress);
exit:
    unlock_psbutton();
    if (target) {
        free(target);
    }
    return res;
}

int format_savedata(appinfo *info) {
    int res = NO_ERROR;
    char *target = save_dir_path(info);
    if (!target) {
        res = ERROR_MEMORY_ALLOC;
        goto exit;
    }
    if (!exists(target)) {
        res = ERROR_NO_SAVE_DIR;
        goto exit;
    }
    lock_psbutton();
    init_progress(file_count(target, 0));
    rmdir(target, incr_progress);
exit:
    unlock_psbutton();
    if (target) {
        free(target);
    }
    return res;
}

void draw_screen(ScreenState state, appinfo *curr, appinfo *choose, int slot) {
    for (int i = 0; i < 3; i++) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        if (state >= MAIN_SCREEN) {
            //vita2d_draw_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
            //vita2d_draw_line(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, WHITE);
            //draw header

            //draw footer
            switch (mainscreen_list_mode) {
                case USE_ICON:
                    draw_icons(curr);
                    break;
                case USE_LIST:
                    draw_list(curr);
                    break;
            }
        }

        if (state >= PRINT_APPINFO) {
            draw_appinfo(state, choose);
        }

        switch (state) {
            case BACKUP_MODE:
            case RESTORE_MODE:
            case DELETE_MODE:
                draw_slots(choose, -1);
                break;
            case BACKUP_CONFIRM:
            case BACKUP_PROGRESS:
            case BACKUP_FAIL:
            case RESTORE_CONFIRM:
            case RESTORE_PROGRESS:
            case RESTORE_FAIL:
            case DELETE_CONFIRM:
            case DELETE_PROGRESS:
            case DELETE_FAIL:
                draw_slots(choose, slot);
                break;
            case FORMAT_MODE:
            case FORMAT_CONFIRM:
            case FORMAT_PROGRESS:
            case FORMAT_FAIL:
                // TODO
                break;
            default:
                break;
        }

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}

ScreenState noslot_state_machine(appinfo *curr, appinfo *choose,
                                 ScreenState confirm_state,
                                 ScreenState progress_state,
                                 ScreenState fail_state,
                                 ScreenState exit_state,
                                 const char *confirm_msg,
                                 int (*progress_func)(appinfo*)) {
    // this hook will skip choice slot state
    ScreenState state = confirm_state;
    int last_error = NO_ERROR;
    while (1) {
        draw_screen(state, curr, choose, -1);
        ScreenState new_state = UNKNOWN;
        if (state == confirm_state) {
            new_state = confirm(confirm_msg, 1.0) == CONFIRM
                      ? progress_state
                      : exit_state;
        } else if (state == progress_state) {
            last_error = progress_func(choose);
            new_state = last_error != NO_ERROR ? fail_state : exit_state;
        } else if (state == fail_state) {
            alert(error_message(last_error), 1.0);
            new_state = exit_state;
        } else {
            return state;
        }
        if (new_state != UNKNOWN) {
            state = new_state;
        }
    }
}

ScreenState slot_state_machine(appinfo *curr, appinfo *choose,
                               ScreenState start_state,
                               ScreenState confirm_state,
                               ScreenState progress_state,
                               ScreenState fail_state,
                               const char *confirm_msg,
                               int (*progress_func)(appinfo*, int)) {
    int slot = -1;
    ScreenState state = start_state;
    int last_error = NO_ERROR;
    while (1) {
        draw_screen(state, curr, choose, slot);
        ScreenState new_state = UNKNOWN;
        if (state == start_state) {
            new_state = on_slot_event(&slot);
            if (new_state == UNKNOWN && slot >= 0) {
                new_state = confirm_state;
            }
        } else if (state == confirm_state) {
            char *tmp = calloc(sizeof(char), 1);
            aprintf(&tmp, confirm_msg, slot);
            new_state = confirm(tmp, 1.0) == CONFIRM
                      ? progress_state
                      : start_state;
            free(tmp);
        } else if (state == progress_state) {
            last_error = progress_func(choose, slot);
            new_state = last_error != NO_ERROR ? fail_state : start_state;
        } else if (state == fail_state) {
            alert(error_message(last_error), 1.0);
            new_state = start_state;
        } else {
            return state;
        }
        if (new_state != UNKNOWN) {
            state = new_state;
        }
    }
}
void mainloop() {
    applist list = {0};
    int ret = get_applist(&list);
    if (ret < 0) {
        // loading error
        return;
    }

    int rows;
    int steps;
    switch (mainscreen_list_mode) {
        case USE_LIST:
            rows = list.count;
            steps = rows - LIST_ROW;
            break;
        case USE_ICON:
        default:
            rows = (list.count / ICONS_COL) + ((list.count % ICONS_COL) ? 1 : 0);
            steps = rows - ICONS_ROW;
            break;
    }
    printf("total: %d row: %d steps: %d\n", list.count, rows, steps);

    if (steps < 0) {
        steps = 0;
    }

    appinfo *head, *tail, *curr, *choose;
    curr = head = tail = list.items;

    int step = 0;

    while (tail->next) {
        tail = tail->next;
    }

    ScreenState state = MAIN_SCREEN;
    int slot = -1;
    while (1) {
        draw_screen(state, curr, choose, slot);
        ScreenState new_state = UNKNOWN;
        while (1) {
            switch (state) {
                case MAIN_SCREEN:
                    new_state = on_mainscreen_event(steps, &step, &curr, &choose);
                    break;
                case PRINT_APPINFO:
                    new_state = on_appinfo_event();
                    break;
                case BACKUP_MODE:
                    new_state = slot_state_machine(curr, choose,
                                                   BACKUP_MODE, BACKUP_CONFIRM,
                                                   BACKUP_PROGRESS, BACKUP_FAIL,
                                                   "Backup savedata to slot %d",
                                                   copy_savedata_to_slot);
                    break;
                case RESTORE_MODE:
                    new_state = slot_state_machine(curr, choose,
                                                   RESTORE_MODE, RESTORE_CONFIRM,
                                                   RESTORE_PROGRESS, RESTORE_FAIL,
                                                   "Restore savedata from slot %d",
                                                   copy_slot_to_savedata);
                    break;
                case DELETE_MODE:
                    new_state = slot_state_machine(curr, choose,
                                                   DELETE_MODE, DELETE_CONFIRM,
                                                   DELETE_PROGRESS, DELETE_FAIL,
                                                   "Delete save slot %d",
                                                   delete_slot);
                    break;
                case FORMAT_MODE:
                    new_state = noslot_state_machine(curr, choose,
                                                     FORMAT_CONFIRM,
                                                     FORMAT_PROGRESS,
                                                     FORMAT_FAIL,
                                                     PRINT_APPINFO,
                                                     "Format game savedata",
                                                     format_savedata);
                    break;
                default:
                    break;
            }
            if (new_state == UNKNOWN) {
                continue;
            }

            state = new_state;
            break;
        }
    }
}

#define MODULE_PATH "ux0:app/SAVEMGR00/sce_sys"

int main() {
    debugNetInit(DEBUG_IP, 18194, DEBUG);

    vita2d_init();
    vita2d_set_clear_color(BLACK);

    font = load_system_fonts();

    kernel_modid = taiLoadStartKernelModule(MODULE_PATH "/kernel.skprx",
                                            0, NULL, 0);
    // if before start VitaShell or this application,
    // will remain garbage in the kernel, taiLoadStartKernelModule will return
    // always 0x8002D013
    if (kernel_modid != 0x8002D013 && kernel_modid < 0) {
        printf("cannot find user module %x\n", user_modid);
        goto error_module_load;
    }
    user_modid = sceKernelLoadStartModule(MODULE_PATH "/user.suprx",
                                          0, NULL, 0, NULL, NULL);
    if (user_modid < 0) {
        printf("cannot find user module %x\n", user_modid);
        goto error_module_load;
    }

    load_config();

    char *base_path = calloc(sizeof(char), 1);
    aprintf(&base_path, "ux0:%s", config.base);
    mkdir(base_path, 0777);
    free(base_path);
    mkdir("ux0:/data/savemgr", 0777);

    sceAppMgrUmount("app0:");
    sceAppMgrUmount("savedata0:");

    if (strncmp(config.list_mode, "icon", 4) == 0) {
        mainscreen_list_mode = USE_ICON;
    } else if (strncmp(config.list_mode, "list", 4) == 0) {
        mainscreen_list_mode = USE_LIST;
    }

    init_input();
    init_console();
    // TODO splash

    mainloop();

    sceKernelExitProcess(0);
    return 0;

error_module_load:
    for (int i = 0; i < 3; i++) {
        vita2d_start_drawing();
        vita2d_clear_screen();
        vita2d_pgf_draw_text(font, 0, 40,
                             RED, 2.0, "not found module files");

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
    sceKernelDelayThread(10 * 1000 * 1000);
    sceKernelExitProcess(1);
    return 1;
}
