#include "3ds/console.h"
#include "utils.h"
#include "littlelogbox.h"
#include "ansi.h"
#include "controls.h"

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

u32 __stacksize__ = 512 * 1024;

#define MAX_FILENAME 64

// Console crap
#define UI_CONSOLE_LOGTOP       20
#define UI_CONSOLE_LOGHEIGHT    8
#define UI_CONSOLE_LOGCOLOR     ANSI_FG_BRIGHT_BLACK
//CONSOLE_ESC(30;1m)

// NOTE: for now, the log can't even scroll, so no need to make it large
#define MAX_LOGMESSAGES 30
#define MAX_LOGMSGLENGTH 100

#define SCROLL_WIDTH 3
#define SCREEN_COLOR C2D_Color32(90, 90, 90, 255)
#define SCROLL_BG C2D_Color32f(0.8, 0.8, 0.8, 1)
#define SCROLL_BAR C2D_Color32f(0.5, 0.5, 0.5, 1)

// ==========================================
//                Logging
// ==========================================

tui_logbox logbox;

void LOGERR(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "E", fmt) }
void LOGWRN(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "W", fmt) }
void LOGINF(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "I", fmt) }
void LOGDBG(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "D", fmt) }
void LOGTRC(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "T", fmt) }

// ==========================================
//               Rendering
// ==========================================

void ui_render_logbox(tui_logbox * lb) {
  char out[51]; // Just wide enough for the screen + null
  for(int i = 0; i < UI_CONSOLE_LOGHEIGHT; i++) {
    tui_logbox_renderline(lb, out, 50, UI_CONSOLE_LOGHEIGHT, i);
    printf(CONSOLE_ESC(%d;%dH) UI_CONSOLE_LOGCOLOR "%s",
           UI_CONSOLE_LOGTOP + i, 0, out);
  }
}

// ==========================================
//                  Main
// ==========================================

bool isn3ds() {
  bool isn3ds = false;
  Result res = APT_CheckNew3DS(&isn3ds);

  if(R_SUCCEEDED(res)) {
    return isn3ds;
  } 
  return false;
}

int main() {
  gfxInitDefault();
  control_setup_defaults();

  // Set this up IMMEDIATELY
  if(tui_logbox_init(&logbox, MAX_LOGMESSAGES, MAX_LOGMSGLENGTH)) return 1;
  tui_logbox_unit_t logbox_lasthead = logbox.head;

  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  //PrintConsole * console_ptr = 
  consoleInit(GFX_TOP, NULL);
  C3D_RenderTarget *screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  if(isn3ds()) {
    LOGDBG("New 3ds detected");
    osSetSpeedupEnable(true);
  } 

  char save_filename[MAX_FILENAME];
  control_config ctrlconfig = { .tool = 0, .scheme = 0, };

  LOGDBG("STARTING MAIN LOOP");

  while (aptMainLoop()) {
    control_inputs inputs = control_get_inputs();
    control_action actions = control_get_action(&ctrlconfig, &inputs);

    if(actions.action == CTRL_MENU) {
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

    C2D_TargetClear(screen, SCREEN_COLOR);
    C2D_SceneBegin(screen);

    if(logbox.head != logbox_lasthead) {
      ui_render_logbox(&logbox);
      logbox_lasthead = logbox.head;
    }

    // draw_layers(&layer_window, &sys);
    // draw_scrollbars(&sys.screen_state);
    // draw_colorpicker(&sys.colors, !sstate.palette_active);

    C3D_FrameEnd(0);

  }
ENDMAINLOOP:;

  C3D_RenderTargetDelete(screen);

  C2D_Fini();
  C3D_Fini();
  // exitRomfs();
  gfxExit();

  tui_logbox_free(&logbox);
  return 0;
}
