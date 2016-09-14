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

vita2d_pgf* debug_font;
uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
uint32_t red = RGBA8(0xFF, 0x00, 0x00, 0xFF);

void drawText(uint32_t y, char* text, uint32_t color){
	int i;
	for (i=0;i<3;i++){
		vita2d_start_drawing();
		vita2d_pgf_draw_text(debug_font, 2, y, color, 1.0, text	);
		vita2d_end_drawing();
		vita2d_wait_rendering_done();
		vita2d_swap_buffers();
	}
}

void drawLoopText(uint32_t y, char *text, uint32_t color) {
    vita2d_pgf_draw_text(debug_font, 2, y, color, 1.0, text);
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

        drawText(20, "cleanup old data", white);
        snprintf(backup, 256, "%s.orig", tmp->eboot);
        ret = sceIoRemove(tmp->eboot);
        if (ret < 0) {
            drawText(40, "error", red);
            goto exit;
        }
        drawText(40, "done", white);
        drawText(60, "restore eboot", white);
        ret = sceIoRename(backup, tmp->eboot);
        if (ret < 0) {
            drawText(80, "error", red);
            goto exit;
        }
        drawText(80, "done", white);
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

        drawText(20, "cleanup old data", white);
        ret = rmdir(patch);
        if (ret < 0) {
            drawText(40, "error", red);
            goto exit;
        }
        drawText(40, "done", white);

        snprintf(backup, 256, "ux0:patch/%s_orig", titleid);
        if (is_dir(backup)) {
            drawText(60, "restore patch", white);
            ret = mvdir(backup, patch);
            if (ret < 0) {
                drawText(80, "error", red);
                goto exit;
            }
            drawText(80, "done", white);
        }
        ret = 0;
    }
exit:
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();
    sceDisplayWaitVblankStart();
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

        drawText(20, "initialize error", red);

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
        sceDisplayWaitVblankStart();
        while (readBtn());
        return -1;
    }
    appinfo *curr = list.items;

    int state = INJECTOR_MAIN;
    int row = 20;
    char buf[256];
    char patch[256], backup[256];

    cleanupPrevInject(&list);

    while (1) {
        vita2d_start_drawing();
        vita2d_clear_screen();
        switch (state) {
            case INJECTOR_MAIN:
                row = 20;
                drawLoopText(row,"Vita Save Manager 0.1",white);
                appinfo *tmp = list.items;
                while (tmp) {
                    row += 20;
                    snprintf(buf, 255, "%s: %s",
                             tmp->title_id,
                             tmp->title);
                    drawLoopText(row, buf, curr == tmp ? green : white);
                    tmp = tmp->next;
                }
                drawLoopText(row + 40, "UP/DOWN Select Item", white);
                drawLoopText(row + 60, "CIRCLE Confirm", white);
                drawLoopText(row + 80, "CROSS Exit", white);

                btn = readBtn();

                if (btn & SCE_CTRL_CIRCLE) state = INJECTOR_TITLE_SELECT;
                else if (btn & SCE_CTRL_CROSS) state = INJECTOR_EXIT;
                else if (btn & SCE_CTRL_UP && curr->prev) curr = curr->prev;
                else if (btn & SCE_CTRL_DOWN && curr->next) curr = curr->next;
                break;
            case INJECTOR_TITLE_SELECT:
                snprintf(buf, 255, "TITLE: %s", curr->title);
                drawLoopText(20, buf, white);
                snprintf(buf, 255, "TITLE_ID: %s", curr->title_id);
                drawLoopText(40, buf, white);

                drawLoopText(100, "CIRCLE start dumper", white);
                drawLoopText(120, "CROSS return to mainmenu", white);

                btn = readBtn();
                if (btn & SCE_CTRL_CIRCLE) {
                    state = INJECTOR_START_DUMPER;
                } else if (btn & SCE_CTRL_CROSS) {
                    state = INJECTOR_MAIN;
                }
                break;
            case INJECTOR_START_DUMPER:
                clearScreen();

                row = 0;
                if (strcmp(curr->dev, "ux0") == 0) {
                    // vitamin or digital
                    snprintf(backup, 256, "%s.orig", curr->eboot);
                    snprintf(buf, 255, "backup %s to %s", curr->eboot, backup);
                    row += 20;
                    drawText(row, buf, white);
                    row += 20;
                    if (sceIoRename(curr->eboot, backup) < 0) {
                        drawText(row, "error", red);

                        drawText(row + 20, "please press circle", green);
                        while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                        state = INJECTOR_TITLE_SELECT;
                        break;
                    }
                    drawText(row, "done", green);

                    snprintf(buf, 255, "install dumper to %s", curr->eboot);
                    row += 20;
                    drawText(row, buf, white);
                    row += 20;
                    if (copyfile("ux0:app/SAVEMGR00/eboot.bin", curr->eboot) < 0) {
                        drawText(row, "error", red);
                        // TODO restore eboot
                        drawText(row + 20, "please press circle", green);
                        while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                        state = INJECTOR_TITLE_SELECT;
                        break;
                    }
                    drawText(row, "done", white);
                } else {
                    sprintf(patch, "ux0:patch/%s", curr->title_id);
                    sprintf(backup, "ux0:patch/%s_orig", curr->title_id);

                    // gro0 cartridge
                    snprintf(buf, 255, "%s/eboot.bin", patch);
                    // need to backup patch dir
                    int ret;
                    if (is_dir(patch) && !is_dumper_eboot(buf)) {
                        snprintf(buf, 255, "backup %s to %s ...", patch, backup);
                        row += 20;
                        drawText(row, buf, white);
                        rmdir(backup);
                        row += 20;
                        ret = mvdir(patch, backup);
                        if (ret < 0) {
                            drawText(row,  "error", red);

                            drawText(row + 20, "please press circle", green);
                            while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                            state = INJECTOR_TITLE_SELECT;
                            break;
                        }
                        drawText(row, "done", green);
                    }

                    // inject dumper to patch
                    snprintf(buf, 255, "install dumper to %s...", patch);
                    row += 20;
                    drawText(row, buf, white);
                    row += 20;
                    ret = copydir("ux0:app/SAVEMGR00", patch);
                    if (ret < 0) {
                        drawText(row, "error", red);
                        // TODO restore patch
                        drawText(row + 20, "please press circle", green);
                        while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                        state = INJECTOR_TITLE_SELECT;
                        break;
                    }
                    drawText(row, "done", white);
                }

                // backup for next cleanup
                int fd = sceIoOpen(TEMP_FILE, SCE_O_WRONLY | SCE_O_CREAT,0777);
                sceIoWrite(fd, curr->title_id, 16);
                sceIoClose(fd);

                drawText(100, "start dumper...", green);

                // TODO store state
                launch(curr->title_id);
                break;
            case INJECTOR_EXIT:
                vita2d_end_drawing();
                vita2d_wait_rendering_done();
                vita2d_swap_buffers();
                sceDisplayWaitVblankStart();
                return 0;
        }

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
        sceDisplayWaitVblankStart();
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
                drawLoopText(20, "Vita Save Dumper 0.1", white);
                drawLoopText(40, "CIRCLE Export", white);
                drawLoopText(60, "TRIANGLE Import", white);
                drawLoopText(80, "CROSS Exit", white);

                int btn = readBtn();
                if (btn & SCE_CTRL_CIRCLE) state = DUMPER_EXPORT;
                if (btn & SCE_CTRL_TRIANGLE) state = DUMPER_IMPORT;
                if (btn & SCE_CTRL_CROSS) state = DUMPER_EXIT;
                break;
            case DUMPER_EXPORT:
                clearScreen();
                snprintf(buf, 256, "export to %s ...", path);
                drawText(20, buf, white);
                dumpSavedataDir("savedata0:", path);
                drawText(40, "done", white);

                drawText(60, "please press circle", green);
                while ((readBtn() & SCE_CTRL_CIRCLE) == 0);
                state = DUMPER_MAIN;
                break;
            case DUMPER_IMPORT:
                clearScreen();
                snprintf(buf, 256, "import from %s ...", path);
                drawText(20, buf, white);
                restoreSavedataDir(path, NULL);
                drawText(40, "done", white);

                drawText(60, "please press circle", green);
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
        sceDisplayWaitVblankStart();
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
