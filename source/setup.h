#ifndef __SETUP_HEADER_
#define __SETUP_HEADER_

#include <3ds.h>

#define VERSION "0.4.1"
#define FOLDER_BASE "/3ds/junkdraw/"
#define SAVE_BASE FOLDER_BASE "saves/"
#define SCREENSHOTS_BASE FOLDER_BASE "screenshots/"

// UI stuff
#define SCROLL_WIDTH 3
#define SCROLL_BG C2D_Color32f(0.8, 0.8, 0.8, 1)
#define SCROLL_BAR C2D_Color32f(0.5, 0.5, 0.5, 1)

#define SCREEN_COLOR C2D_Color32(90, 90, 90, 255)
#define CANVAS_BG_COLOR C2D_Color32(255, 255, 255, 255)
#define CANVAS_LAYER_COLOR C2D_Color32(0, 0, 0, 0)

#define PALETTE_COLORS 64
#define PALETTE_SWATCHWIDTH 18
#define PALETTE_SWATCHMARGIN 2
#define PALETTE_SELECTED_COLOR C2D_Color32f(1, 0, 0, 1)
#define PALETTE_BG C2D_Color32f(0.3, 0.3, 0.3, 1)
#define PALETTE_STARTINDEX 1
#define PALETTE_MINISIZE 10
#define PALETTE_MINIPADDING 1
#define PALETTE_OFSX 10
#define PALETTE_OFSY 10

#define NUM_LASTCOLORS 32

#define MAINMENU_TOP 6 // Where to put the menu?
#define MAINMENU_TITLE "Main menu:"
// #define MAINMENU_ITEMS(animon) "New\0Save\0Load\0Export PNG\0"(animon)
#define MAINMENU_ITEMS                                                         \
  "New\0Save\0Load\0Export PNG\0Export GIF\0Options\0Exit App\0"
#define MAINMENU_NEW 0
#define MAINMENU_SAVE 1
#define MAINMENU_LOAD 2
#define MAINMENU_EXPORT 3
#define MAINMENU_EXPORTGIF 4
#define MAINMENU_OPTIONS 5
#define MAINMENU_EXIT 6

#define PRINTDATA_WIDTH 40
#define STATUS_MAINCOLOR 36
#define STATUS_ACTIVECOLOR 37
#define FILELOAD_MENUCOUNT 10

// Filesys
#define MAX_FILENAME 64
#define MAX_FILENAMESHOW 5
#define MAX_FILEPATH 256
#define MAX_TEMPSTRING 2048
#define MAX_ALLFILENAMES 65535

// Drawing system
#define MIN_ZOOMPOWER -2
#define MAX_ZOOMPOWER 4
#define MIN_WIDTH 1
#define MAX_WIDTH 64
#define MAX_PAGE 0xFFFF

#define DEFAULT_PALETTE_SPLIT 64
#define DEFAULT_START_LAYER 1
#define DEFAULT_PALETTE_STARTINDEX 1

#define LAYER_COUNT 2
#define TEXTURE_WIDTH 1024
#define TEXTURE_HEIGHT 1024

// Scanning
#define MAX_FRAMELINES 1000

#define BREPEAT_DELAY 20
#define BREPEAT_INTERVAL 7

// Release mode is a special mode
// #define RELEASE

// These are all compiler config things, which may be applied to any
// individually compiled portion of this app. Probably not the right way to do
// this.

#ifdef RELEASE
// Put anything that goes specifically for release in here
#else
// Put everything for debug mode in here
#define DEBUG_PRINT
#define DEBUG_PRINT_TIME
#define DEBUG_RUNTESTS

#define DEBUG_PRINT_MINROW 21
#define DEBUG_PRINT_ROWS 5

extern u8 _db_prnt_row; // What the hell is this
#define DEBUG_PRINT_SPECIAL()                                                  \
  {                                                                            \
    printf("\x1b[%d;1H\x1b[33m", _db_prnt_row + DEBUG_PRINT_MINROW);           \
    _db_prnt_row = (_db_prnt_row + 1) % DEBUG_PRINT_ROWS;                      \
  }

#endif

// -------- LOGGING ----------

#ifdef DEBUG_PRINT_TIME
#define LOGTIME()                                                              \
  {                                                                            \
    time_t rawtime = time(NULL);                                               \
    struct tm *timeinfo = localtime(&rawtime);                                 \
    printf("[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min,           \
           timeinfo->tm_sec);                                                  \
  }
#else
#define LOGTIME()
#endif

#ifdef DEBUG_PRINT
#ifdef DEBUG_PRINT_SPECIAL
#undef LOGDBG
#define LOGDBG(f_, ...)                                                        \
  {                                                                            \
    DEBUG_PRINT_SPECIAL();                                                     \
    LOGTIME();                                                                 \
    printf((f_), ##__VA_ARGS__);                                               \
  }
#else
#undef LOGDBG
#define LOGDBG(f_, ...)                                                        \
  {                                                                            \
    LOGTIME();                                                                 \
    printf((f_), ##__VA_ARGS__);                                               \
  }
#endif
#else
#undef LOGDBG
#define LOGDBG(f_, ...)
#endif

#endif
