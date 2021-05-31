
#ifndef __HEADER_MYUTILS
#define __HEADER_MYUTILS

#include <3ds.h>
#include <stdio.h>

#ifdef DEBUG_PRINT
#define LOGDBG(f_, ...) printf((f_), ## __VA_ARGS__)
#else
#define LOGDBG(f_, ...)
#endif

#define MAX_MENU_ITEMS 20

// GENERAL STUFF
u32 u32_clamp(u32 num, u32 minimum, u32 maximum);

// COLOR STUFF
u32 rgb24_to_rgba32c(u32 rgb);
u32 rgba32c_to_rgba16c_32(u32 val);
u16 rgba32c_to_rgba16c(u32 val);
u32 rgba16c_to_rgba32c(u16 val);
void convert_palette(u32 * original, u16 * destination, u16 size);

// MENU STUFF
s8 easy_menu(const char * title, const char * menu_items, u8 top, u32 exit_btns);

#endif
