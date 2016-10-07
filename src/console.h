#ifndef __CONSOLE_H__
#define __CONSOLE_H__

void init_console();
void draw_text(uint32_t y, char* text, uint32_t color);
void draw_loop_text(uint32_t y, char *text, uint32_t color);
void clear_screen();

#endif
