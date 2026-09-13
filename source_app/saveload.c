#include "saveload.h"
#include "littlemenu.h"

#include <dirent.h>

#ifndef __GLIBC_USE
#define __GLIBC_USE(F) 0
#endif

#include "littlemenu_extra.h"
#include "utils.h"

#define JDSL_DIRECTORY        "/3ds/junkdraw/"
#define JDSL_DIRECTORY_SAVES  JDSL_DIRECTORY "saves/"

int saveload_fill_loadmenu(tui_menu * menu, tui_menu_alert * alert_ref, const char * directory) {
  DIR *dir = opendir(directory);

  if (!dir) {
    LOGERR("Couldn't open dir %s\n", directory);
    return -1;
  }

  struct dirent *entry = readdir(dir);

  while (entry != NULL) {
    if (entry->d_type == DT_DIR) {
      int err;
      TUIMXITEM_ALERT_COPY(menu, alert_ref, err, entry->d_name);
      if(err) { return err; }
    }
    entry = readdir(dir);
  }

  return 0;
}


#define _JDSL_ITEMFAILCHECK(err) \
  if(err) { \
    LOGERR("Can't allocate menu item for load!"); \
    tui_menu_free(menu); free(menu); \
    return NULL; \
  }

tui_menu * saveload_create_load_submenu(
    tui_menu_item_data * data, tui_menu * parent, tui_menu_unit_t pos) {
  // Unfortunately, we always must memcpy out because of strict pointer aliasing.
  // The alert is meant to be READONLY though so it's fine.
  tui_menu_alert tma;
  memcpy(&tma, data->raw, sizeof(tui_menu_alert));
  tui_menu * menu = malloc(sizeof(tui_menu));
  if(!menu) {
    LOGERR("Can't allocate menu for load!");
    return NULL;
  }
  tui_menu_init(menu, parent->height);
  int err = saveload_fill_loadmenu(menu, &tma, JDSL_DIRECTORY_SAVES);
  _JDSL_ITEMFAILCHECK(err);
  TUIMITEM_BASIC(menu, err, "Cancel", 1);
  _JDSL_ITEMFAILCHECK(err);
  return menu;
}
