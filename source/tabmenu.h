#ifndef __HEADER_3DSJUNKDRAW_TABMENU__
#define __HEADER_3DSJUNKDRAW_TABMENU__

#include "littlemenu.h"
#include "vector.h"
#include "ansi.h"

#define JDTM_MAXNAME 32
#define JDTM_MAXSTYLE 32 // WAY too high but whatever
// Use the last two bits for tableft/tabright. This MAY become an issue if the tuimenu
// gets tons of actions in the future
#define JDTM_ACTION_TABLEFT (1 << 14)
#define JDTM_ACTION_TABRIGHT (1 << 15)

#define JDTM_BASICSTYLE_DEFAULT ANSI_RESET
#define JDTM_TABSTYLE_DEFAULT ANSI_INVERT_ON
#define JDTM_SELECTSTYLE_DEFAULT ANSI_FG_CYAN

typedef int tabmenu_unit_t;

typedef struct {
  char name[JDTM_MAXNAME];
  tui_menu menu;
} tabmenu_item;

VECTOR_DECLARE(tabmenu_item);

// A wrapper around multiple menus together in a tabbed interface.
typedef struct {
  vector_tabmenu_item tabs;       // The top tabs + the menus they represent
  tabmenu_unit_t current;         // Set to -1 to "disable" the menu
  tabmenu_unit_t height;          // The height of the TOTAL menus
  char tabstyle[JDTM_MAXSTYLE];
  char selectstyle[JDTM_MAXSTYLE];
  char basicstyle[JDTM_MAXSTYLE];
} tabmenu;

int tabmenu_init(tabmenu * tm, tabmenu_unit_t height);
void tabmenu_free(tabmenu * tm);

// Grow the menu by one slot, adding the given name, and returning an 
// initialized pointer to the menu for you to fill out as you please.
// You can pass NULL for name to initialize a menu WITHOUT a tab
tui_menu * tabmenu_grow(tabmenu * tm, const char * name);

// Tabmenus are just a collection of menus. As such, actions are just tui_menu_actions.
// HOWEVER, we do intercept a couple "special" action values to allow moving 
// through the tabs
tui_menu_result tabmenu_run(tabmenu * tm, tui_menu_action action);

// Render the given line of the ENTIRE tabmenu.
void tabmenu_renderline(tabmenu * tm, char * out, tui_menu_unit_t width, tui_menu_unit_t line);

#endif
