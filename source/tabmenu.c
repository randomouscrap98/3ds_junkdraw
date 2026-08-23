#include "tabmenu.h"
#include "littlemenu.h"
#include "utils.h"

#include <string.h>

VECTOR_DEFINE(tabmenu_item);

int tabmenu_init(tabmenu * tm, tabmenu_unit_t height) {
  int err = vector_tabmenu_item_init(&tm->tabs);
  if(err) goto ERROR_RETURN;
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

static inline void tabmenu_cleartabs(tabmenu * tm) {
  for(size_t i = 0; i < tm->tabs.length; i++) {
    tui_menu_free(&tm->tabs.array[i].menu);
  }
  vector_tabmenu_item_clear(&tm->tabs);
}

static inline tui_menu * tabmenu_currentmenu(tabmenu * tm) {

}

void tabmenu_free(tabmenu * tm) {
  tabmenu_cleartabs(tm);
  vector_tabmenu_item_free(&tm->tabs);
}

void tabmenu_settab(tabmenu * tm, tabmenu_unit_t tab) {
  // Reset the current menu, then move to the new menu
  int err = tui_menu_reset(tm);
  if(err) LOGERR("Can't reset ");
}

// int tabmenu_grow(tabmenu * tm, const char * name, tui_menu * menu) {
//   if(!tm->tabs) {
//     tm->max_tabs = TUIMENU_ITEMGROW; // Reuse this just because
//     tm->tabs = malloc(sizeof(tabmenu_item) * tm->max_tabs);
//     if(!tm->tabs) {
//       return 1;
//     }
//   } else if(tm->num_tabs >= tm->max_tabs) {
//     tm->max_tabs += TUIMENU_ITEMGROW;
//     tabmenu_item * tmp = realloc(tm->tabs, sizeof(tabmenu_item) * tm->max_tabs);
//     if(!tmp) {
//       return 1;
//     }
//     tm->tabs = tmp;
//   }
//   tui_menu_init(&tm->tabs[tm->num_tabs].menu, tm->height - 1);
//   snprintf(tm->tabs[tm->num_tabs].name, JDTM_MAXNAME, "%s", name);
//   tm->num_tabs++;
//   return 0;
// }

tui_menu_result tabmenu_run(tabmenu * tm, tui_menu_action action) {
  if(tm->current < 0 || tm->current >= tm->num_tabs) {
    return (tui_menu_result) {
      .result = -1,
      .running = 0
    };
  }
  tabmenu_unit_t offset = 0;
  if(action.action & JDTM_ACTION_TABLEFT) { offset = -1; }
  if(action.action & JDTM_ACTION_TABRIGHT) { offset = 1; }
  // Reset the old menu so it can be fresh next time it is selected
  if(offset != 0) {
    tui_menu_reset(&tm->tabs[tm->current].menu);
  }
  TUIMENU_LOOP_VALUE(tm->current, offset, 0, tm->num_tabs - 1, TUIMENU_LOOP);
  return tui_menu_run(&tm->tabs[tm->current].menu, action);
}
