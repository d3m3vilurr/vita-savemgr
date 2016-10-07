#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/appmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/ctrl.h>
#include <psp2/system_param.h>
#include <psp2/rtc.h>
#include <vita2d.h>

#include "common.h"
#include "config.h"
#include "appdb.h"
#include "console.h"
#include "file.h"
#include "font.h"
#include "button.h"

#define PAGE_ITEM_COUNT 20

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
    DUMPER_SLOT_SELECT,
    DUMPER_EXPORT,
    DUMPER_IMPORT,
    DUMPER_EXIT,
};

int ret;
int btn;
char buf[256];

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

    char backup[256];

    draw_start();
    if (is_encrypted_eboot(info.eboot)) {
        char patch[256];
        char patch_eboot[256];
        snprintf(patch, 256, "ux0:patch/%s", info.title_id);
        snprintf(patch_eboot, 256, "ux0:patch/%s/eboot.bin", info.title_id);
        if (!is_dumper_eboot(patch_eboot)) {
            return 0;
        }

        draw_text(0, "Cleaning up old data...", white);
        ret = rmdir(patch);
        if (ret < 0) {
            draw_text(1, "Error", red);
            goto exit;
        }
        draw_text(1, "Done", white);

        snprintf(backup, 256, "ux0:patch/%s_orig", info.title_id);
        if (is_dir(backup)) {
            draw_text(3, "Restoring patch...", white);
            ret = mvdir(backup, patch);
            if (ret < 0) {
                draw_text(4, "Error", red);
                goto exit;
            }
            draw_text(4, "Done", white);
        }
        ret = 0;
    } else {
        draw_text(0, "Cleaning up old data...", white);
        snprintf(backup, 256, "%s.orig", info.eboot);
        ret = sceIoRemove(info.eboot);
        if (ret < 0) {
            draw_text(1, "Error", red);
            goto exit;
        }
        draw_text(1, "Done", white);
        draw_text(3, "Restoring eboot", white);
        ret = sceIoRename(backup, info.eboot);
        if (ret < 0) {
            draw_text(4, "Error", red);
            goto exit;
        }
        draw_text(4, "Done", white);
        ret = 0;
    }
exit:
    draw_end();
    return ret;
}

void print_game_list(appinfo *head, appinfo *tail, appinfo *curr) {
    appinfo *tmp = head;
    int i = 2;
    while (tmp) {
        snprintf(buf, 256, "%s: %s", tmp->title_id, tmp->title);
        draw_loop_text(i, buf, curr == tmp ? green : white);
        if (tmp == tail) {
            break;
        }
        tmp = tmp->next;
        i++;
    }
}

void make_save_slot_string(int slot) {
    snprintf(buf, 256, "ux0:/data/rinCheat/%s_SLOT%d/sce_sys/param.sfo", app_titleid, slot);
    char date[20] = {0};
    if (!exists(buf)) {
        snprintf(date, 20, "none");
    } else {
        SceIoStat stat = {0};
        sceIoGetstat(buf, &stat);

        SceDateTime time;
        SceRtcTick tick_utc;
        SceRtcTick tick_local;
        sceRtcGetTick(&stat.st_mtime, &tick_utc);
        sceRtcConvertUtcToLocalTime(&tick_utc, &tick_local);
        sceRtcSetTick(&time, &tick_local);

        snprintf(date, 20, "%04d-%02d-%02d %02d:%02d:%02d",
                 time.year, time.month, time.day, time.hour, time.minute, time.second);
    }
    snprintf(buf, 256, "Slot %d - %s", slot, date);
}
void print_save_slots(int curr_slot) {
    int r = 4;
    for (int i = 0; i < 10; i++) {
        make_save_slot_string(i);
        draw_loop_text(r + i, buf, curr_slot == i ? green : white);
    }
}

char *concat(char *str1, char *str2) {
    char tmp[256];
    snprintf(tmp, 256, "%s%s", str1, str2);
    memcpy(buf, tmp, 256);
    return buf;
}

#define WAIT_AND_MOVE(row, next) \
    draw_text((row), concat("Please press ", ICON_ENTER), green); \
    do { \
        btn = read_btn(); \
    } while ((btn & SCE_CTRL_HOLD) != 0 || (btn & SCE_CTRL_ENTER) == 0); \
    state = (next);

#define PASS_OR_MOVE(row, next) \
    if (ret < 0) { \
        snprintf(buf, 256, "Error 0x%08X", ret); \
        draw_text((row), buf, red); \
        WAIT_AND_MOVE((row) + 2, (next)); \
        break; \
    } \
    draw_text((row), "Done", green);

int injector_main() {
    char version_string[256];
    snprintf(version_string, 256, "Vita Save Manager %s", VERSION);

    applist list = {0};

    ret = get_applist(&list);
    if (ret < 0) {
        draw_start();

        snprintf(buf, 256, "Initialization error, %x", ret);
        draw_text(0, buf, red);

        draw_end();

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
        draw_start();

        switch (state) {
            case INJECTOR_MAIN:
                draw_loop_text(0, version_string, white);

                print_game_list(head, tail, curr);

                draw_loop_text(24, concat(ICON_UPDOWN, " - Select Item"), white);
                draw_loop_text(25, concat(ICON_ENTER, " - Confirm"), white);
                draw_loop_text(26, concat(ICON_CANCEL, " - Exit"), white);

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
                draw_loop_text(0, version_string, white);
                snprintf(buf, 255, "TITLE: %s", curr->title);
                draw_loop_text(2, buf, white);
                snprintf(buf, 255, "TITLE_ID: %s", curr->title_id);
                draw_loop_text(3, buf, white);

                draw_loop_text(25, concat(ICON_ENTER, " - Start Dumper"), white);
                draw_loop_text(26, concat(ICON_CANCEL, " - Return to Main Menu"), white);


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
                clear_screen();
                draw_text(0, version_string, white);

                char backup[256];

                // cartridge & digital encrypted games
                if (!exists(curr->eboot)) {
                    if (strcmp(curr->dev, "gro0") == 0) {
                        draw_text(2, "Cartridge not inserted", red);
                    } else {
                        draw_text(2, "Cannot find game", red);
                    }

                    WAIT_AND_MOVE(4, INJECTOR_TITLE_SELECT);
                    break;
                }

                if (is_encrypted_eboot(curr->eboot)) {
                    char patch[256];
                    draw_text(2, "Injecting (encrypted game)...", white);
                    sprintf(patch, "ux0:patch/%s", curr->title_id);
                    sprintf(backup, "ux0:patch/%s_orig", curr->title_id);

                    snprintf(buf, 255, "%s/eboot.bin", patch);
                    // need to backup patch dir
                    if (is_dir(patch) && !is_dumper_eboot(buf)) {
                        snprintf(buf, 255, "Backing up %s to %s...", patch, backup);
                        draw_text(4, buf, white);
                        rmdir(backup);
                        ret = mvdir(patch, backup);
                        PASS_OR_MOVE(5, INJECTOR_TITLE_SELECT);
                    }

                    // inject dumper to patch
                    snprintf(buf, 255, "Installing dumper to %s...", patch);
                    draw_text(7, buf, white);
                    ret = copydir("ux0:app/SAVEMGR00", patch);
                    // TODO restore patch
                    PASS_OR_MOVE(8, INJECTOR_TITLE_SELECT);

                    snprintf(patch, 255, "ux0:patch/%s/sce_sys/param.sfo", curr->title_id);
                    //Restoring or Copying?
                    snprintf(buf, 255, "Copying param.sfo to %s...", patch);
                    draw_text(10, buf, white);

                    snprintf(buf, 255, "%s:app/%s/sce_sys/param.sfo", curr->dev, curr->title_id);
                    ret = copyfile(buf, patch);

                    PASS_OR_MOVE(11, INJECTOR_TITLE_SELECT);
                } else {
                    draw_text(2, "Injecting (decrypted game)...", white);
                    ret = -1;

                    if (strcmp(curr->dev, "gro0") == 0) {
                        draw_text(4, "Game not supported", red);
                        draw_text(5, "Please send a bug report on github", white);

                        PASS_OR_MOVE(7, INJECTOR_TITLE_SELECT);
                    }
                    // vitamin or digital
                    snprintf(backup, 256, "%s.orig", curr->eboot);
                    snprintf(buf, 255, "Backing up %s to %s...", curr->eboot, backup);
                    draw_text(4, buf, white);
                    ret = sceIoRename(curr->eboot, backup);
                    PASS_OR_MOVE(5, INJECTOR_TITLE_SELECT);

                    snprintf(buf, 255, "Installing dumper to %s...", curr->eboot);
                    draw_text(7, buf, white);
                    ret = copyfile("ux0:app/SAVEMGR00/eboot.bin", curr->eboot);
                    // TODO if error, need restore eboot
                    PASS_OR_MOVE(8, INJECTOR_TITLE_SELECT);
                }

                // backup for next cleanup
                int fd = sceIoOpen(TEMP_FILE, SCE_O_WRONLY | SCE_O_CREAT,0777);
                sceIoWrite(fd, curr, sizeof(appinfo));
                sceIoClose(fd);

                draw_text(13, "DO NOT CLOSE APPLICATION MANUALLY!", red);

                // wait 3sec
                sceKernelDelayThread(3000000);

                draw_text(15, "Starting dumper...", green);

                // TODO store state
                launch(curr->title_id);
                break;
            case INJECTOR_EXIT:
                draw_end();
                return 0;
        }

        draw_end();
    }
}

int dumper_main() {
    int state = DUMPER_MAIN;

    char save_dir[256], backup_dir[256];

    char version_string[256];
    snprintf(version_string, 256, "Vita Save Manager %s", VERSION);

    sceIoMkdir("ux0:/data/rinCheat", 0777);
    sceIoMkdir("ux0:/data/savemgr", 0777);

    appinfo info;

    int fd = sceIoOpen(TEMP_FILE, SCE_O_RDONLY, 0777);

    if (fd < 0) {
        draw_start();

        draw_text(0, "Cannot find inject data", red);
        WAIT_AND_MOVE(2, DUMPER_EXIT);

        draw_end();
        launch(SAVE_MANAGER);
        return -1;
    }

    sceIoRead(fd, &info, sizeof(appinfo));
    sceIoClose(fd);

    if (strcmp(info.title_id, app_titleid) != 0) {
        draw_start();

        draw_text(0, "Wrong inject information", red);
        WAIT_AND_MOVE(2, DUMPER_EXIT);

        draw_end();
        launch(SAVE_MANAGER);
        return -2;
    }

    if (strcmp(info.title_id, info.real_id) == 0) {
        sprintf(save_dir, "savedata0:");
    } else {
        sprintf(save_dir, "ux0:user/00/savedata/%s", info.real_id);
    }

    int slot = 0;
    while (1) {
        draw_start();

        sprintf(backup_dir, "ux0:/data/rinCheat/%s_SLOT%d", info.title_id, slot);

        switch (state) {
            case DUMPER_MAIN:
                draw_loop_text(0, version_string, white);
                draw_loop_text(2, "DO NOT CLOSE APPLICATION MANUALLY!", red);

                print_save_slots(slot);

                draw_loop_text(24, concat(ICON_UPDOWN, " - Select Slot"), white);
                draw_loop_text(25, concat(ICON_ENTER, " - Confirm"), white);
                draw_loop_text(26, concat(ICON_CANCEL, " - Exit"), white);

                btn = read_btn();

                if (btn & SCE_CTRL_ENTER) {
                    state = DUMPER_SLOT_SELECT;
                    break;
                }
                if (btn & SCE_CTRL_CANCEL && (btn & SCE_CTRL_HOLD) == 0) {
                    state = DUMPER_EXIT;
                    break;
                }
                if ((btn & SCE_CTRL_UP) && slot > 0) {
                    slot -= 1;
                    break;
                }
                if ((btn & SCE_CTRL_DOWN) && slot < 9) {
                    slot += 1;
                    break;
                }
                break;
            case DUMPER_SLOT_SELECT:
                draw_loop_text(0, version_string, white);
                draw_loop_text(2, "DO NOT CLOSE APPLICATION MANUALLY!", red);
                make_save_slot_string(slot);
                draw_loop_text(4, concat("SELECT: ", buf), white);

                draw_loop_text(24, concat(ICON_ENTER, " - Export"), white);
                draw_loop_text(25, concat(ICON_TRIANGLE, " - Import"), white);
                draw_loop_text(26, concat(ICON_CANCEL, " - Return to Select Slot"), white);
                btn = read_btn();
                if (btn & SCE_CTRL_HOLD) break;
                if (btn & SCE_CTRL_ENTER) state = DUMPER_EXPORT;
                if (btn & SCE_CTRL_TRIANGLE) state = DUMPER_IMPORT;
                if (btn & SCE_CTRL_CANCEL) state = DUMPER_MAIN;
                break;
            case DUMPER_EXPORT:
                clear_screen();
                draw_text(0, version_string, white);
                draw_text(2, "DO NOT CLOSE APPLICATION MANUALLY!", red);

                snprintf(buf, 256, "Exporting to %s...", backup_dir);
                draw_text(4, buf, white);
                ret = copydir(save_dir, backup_dir);
                PASS_OR_MOVE(5, DUMPER_SLOT_SELECT);
                WAIT_AND_MOVE(7, DUMPER_SLOT_SELECT);
                break;
            case DUMPER_IMPORT:
                clear_screen();
                draw_text(0, version_string, white);
                draw_text(2, "DO NOT CLOSE APPLICATION MANUALLY!", red);

                snprintf(buf, 256, "Importing from %s...", backup_dir);
                draw_text(4, buf, white);
                ret = copydir(backup_dir, "savedata0:");
                PASS_OR_MOVE(5, DUMPER_SLOT_SELECT);
                WAIT_AND_MOVE(7, DUMPER_SLOT_SELECT);
                break;
            case DUMPER_EXIT:
                launch(SAVE_MANAGER);
                break;
        }
        draw_end();
    }
}

int main() {
    vita2d_init();
    init_console();
    load_config();

    if (strcmp(app_titleid, SAVE_MANAGER) == 0) {
        injector_main();
    } else {
        dumper_main();
    }
    sceKernelExitProcess(0);
}
