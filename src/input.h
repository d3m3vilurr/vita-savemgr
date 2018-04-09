#ifndef __INPUT_H__
#define __INPUT_H__

void lock_psbutton();
void unlock_psbutton();
int read_buttons();
int read_touchscreen(point *p);

#endif
