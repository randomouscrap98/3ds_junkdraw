#ifndef __HEADER_3DSJUNKDRAW_TABMENU__
#define __HEADER_3DSJUNKDRAW_TABMENU__

#include "littlemenu.h"
#include "vector.h"

#define JDTM_MAXNAME 32
// Use the last two bits for tableft/tabright. This MAY become an issue if the tuimenu
// gets tons of actions in the future
#define JDTM_ACTION_TABLEFT (1 << 14)
#define JDTM_ACTION_TABRIGHT (1 << 15)

typedef int tabmenu_unit_t;

typedef struct {
  char name[JDTM_MAXNAME];
  tui_menu menu;
} tabmenu_item;

VECTOR_DECLARE(tabmenu_item);

// A wrapper around multiple menus together in a tabbed interface.
typedef struct {
  vector_tabmenu_item tabs;       // The top tabs
  vector_tabmenu_item menustack;  // The menus stacked within CURRENT tab
  tabmenu_unit_t current;   // Set to -1 to "disable" the menu
  tabmenu_unit_t height;
} tabmenu;

int tabmenu_init(tabmenu * tm, tabmenu_unit_t height);
void tabmenu_free(tabmenu * tm);

// Grow the menu by one slot, adding the given name, and returning an 
// initialized pointer to the menu for you to fill out as you please.
// This is in contrast to the menu system where you push the data directly,
// as the tui_menu items need no initializer, but the entire tui_menu does.
// int tabmenu_grow(tabmenu * tm, const char * name, tui_menu * menu);

// Tabmenus are just a collection of menus. As such, actions are just tui_menu_actions.
// HOWEVER, we do intercept a couple "special" action values to allow moving 
// through the tabs
tui_menu_result tabmenu_run(tabmenu * tm, tui_menu_action action);

#endif
