#ifndef __CONSOLE_H__
#define __CONSOLE_H__

typedef enum {
    NONE,
    SIMPLE,
    WARNING,
    ERROR,
    CONFIRM_AND_CANCEL,
} popup_type;

typedef enum {
    LEFT,
    RIGHT,
    CENTER,
} line_align;

typedef struct popup_line {
    const char *string;
    line_align align;
    int padding[4];
    int color;
    int width;
} popup_line;

void init_console();
void draw_start();
void draw_end();
void draw_text(uint32_t y, const char* text, uint32_t color);
void draw_loop_text(uint32_t y, const char *text, uint32_t color);
void clear_screen();

void draw_popup(popup_type type, popup_line lines[]);
void open_popup(popup_type type, popup_line lines[]);
void close_popup();

#endif
