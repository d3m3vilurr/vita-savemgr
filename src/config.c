#include <psp2/appmgr.h>

#include "config.h"

char app_titleid[16];
char app_title[256];

void load_config() {
    sceAppMgrAppParamGetString(0, 9, app_title , 256);
    sceAppMgrAppParamGetString(0, 12, app_titleid , 16);
}
