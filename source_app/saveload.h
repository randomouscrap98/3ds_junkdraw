#ifndef __HEADER_JD_SAVELOAD__
#define __HEADER_JD_SAVELOAD__

#include "littlemenu.h"

// Create the load submenu. You can put this anywhere in any submenu, so
// long as the alert_copy has ALL the fields set to send alerts to
#define JDSL_LOAD_SUBMENU(tm, err, _name, _alert_copy) { \
  tui_menu_item_data * _subdat; \
  TUIMITEM_SUBMENU(tm, err, _name, saveload_create_load_submenu, \
      tui_menu_submenu_destroy_malloc_menu, _subdat, 0); \
  if(!err) { memcpy(_subdat->raw, (_alert_copy), sizeof(tui_menu_alert)); } \
}

// Direct load submenu creation (should you want to use it). It requires 
// a lot of weird setup though, try not to use this directly.
tui_menu * saveload_create_load_submenu(
    tui_menu_item_data * data, tui_menu * parent, tui_menu_unit_t pos);

#endif
