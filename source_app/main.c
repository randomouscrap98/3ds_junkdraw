#include "3ds/console.h"
#include "littlemenu.h"
#include "utils.h"
#include "ansi.h"
#include "datacontainer.h"

#include "controls.h"
#include "logging.h"

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>


u32 __stacksize__ = 512 * 1024;

#define MAX_FILENAME 64
#define MAX_DRAW_DATA ((u32)5000000)
#define MAX_WARNMSG   512

// Version info?
#define VERSION "0.6.0"
#define VERSIONSTRING "- Junkdraw "VERSION" -"

// Console crap
#define UI_CONSOLE_LOGTOP         20
#define UI_CONSOLE_LOGHEIGHT      8
#define UI_CONSOLE_LOGCOLOR       ANSI_BG_BLACK ANSI_FG_BRIGHT_BLACK ANSI_INVERT_OFF
#define UI_CONSOLE_CONTROLTOP     0
#define UI_CONSOLE_CONTROLSCOLOR  ANSI_BG_BLACK ANSI_FG_BRIGHT_BLACK ANSI_INVERT_ON
#define UI_CONSOLE_MENUTOP        6
#define UI_CONSOLE_MENUHEIGHT     12
#define UI_CONSOLE_MENUBARCOLOR   ANSI_BG_BLACK ANSI_FG_WHITE ANSI_INVERT_ON
#define UI_CONSOLE_MENUCOLOR      ANSI_BG_BLACK ANSI_FG_WHITE ANSI_INVERT_OFF
#define UI_CONSOLE_MENUSELECTCOLOR  ANSI_BG_BLACK ANSI_FG_CYAN ANSI_INVERT_ON

#define SCROLL_WIDTH    3
#define SCREEN_COLOR    C2D_Color32(90, 90, 90, 255)
#define SCROLL_BG       C2D_Color32f(0.8, 0.8, 0.8, 1)
#define SCROLL_BAR      C2D_Color32f(0.5, 0.5, 0.5, 1)

#define MAIN_MODE_DRAW        0
#define MAIN_MODE_MENU        1
#define MAIN_MODE_FAILURE     2
#define MAIN_MODE_EXIT        3

// ==========================================
//              Global Data
// ==========================================

typedef struct {
  DataContainer drawdata;
  vector_tui_menu menuvec;
  C3D_RenderTarget * drawscreen;
  char warnmsg[MAX_WARNMSG];
} MainSystem;

int mainsystem_init(MainSystem * ms) {
  int err = datacontainer_init(&ms->drawdata, MAX_DRAW_DATA);
  if(err) { return err; }
  err = vector_tui_menu_init(&ms->menuvec);
  if(err) { return err; }
  ms->drawscreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
  if(!ms->drawscreen) { return 1; }
  ms->warnmsg[0] = 0;
  return 0;
}

// Returns the active menu and alert message
void mainsystem_getactivemenu(MainSystem * ms, tui_menu ** menu, char ** msg) {
  if(ms->warnmsg[0] != 0) {
    *menu = ms->menuvec.array + 1;
    *msg = ms->warnmsg;
  } else {
    *menu = ms->menuvec.array;
    *msg = NULL;
  }
}

void mainsystem_free(MainSystem * ms) {
  datacontainer_free(&ms->drawdata);
  for(size_t i = 0; i < ms->menuvec.length; i++) {
    tui_menu_free(ms->menuvec.array + i);
  }
  vector_tui_menu_free(&ms->menuvec);
  C3D_RenderTargetDelete(ms->drawscreen);
}

// ==========================================
//                 Menu
// ==========================================

#define _MENUINIT(mc, mp) { \
  size_t idx; \
  vector_tui_menu_increment(mc, &idx); \
  mp = (mc)->array + idx; \
  tui_menu_init(mp, UI_CONSOLE_MENUHEIGHT - 1); \
}

// Setup the main menu within the given vector. The main menu itself will be the first
// menu within the container. The "confirm" menu is #2
int main_menu_init(MainSystem * ms) {
  // Reserve space for all the submenus. Make sure you always reserve
  // more than enough space so the pointers don't change.
  int err = vector_tui_menu_reserve(&ms->menuvec, 16);
  if(err) {
    LOGERR("Can't allocate space for menu!");
    return err;
  }
  tui_menu * menu;
  tui_menu * confirmmenu;
  tui_menu * editmenu;
  tui_menu * exportmenu;
  tui_menu * optionsmenu;
  tui_menu * sessionmenu;
  tui_menu * canvasmenu;
  // --- MAIN menu ---
  _MENUINIT(&ms->menuvec, menu);
  _MENUINIT(&ms->menuvec, confirmmenu);
  _MENUINIT(&ms->menuvec, editmenu);
  _MENUINIT(&ms->menuvec, exportmenu);
  _MENUINIT(&ms->menuvec, optionsmenu);
  _MENUINIT(&ms->menuvec, sessionmenu);
  _MENUINIT(&ms->menuvec, canvasmenu);
  TUIMITEM_SUBMENU_EXISTING(menu, err, "Edit", editmenu);
  if(err) { return err; }
  TUIMITEM_SUBMENU_EXISTING(menu, err, "Export", exportmenu);
  if(err) { return err; }
  TUIMITEM_SUBMENU_EXISTING(menu, err, "Options", optionsmenu);
  if(err) { return err; }
  TUIMITEM_SUBMENU_EXISTING(menu, err, "Session Options", sessionmenu);
  if(err) { return err; }
  TUIMITEM_SUBMENU_EXISTING(menu, err, "Canvas Options", canvasmenu);
  if(err) { return err; }
  TUIMITEM_BASIC(menu, err, "Exit App", 0);
  if(err) { return err; }
  // --- CONFIRM menu ---
  TUIMITEM_BASIC(confirmmenu, err, "No", 1);
  if(err) { return err; }
  TUIMITEM_SUBMENU_EXISTING(confirmmenu, err, "Yes", 0);
  if(err) { return err; }
  // --- EDIT menu ---
  _MENUINIT(&ms->menuvec, editmenu);

  return 0;
}

// ==========================================
//               Rendering
// ==========================================

void ui_render_controls() {
  ANSI_GOTO(UI_CONSOLE_CONTROLTOP, 1);
  printf(UI_CONSOLE_CONTROLSCOLOR);
  printf("     L - color picker        R - general modifier ");
  printf("LFT/RT - line width     UP/DWN - zoom (+R - page) ");
  printf("SELECT - change layers   START - menu             ");
  printf("  ABXY - change tools    C-PAD - scroll canvas    ");
  printf(" R+B/A - undo/redo    COLP+L+R - change palette   ");
}

void ui_render_logbox(tui_logbox * lb) {
  ANSI_GOTO(UI_CONSOLE_LOGTOP, 1);
  printf(UI_CONSOLE_LOGCOLOR);
  char out[51]; // Just wide enough for the screen + null
  for(int i = 0; i < UI_CONSOLE_LOGHEIGHT; i++) {
    tui_logbox_renderline(lb, out, 50, UI_CONSOLE_LOGHEIGHT, i);
    // For efficiency: we know these lines all fill the entire width, so no 
    // need to change cursor position or newline or anything
    printf("%s", out);
  }
}

void ui_render_menu(tui_menu * menu, char * alert, int menu_open) {
  ANSI_GOTO(UI_CONSOLE_MENUTOP, 1);
  if(menu_open) {
    char out[51];
    tui_menu_renderpath(menu, VERSIONSTRING, out, 50);
    printf(UI_CONSOLE_MENUBARCOLOR "%s", out);
    for(int i = 0; i < UI_CONSOLE_MENUHEIGHT - 1; i++) {
      tui_menu_renderline(menu, out, 50, i);
      if(tui_menu_iscurrent(menu, i)) {
        printf(UI_CONSOLE_MENUSELECTCOLOR "%s", out);
      } else {
        printf(UI_CONSOLE_MENUCOLOR "%s", out);
      }
    }
  } else {
    printf(UI_CONSOLE_MENUCOLOR);
    for(int i = 0; i < UI_CONSOLE_MENUHEIGHT; i++) {
      printf("%50s", " ");
    }
    ANSI_GOTO(UI_CONSOLE_MENUTOP, 1);
    printf("%s", VERSIONSTRING);
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
  if(logging_init()) return 1;

  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  //PrintConsole * console_ptr = 
  consoleInit(GFX_TOP, NULL);

  if(isn3ds()) {
    LOGDBG("New 3ds detected");
    osSetSpeedupEnable(true);
  } 

  char save_filename[MAX_FILENAME];
  control_config ctrlconfig = { .tool = 0, .scheme = 0, };
  MainSystem system;
  int mode = MAIN_MODE_DRAW;

  if(mainsystem_init(&system)) {
    LOGDBG("CAN'T INITIALIZE MAIN SYSTEM");
    mode = MAIN_MODE_FAILURE;
  }

  if(main_menu_init(&system)) {
    LOGDBG("CAN'T INITIALIZE MAIN MENU");
    mode = MAIN_MODE_FAILURE;
  }

  ui_render_controls();
  ui_render_menu(system.menuvec.array, NULL, 0);

  LOGDBG("STARTING MAIN LOOP");

  while (aptMainLoop()) {
    control_inputs inputs = control_get_inputs();
    control_action actions = control_get_action(&ctrlconfig, &inputs);
    tui_menu * act_menu;
    char * act_status;
    mainsystem_getactivemenu(&system, &act_menu, &act_status);

    switch(mode) {
      case MAIN_MODE_DRAW:;
        if(actions.action == CTRL_MENU) {
          LOGTRC("OPEN MENU");
          mode = MAIN_MODE_MENU;
        }
        break;
      case MAIN_MODE_MENU:;
        // Only run the main menu, unless we stop running
        tui_menu_result mres = tui_menu_run(act_menu, actions.menuaction);
        if(mres.error) {
          LOGERR("Menu error?");
        }
        if(actions.action == CTRL_MENU || !mres.running) {
          LOGTRC("CLOSE MENU");
          mode = MAIN_MODE_DRAW;
        }
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

    C2D_TargetClear(system.drawscreen, SCREEN_COLOR);
    C2D_SceneBegin(system.drawscreen);

    // ---- CONSOLE ----
    if(actions.menuaction.action) {
      ui_render_menu(act_menu, act_status, mode == MAIN_MODE_MENU);
    }
    logging_try_render(ui_render_logbox, 0);

    // draw_layers(&layer_window, &sys);
    // draw_scrollbars(&sys.screen_state);
    // draw_colorpicker(&sys.colors, !sstate.palette_active);

    C3D_FrameEnd(0);

  }
ENDMAINLOOP:;

  mainsystem_free(&system);

  C2D_Fini();
  C3D_Fini();
  // exitRomfs();
  gfxExit();

  logging_free();
  return 0;
}
