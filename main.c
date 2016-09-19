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

#define APPLICATION_NAME "Vita Save Dumper 0.4.1+"

vita2d_pgf* debug_font;
uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
uint32_t red = RGBA8(0xFF, 0x00, 0x00, 0xFF);

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

int fetch_realid_from_temp(char *buf) {
    int fd = sceIoOpen(TEMP_FILE, SCE_O_RDONLY, 0777);
    if (fd <= 0) {
        return -1;
    }

    sceIoLseek(fd, 16, SCE_SEEK_SET);
    sceIoRead(fd, buf, 16);
    sceIoClose(fd);

    return 0;
}

int cleanup_prev_inject(applist *list) {
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

    if (strcmp(tmp->dev, "ux0") == 0 && is_dumped_eboot( tmp->eboot )) {
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

#define WAIT_AND_MOVE(next) \
    while ((readBtn() & SCE_CTRL_CIRCLE) == 0); \
    state = (next)

#define PASS_OR_MOVE(row, next) \
    if (ret < 0) { \
        drawText((row), "error", red); \
        drawText((row) + 2, "please press circle", green); \
        WAIT_AND_MOVE((next)); \
        break; \
    } \
    drawText((row), "done", green);

int injector_main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));

    uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
    uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
    uint32_t red = RGBA8(0xFF, 0x00, 0x00, 0xFF);

    int btn;
    char buf[256];
    applist list = {0};

    int ret = get_applist(&list);
    if (ret < 0) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        snprintf(buf, 256, "initialize error, %x", ret);
        drawText(0, buf, red);

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();

        while (readBtn());
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
                drawLoopText(0, APPLICATION_NAME, white);
                print_game_list(head, tail, curr);
                drawLoopText(24, "UP/DOWN Select Item", white);
                drawLoopText(25, "CIRCLE Confirm", white);
                drawLoopText(26, "CROSS Exit", white);

                btn = readBtn();
                if (btn & SCE_CTRL_CIRCLE) {
                    state = INJECTOR_TITLE_SELECT;
                    break;
                }
                if (btn & SCE_CTRL_CROSS) {
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

                char patch[256], backup[256], gameType[32];
				
								
				
				
				
				
				
				
				//check if eboot is a dumped version
				/*
				SceUID file = sceIoOpen( curr->eboot , SCE_O_RDONLY, 0777);
				sceIoLseek( file, 0, SEEK_SET );
				char *bytes = malloc( 3 );
				sceIoRead( file, bytes, 3 );
				sceIoClose( file );
				char header[4];
				snprintf(header, 4, "%s", bytes);
				//drawText(0, header, white);
											
				int is_dumped = (strcmp( header, "SCE" ) == 0);
				*/
				
				int is_dumped = is_dumped_eboot( curr->eboot );
											
				/*
				char message[256];
				snprintf(message, 256, "Resultat %d", is_dumped);
				drawText(1, message, white);
				*/
				
				
				
				
                if (strcmp(curr->dev, "ux0") == 0 && is_dumped) {
                    // vitamin or digital dumped
					snprintf(gameType, 32, "DUMPED GAME");
                    snprintf(backup, 256, "%s.orig", curr->eboot);
                    snprintf(buf, 255, "[%s] backup %s to %s ", gameType, curr->eboot, backup);
                    drawText(0, buf, white);
                    ret = sceIoRename(curr->eboot, backup);
                    PASS_OR_MOVE(1, INJECTOR_TITLE_SELECT);

                    snprintf(buf, 255, "install dumper to %s", curr->eboot);
                    drawText(2, buf, white);
                    ret = copyfile("ux0:app/SAVEMGR00/eboot.bin", curr->eboot);
                    // TODO if error, need restore eboot
                    PASS_OR_MOVE(3, INJECTOR_TITLE_SELECT);
                } else {
					// cartridge and digital undumped
                    sprintf(patch, "ux0:patch/%s", curr->title_id);
                    sprintf(backup, "ux0:patch/%s_orig", curr->title_id);
					
					if (strcmp(curr->dev, "gro0") == 0) {
						snprintf(gameType, 32, "CARTRIDGE GAME");
					} else {
						if (is_dumped) {
							snprintf(gameType, 32, "DUMPED GAME");
						} else {
							snprintf(gameType, 32, "UNDUMPED GAME");
						}						
					}

                    // gro0 cartridge
                    if (!exists(curr->eboot)) {
                        drawText(0, "cartridge not inserted", red);

                        drawText(2, "please press circle", green);
                        WAIT_AND_MOVE(INJECTOR_TITLE_SELECT);
                        break;
                    }

                    snprintf(buf, 255, "%s/eboot.bin", patch);
                    // need to backup patch dir
                    int ret;
                    if (is_dir(patch) && !is_dumper_eboot(buf)) {
						snprintf(buf, 255, "[%s] backup %s to %s ...", gameType, patch, backup);
                        drawText(0, buf, white);
                        rmdir(backup);
                        ret = mvdir(patch, backup);
                        PASS_OR_MOVE(1, INJECTOR_TITLE_SELECT);
                    }

                    // inject dumper to patch
                    snprintf(buf, 255, "install dumper to %s...", patch);
                    drawText(4, buf, white);
                    ret = copydir("ux0:app/SAVEMGR00", patch);
                    // TODO restore patch
                    PASS_OR_MOVE(5, INJECTOR_TITLE_SELECT);

                    snprintf(patch, 255, "ux0:patch/%s/sce_sys/param.sfo", curr->title_id);
                    snprintf(buf, 255, "recopy param.sfo to %s...", patch);
                    drawText(7, buf, white);

                    snprintf(buf, 255, "%s:app/%s/sce_sys/param.sfo", curr->dev, curr->title_id);
                    ret = copyfile(buf, patch);

                    PASS_OR_MOVE(8, INJECTOR_TITLE_SELECT);
                }

                // backup for next cleanup
                int fd = sceIoOpen(TEMP_FILE, SCE_O_WRONLY | SCE_O_CREAT,0777);
                sceIoWrite(fd, curr->title_id, 16);
                sceIoWrite(fd, curr->real_id, 16);
                sceIoClose(fd);

                drawText(10, "DO NOT CLOSE APPLICATION MANUALLY", red);

                // wait 3sec
                sceKernelDelayThread(3000000);

                drawText(10, "start dumper...", green);

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

    char titleid[16], realid[16], title[256];
    sceAppMgrAppParamGetString(0, 9, title , 256);
    sceAppMgrAppParamGetString(0, 12, titleid , 256);

    sceIoMkdir("ux0:/data/rinCheat", 0777);
    sceIoMkdir("ux0:/data/savemgr", 0777);

    if (fetch_realid_from_temp(realid) < 0) {
        strcpy(realid, titleid);
    }

    char buf[256];
    char from[256];
    char to[256];
    if (strcmp(titleid, realid) == 0) {
        sprintf(from, "savedata0:");
    } else {
        sprintf(from, "ux0:/user/00/savedata/%s", realid);
    }
    sprintf(to, "ux0:/data/rinCheat/%s_SAVEDATA", titleid);

    while (1) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        switch (state) {
            case DUMPER_MAIN:
                drawLoopText(0, APPLICATION_NAME, white);
                drawLoopText(2, "DO NOT CLOSE APPLICATION MANUALLY", red);

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
                drawText(0, APPLICATION_NAME, white);
                drawText(2, "DO NOT CLOSE APPLICATION MANUALLY", red);

                snprintf(buf, 256, "export to %s ...", to);
                drawText(4, buf, white);
                dumpSavedataDir(from, to);
                drawText(5, "done", white);

                drawText(7, "please press circle", green);
                WAIT_AND_MOVE(DUMPER_MAIN);
                break;
            case DUMPER_IMPORT:
                clearScreen();
                drawText(0, APPLICATION_NAME, white);
                drawText(2, "DO NOT CLOSE APPLICATION MANUALLY", red);

                snprintf(buf, 256, "import to %s ...", from);
                drawText(4, buf, white);
                restoreSavedataDir(to, from);
                drawText(5, "done", white);

                drawText(7, "please press circle", green);
                WAIT_AND_MOVE(DUMPER_MAIN);
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
