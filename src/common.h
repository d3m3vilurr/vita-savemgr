#ifndef __COMMON_H__
#define __COMMON_H__

#include <vita2d.h>

#define SAVE_MANAGER    "SAVEMGR00"

#define ICON_CIRCLE   "\xe2\x97\x8b"
#define ICON_CROSS    "\xe2\x95\xb3"
#define ICON_SQUARE   "\xe2\x96\xa1"
#define ICON_TRIANGLE "\xe2\x96\xb3"
#define ICON_UPDOWN   "\xe2\x86\x95"

#define black RGBA8(0x00, 0x00, 0x00, 0xFF)
#define white RGBA8(0xFF, 0xFF, 0xFF, 0xFF)
#define green RGBA8(0x00, 0xFF, 0x00, 0xFF)
#define red   RGBA8(0xFF, 0x00, 0x00, 0xFF)

#define TEMP_FILE   "ux0:data/savemgr/tmp"
#define CONFIG_FILE "ux0:data/savemgr/config"

#define SCE_CTRL_HOLD 0x80000000

extern int SCE_CTRL_ENTER;
extern int SCE_CTRL_CANCEL;
extern char ICON_ENTER[4];
extern char ICON_CANCEL[4];

#endif
