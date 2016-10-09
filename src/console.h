#ifndef __CONSOLE_H__
#define __CONSOLE_H__

void init_console();
void draw_start();
void draw_end();
void draw_text(uint32_t y, const char* text, uint32_t color);
void draw_loop_text(uint32_t y, const char *text, uint32_t color);
void clear_screen();

#endif
