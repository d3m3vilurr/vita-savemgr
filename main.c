#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/appmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/ctrl.h>
#include <vita2d.h>

#include "appdb.h"
#include "file.h"
#include "savedata.h"

#define TEMP_FILE "ux0:data/savemgr/tmp"
#define PAGE_ITEM_COUNT 20
#define SCREEN_ROW 27
#define ROW_HEIGHT 20

vita2d_pgf* debug_font;
uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
uint32_t red = RGBA8(0xFF, 0x00, 0x00, 0xFF);

void drawText(uint32_t y, char* text, uint32_t color){
	int i;
	for (i=0;i<3;i++){
		vita2d_start_drawing();
		vita2d_pgf_draw_text(debug_font, 2, (y + 1) * ROW_HEIGHT, color, 1.0, text	);
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

int readBtn() {
    SceCtrlData pad = {0};
    static int old;
    int btn;

    sceCtrlPeekBufferPositive(0, &pad, 1);

    btn = pad.buttons & ~old;
    old = pad.buttons;
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

int cleanupPrevInject(applist *list) {
    int fd = sceIoOpen(TEMP_FILE, SCE_O_RDONLY, 0777);
    if (fd <= 0) {
        return 0;
    }
    char titleid[16];
    sceIoRead(fd, titleid, 16);
    sceIoClose(fd);

    sceIoRemove(TEMP_FILE);

    appinfo *tmp = list->items;
    while (tmp) {
        if (strcmp(tmp->title_id, titleid) == 0) {
            break;
        }
        tmp = tmp->next;
    }

    if (tmp == NULL) {
        return -1;
    }

    int ret;
    char backup[256];

    if (strcmp(tmp->dev, "ux0") == 0) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        drawText(0, "cleanup old data", white);
        snprintf(backup, 256, "%s.orig", tmp->eboot);
        ret = sceIoRemove(tmp->eboot);
        if (ret < 0) {
            drawText(1, "error", red);
            goto exit;
        }
        drawText(1, "done", white);
        drawText(2, "restore eboot", white);
        ret = sceIoRename(backup, tmp->eboot);
        if (ret < 0) {
            drawText(3, "error", red);
            goto exit;
        }
        drawText(3, "done", white);
        ret = 0;
    } else {
        char patch[256];
        char eboot[256];
        snprintf(patch, 256, "ux0:patch/%s", titleid);
        snprintf(eboot, 256, "ux0:patch/%s/eboot.bin", titleid);
        if (!is_dumper_eboot(eboot)) {
            return 0;
        }

        vita2d_start_drawing();
        vita2d_clear_screen();

        drawText(0, "cleanup old data", white);
        ret = rmdir(patch);
        if (ret < 0) {
            drawText(1, "error", red);
            goto exit;
        }
        drawText(1, "done", white);

        snprintf(backup, 256, "ux0:patch/%s_orig", titleid);
        if (is_dir(backup)) {
            drawText(2, "restore patch", white);
            ret = mvdir(backup, patch);
            if (ret < 0) {
                drawText(3, "error", red);
                goto exit;
            }
            drawText(3, "done", white);
        }
        ret = 0;
    }
exit:
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();
    return ret;
}

int injector_main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));

    uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
    uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
    uint32_t red = RGBA8(0xFF, 0x00, 0x00, 0xFF);

    int btn;

    applist list;
    if (get_applist(&list) < 0) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        drawText(0, "initialize error", red);

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
        while (readBtn());
        return -1;
    }
    appinfo *head, *tail, *curr;
    curr = head = tail = list.items;
    int i = 0;
    while (tail && tail->next) {
        i++;
        if (i == PAGE_ITEM_COUNT) {
            break;
        }
        tail = tail->next;
    }

    int state = INJECTOR_MAIN;
    char buf[256];
    char patch[256], backup[256];

    cleanupPrevInject(&list);

    while (1) {
        vita2d_start_drawing();
        vita2d_clear_screen();
        switch (state) {
            case INJECTOR_MAIN:
                drawLoopText(0, "Vita Save Manager 0.1", white);
                appinfo *tmp = head;
                i = 2;
                while(tmp) {
                    snprintf(buf, 255, "%s: %s",
                             tmp->title_id,
                             tmp->title);
                    drawLoopText(i, buf, curr == tmp ? green : white);
                    if (tmp == tail) {
                        break;
                    }
                    tmp = tmp->next;
                    i++;
                }

                drawLoopText(24, "UP/DOWN Select Item", white);
                drawLoopText(25, "CIRCLE Confirm", white);
                drawLoopText(26, "CROSS Exit", white);

                btn = readBtn();

                if (btn & SCE_CTRL_CIRCLE) state = INJECTOR_TITLE_SELECT;
                else if (btn & SCE_CTRL_CROSS) state = INJECTOR_EXIT;
                else if (btn & SCE_CTRL_UP && curr->prev) {
                    if (curr == head) {
                        head = head->prev;
                        tail = tail->prev;
                    }
                    curr = curr->prev;
                }
                else if (btn & SCE_CTRL_DOWN && curr->next) {
                    if (curr == tail) {
                        tail = tail->next;
                        head = head->next;
                    }
                    curr = curr->next;
                }
                break;
            case INJECTOR_TITLE_SELECT:
                snprintf(buf, 255, "TITLE: %s", curr->title);
                drawLoopText(0, buf, white);
                snprintf(buf, 255, "TITLE_ID: %s", curr->title_id);
                drawLoopText(1, buf, white);

                drawLoopText(25, "CIRCLE start dumper", white);
                drawLoopText(26, "CROSS return to mainmenu", white);

                btn = readBtn();
                if (btn & SCE_CTRL_CIRCLE) {
                    state = INJECTOR_START_DUMPER;
                } else if (btn & SCE_CTRL_CROSS) {
                    state = INJECTOR_MAIN;
                }
                break;
            case INJECTOR_START_DUMPER:
                clearScreen();

                if (strcmp(curr->dev, "ux0") == 0) {
                    // vitamin or digital
                    snprintf(backup, 256, "%s.orig", curr->eboot);
                    snprintf(buf, 255, "backup %s to %s", curr->eboot, backup);
                    drawText(0, buf, white);
                    if (sceIoRename(curr->eboot, backup) < 0) {
                        drawText(1, "error", red);

                        drawText(3, "please press circle", green);
                        while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                        state = INJECTOR_TITLE_SELECT;
                        break;
                    }
                    drawText(1, "done", green);

                    snprintf(buf, 255, "install dumper to %s", curr->eboot);
                    drawText(2, buf, white);
                    if (copyfile("ux0:app/SAVEMGR00/eboot.bin", curr->eboot) < 0) {
                        drawText(3, "error", red);
                        // TODO restore eboot
                        drawText(5, "please press circle", green);
                        while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                        state = INJECTOR_TITLE_SELECT;
                        break;
                    }
                    drawText(3, "done", white);
                } else {
                    sprintf(patch, "ux0:patch/%s", curr->title_id);
                    sprintf(backup, "ux0:patch/%s_orig", curr->title_id);

                    // gro0 cartridge
                    snprintf(buf, 255, "%s/eboot.bin", patch);
                    // need to backup patch dir
                    int ret;
                    if (is_dir(patch) && !is_dumper_eboot(buf)) {
                        snprintf(buf, 255, "backup %s to %s ...", patch, backup);
                        drawText(0, buf, white);
                        rmdir(backup);
                        ret = mvdir(patch, backup);
                        if (ret < 0) {
                            drawText(1,  "error", red);

                            drawText(3, "please press circle", green);
                            while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                            state = INJECTOR_TITLE_SELECT;
                            break;
                        }
                        drawText(1, "done", green);
                    }

                    // inject dumper to patch
                    snprintf(buf, 255, "install dumper to %s...", patch);
                    drawText(4, buf, white);
                    ret = copydir("ux0:app/SAVEMGR00", patch);
                    if (ret < 0) {
                        drawText(5, "error", red);
                        // TODO restore patch
                        drawText(7, "please press circle", green);
                        while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                        state = INJECTOR_TITLE_SELECT;
                        break;
                    }
                    drawText(5, "done", white);
                }

                // backup for next cleanup
                int fd = sceIoOpen(TEMP_FILE, SCE_O_WRONLY | SCE_O_CREAT,0777);
                sceIoWrite(fd, curr->title_id, 16);
                sceIoClose(fd);

                drawText(7, "start dumper...", green);

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

    char titleid[16], title[256];
    sceAppMgrAppParamGetString(0, 9, title , 256);
    sceAppMgrAppParamGetString(0, 12, titleid , 256);

    sceIoMkdir("ux0:/data/rinCheat", 0777);
    sceIoMkdir("ux0:/data/savemgr", 0777);

    char buf[256];
    char path[256];
    sprintf(path,"ux0:/data/rinCheat/%s_SAVEDATA", titleid);

    while (1) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        switch (state) {
            case DUMPER_MAIN:
                drawLoopText(0, "Vita Save Dumper 0.1", white);
                drawLoopText(24, "CIRCLE Export", white);
                drawLoopText(25, "TRIANGLE Import", white);
                drawLoopText(26, "CROSS Exit", white);

                int btn = readBtn();
                if (btn & SCE_CTRL_CIRCLE) state = DUMPER_EXPORT;
                if (btn & SCE_CTRL_TRIANGLE) state = DUMPER_IMPORT;
                if (btn & SCE_CTRL_CROSS) state = DUMPER_EXIT;
                break;
            case DUMPER_EXPORT:
                clearScreen();
                snprintf(buf, 256, "export to %s ...", path);
                drawText(0, buf, white);
                dumpSavedataDir("savedata0:", path);
                drawText(1, "done", white);

                drawText(3, "please press circle", green);
                while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                state = DUMPER_MAIN;
                break;
            case DUMPER_IMPORT:
                clearScreen();
                snprintf(buf, 256, "import from %s ...", path);
                drawText(0, buf, white);
                restoreSavedataDir(path, NULL);
                drawText(1, "done", white);

                drawText(3, "please press circle", green);
                while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                state = DUMPER_MAIN;
                break;
            case DUMPER_EXIT:
                launch("SAVEMGR00");
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

    debug_font = vita2d_load_default_pgf();

    char titleid[16], title[256];
    sceAppMgrAppParamGetString(0, 9, title , 256);
    sceAppMgrAppParamGetString(0, 12, titleid , 256);

    if (strcmp(titleid, "SAVEMGR00") == 0) {
        injector_main();
    } else {
        dumper_main();
    }
	sceKernelExitProcess(0);
}
