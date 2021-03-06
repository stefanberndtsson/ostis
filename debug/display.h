#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL.h>

#define FONT_SMALL 0
#define FONT_LARGE 1
#define FONT_SMINV 2
#define FONT_LGINV 3

void display_setup();
void display_clear_screen();
void display_put_char(int, int, int, unsigned char);
void display_put_pixel_screen(int, int, long);
void display_render_screen();
void display_swap_screen();
void debug_raise_window();
SDL_Surface *display_get_screen();
extern int debug_window_id;
extern int debug_update_win;

#endif
