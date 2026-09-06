#include "tabmenu.h"
#include "littlemenu.h"
#include "utils.h"

#include <string.h>

VECTOR_DEFINE(tabmenu_item);

int tabmenu_init(tabmenu * tm, tabmenu_unit_t height) {
  int err = vector_tabmenu_item_init(&tm->tabs);
  if(err) goto ERROR_RETURN;
  err = vector_tabmenu_item_init(&tm->submenus);
  if(err) goto ERROR_TABS;
  tm->current = -1;
  tm->height = height;
  strcpy(tm->basicstyle, JDTM_BASICSTYLE_DEFAULT);
  strcpy(tm->tabstyle, JDTM_TABSTYLE_DEFAULT);
  strcpy(tm->selectstyle, JDTM_SELECTSTYLE_DEFAULT);
  return 0;
ERROR_TABS:
  vector_tabmenu_item_free(&tm->tabs);
ERROR_RETURN:
  return 1;
}

// delete ALL menus (tabs and submenus)
static inline void tabmenu_deletemenus(tabmenu * tm) {
  for(size_t i = 0; i < tm->tabs.length; i++) {
    tui_menu_free(&tm->tabs.array[i].menu);
  }
  vector_tabmenu_item_clear(&tm->tabs);
  for(size_t i = 0; i < tm->submenus.length; i++) {
    tui_menu_free(&tm->submenus.array[i].menu);
  }
  vector_tabmenu_item_clear(&tm->submenus);
}

static inline tabmenu_unit_t tabmenu_current(tabmenu * tm) {
  return ((tm->current % tm->tabs.length) + tm->tabs.length) % tm->tabs.length;
}

// Retrieve the menu pointed to by "current". Current is modulus into range
static inline tui_menu * tabmenu_currentmenu(tabmenu * tm) {
  return &tm->tabs.array[tabmenu_current(tm)].menu;
}

void tabmenu_free(tabmenu * tm) {
  tabmenu_deletemenus(tm);
  vector_tabmenu_item_free(&tm->tabs);
  vector_tabmenu_item_free(&tm->submenus);
}

void tabmenu_settab(tabmenu * tm, tabmenu_unit_t tab) {
  // Reset the current menu, then move to the new menu
  // (also reset that one)
  int err = tui_menu_reset(tabmenu_currentmenu(tm));
  if(err) { 
    LOGERR("Can't reset last menu in settab");
    return;
  }
  tm->current = tab;
  err = tui_menu_reset(tabmenu_currentmenu(tm));
  if(err) { 
    LOGERR("Can't reset new menu in settab");
    return;
  }
}

#define JDTM_MGROW(tm, vecname, name) { \
  size_t pos; \
  int err = vector_tabmenu_item_increment(&tm->vecname, &pos); \
  if(err) { LOGERR("No memory to create menu"); return NULL; } \
  snprintf(tm->vecname.array[pos].name, JDTM_MAXNAME, "%s", name); \
  tui_menu * result = &tm->vecname.array[pos].menu; \
  tui_menu_init(result, tm->height - 1); \
  return result; \
}

tui_menu * tabmenu_grow(tabmenu * tm, const char * name) {
  JDTM_MGROW(tm, tabs, name);
}

tui_menu * tabmenu_grow_submenu(tabmenu * tm, const char * name) {
  JDTM_MGROW(tm, submenus, name);
}

// tui_menu_result tabmenu_run(tabmenu * tm, tui_menu_action action) {
//   if(tm->current < 0 || tm->current >= tm->num_tabs) {
//     return (tui_menu_result) {
//       .result = -1,
//       .running = 0
//     };
//   }
//   tabmenu_unit_t offset = 0;
//   if(action.action & JDTM_ACTION_TABLEFT) { offset = -1; }
//   if(action.action & JDTM_ACTION_TABRIGHT) { offset = 1; }
//   // Reset the old menu so it can be fresh next time it is selected
//   if(offset != 0) {
//     tui_menu_reset(&tm->tabs[tm->current].menu);
//   }
//   TUIMENU_LOOP_VALUE(tm->current, offset, 0, tm->num_tabs - 1, TUIMENU_LOOP);
//   return tui_menu_run(&tm->tabs[tm->current].menu, action);
// }
