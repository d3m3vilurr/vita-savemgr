#ifndef __DISPLAY_H__
#define __DISPLAY_H__

enum {
    CANCEL = 0,
    CONFIRM = 1
};

void init_console();
int confirm(const char *msg, float zoom);
int alert(const char *msg, float zoom);

void init_progress(int max);
void incr_progress();

#endif
