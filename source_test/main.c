#include <3ds.h>

u32 __stacksize__ = 512 * 1024;

#include "utils.h"

#include <citro2d.h>
#include <citro3d.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define BREPEAT_DELAY 20
#define BREPEAT_INTERVAL 7
#define STATUS_MAINCOLOR 36
#define STATUS_WARNING 33

#define PRINTCLEAR() {                                                       \
  printf_flush("\x1b[%d;2H%-250s", MAINMENU_TOP, "");                        \
}

#define PRINTGENERAL(x, col, ...) {                                          \
  printf_flush("\x1b[%d;1H%-150s\x1b[%d;2H\x1b[%dm", MAINMENU_TOP, "",       \
               MAINMENU_TOP, col);                                           \
  printf_flush(x, ##__VA_ARGS__);                                            \
}

#define PRINTERR(x, ...) PRINTGENERAL(x, 31, ##__VA_ARGS__)
#define PRINTWARN(x, ...) PRINTGENERAL(x, 33, ##__VA_ARGS__)
#define PRINTINFO(x, ...) PRINTGENERAL(x, 37, ##__VA_ARGS__)

static inline void logbase(u8 color, const char * fmt, va_list args) {
  static u8 _db_prnt_num = 0;
  //printf("\x1b[%d;1H%50s", _db_prnt_row + DEBUG_PRINT_MINROW, "");
  printf("\x1b[%dm", color);
  _db_prnt_num = (_db_prnt_num + 1) % 100;
  time_t rawtime = time(NULL);
  struct tm *timeinfo = localtime(&rawtime);
  printf("[%02d|%02d:%02d] ", _db_prnt_num, timeinfo->tm_hour, timeinfo->tm_min);
  vprintf(fmt, args);
}

void LOGDBG(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logbase(STATUS_WARNING, fmt, args);
  va_end(args);
}

void LOGTRACE(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logbase(STATUS_MAINCOLOR, fmt, args);
  va_end(args);
}

// -- MENU/PRINT STUFF --

int main(int argc, char **argv) {
  gfxInitDefault();
  hidSetRepeatParameters(BREPEAT_DELAY, BREPEAT_INTERVAL);

  // Enable the higher clock speed on New 3DS
  osSetSpeedupEnable(true);

  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  //PrintConsole * console_ptr = 
  consoleInit(GFX_TOP, NULL);
  C3D_RenderTarget *screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  LOGTRACE("INITIALIZED");

  // Run tests... here?? Or run some amount of tests per frame maybe...

  while (aptMainLoop()) {
    hidScanInput();

    u32 kDown = hidKeysDown();
    u32 kUp = hidKeysUp();
    u32 kRepeat = hidKeysDownRepeat();
    u32 kHeld = hidKeysHeld();
    circlePosition pos;
    touchPosition current_touch;
    hidTouchRead(&current_touch);
    hidCircleRead(&pos);

    if(kUp & KEY_START) {
      break;
    }

    // =======================================
    // Render the scene
    // =======================================
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    // -- LAYER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE,
                   GPU_ZERO);

    C2D_Flush();

    // -- OTHER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA);

    C2D_TargetClear(screen, 0xFFFFFFFF);
    C2D_SceneBegin(screen);

    // draw_layers(&layer_window, &sys);
    // draw_scrollbars(&sys.screen_state);
    // draw_colorpicker(&sys.colors, !sstate.palette_active);

    C3D_FrameEnd(0);
  }
ENDMAINLOOP:;

  C3D_RenderTargetDelete(screen);

  C2D_Fini();
  C3D_Fini();
  gfxExit();
  return 0;
}
