#ifndef SCREEN_H
#define SCREEN_H

void screen_init();
void screen_putpixel(int, int, long, int);
void screen_swap();
void screen_disable(int);
int screen_check_disable();

#endif
