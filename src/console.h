#ifndef __CONSOLE_H__
#define __CONSOLE_H__

typedef enum {
    NONE,
    WARNING,
    CONFIRM_AND_CANCEL,
} popup_type;

void init_console();
void draw_start();
void draw_end();
void draw_text(uint32_t y, const char* text, uint32_t color);
void draw_loop_text(uint32_t y, const char *text, uint32_t color);
void clear_screen();

void draw_popup(popup_type type, const char *lines[]);
void open_popup(popup_type type, const char *lines[]);
void close_popup();

#endif
