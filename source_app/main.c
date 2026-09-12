#ifndef __GLIBC_USE
#define __GLIBC_USE(F) 0
#endif

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include "3ds/console.h"

// source
#include "littlemenu_extra.h"
#include "utils.h"
#include "ansi.h"
#include "datacontainer.h"
#include "layer.h"
#include "layercompositor.h"
#include "layerwindow.h"

// source_app
#include "controls.h"
#include "logging.h"
#include "cpad.h"


u32 __stacksize__ = 512 * 1024;

#define MAX_FILENAME  64
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

void header_res_decode(resolutionid_t resolution, layerdim_t * width, layerdim_t * height) {
  switch(resolution) {
    // Unfortunately, both the 500 and 250 versions have the same limitations
    // because the library needs that 8 pixel buffer around the edge of the texture...
    case 1:
      *width = 500;
      *height = 500;
      break;
    case 2:
      *width = 320;
      *height = 240;
      break;
    // case 3 reserved
    default: // also case 0
      *width = 1000;
      *height = 1000;
      break;
  }
}

// ==========================================
//              Global Data
// ==========================================

typedef struct {
  DataContainer drawdata;
  LayerWindow layerwindow;
  LayerCompositor compositor;
  tui_menu_extra mainmenu;
  CpadProfile cpad;
  // May move into separate systems later
  page_t page;
  layer_t layer;
  onion_t onions;
} MainSystem;

int mainsystem_init(MainSystem * ms) {
  ms->page = 0;   // just for safety
  ms->layer = 0;
  ms->onions = 0;
  ms->cpad = cpadprofile_default();
  int err = datacontainer_init(&ms->drawdata, MAX_DRAW_DATA);
  if(err) { return err; }
  err = layercompositor_init_screen(&ms->compositor, GFX_BOTTOM);
  if(err) { return err; }
  tui_menu_extra_init(&ms->mainmenu, UI_CONSOLE_MENUHEIGHT);
  layerwindow_init(&ms->layerwindow, &ms->drawdata, JDL_TYPE_HARDWARE);
  return 0;
}

int mainsystem_newdrawing(MainSystem * ms) {
  DataHeader dh;
  dataheader_default(&dh);
  layerdim_t width, height;
  header_res_decode(dh.resolution_id, &width, &height);
  int err = layerwindow_reset(&ms->layerwindow, width, height, dh.layer_count, 0);
  if(err) {
    return err;
  }
  datacontainer_reset(&ms->drawdata);
  datacontainer_setheader(&ms->drawdata, &dh);
  LOGDBG("New drawing: %dx%d", width, height);
  ms->page = 0;
  ms->layer = 0;
  // DON'T reset onions!
  return 0;
}

void mainsystem_free(MainSystem * ms) {
  datacontainer_free(&ms->drawdata);
  tui_menu_extra_free(&ms->mainmenu);
  layercompositor_free(&ms->compositor);
  layerwindow_free(&ms->layerwindow);
}

void mainsystem_calc_layerdraw(MainSystem * ms, LayerDraw * ld, size_t * total_layers) {
  DataHeader dh;
  datacontainer_getheader(&ms->drawdata, &dh);
  *total_layers = 0;
  //*total_layers = dh.layer_count * (1 + ms->onions);
  //size_t lidx = 0;
  // TODO: put onion skins here back to front. You can use the same loop and just
  // change NOMOD to the onion skin thing and iterate over the pages using a PageRange,
  // or you can... do something else I guess...
  // The topmost actual layer (layers are back to front)
  for(layer_t i = dh.layer_count - 1; i >= 0; i--) {
    Layer * layer = layerwindow_getlayer(&ms->layerwindow, ms->page, ms->layer);
    ld[*total_layers] = LAYERDRAW_NOMOD(layer);
    (*total_layers)++;
  }
}

// Take input and use it to modify internal offset of compositor
void mainsystem_run_offset(MainSystem * ms, control_inputs * inputs) {
  layercompositor_offset(&ms->compositor,
    layerwindow_getlayer(&ms->layerwindow, ms->page, ms->layer),
    cpadprofile_translate(&ms->cpad, inputs->cpos.dx, ms->compositor.offset_x),
    cpadprofile_translate(&ms->cpad, -inputs->cpos.dy, ms->compositor.offset_y));
}

// ==========================================
//                 Menu
// ==========================================

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
    printf(UI_CONSOLE_MENUBARCOLOR);
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
  LayerDraw layers[JDDC_MAXLAYERS];
  size_t layers_count;

  if(mainsystem_init(&system)) {
    LOGDBG("CAN'T INITIALIZE MAIN SYSTEM");
    mode = MAIN_MODE_FAILURE;
  }

  if(main_menu_init(&system)) {
    LOGDBG("CAN'T INITIALIZE MAIN MENU");
    mode = MAIN_MODE_FAILURE;
  }

  if(mainsystem_newdrawing(&system)) {
    LOGERR("Can't initialize new drawing with given parameters!");
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
    // Other controls?
    // =======================================
    mainsystem_run_offset(&system, &inputs);

    // =======================================
    // Render the scene
    // =======================================
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    // -- LAYER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE,
                   GPU_ZERO);
    C2D_Flush();

    // ---- FINAL COMPOSITE? ----
    // FOR NOW, we can simply grab the layers directly out of the system.
    mainsystem_calc_layerdraw(&system, layers, &layers_count);
    layercompositor_draw(&system.compositor, layers, layers_count);

    // ---- CONSOLE ----
    if(actions.menuaction.action) {
      ui_render_menu(&system.mainmenu, mode == MAIN_MODE_MENU);
    }
    logging_try_render(ui_render_logbox, 0);

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
