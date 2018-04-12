#ifndef __INPUT_H__
#define __INPUT_H__

void init_input();
void lock_psbutton();
void unlock_psbutton();
int read_buttons();
int read_touchscreen(point *p);

#endif
