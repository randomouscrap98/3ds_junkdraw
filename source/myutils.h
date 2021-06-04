
#ifndef __HEADER_MYUTILS
#define __HEADER_MYUTILS

#include <3ds.h>
#include <stdio.h>

#ifdef DEBUG_PRINT_TIME
#define LOGTIME() { time_t rawtime = time(NULL); \
   struct tm * timeinfo = localtime(&rawtime); \
   printf("[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec); }
#else
#define LOGTIME()
#endif

#ifdef DEBUG_PRINT
#ifdef DEBUG_PRINT_SPECIAL
#define LOGDBG(f_, ...) {printf(DEBUG_PRINT_SPECIAL);LOGTIME();printf((f_), ## __VA_ARGS__);}
#else
#define LOGDBG(f_, ...) {LOGTIME();printf((f_), ## __VA_ARGS__);}
#endif
#else
#define LOGDBG(f_, ...)
#endif

//Some of these should be namespaced or whatever
#define MAX_MENU_ITEMS 20
#define ENTERTEXT_CHAR "_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define ENTERTEXT_CHARARRSIZE 50

// GENERAL STUFF
#define UTILS_CLAMP(x, mn, mx) (x <= mn ? mn : x >= mx ? mx : x)

// COLOR STUFF
u32 rgb24_to_rgba32c(u32 rgb);
u32 rgba32c_to_rgba16c_32(u32 val);
u16 rgba32c_to_rgba16c(u32 val);
u32 rgba16c_to_rgba32c(u16 val);
u16 rgba32c_to_rgba16(u32 val); //This is proper rgbaa16
u32 rgba16_to_rgba32c(u16 val);
void convert_palette(u32 * original, u16 * destination, u16 size);

// MENU STUFF
s8 easy_menu(const char * title, const char * menu_items, u8 top, u32 exit_btns);
bool easy_confirm(const char * title, u8 top); 
bool easy_warn(const char * warn, const char * title, u8 top);
bool enter_text_fixed(const char * title, u8 top, char * container, 
      u8 max_entry, bool clear, u32 exit_btns);

#endif
