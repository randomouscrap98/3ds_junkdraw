#ifndef __SETUP_HEADER_
#define __SETUP_HEADER_

#include <3ds.h>

#define VERSION "0.2.3"
#define FOLDER_BASE "/3ds/junkdraw/"
#define SAVE_BASE FOLDER_BASE"saves/"
#define SCREENSHOTS_BASE FOLDER_BASE"screenshots/"

#define SCROLL_WIDTH 3
#define SCROLL_BG C2D_Color32f(0.8,0.8,0.8,1)
#define SCROLL_BAR C2D_Color32f(0.5,0.5,0.5,1)

#define MAINMENU_TOP 6
#define MAINMENU_TITLE "Main menu:"
#define MAINMENU_ITEMS "New\0Save\0Load\0Exit App\0"
#define MAINMENU_NEW 0
#define MAINMENU_SAVE 1
#define MAINMENU_LOAD 2
#define MAINMENU_EXIT 3

//Release mode is a special mode
//#define RELEASE

// These are all compiler config things, which may be applied to any
// individually compiled portion of this app. Probably not the right way to do
// this.

#ifdef RELEASE
//Put anything that goes specifically for release in here
#else
//Put everything for debug mode in here
#define DEBUG_PRINT
#define DEBUG_PRINT_TIME
#define DEBUG_RUNTESTS

#define DEBUG_PRINT_MINROW 21
#define DEBUG_PRINT_ROWS 5

extern u8 _db_prnt_row; // What the hell is this
#define DEBUG_PRINT_SPECIAL() { printf("\x1b[%d;1H\x1b[33m", _db_prnt_row + DEBUG_PRINT_MINROW); \
   _db_prnt_row = (_db_prnt_row + 1) % DEBUG_PRINT_ROWS; }

#endif

// -------- LOGGING ----------

#ifdef DEBUG_PRINT_TIME
#define LOGTIME() { time_t rawtime = time(NULL); \
   struct tm * timeinfo = localtime(&rawtime); \
   printf("[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec); }
#else
#define LOGTIME()
#endif

#ifdef DEBUG_PRINT
#ifdef DEBUG_PRINT_SPECIAL
#define LOGDBG(f_, ...) {DEBUG_PRINT_SPECIAL();LOGTIME();printf((f_), ## __VA_ARGS__);}
#else
#define LOGDBG(f_, ...) {LOGTIME();printf((f_), ## __VA_ARGS__);}
#endif
#else
#define LOGDBG(f_, ...)
#endif

#endif
