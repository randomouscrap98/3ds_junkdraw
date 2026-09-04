#ifndef __GLIBC_USE
#define __GLIBC_USE(F) 0
#endif

#include "3ds/console.h"
#include "littlemenu_extra.h"
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
#define VERSIONSTRING "Junkdraw "VERSION""

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
  tui_menu_extra mainmenu;
  C3D_RenderTarget * drawscreen;
} MainSystem;

int mainsystem_init(MainSystem * ms) {
  int err = datacontainer_init(&ms->drawdata, MAX_DRAW_DATA);
  if(err) { return err; }
  ms->drawscreen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
  if(!ms->drawscreen) { return 1; }
  tui_menu_extra_init(&ms->mainmenu, UI_CONSOLE_MENUHEIGHT);
  return 0;
}

// // Returns the active menu and alert message
// void mainsystem_getactivemenu(MainSystem * ms, tui_menu ** menu, char ** msg) {
//   if(ms->warnmsg[0] != 0) {
//     *menu = ms->menuvec.array + 1;
//     *msg = ms->warnmsg;
//   } else {
//     *menu = ms->menuvec.array;
//     *msg = NULL;
//   }
// }

void mainsystem_free(MainSystem * ms) {
  datacontainer_free(&ms->drawdata);
  tui_menu_extra_free(&ms->mainmenu);
  C3D_RenderTargetDelete(ms->drawscreen);
}

// ==========================================
//                 Menu
// ==========================================

// // There is a menu system where you can setup an optional warning + work to do 
// // (which itself could be a menu). This allows you to create a menu item where 
// // you can throw up a customized warning message before 
// typedef struct {
//   MainSystem * system;
//   char * (*should_warn)(MainSystem * ms);
//   tui_menu * (*work)(MainSystem * ms);
//   char warning[TUIMENU_MAXENUMTOTAL - sizeof(void *) * 4];
// } optional_menu;
// 
// // When user selects yes on the warning menu, run their desired work, which MAY produce
// // a menu, or may just "do the menu"
// static inline tui_menu * optional_menu_warning_yes_create(
//     tui_menu_item_data * data, tui_menu * parent, tui_menu_unit_t pos) {
//   optional_menu * om = (optional_menu*)&data->raw;
//   om->system->warnmsg[0] = 0;     // No more menu
//   return om->work(om->system);
// }
// 
// // the "menu create" function for a menu item which can optionally throw up a warning
// // before doing its work, or just directly do the work otherwise.
// static inline tui_menu * optional_menu_warning_create(
//     tui_menu_item_data * data, tui_menu * parent, tui_menu_unit_t pos) {
//   optional_menu * om = (optional_menu*)&data->raw;
//   char * warning = om->should_warn(om->system);
//   if(warning) {
//     snprintf(om->system->warnmsg, sizeof(om->system->warnmsg), "%s", warning);
//     // Setup a temporary confirm menu with a yes that is another submenu item that redirects to
//     // our 
//     tui_menu * warnmenu = malloc(sizeof(tui_menu));
//     if(!warnmenu) {
//       LOGERR("Couldn't create warning submenu!");
//       return NULL;
//     }
//     int err;
//     TUIMITEM_BASIC(warnmenu, err, "No", 1);
//     if(err) { 
//       LOGERR("Couldn't create warning submenu 'no'!");
//       return NULL; 
//     }
//     tui_menu_item_data * yesdat;
//     TUIMITEM_SUBMENU(warnmenu, err, "Yes", 
//         optional_menu_warning_yes_create, tui_menu_submenu_destroy_malloc_menu, yesdat);
//     if(err) { 
//       LOGERR("Couldn't create warning submenu 'yes'!");
//       return NULL; 
//     }
//     memcpy((optional_menu *)&yesdat->raw, om, sizeof(optional_menu));
//     return warnmenu;
//   } else {
//     om->system->warnmsg[0] = 0;   // No more menu
//     return om->work(om->system);
//   }
// }

#define _SUBMENU_INIT(_ms, _name) \
  tui_menu * _name = tui_menu_extra_new_submenu(&(_ms)->mainmenu); \
  if(_name == NULL) { \
    LOGERR("Can't initialize submenu"); \
    return 1; \
  }

// Setup the main menu within the given vector. The main menu itself will be the first
// menu within the container. The "confirm" menu is #2
int main_menu_init(MainSystem * ms) {
  // Setup all the submenus so they're available for the main menu
  // (at least allocate them)
  _SUBMENU_INIT(ms, editmenu);
  _SUBMENU_INIT(ms, exportmenu);
  _SUBMENU_INIT(ms, optionsmenu);
  _SUBMENU_INIT(ms, sessionmenu);
  _SUBMENU_INIT(ms, canvasmenu);
  // --- MAIN menu ---
  int err;
  TUIMITEM_SUBMENU_EXISTING(&ms->mainmenu.menu, err, "Edit", editmenu);
  if(err) { return err; }
  TUIMITEM_SUBMENU_EXISTING(&ms->mainmenu.menu, err, "Export", exportmenu);
  if(err) { return err; }
  TUIMITEM_SUBMENU_EXISTING(&ms->mainmenu.menu, err, "Options", optionsmenu);
  if(err) { return err; }
  TUIMITEM_SUBMENU_EXISTING(&ms->mainmenu.menu, err, "Session Options", sessionmenu);
  if(err) { return err; }
  TUIMITEM_SUBMENU_EXISTING(&ms->mainmenu.menu, err, "Canvas Options", canvasmenu);
  if(err) { return err; }
  TUIMITEM_BASIC(&ms->mainmenu.menu, err, "Exit App", 0);
  if(err) { return err; }
  // --- EDIT menu ---

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

void ui_render_menu(tui_menu_extra * menu, int menu_open) {
  ANSI_GOTO(UI_CONSOLE_MENUTOP, 1);
  if(menu_open) {
    char out[51];
    printf(UI_CONSOLE_MENUBARCOLOR "%s", out);
    for(int i = 0; i < UI_CONSOLE_MENUHEIGHT; i++) {
      int type = tui_menu_extra_renderline(menu, VERSIONSTRING, out, 48, i);
      if(type & TUIMENUX_STATUSLINE) {
        printf(UI_CONSOLE_MENUBARCOLOR);
      } else if(type == TUIMENUX_ALERTLINE) {
        printf(ANSI_FG_MAGENTA ANSI_INVERT_ON);
      } else if(type & TUIMENUX_SELECTLINE) {
        printf(UI_CONSOLE_MENUSELECTCOLOR);
      } else if(type & TUIMENUX_MENULINE) {
        printf(UI_CONSOLE_MENUCOLOR);
      }
      printf(" %s ", out);
    }
  } else {
    printf(UI_CONSOLE_MENUCOLOR);
    for(int i = 0; i < UI_CONSOLE_MENUHEIGHT; i++) {
      printf("%50s", " ");
    }
    ANSI_GOTO(UI_CONSOLE_MENUTOP, 1);
    printf(" %s ", VERSIONSTRING);
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
  ui_render_menu(&system.mainmenu, 0);

  LOGDBG("STARTING MAIN LOOP");

  while (aptMainLoop()) {
    control_inputs inputs = control_get_inputs();
    control_action actions = control_get_action(&ctrlconfig, &inputs);
    //tui_menu * act_menu;
    //char * act_status;
    //mainsystem_getactivemenu(&system, &act_menu, &act_status);

    switch(mode) {
      case MAIN_MODE_DRAW:;
        if(actions.action == CTRL_MENU) {
          LOGTRC("OPEN MENU");
          mode = MAIN_MODE_MENU;
        }
        break;
      case MAIN_MODE_MENU:;
        // Only run the main menu, unless we stop running
        tui_menu_result mres = tui_menu_run(&system.mainmenu.menu, actions.menuaction);
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
      ui_render_menu(&system.mainmenu, mode == MAIN_MODE_MENU);
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
