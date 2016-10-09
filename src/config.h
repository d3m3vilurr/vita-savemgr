#ifndef __CONFIG_H__
#define __CONFIG_H__

extern char app_titleid[16];
extern char app_title[256];

typedef struct configure {
    const char *base;
    const char *slot_format;
    const char *full_path_format;
} configure;

extern configure config;

void load_config();

#endif
