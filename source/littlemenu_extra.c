#include "littlemenu_extra.h"
#include "littlemenu.h"
#include "littletextbox.h"

void tui_menu_extra_init(tui_menu_extra * tme, tui_menu_unit_t height) {
  tme->alert[0] = 0;
  tui_textwrap_init(&tme->alertwrap, tme->alert);
  return tui_menu_init(&tme->menu, height - 1);
}

void tui_menu_extra_free(tui_menu_extra * tme) {
  tui_menu_free(&tme->menu);
}

static inline int tui_menu_extra_renderline_menu(tui_menu * menu, char * out, 
    tui_menu_unit_t width, tui_menu_unit_t line) {
  int result = 0;
  tui_menu_renderline(menu, out, width, line);
  result |= TUIMENUX_MENULINE;
  if(tui_menu_iscurrent(menu, line)) { 
    result |= TUIMENUX_SELECTLINE;
  }
  return result;
}

int tui_menu_extra_renderline(tui_menu_extra * tme, const char * prefix, char * out, 
    tui_menu_unit_t width, tui_menu_unit_t line) {
  int result = 0;
  if(line == 0) {
    tui_menu_renderpath(&tme->menu, prefix, out, width);
    result |= TUIMENUX_STATUSLINE;
  } else {
    if(tme->alert[0] == 0) {
      // Normal menu
      result |= tui_menu_extra_renderline_menu(&tme->menu, out, width, line - 1);
    } else {
      result |= TUIMENUX_ALERTLINE;
      // MUST make sure height is calculated IF an alert is there.
      tui_textwrap_config config = TUITEXTWRAP_CONFIG(width, 0); // Line doesn't matter
      tui_textbox_unit_t height = tui_textwrap_height(&tme->alertwrap, config);
      if((line - 1) >= height) {
        // Normal menu
        result |= tui_menu_extra_renderline_menu(&tme->menu, out, width, line - 1 - height);
      } else {
        // THIS is the alert line
        config.line = line - 1;
        tui_textwrap_renderline(&tme->alertwrap, out, config);
      }
    }
  }
  return result;
}

tui_menu_result tui_menu_alert_no_callback(tui_menu_item_data * data, 
    tui_menu_unit_t pos, tui_menu_action action) {
  (void)pos;
  (void)action;
  ((char *)data->ptr)[0] = 0; // this should point to the menu alert
  return TUIMENU_CANCELRESULT;
}

tui_menu * tui_menu_alert_create_yes_submenu(
    tui_menu_item_data * data, tui_menu * parent, tui_menu_unit_t pos) {
  // Unfortunately, we always must memcpy out because of strict pointer aliasing.
  // The alert is meant to be READONLY though so it's fine.
  tui_menu_alert tma;
  memcpy(&tma, data->raw, sizeof(tui_menu_alert));
  // Must exit the alert
  tma.menu->alert[0] = 0;     // No more alert
  // tui_menu_run(&tma.menu->menu, TUIMENU_CANCELACTION);
  return tma.work(tma.userdata, parent, pos);
}


tui_menu * tui_menu_alert_create_menu(
    tui_menu_item_data * data, tui_menu * parent, tui_menu_unit_t pos) {
  // Unfortunately, we always must memcpy out because of strict pointer aliasing,
  // The alert is meant to be READONLY though so it's fine.
  tui_menu_alert tma;
  memcpy(&tma, data->raw, sizeof(tui_menu_alert));
  if(tma.should_alert(tma.userdata, tma.menu->alert, sizeof(tma.menu->alert), parent, pos)) {
    // Setup a temporary confirm menu with a yes that is another submenu item that redirects to
    // the user's work item (or just exits normally)
    tui_menu * alertmenu = (tui_menu *)malloc(sizeof(tui_menu));
    if(!alertmenu) { return NULL; }
    tui_menu_init(alertmenu, parent->height);
    int err;
    tui_menu_item_data nodat = {0};
    nodat.ptr = tma.menu->alert;
    TUIMITEM_CALLBACK(alertmenu, err, TUIMENUX_NO, tui_menu_alert_no_callback, nodat);
    if(err) { 
      free(alertmenu); 
      return NULL; 
    }
    tui_menu_item_data * yesdat;
    // WARN: We're setting "not temporary" on the user's menu FOR them!!
    TUIMITEM_SUBMENU(alertmenu, err, TUIMENUX_YES, tui_menu_alert_create_yes_submenu, 
        tui_menu_submenu_destroy_malloc_menu, yesdat, 0);
    if(err) { 
      free(alertmenu);
      return NULL; 
    }
    memcpy(yesdat->raw, &tma, sizeof(tui_menu_alert));
    return alertmenu;
  } else {
    tma.menu->alert[0] = 0;   // No more menu
    return tma.work(tma.userdata, parent, pos);
  }
}
