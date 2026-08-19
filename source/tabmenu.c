#include "tabmenu.h"
#include "littlemenu.h"

VECTOR_DEFINE(tabmenu_item);

int tabmenu_init(tabmenu * tm, tabmenu_unit_t height) {
  int err = vector_tabmenu_item_init(&tm->tabs);
  if(err) goto ERROR_RETURN;
  err = vector_tabmenu_item_init(&tm->menustack);
  if(err) goto ERROR_TABS;
  tm->current = -1;
  tm->height = height;
  return 0;
ERROR_TABS:
  vector_tabmenu_item_free(&tm->tabs);
ERROR_RETURN:
  return 1;
}

static inline void tabmenu_clearstack(tabmenu * tm) {
  for(size_t i = 0; i < tm->menustack.length; i++) {
    tui_menu_free(&tm->menustack.array[i].menu);
  }
  vector_tabmenu_item_clear(&tm->menustack);
}

static inline void tabmenu_cleartabs(tabmenu * tm) {
  for(size_t i = 0; i < tm->tabs.length; i++) {
    tui_menu_free(&tm->tabs.array[i].menu);
  }
  vector_tabmenu_item_clear(&tm->tabs);
}

void tabmenu_free(tabmenu * tm) {
  tabmenu_cleartabs(tm);
  tabmenu_clearstack(tm);
  vector_tabmenu_item_free(&tm->tabs);
  vector_tabmenu_item_free(&tm->menustack);
}

// Set the current tab. Will RESET current tab even if you select the same one
void tabmenu_settab(tabmenu * tm, tabmenu_unit_t tab) {
  tabmenu_clearstack(tm); // clear out cruft from other tabs
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
