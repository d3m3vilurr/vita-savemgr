#ifndef __COMMON_H__
#define __COMMON_H__

#include <vita2d.h>

// clang-format off
#define SAVE_MANAGER  "SAVEMGR00"

#define ICON_CIRCLE   "\xe2\x97\x8b"
#define ICON_CROSS    "\xe2\x95\xb3"
#define ICON_SQUARE   "\xe2\x96\xa1"
#define ICON_TRIANGLE "\xe2\x96\xb3"
#define ICON_UPDOWN   "\xe2\x86\x95"


#define SCREEN_WIDTH                960
#define SCREEN_HEIGHT               544
#define SCREEN_HALF_WIDTH           (SCREEN_WIDTH / 2)
#define SCREEN_HALF_HEIGHT          (SCREEN_HEIGHT / 2)
#define HEADER_HEIGHT               40
#define FOOTER_HEIGHT               0
#define ICONS_ROW                   4
#define ICONS_COL                   7

#define ITEMS_PANEL_PADDING         5
#define ITEMS_PANEL_WIDTH           (SCREEN_WIDTH)
#define ITEMS_PANEL_INNER_WIDTH     (ITEMS_PANEL_WIDTH - (ITEMS_PANEL_PADDING * 2))
#define ITEMS_PANEL_HEIGHT          (SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT)
#define ITEMS_PANEL_INNER_HEIGHT    (ITEMS_PANEL_HEIGHT - (ITEMS_PANEL_PADDING * 2))
#define ITEMS_PANEL_TOP             (HEADER_HEIGHT)
#define ITEMS_PANEL_LEFT            (0)
#define ITEMS_INNER_TOP             (ITEMS_PANEL_TOP + ITEMS_PANEL_PADDING)
#define ITEMS_INNER_LEFT            (ITEMS_PANEL_LEFT + ITEMS_PANEL_PADDING)
#define ITEM_BOX_MARGIN             5
#define ITEM_BOX_PADDING            0
#define ITEM_BOX_TOP(y, h)          (ITEMS_INNER_TOP + (ITEM_BOX_MARGIN * ((y) + 1)) + ((h) * (y)))
#define ITEM_BOX_LEFT(x, w)         (ITEMS_INNER_LEFT + (ITEM_BOX_MARGIN * ((x) + 1)) + ((w) * (x)))
#define ITEM_BOX_WIDTH(col)         (int)((ITEMS_PANEL_INNER_WIDTH - (ITEM_BOX_MARGIN * ((col) + 1))) / (col))
#define ITEM_BOX_HEIGHT(row)        (int)((ITEMS_PANEL_INNER_HEIGHT - (ITEM_BOX_MARGIN * ((row) + 1))) / (row))

#define ICON_BOX_WIDTH              ITEM_BOX_WIDTH(ICONS_COL)
#define ICON_BOX_HEIGHT             ITEM_BOX_HEIGHT(ICONS_ROW)
#define ICON_WIDTH                  (ICON_BOX_WIDTH - (ITEM_BOX_PADDING * 2))
#define ICON_HEIGHT                 (ICON_BOX_HEIGHT - (ITEM_BOX_PADDING * 2))
#define ICON_TOP(y)                 (ITEM_BOX_TOP((y), ICON_BOX_HEIGHT) + ITEM_BOX_PADDING)
#define ICON_LEFT(x)                (ITEM_BOX_LEFT((x), ICON_BOX_WIDTH) + ITEM_BOX_PADDING)

#define LIST_ROW                    8
#define LIST_BOX_WIDTH              ITEM_BOX_WIDTH(1)
#define LIST_BOX_HEIGHT             ITEM_BOX_HEIGHT(LIST_ROW)
#define LIST_WIDTH                  (LIST_BOX_WIDTH - (ITEM_BOX_PADDING * 2))
#define LIST_HEIGHT                 (LIST_BOX_HEIGHT - (ITEM_BOX_PADDING * 2))
#define LIST_TOP(y)                 (ITEM_BOX_TOP((y), LIST_BOX_HEIGHT) + ITEM_BOX_PADDING)
#define LIST_LEFT                   (ITEM_BOX_LEFT(0, LIST_BOX_WIDTH) + ITEM_BOX_PADDING)
#define LIST_TEXT_LEFT              LIST_LEFT + LIST_HEIGHT + ITEM_BOX_MARGIN
#define LIST_TEXT_WIDTH             LIST_WIDTH - LIST_HEIGHT - ITEM_BOX_MARGIN

#define APPINFO_PANEL_TOP           (ITEMS_PANEL_TOP)
#define APPINFO_PANEL_LEFT          (ITEMS_PANEL_LEFT)
#define APPINFO_PANEL_WIDTH         (ITEMS_PANEL_WIDTH / 2)
#define APPINFO_PANEL_HEIGHT        (ITEMS_PANEL_HEIGHT)
#define APPINFO_PANEL_PADDING       5

#define APPINFO_DESC_TOP            (int)(APPINFO_PANEL_TOP + (APPINFO_PANEL_HEIGHT / 2) + APPINFO_PANEL_PADDING)
#define APPINFO_DESC_LEFT           (APPINFO_PANEL_LEFT + APPINFO_PANEL_PADDING)
#define APPINFO_DESC_WIDTH          (APPINFO_PANEL_WIDTH - (APPINFO_PANEL_PADDING * 2))
#define APPINFO_DESC_HEIGHT         (int)((APPINFO_PANEL_HEIGHT / 2) - (APPINFO_PANEL_PADDING * 2))
#define APPINFO_DESC_PADDING        5

#define APPINFO_ICON_PADDING        40
#define APPINFO_ICON_TOP            (APPINFO_PANEL_TOP + APPINFO_PANEL_PADDING + APPINFO_ICON_PADDING)
#define APPINFO_ICON_LEFT           (APPINFO_PANEL_LEFT + APPINFO_PANEL_PADDING + APPINFO_ICON_PADDING)
#define APPINFO_ICON_WIDTH          (int)((APPINFO_PANEL_WIDTH / 2) - (APPINFO_PANEL_PADDING * 2) - (APPINFO_ICON_PADDING * 2))
#define APPINFO_ICON_HEIGHT         (int)((APPINFO_PANEL_HEIGHT / 2) - (APPINFO_PANEL_PADDING * 2) - (APPINFO_ICON_PADDING * 2))

#define APPINFO_BUTTONS_TOP         (APPINFO_PANEL_TOP + APPINFO_PANEL_PADDING)
#define APPINFO_BUTTONS_LEFT        (int)(APPINFO_PANEL_LEFT + (APPINFO_PANEL_WIDTH / 2) + APPINFO_PANEL_PADDING)
#define APPINFO_BUTTONS_WIDTH       (int)((APPINFO_PANEL_WIDTH / 2) - (APPINFO_PANEL_PADDING * 2))
#define APPINFO_BUTTONS_HEIGHT      (int)((APPINFO_PANEL_HEIGHT / 2) - (APPINFO_PANEL_PADDING * 2))

#define APPINFO_BUTTON              4
#define APPINFO_BUTTON_MARGIN       5
#define APPINFO_BUTTON_WIDTH        (int)(APPINFO_BUTTONS_WIDTH - (APPINFO_BUTTON_MARGIN * 2))
#define APPINFO_BUTTON_HEIGHT       (int)((APPINFO_BUTTONS_HEIGHT - (APPINFO_BUTTON_MARGIN * (APPINFO_BUTTON + 1))) / APPINFO_BUTTON)
#define APPINFO_BUTTON_LEFT         (APPINFO_BUTTONS_LEFT + APPINFO_BUTTON_MARGIN)
#define APPINFO_BUTTON_TOP(x)       (APPINFO_BUTTONS_TOP + APPINFO_BUTTON_MARGIN + (APPINFO_BUTTON_HEIGHT + APPINFO_BUTTON_MARGIN) * (x))

#define SLOT_PANEL_TOP              (ITEMS_PANEL_TOP)
#define SLOT_PANEL_LEFT             (APPINFO_PANEL_LEFT + APPINFO_PANEL_WIDTH)
#define SLOT_PANEL_WIDTH            (ITEMS_PANEL_WIDTH / 2)
#define SLOT_PANEL_HEIGHT           (ITEMS_PANEL_HEIGHT)
#define SLOT_PANEL_PADDING          5

#define SLOT_HEADER_TOP             (ITEMS_PANEL_TOP + SLOT_PANEL_PADDING)
#define SLOT_HEADER_LEFT            (ITEMS_PANEL_LEFT + SLOT_PANEL_PADDING)
#define SLOT_HEADER_HEIGHT          40

#define SLOT_BUTTONS_TOP            (SLOT_HEADER_TOP + SLOT_HEADER_HEIGHT)
#define SLOT_BUTTONS_HEIGHT         (SLOT_PANEL_HEIGHT - SLOT_HEADER_HEIGHT - (SLOT_PANEL_PADDING * 2))
#define SLOT_BUTTON                 10
#define SLOT_BUTTON_MARGIN          5
#define SLOT_BUTTON_WIDTH           (int)(SLOT_PANEL_WIDTH - (SLOT_BUTTON_MARGIN * 2))
#define SLOT_BUTTON_HEIGHT          (int)((SLOT_BUTTONS_HEIGHT - (SLOT_BUTTON_MARGIN * (SLOT_BUTTON + 1))) / SLOT_BUTTON)
#define SLOT_BUTTON_LEFT            (SLOT_PANEL_LEFT + SLOT_BUTTON_MARGIN)
#define SLOT_BUTTON_TOP(x)          (SLOT_BUTTONS_TOP + SLOT_BUTTON_MARGIN + (SLOT_BUTTON_HEIGHT + SLOT_BUTTON_MARGIN) * (x))

#define ICON_BUF_SIZE               (50 * 1024)

#define BLACK               RGBA8(0x00, 0x00, 0x00, 0xFF)
#define LIGHT_SLATE_GRAY    RGBA8(0x77, 0x88, 0x99, 0xFF)
#define LIGHT_GRAY          RGBA8(0xD3, 0xD3, 0xD3, 0xD3)
#define WHITE               RGBA8(0xFF, 0xFF, 0xFF, 0xFF)
#define LIGHT_SKY_BLUE      RGBA8(0x87, 0xCE, 0xEB, 0xFF)
#define RED                 RGBA8(0xFF, 0x00, 0x00, 0xFF)
#define GREEN               RGBA8(0x00, 0x80, 0x00, 0xFF)

#define TEMP_FILE   "ux0:data/savemgr/tmp"
#define CONFIG_FILE "ux0:data/savemgr/config.ini"

#define OLD_RINCHEAT_SAVEDIR          "/data/rinCheat"
#define OLD_RINCHEAT_SAVE_FORMAT      "%s_SAVEDATA"
#define OLD_RINCHEAT_SAVE_SLOT_FORMAT "%s_SAVEDATA_SLOT_%d"

#define DEFAULT_BASE_SAVEDIR          "/data/savegames"
#define DEFAULT_SAVE_SLOT_FORMAT      "%s/SLOT%d"
#define DEFAULT_LIST_MODE             "icon"

#define SCE_CTRL_HOLD 0x80000000
// clang-format on

extern int SCE_CTRL_ENTER;
extern int SCE_CTRL_CANCEL;
extern char ICON_ENTER[4];
extern char ICON_CANCEL[4];

extern vita2d_pgf *font;
extern char *confirm_msg;
extern int confirm_msg_width;
extern char *close_msg;
extern int close_msg_width;

typedef struct point {
    int x;
    int y;
} point;

typedef struct rectangle {
    int left;
    int top;
    int right;
    int bottom;
} rectangle;

typedef struct icon_data {
    uint8_t *buf;
    vita2d_texture *texture;
    rectangle touch_area;
} icon_data;

#endif
