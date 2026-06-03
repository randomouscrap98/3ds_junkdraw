#ifndef __SETUP_HEADER_
#define __SETUP_HEADER_

// NOTE: this is meant to included only once, in main!

#include <3ds.h>
#include <stdarg.h>
#include <stdio.h>

#include "log.h"

#define FOLDER_BASE "/3ds/junkdraw/"
#define SAVE_BASE FOLDER_BASE "saves/"
#define SCREENSHOTS_BASE FOLDER_BASE "screenshots/"
#define SETTINGS_PATH FOLDER_BASE "settings.ini"

// UI stuff
#define SCROLL_WIDTH 3
#define SCROLL_BG C2D_Color32f(0.8, 0.8, 0.8, 1)
#define SCROLL_BAR C2D_Color32f(0.5, 0.5, 0.5, 1)

#define SCREEN_COLOR C2D_Color32(90, 90, 90, 255)
#define CANVAS_BG_COLOR C2D_Color32(255, 255, 255, 255)
#define CANVAS_LAYER_COLOR C2D_Color32(0, 0, 0, 0)

#define MAINMENU_TOP 7 // Where to put the menu?
#define MAINMENU_TITLE "Junkdraw menu"
#define MAINMENU_ITEMS                                                         \
  "Edit\0Save\0Load\0New\0Export\0Options\0Session Options\0Exit App\0"
#define MAINMENU_EDIT 0
#define MAINMENU_SAVE 1
#define MAINMENU_LOAD 2
#define MAINMENU_NEW 3
#define MAINMENU_EXPORT 4
#define MAINMENU_OPTIONS 5
#define MAINMENU_RUNTIMEOPTIONS 6
#define MAINMENU_EXIT 7

#define PRINTDATA_WIDTH 40
#define STATUS_MAINCOLOR 36
#define STATUS_WARNING 33
#define STATUS_ERROR 31
#define STATUS_ACTIVECOLOR 37

#define MAX_MENULIST 12

// Filesys
#define MAX_FILENAME 64
#define MAX_FILENAMESHOW 5
#define MAX_FILEPATH 256
#define MAX_TEMPSTRING 2048
#define MAX_ALLFILENAMES 65535

// Networking
#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
#define SOC_MAXCLIENTS 1

// Drawing system
#define MIN_ZOOMPOWER -2
#define MAX_ZOOMPOWER 4
#define MIN_WIDTH 1
#define MAX_WIDTH 64
#define MAX_PAGE 998 // 0xFFFF

#define DEFAULT_PALETTE_SPLIT 64
#define DEFAULT_START_LAYER 1
#define DEFAULT_PALETTE_STARTINDEX 1

#define LAYER_COUNT 2
#define TEXTURE_WIDTH 1024
#define TEXTURE_HEIGHT 1024

#define BREPEAT_DELAY 20
#define BREPEAT_INTERVAL 7

// Undo
#define MAX_UNDO 99

// These are all compiler config things, which may be applied to any
// individually compiled portion of this app. Probably not the right way to do
// this.

// Release mode is a special mode
// #define RELEASE

// Much more logging
// #define TRACE_PRINT

#ifdef RELEASE
// Put anything that goes specifically for release in here
#else
// Put everything for debug mode in here
#define DEBUG_PRINT
#define DEBUG_RUNTESTS
#endif

// -------- LOGGING ----------

#define DEBUG_PRINT_MINROW 21
#define DEBUG_PRINT_ROWS 5

static inline void logbase(u8 color, const char * fmt, va_list args) {
  static u8 _db_prnt_row = 0;
  static u8 _db_prnt_num = 0;
  printf("\x1b[%d;1H%50s", _db_prnt_row + DEBUG_PRINT_MINROW, "");
  printf("\x1b[%d;1H\x1b[%dm", _db_prnt_row + DEBUG_PRINT_MINROW, color);
  _db_prnt_row = (_db_prnt_row + 1) % DEBUG_PRINT_ROWS;
  _db_prnt_num = (_db_prnt_num + 1) % 100;
  time_t rawtime = time(NULL);
  struct tm *timeinfo = localtime(&rawtime);
  printf("[%02d|%02d:%02d] ", _db_prnt_num, timeinfo->tm_hour, timeinfo->tm_min);
  //printf("[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  vprintf(fmt, args);
}

// void LOGERR(const char *fmt, ...) {
// #ifdef DEBUG_PRINT
//   LOGBASE(fmt, STATUS_ERROR);
// #endif
// }

void LOGDBG(const char *fmt, ...) {
#ifdef DEBUG_PRINT
  va_list args;
  va_start(args, fmt);
  logbase(STATUS_WARNING, fmt, args);
  va_end(args);
#endif
}

void LOGTRACE(const char *fmt, ...) {
#ifdef TRACE_PRINT
  va_list args;
  va_start(args, fmt);
  LOGBASE(STATUS_WARNING, fmt, args);
  va_end(args);
#endif
}

#endif
