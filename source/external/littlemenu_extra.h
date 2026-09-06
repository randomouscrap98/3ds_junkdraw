#ifndef __HEADER_LITTLETUI_MENU_EXTRA__
#define __HEADER_LITTLETUI_MENU_EXTRA__

#include "littlemenu.h"
#include "littletextbox.h"
#include "linkedlist.h"

#include <stdio.h>

LL_BASICSTRUCT(tui_menu_ll, tui_menu);
LL_PROTOTYPE(tui_menu_ll);

#ifndef TUIMENUX_MAXALERT
#define TUIMENUX_MAXALERT 512
#endif
#ifndef TUIMENUX_YES
#define TUIMENUX_YES "Yes"
#endif
#ifndef TUIMENUX_NO
#define TUIMENUX_NO "No"
#endif

#define TUIMENUX_STATUSLINE     (1 << 0)
#define TUIMENUX_MENULINE       (1 << 1)  // SOME menu line (including alert)
#define TUIMENUX_ALERTLINE      (1 << 2)  // SOME alert line (text OR menu)
#define TUIMENUX_SELECTLINE     (1 << 3)  // SOME select line (normal menu or alert)

// An expanded menu system that allows storage for submenus cleaned up with the
// main menu, and for alerts within the menu rendering system which can OPTIONALLY
// run on behalf of a new "alert" menu item type (which is a wrapper around submenu type).
// Rendering also assumes a status line at the top of the menu with the path.
typedef struct {
  tui_menu menu;      // The actual internal menu
  tui_textwrap alertwrap;
  tui_menu_ll * submenus;
  char alert[TUIMENUX_MAXALERT];
} tui_menu_extra;

void tui_menu_extra_init(tui_menu_extra * tme, tui_menu_unit_t height);
void tui_menu_extra_free(tui_menu_extra * tme);
// Add a new submenu; it will be returned initialized and ready for items to be added
tui_menu * tui_menu_extra_new_submenu(tui_menu_extra * tme);
// Render a line into out, and indicate WHICH kind of line it is. The extra menu automatically
// renders a status line and alert box into the menu region for you
int tui_menu_extra_renderline(tui_menu_extra * tm, const char * prefix, char * out, 
    tui_menu_unit_t width, tui_menu_unit_t line);

// Allows the creation of tui menu items which can have an optional alert,
// which inserts a special alert menu when should_alert returns an alert.
// This is stored in the "raw" field of a menu item.
typedef struct {
  tui_menu_extra * menu;
  void * userdata;
  int (*should_alert)(void * udata, char * alert, size_t alertlen, 
      tui_menu * parent, tui_menu_unit_t pos);
  tui_menu * (*work)(void * udata, tui_menu * parent, tui_menu_unit_t pos);
} tui_menu_alert;

// When user selects yes on the warning menu, run their desired work, which MAY produce
// a menu, or may just "do the menu"
tui_menu * tui_menu_alert_create_yes_menu(
    tui_menu_item_data * data, tui_menu * parent, tui_menu_unit_t pos);

// the "menu create" function for a menu item which can optionally throw up a warning
// before doing its work, or just directly do the work otherwise.
tui_menu * tui_menu_alert_create_menu(
    tui_menu_item_data * data, tui_menu * parent, tui_menu_unit_t pos);

#define TUIMXITEM_ALERT(tme, err, _name, _shouldalert, _work, _userdata) { \
  tui_menu_item_data * _tmpdat; \
  TUIMITEM_SUBMENU(&(tme)->menu, err, _name, tui_menu_alert_create_menu, \
      tui_menu_submenu_destroy_malloc_menu, _tmpdat, 1); \
  if(!err) { \
    tui_menu_alert tma; \
    tma.menu = tme; \
    tma.userdata = _userdata; \
    tma.should_alert = _shouldalert; \
    tma.work = _work; \
    memcpy(_tmpdat->raw, &tma, sizeof(tui_menu_alert)); \
  } \
}

#endif
