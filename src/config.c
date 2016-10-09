#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/io/fcntl.h>
#include <psp2/appmgr.h>

#include "common.h"
#include "config.h"
#include "file.h"
#include "ini.h"

char app_titleid[16];
char app_title[256];

configure config = {0};

static int handler(void* out,
                   const char* section, const char* name, const char* value) {
    configure *p = (configure*)out;

    if (strcmp(name, "base") == 0) {
        p->base = strdup(value);
    } else if (strcmp(name, "slot_format") == 0) {
        p->slot_format = strdup(value);
    }
    return 1;
}

void load_config() {
    sceAppMgrAppParamGetString(0, 9, app_title , 256);
    sceAppMgrAppParamGetString(0, 12, app_titleid , 16);

    ini_parse(CONFIG_FILE, handler, &config);

    if (!config.base) {
        config.base = strdup(DEFAULT_BASE_SAVEDIR);
    }
    if (!config.slot_format) {
        config.slot_format = strdup(DEFAULT_SAVE_SLOT_FORMAT);
    }
    int length = strlen(config.base) + strlen(config.slot_format) + 6;
    char format[length];
    memset(format, 0, length);
    snprintf(format, length, "ux0:%s/%s", config.base, config.slot_format);
    config.full_path_format = strdup(format);
}
