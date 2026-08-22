#include "littlemenu.h"

void tui_menu_init(tui_menu * tm, tui_menu_unit_t height) {
  tm->items = NULL;
  tm->maxitems = 0;
  tm->numitems = 0;
  tm->height = height;
  tm->valuewidth = TUIMENU_VALUEWIDTH;
  tm->name_padleft = TUIMENU_NAMEPADLEFT;
  tm->name_padright = TUIMENU_NAMEPADRIGHT;
  tm->ui_start[0] = 0;
  tm->ui_empty = TUIMENU_UI_EMPTY;
  tm->loop = TUIMENU_LOOP;
  tm->current = 0;
  tm->top = 0;
  strcpy(tm->ui_left, TUIMENU_UI_LEFT);
  strcpy(tm->ui_right, TUIMENU_UI_RIGHT);
  strcpy(tm->ui_listedge, TUIMENU_UI_LISTEDGE);
  strcpy(tm->ui_nametruncate, TUIMENU_UI_NAMETRUNCATE);
  tm->parent = NULL;
}

// Get a currently open submenu. Submenus are open if the menu item in question
// HAS a menu assigned to it. You could technically have multiple submenus open
// at once, but only the one "currently" selected will count. This may let you do
// interesting things...
static inline tui_menu_item * tui_menu_get_submenu(tui_menu* tm, tui_menu_unit_t pos) {
  if(pos < 0 || pos >= tm->numitems) {
    return NULL;
  }
  tui_menu_item * item = tm->items + pos;
  if(item->type != TUIMENU_TYPE_SUBMENU || !item->data.submenu.menu) {
    return NULL;
  } 
  return item;
}

static inline int tui_menu_exit_submenu_all(tui_menu * tm);

// Exit the child submenu, with FULL CLEANUP for the entire chain underneath. All menus
// inside child have every submenu closed and cleaned up.
static inline int tui_menu_exit_submenu(tui_menu * tm, tui_menu_unit_t pos) {
  tui_menu_item * tmi = tui_menu_get_submenu(tm, pos);
  if(!tmi) return 0; // no submenu is ok, don't error
  // Exit all children from bottom up
  int err = tui_menu_exit_submenu_all(tmi->data.submenu.menu);
  if(err) return err;
  // Now, we can cleanup our own submenu. Note we do not modify the parent pointer in
  // the child; this isn't necessary as we're "disconnecting" the menu anyway, and we don't
  // know how the user is going to cleanup that menu
  err = tmi->data.submenu.destroy_menu(&tmi->data.submenu.data, tmi->data.submenu.menu, pos);
  if(err) return err;
  tmi->data.submenu.menu = NULL;
  return 0;
}

// Exit ALL submenus. We can safely call this on all items even if they are not submenus
static inline int tui_menu_exit_submenu_all(tui_menu * tm) {
  for(tui_menu_unit_t i = 0; i < tm->numitems; i++) {
    int err = tui_menu_exit_submenu(tm, i);
    if(err) return err;
  }
  return 0;
}

// Begin a submenu using the given item. If we're already in a submenu, we
// exit first.
static inline int tui_menu_enter_submenu(tui_menu * tm, tui_menu_unit_t pos) {
  int err = 0;
  tui_menu_item * tmi = tui_menu_get_submenu(tm, pos);
  if(tmi) { // already open
    err = tui_menu_exit_submenu(tm, pos);
    if(err) return err;
  }
  // Not a valid location to enter submenu
  if(pos < 0 || pos >= tm->numitems || tm->items[pos].type != TUIMENU_TYPE_SUBMENU) {
    return -1;
  }
  tmi = tm->items + pos;
  tmi->data.submenu.menu = tmi->data.submenu.create_menu(&tmi->data.submenu.data, tm, pos);
  if(tmi->data.submenu.menu == NULL) {
    return 1;
  }
  tmi->data.submenu.menu->parent = tm;
  // Some courtesy housekeeping so you don't have to
  if(TUIMENU_SUBMENU_FORCEHEIGHT) tmi->data.submenu.menu->height = tm->height;
  if(TUIMENU_SUBMENU_RESET) tui_menu_reset_ui(tmi->data.submenu.menu);
  return 0;
}

void tui_menu_renderpath(tui_menu* tm, const char * prefix, char * out, tui_menu_unit_t width) {
  snprintf(out, width + 1, "%s", prefix);
  tui_menu_item * next = tui_menu_get_submenu(tm, tm->current);
  tui_menu_unit_t len = strlen(out);
  while(len < width && next) {
    snprintf(out + len, width + 1 - len, TUIMENU_UI_SUBMENU_PATH "%s", next->name);
    next = tui_menu_get_submenu(next->data.submenu.menu, next->data.submenu.menu->current);
    len += strlen(out + len);
  }
  if(len < width) {
    snprintf(out + len, width + 1 - len, "%*s", width - len, "");
  }
}

void tui_menu_reset_ui(tui_menu * tm) {
  tui_menu_item * submenu = tui_menu_get_submenu(tm, tm->current);
  if(submenu) { return tui_menu_reset_ui(submenu->data.submenu.menu); }
  tm->top = 0;
  tm->current = 0;
}

int tui_menu_clear(tui_menu * tm) {
  int err = tui_menu_exit_submenu_all(tm);
  if(err) return err;
  tm->numitems = 0;
  tui_menu_reset_ui(tm);
  return 0;
}

void tui_menu_free(tui_menu * tm) {
  if(!tm) return;
  // Have to just ignore error?
  tui_menu_exit_submenu_all(tm);
  if(tm->items) {
    free(tm->items);
    tm->items = NULL;
  }
  tm->numitems = 0;
  tm->maxitems = 0;
}

int tui_menu_push(tui_menu * tm, tui_menu_item * item) {
  if(!tm->items) {
    tm->maxitems = TUIMENU_ITEMGROW;
    tm->items = malloc(sizeof(tui_menu_item) * tm->maxitems);
    if(!tm->items) {
      return 1;
    }
  } else if(tm->numitems >= tm->maxitems) {
    tm->maxitems += TUIMENU_ITEMGROW;
    tui_menu_item * tmp = realloc(tm->items, sizeof(tui_menu_item) * tm->maxitems);
    if(!tmp) {
      return 1;
    }
    tm->items = tmp;
  }
  tm->items[tm->numitems++] = *item;
  return 0;
}

int tui_menu_iscurrent(tui_menu * tm, tui_menu_unit_t line) {
  tui_menu_item * submenu = tui_menu_get_submenu(tm, tm->current);
  if(submenu) { return tui_menu_iscurrent(submenu->data.submenu.menu, line); }
  return tm->top + line == tm->current;
}

void tui_menu_renderline(tui_menu * tm, char * out, tui_menu_unit_t width, tui_menu_unit_t line) {
  tui_menu_item * submenu = tui_menu_get_submenu(tm, tm->current);
  if(submenu) { return tui_menu_renderline(submenu->data.submenu.menu, out, width, line); }
  if(line >= tm->height) {
    out[0] = 0;
    return;
  }
  // Pre-emptively fill with empty.
  memset(out, tm->ui_empty, width);
  out[width] = 0;
  // Only consider the listedge if we're asked to render the edge and there is one
  if(strlen(tm->ui_listedge) && tm->height < tm->numitems) {
    if((line == 0 && tm->top > 0) || (line == tm->height - 1 && tm->top + tm->height < tm->numitems)) {
      strcpy(out, tm->ui_listedge);
      out[strlen(out)] = tm->ui_empty;
      return;
    }
  }
  tui_menu_unit_t item_idx = tm->top + line;
  if(item_idx >= tm->numitems) {
    return;
  }
  tui_menu_item * item = tm->items + item_idx;
  tui_menu_unit_t namespace = width - tm->valuewidth - tm->name_padleft - tm->name_padright;
  // Only draw the text if there's enough space to do so.
  if(namespace > 0) {
    int namelen = strlen(item->name);
    char * namepos = out + tm->name_padleft;
    int nametrunclen = namespace - (int)strlen(tm->ui_nametruncate);
    if(namelen > namespace && nametrunclen > 0) {
      snprintf(namepos, namespace + 1, "%.*s%s", nametrunclen, item->name, tm->ui_nametruncate);
    } else {
      snprintf(namepos, namespace + 1, "%s", item->name);
    }
    // TODO: Slowwww but ugh... will make it faster later
    namepos[strlen(namepos)] = tm->ui_empty;
  }
  if(tm->valuewidth > 0) {
    char * valuepos = out + (width - tm->valuewidth);
    // TODO: For now, we don't actually use the left/right
    if(item->type == TUIMENU_TYPE_NUMBER) {
      snprintf(valuepos, tm->valuewidth + 1, "%d", *item->data.number.value);
    }
    else if(item->type == TUIMENU_TYPE_FLOAT) {
      snprintf(valuepos, tm->valuewidth + 1, "%.*f", item->data.floatingpoint.precision,
               *item->data.floatingpoint.value);
    }
    else if(item->type == TUIMENU_TYPE_ENUM) {
      // Need to find the string inside... slow and may expectedly fail (thus leaving empty)
      int pos = 0;
      char * next = item->data.enumerator.values;
      while(next[0] && (next - item->data.enumerator.values) < TUIMENU_MAXENUMTOTAL) {
        if(pos == *item->data.enumerator.value) {
          snprintf(valuepos, tm->valuewidth + 1, "%s", next);
          break;
        }
        next += (strlen(next) + 1);
        pos++;
      }
    }
    else if (item->type == TUIMENU_TYPE_SUBMENU) {
      snprintf(valuepos, tm->valuewidth + 1, "%*s", tm->valuewidth, TUIMENU_UI_SUBMENU);
    }
    int valuelen = strlen(valuepos);
    if(valuelen < tm->valuewidth) {
      valuepos[valuelen] = tm->ui_empty;
    }
  }
  out[width] = 0;
}

tui_menu_result tui_menu_run(tui_menu * tm, tui_menu_action action) {
  tui_menu_result result;
  result.running = 1;
  result.result = -1;
  result.error = 0;
  // Redirect calls to the submenu BUT if it's a fullstop, we can exit NOW (ignore other commands)
  if(action.action & TUIMENU_ACTION_FULLSTOP) {
    // SHOULD recursively exit all submenus
    result.error = tui_menu_exit_submenu(tm, tm->current);
    result.running = 0;
    return result;
  }
  tui_menu_item * submenu = tui_menu_get_submenu(tm, tm->current);
  if(submenu) { return tui_menu_run(submenu->data.submenu.menu, action); }
  if(action.action & (TUIMENU_ACTION_MOVE | TUIMENU_ACTION_POSITION)) {
    if(action.action & TUIMENU_ACTION_POSITION) {
      tm->current = action.offset;
      action.offset = 0;
    } 
    // Whether forcing a position or moving with an offset, still want to loop (safety)
    TUIMENU_LOOP_VALUE(tm->current, action.offset, 0, tm->numitems - 1, tm->loop);
    uint8_t has_listedge = strlen(tm->ui_listedge) > 0;
    if(tm->height < tm->numitems) { // Only worry about "top" if there's too many items
      tui_menu_unit_t topmin = 0; // inclusive
      tui_menu_unit_t topmax = tm->numitems - tm->height; // inclusive
      if(tm->top >= tm->current) { // If cursor past top
        tm->top = tm->current;
        // current is not allowed to be AT top if there's a list edge
        if(has_listedge) { tm->top--; }
        if(tm->top < topmin) tm->top = topmin;
      }
      if(tm->top <= tm->current - (tm->height - 1)) { // if cursor past bottom row
        tm->top = tm->current - (tm->height - 1);
        // Current is not allowed to be AT the bottom if there's a list edge
        if(has_listedge) { tm->top++; }
        if(tm->top > topmax) tm->top = topmax;
      }
    } else {
      tm->top = 0; // Just reset always, might fix some weirdness if menu resized
    }
  }
  tui_menu_item * item = NULL;
  if(tm->current < tm->numitems && tm->current >= 0) {
    item = tm->items + tm->current;
  }
  if((action.action & TUIMENU_ACTION_VALUE) && item) {
    if(item->type == TUIMENU_TYPE_ENUM) {
      TUIMENU_LOOP_VALUE(*item->data.enumerator.value, action.offset, 0, 
                     item->data.enumerator.numitems - 1, item->loop);
    } else if(item->type == TUIMENU_TYPE_FLOAT) {
      TUIMENU_LOOP_VALUE(*item->data.floatingpoint.value, 
                     action.offset * item->data.floatingpoint.step, 
                     item->data.floatingpoint.min, 
                     item->data.floatingpoint.max, 
                     item->loop);
    } else if(item->type == TUIMENU_TYPE_NUMBER) {
      TUIMENU_LOOP_VALUE(*item->data.number.value, 
                     action.offset * item->data.number.step, 
                     item->data.number.min, 
                     item->data.number.max, 
                     item->loop);
    }
  }
  // We're allowed to modify the action because it's pass by value. Selecting a quit
  // basic item is the SAME as canceling. 
  if ((action.action & TUIMENU_ACTION_ACCEPT) && item && 
       item->type == TUIMENU_TYPE_BASIC && item->data.basic.quit) {
    action.action |= TUIMENU_ACTION_CANCEL;
  }
  if(action.action & TUIMENU_ACTION_CANCEL) {
    // If we have a parent, exit THEIR submenu (it's us). otherwise, exit
    // only our own submenus. We're still running if we have a parent
    if(tm->parent) {
      result.error = tui_menu_exit_submenu(tm->parent, tm->parent->current);
    } else {
      result.error = tui_menu_exit_submenu(tm, tm->current);
      result.running = 0;
    }
  } else if(action.action & TUIMENU_ACTION_ACCEPT && item) {
    if(item->type == TUIMENU_TYPE_BASIC) {
      result.running = 0;   // TODO: does selecting on a basic item really mean the menu is done? Probably...
      result.result = tm->current;
    } else if(item->type == TUIMENU_TYPE_CALLBACK && item->data.callback.callback) {
      result = item->data.callback.callback(&item->data.callback.data, tm->current, action);
    } else if(item->type == TUIMENU_TYPE_SUBMENU) {
      result.error = tui_menu_enter_submenu(tm, tm->current);
    }
  }
  return result;
}
