#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"
#include "appdb.h"

#define APP_DB "ur0:/shell/db/app.db"

static int get_applist_callback(void *data, int argc, char **argv, char **cols) {
    applist *list = (applist*)data;
    appinfo *info = calloc(1, sizeof(appinfo));
    if (list->count == 0) {
        list->items = info;
    } else {
        appinfo *tmp = list->items;
        while(tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = info;
        info->prev = tmp;
    }
    list->count += 1;
    strcpy(info->title_id, argv[0]);
    strcpy(info->real_id, argv[1]);
    strcpy(info->title, argv[2]);
    strcpy(info->eboot, argv[3]);
    strcpy(info->dev, argv[4]);
    strcpy(info->iconpath, argv[5]);
    for (int i = 0; i < 256; i++) {
        if (info->title[i] == '\n') {
            info->title[i] = ' ';
        }
    }
    return 0;
}

int get_applist(applist *list) {
    char *query = "select a.titleid, b.realid, c.title, d.ebootbin,"
                  "       rtrim(substr(d.ebootbin, 0, 5), ':') as dev,"
                  "       e.iconpath"
                  "  from (select titleid"
                  "          from tbl_appinfo"
                  "         where key = 566916785"
                  "           and titleid like 'PCS%'"
                  "         order by titleid) a,"
                  "       (select titleid, val as realid"
                  "          from tbl_appinfo"
                  "         where key = 278217076) b,"
                  "       tbl_appinfo_icon c,"
                  "       (select titleid, val as ebootbin"
                  "          from tbl_appinfo"
                  "         where key = 3022202214) d,"
                  "       (select titleid, iconpath"
                  "          from tbl_appinfo_icon"
                  "         where type = 0) e"
                  " where a.titleid = b.titleid"
                  "   and a.titleid = c.titleid"
                  "   and a.titleid = d.titleid"
                  "   and a.titleid = e.titleid";

    sqlite3 *db;
    int ret = sqlite3_open(APP_DB, &db);
    if (ret) {
        return -1;
    }
    char *errMsg;
    ret = sqlite3_exec(db, query, get_applist_callback, (void *)list, &errMsg);
    if (ret != SQLITE_OK) {
        sqlite3_close(db);
        return -2;
    }
    sqlite3_close(db);

    if (list->count <= 0) {
        return -3;
    }
    return 0;
}

void load_icon(appinfo *info) {
    if (info->icon.texture) {
        return;
    }

    if (!info->icon.buf) {
        info->icon.buf = calloc(sizeof(uint8_t), ICON_BUF_SIZE);
        FILE *f = fopen(info->iconpath, "r");
        fread(info->icon.buf, sizeof(uint8_t), ICON_BUF_SIZE, f);
        fclose(f);
    }

    info->icon.texture = vita2d_load_PNG_buffer(info->icon.buf);
}

void unload_icon(appinfo *info) {
    if (info->icon.buf) {
        free(info->icon.buf);
        info->icon.buf = NULL;
    }
    if (info->icon.texture) {
        vita2d_free_texture(info->icon.texture);
        info->icon.texture = NULL;
    }
}
