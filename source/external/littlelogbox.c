#include "littlelogbox.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int tui_logbox_init(tui_logbox * lb, tui_logbox_unit_t maxmessages, 
    tui_logbox_unit_t msglength) {
  lb->ui_empty = TUILOGBOX_UI_EMPTY;
  lb->head = 0;
  lb->msglength = msglength;
  lb->maxmessages = maxmessages;
#ifdef TUILOGBOX_INCLUDESECONDS
  lb->include_seconds = 1;
#else
  lb->include_seconds = 0;
#endif
  // This pre-emptively sets all log messages to empty string
  lb->log = calloc(msglength * maxmessages, sizeof(char));
  if(lb->log == NULL) {
    return 1;
  }
  return lb->log == NULL;
}

void tui_logbox_free(tui_logbox * lb) {
  if(lb->log) {
    free(lb->log);
    lb->log = NULL;
  }
}

char * tui_logbox_msg(tui_logbox * lb, tui_logbox_unit_t idx) {
  return lb->log + ((((idx % lb->maxmessages) + lb->maxmessages) % 
        lb->maxmessages) * lb->msglength);
}

void tui_logbox_log(tui_logbox * lb, const char * prefix, const char * fmt, ...) {
  va_list args;
  va_start(args, fmt);
  tui_logbox_logargs(lb, prefix, fmt, args);
  va_end(args);
}

void tui_logbox_logargs(tui_logbox * lb, const char * prefix, 
    const char * fmt, va_list args) {
  char * next = tui_logbox_msg(lb, lb->head);
  time_t rawtime = time(NULL);
  struct tm *timeinfo = localtime(&rawtime);
  if(lb->include_seconds) {
    snprintf(next, (lb)->msglength, "%s[%02d:%02d:%02d] ", prefix, 
        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  } else {
    snprintf(next, (lb)->msglength, "%s[%02d:%02d] ", prefix, 
        timeinfo->tm_hour, timeinfo->tm_min);
  }
  tui_logbox_unit_t len = strlen(next);
  if(len >= (lb)->msglength) return;
  vsnprintf(next + len, (lb)->msglength - len, fmt, args);
  (lb)->head = ((lb)->head + 1) % (lb)->maxmessages;
}

void tui_logbox_renderline(tui_logbox * lb, char * out, 
    tui_logbox_unit_t width, tui_logbox_unit_t height, tui_logbox_unit_t line) {
  if(line >= height) {
    out[0] = 0;
    return;
  }
  // Pre-emptively fill with empty.
  memset(out, lb->ui_empty, width);
  out[width] = 0;
  // For now, current is always at bottom, and all log messages are cut off. This
  // vastly simplifies rendering
  //tui_logbox_unit_t offset = height - line - 1;
  char * msg = tui_logbox_msg(lb, lb->head - height + line);
  snprintf(out, lb->msglength, "%s", msg);
  tui_logbox_unit_t len = strlen(out);
  if(len < width) out[len] = lb->ui_empty;
  out[width] = 0;
}
