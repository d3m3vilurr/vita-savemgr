#ifndef __APPDB_H__
#define __APPDB_H__

#include <stdint.h>
#include <vita2d.h>

#include "common.h"

typedef struct appinfo {
    char title_id[16];
    char real_id[16];
    char title[256];
    char eboot[256];
    char dev[5];
    char iconpath[256];
    icon_data icon;
    struct appinfo *next;
    struct appinfo *prev;
} appinfo;

typedef struct applist {
    uint32_t count;
    appinfo *items;
} applist;

int get_applist(applist *list);

void load_icon(appinfo *info);
void unload_icon(appinfo *icon);
#endif
