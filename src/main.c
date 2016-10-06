#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/ctrl.h>
#include <psp2/system_param.h>
#include <vita2d.h>

#include "appdb.h"
#include "file.h"
#include "font.h"

#define SAVE_MANAGER    "SAVEMGR00"
#define TEMP_FILE       "ux0:data/savemgr/tmp"

#define PAGE_ITEM_COUNT 20
#define SCREEN_ROW      27
#define ROW_HEIGHT      20

vita2d_pgf* debug_font;
uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
uint32_t red = RGBA8(0xFF, 0x00, 0x00, 0xFF);

const char *ICON_CIRCLE = "\xe2\x97\x8b";
const char *ICON_CROSS = "\xe2\x95\xb3";
const char *ICON_SQUARE = "\xe2\x96\xa1";
const char *ICON_TRIANGLE = "\xe2\x96\xb3";
const char *ICON_UPDOWN = "\xe2\x86\x95";

int enter_button = 0;
int SCE_CTRL_ENTER;
int SCE_CTRL_CANCEL;
char ICON_ENTER[4];
char ICON_CANCEL[4];

#define SCE_CTRL_HOLD 0x80000000

void drawText(uint32_t y, char* text, uint32_t color){
    int i;
    for (i=0;i<3;i++){
        vita2d_start_drawing();
        vita2d_pgf_draw_text(debug_font, 2, (y + 1) * ROW_HEIGHT, color, 1.0, text);
        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}

void drawLoopText(uint32_t y, char *text, uint32_t color) {
    vita2d_pgf_draw_text(debug_font, 2, (y + 1) * ROW_HEIGHT, color, 1.0, text);
}

void clearScreen(){
    int i;
    for (i=0;i<3;i++){
        vita2d_start_drawing();
        vita2d_clear_screen();
        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}

enum {
    INJECTOR_MAIN = 1,
    INJECTOR_TITLE_SELECT,
    INJECTOR_BACKUP_PATCH,
    INJECTOR_START_DUMPER,
    INJECTOR_RESTORE_PATCH,
    INJECTOR_EXIT,
};

enum {
    DUMPER_MAIN = 1,
    DUMPER_EXPORT,
    DUMPER_IMPORT,
    DUMPER_EXIT,
};

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

int launch(const char *titleid) {
    char uri[32];
    sprintf(uri, "psgm:play?titleid=%s", titleid);

    sceKernelDelayThread(10000);
    sceAppMgrLaunchAppByUri(0xFFFFF, uri);
    sceKernelDelayThread(10000);
    sceAppMgrLaunchAppByUri(0xFFFFF, uri);

    sceKernelExitProcess(0);

    return 0;
}

int cleanup_prev_inject(applist *list) {
    int fd = sceIoOpen(TEMP_FILE, SCE_O_RDONLY, 0777);
    if (fd <= 0) {
        return 0;
    }
    appinfo info;
    sceIoRead(fd, &info, sizeof(appinfo));
    sceIoClose(fd);

    sceIoRemove(TEMP_FILE);

    int ret;
    char backup[256];

    if (is_encrypted_eboot(info.eboot)) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        char patch[256];
        char patch_eboot[256];
        snprintf(patch, 256, "ux0:patch/%s", info.title_id);
        snprintf(patch_eboot, 256, "ux0:patch/%s/eboot.bin", info.title_id);
        if (!is_dumper_eboot(patch_eboot)) {
            return 0;
        }

        drawText(0, "Cleaning up old data...", white);
        ret = rmdir(patch);
        if (ret < 0) {
            drawText(1, "Error", red);
            goto exit;
        }
        drawText(1, "Done", white);

        snprintf(backup, 256, "ux0:patch/%s_orig", info.title_id);
        if (is_dir(backup)) {
            drawText(3, "Restoring patch...", white);
            ret = mvdir(backup, patch);
            if (ret < 0) {
                drawText(4, "Error", red);
                goto exit;
            }
            drawText(4, "Done", white);
        }
        ret = 0;
    } else {
        vita2d_start_drawing();
        vita2d_clear_screen();

        drawText(0, "Cleaning up old data...", white);
        snprintf(backup, 256, "%s.orig", info.eboot);
        ret = sceIoRemove(info.eboot);
        if (ret < 0) {
            drawText(1, "Error", red);
            goto exit;
        }
        drawText(1, "Done", white);
        drawText(3, "Restoring eboot", white);
        ret = sceIoRename(backup, info.eboot);
        if (ret < 0) {
            drawText(4, "Error", red);
            goto exit;
        }
        drawText(4, "Done", white);
        ret = 0;
    }
exit:
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();
    return ret;
}

void print_game_list(appinfo *head, appinfo *tail, appinfo *curr) {
    appinfo *tmp = head;
    int i = 2;
    char buf[256];
    while (tmp) {
        snprintf(buf, 256, "%s: %s", tmp->title_id, tmp->title);
        drawLoopText(i, buf, curr == tmp ? green : white);
        if (tmp == tail) {
            break;
        }
        tmp = tmp->next;
        i++;
    }
}

#define concat(str1, str2) \
    snprintf(buf, 256, "%s%s", str1, str2) ? buf : ""

#define WAIT_AND_MOVE(row, next) \
    drawText((row), concat("Please press ", ICON_ENTER), green); \
    do { \
        btn = read_btn(); \
    } while ((btn & SCE_CTRL_HOLD) != 0 || (btn & SCE_CTRL_ENTER) == 0); \
    state = (next);

#define PASS_OR_MOVE(row, next) \
    if (ret < 0) { \
        snprintf(buf, 256, "Error 0x%08X", ret); \
        drawText((row), buf, red); \
        WAIT_AND_MOVE((row) + 2, (next)); \
        break; \
    } \
    drawText((row), "Done", green);

int injector_main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));

    int btn;
    char buf[256];
    char version_string[256];
    snprintf(version_string, 256, "Vita Save Manager %s", VERSION);

    applist list = {0};

    int ret = get_applist(&list);
    if (ret < 0) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        snprintf(buf, 256, "Initialization error, %x", ret);
        drawText(0, buf, red);

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();

        while (read_btn());
        return -1;
    }
    appinfo *head, *tail, *curr;
    curr = head = tail = list.items;

    int i = 0;
    while (tail->next) {
        i++;
        if (i == PAGE_ITEM_COUNT) {
            break;
        }
        tail = tail->next;
    }

    int state = INJECTOR_MAIN;

    cleanup_prev_inject(&list);

    while (1) {
        vita2d_start_drawing();
        vita2d_clear_screen();
        switch (state) {
            case INJECTOR_MAIN:
                drawLoopText(0, version_string, white);

                print_game_list(head, tail, curr);

                drawLoopText(24, concat(ICON_UPDOWN, " - Select Item"), white);
                drawLoopText(25, concat(ICON_ENTER, " - Confirm"), white);
                drawLoopText(26, concat(ICON_CANCEL, " - Exit"), white);

                btn = read_btn();
                if (btn & SCE_CTRL_ENTER) {
                    state = INJECTOR_TITLE_SELECT;
                    break;
                }
                if (btn & SCE_CTRL_CANCEL && (btn & SCE_CTRL_HOLD) == 0) {
                    state = INJECTOR_EXIT;
                    break;
                }
                if ((btn & SCE_CTRL_UP) && curr->prev) {
                    if (curr == head) {
                        head = head->prev;
                        tail = tail->prev;
                    }
                    curr = curr->prev;
                    break;
                }
                if ((btn & SCE_CTRL_DOWN) && curr->next) {
                    if (curr == tail) {
                        tail = tail->next;
                        head = head->next;
                    }
                    curr = curr->next;
                    break;
                }
                break;
            case INJECTOR_TITLE_SELECT:
                drawLoopText(0, version_string, white);
                snprintf(buf, 255, "TITLE: %s", curr->title);
                drawLoopText(2, buf, white);
                snprintf(buf, 255, "TITLE_ID: %s", curr->title_id);
                drawLoopText(3, buf, white);

                drawLoopText(25, concat(ICON_ENTER, " - Start Dumper"), white);
                drawLoopText(26, concat(ICON_CANCEL, " - Return to Main Menu"), white);


                btn = read_btn();
                if (btn & SCE_CTRL_HOLD) {
                    break;
                }
                if (btn & SCE_CTRL_ENTER) {
                    state = INJECTOR_START_DUMPER;
                } else if (btn & SCE_CTRL_CANCEL) {
                    state = INJECTOR_MAIN;
                }
                break;
            case INJECTOR_START_DUMPER:
                clearScreen();
                drawText(0, version_string, white);

                char backup[256];

                // cartridge & digital encrypted games
                if (!exists(curr->eboot)) {
                    if (strcmp(curr->dev, "gro0") == 0) {
                        drawText(2, "Cartridge not inserted", red);
                    } else {
                        drawText(2, "Cannot find game", red);
                    }

                    WAIT_AND_MOVE(4, INJECTOR_TITLE_SELECT);
                    break;
                }

                if (is_encrypted_eboot(curr->eboot)) {
                    char patch[256];
                    drawText(2, "Injecting (encrypted game)...", white);
                    sprintf(patch, "ux0:patch/%s", curr->title_id);
                    sprintf(backup, "ux0:patch/%s_orig", curr->title_id);

                    snprintf(buf, 255, "%s/eboot.bin", patch);
                    // need to backup patch dir
                    int ret;
                    if (is_dir(patch) && !is_dumper_eboot(buf)) {
                        snprintf(buf, 255, "Backing up %s to %s...", patch, backup);
                        drawText(4, buf, white);
                        rmdir(backup);
                        ret = mvdir(patch, backup);
                        PASS_OR_MOVE(5, INJECTOR_TITLE_SELECT);
                    }

                    // inject dumper to patch
                    snprintf(buf, 255, "Installing dumper to %s...", patch);
                    drawText(7, buf, white);
                    ret = copydir("ux0:app/SAVEMGR00", patch);
                    // TODO restore patch
                    PASS_OR_MOVE(8, INJECTOR_TITLE_SELECT);

                    snprintf(patch, 255, "ux0:patch/%s/sce_sys/param.sfo", curr->title_id);
                    //Restoring or Copying?
                    snprintf(buf, 255, "Copying param.sfo to %s...", patch);
                    drawText(10, buf, white);

                    snprintf(buf, 255, "%s:app/%s/sce_sys/param.sfo", curr->dev, curr->title_id);
                    ret = copyfile(buf, patch);

                    PASS_OR_MOVE(11, INJECTOR_TITLE_SELECT);
                } else {
                    drawText(2, "Injecting (decrypted game)...", white);
                    ret = -1;

                    if (strcmp(curr->dev, "gro0") == 0) {
                        drawText(4, "Game not supported", red);
                        drawText(5, "Please send a bug report on github", white);

                        PASS_OR_MOVE(7, INJECTOR_TITLE_SELECT);
                    }
                    // vitamin or digital
                    snprintf(backup, 256, "%s.orig", curr->eboot);
                    snprintf(buf, 255, "Backing up %s to %s...", curr->eboot, backup);
                    drawText(4, buf, white);
                    ret = sceIoRename(curr->eboot, backup);
                    PASS_OR_MOVE(5, INJECTOR_TITLE_SELECT);

                    snprintf(buf, 255, "Installing dumper to %s...", curr->eboot);
                    drawText(7, buf, white);
                    ret = copyfile("ux0:app/SAVEMGR00/eboot.bin", curr->eboot);
                    // TODO if error, need restore eboot
                    PASS_OR_MOVE(8, INJECTOR_TITLE_SELECT);
                }

                // backup for next cleanup
                int fd = sceIoOpen(TEMP_FILE, SCE_O_WRONLY | SCE_O_CREAT,0777);
                sceIoWrite(fd, curr, sizeof(appinfo));
                sceIoClose(fd);

                drawText(13, "DO NOT CLOSE APPLICATION MANUALLY!", red);

                // wait 3sec
                sceKernelDelayThread(3000000);

                drawText(15, "Starting dumper...", green);

                // TODO store state
                launch(curr->title_id);
                break;
            case INJECTOR_EXIT:
                vita2d_end_drawing();
                vita2d_wait_rendering_done();
                vita2d_swap_buffers();
                return 0;
        }

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}

int dumper_main() {
    int state = DUMPER_MAIN;

    char title[256], titleid[256], buf[256], save_dir[256], backup_dir[256];

    char version_string[256];
    snprintf(version_string, 256, "Vita Save Manager %s", VERSION);

    sceAppMgrAppParamGetString(0, 9, title , 256);
    sceAppMgrAppParamGetString(0, 12, titleid , 256);

    sceIoMkdir("ux0:/data/rinCheat", 0777);
    sceIoMkdir("ux0:/data/savemgr", 0777);

    appinfo info;

    vita2d_start_drawing();
    vita2d_clear_screen();

    int fd = sceIoOpen(TEMP_FILE, SCE_O_RDONLY, 0777);
    int ret;
    int btn;

    if (fd < 0) {
        ret = -1;
        drawText(0, "Cannot find inject data", red);

        WAIT_AND_MOVE(2, DUMPER_EXIT);

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
        launch(SAVE_MANAGER);
        return ret;
    }

    sceIoRead(fd, &info, sizeof(appinfo));
    sceIoClose(fd);

    if (strcmp(info.title_id, titleid) != 0) {
        ret = -2;

        drawText(0, "Wrong inject information", red);

        WAIT_AND_MOVE(2, DUMPER_EXIT);

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
        launch(SAVE_MANAGER);
        return ret;
    }

    sprintf(backup_dir, "ux0:/data/rinCheat/%s_SAVEDATA", info.title_id);
    if (strcmp(info.title_id, info.real_id) == 0) {
        sprintf(save_dir, "savedata0:");
    } else {
        sprintf(save_dir, "ux0:user/00/savedata/%s", info.real_id);
    }

    while (1) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        switch (state) {
            case DUMPER_MAIN:
                drawLoopText(0, version_string, white);
                drawLoopText(2, "DO NOT CLOSE APPLICATION MANUALLY!", red);

                drawLoopText(24, concat(ICON_ENTER, " - Export"), white);
                drawLoopText(25, concat(ICON_TRIANGLE, " - Import"), white);
                drawLoopText(26, concat(ICON_CANCEL, " - Exit"), white);

                btn = read_btn();
                if (btn & SCE_CTRL_HOLD) break;
                if (btn & SCE_CTRL_ENTER) state = DUMPER_EXPORT;
                if (btn & SCE_CTRL_TRIANGLE) state = DUMPER_IMPORT;
                if (btn & SCE_CTRL_CANCEL) state = DUMPER_EXIT;
                break;
            case DUMPER_EXPORT:
                clearScreen();
                drawText(0, version_string, white);
                drawText(2, "DO NOT CLOSE APPLICATION MANUALLY!", red);

                snprintf(buf, 256, "Exporting to %s...", backup_dir);
                drawText(4, buf, white);
                ret = copydir(save_dir, backup_dir);
                PASS_OR_MOVE(5, DUMPER_MAIN);
                WAIT_AND_MOVE(7, DUMPER_MAIN);
                break;
            case DUMPER_IMPORT:
                clearScreen();
                drawText(0, version_string, white);
                drawText(2, "DO NOT CLOSE APPLICATION MANUALLY!", red);

                snprintf(buf, 256, "Importing from %s...", backup_dir);
                drawText(4, buf, white);
                ret = copydir(backup_dir, "savedata0:");
                PASS_OR_MOVE(5, DUMPER_MAIN);
                WAIT_AND_MOVE(7, DUMPER_MAIN);
                break;
            case DUMPER_EXIT:
                launch(SAVE_MANAGER);
                break;
        }
        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}

int main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));

    debug_font = load_system_fonts();

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

    char titleid[16], title[256];
    sceAppMgrAppParamGetString(0, 9, title , 256);
    sceAppMgrAppParamGetString(0, 12, titleid , 256);

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

    if (strcmp(titleid, SAVE_MANAGER) == 0) {
        injector_main();
    } else {
        dumper_main();
    }
    sceKernelExitProcess(0);
}
