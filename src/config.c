#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/io/fcntl.h>

#include "common.h"
#include "config.h"
#include "file.h"
#include "ini.h"
#include "util.h"

char app_titleid[16];
char app_title[256];

configure config = {0};

static int
handler(void *out, const char *section, const char *name, const char *value) {
    configure *p = (configure *)out;

    if (strcmp(name, "base") == 0) {
        p->base = strdup(value);

        if (strncmp("ux0:", value, 4) == 0 || strncmp("ur0:", value, 4) == 0 ||
            strncmp("uma0:", value, 4) == 0) {
            p->base = strdup(value);
        } else {
            char *base_path = calloc(sizeof(char), 1);
            aprintf(&base_path, "ux0:%s", value);
            p->base = strdup(base_path);
            free(base_path);
        }
    } else if (strcmp(name, "slot_format") == 0) {
        p->slot_format = strdup(value);
    } else if (strcmp(name, "list_mode") == 0) {
        p->list_mode = strdup(value);
    } else if (strcmp(name, "use_dpad") == 0) {
        p->use_dpad = strncmp(value, "true", 4) == 0;
    }
    return 1;
}

void load_config() {
    sceAppMgrAppParamGetString(0, 9, app_title, 256);
    sceAppMgrAppParamGetString(0, 12, app_titleid, 16);

    ini_parse(CONFIG_FILE, handler, &config);

    if (!config.base) {
        config.base = strdup(DEFAULT_BASE_SAVEDIR);
    }
    if (!config.slot_format) {
        config.slot_format = strdup(DEFAULT_SAVE_SLOT_FORMAT);
    }
    if (!config.list_mode) {
        config.list_mode = strdup(DEFAULT_LIST_MODE);
    }
}

void save_config() {
    FILE *f = fopen(CONFIG_FILE, "w");
    fprintf(f, "%s = %s\n", "base", config.base);
    fprintf(f, "%s = %s\n", "slot_format", config.slot_format);
    fprintf(f, "%s = %s\n", "list_mode", config.list_mode);
    fprintf(f, "%s = %s\n", "use_dpad", config.use_dpad ? "true" : "false");
    fclose(f);
}
