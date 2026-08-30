#include "logging.h"

#include "utils.h"

static tui_logbox logbox; // Global makes it easier to implement functions below
static tui_logbox_unit_t logbox_lasthead;

#ifdef DO_LOGERROR
void LOGERR(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "E", fmt) }
#else
void LOGERR(const char * fmt, ...) {  }
#endif
#ifdef DO_LOGWARNING
void LOGWRN(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "W", fmt) }
#else
void LOGWRN(const char * fmt, ...) { }
#endif
#ifdef DO_LOGINFO
void LOGINF(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "I", fmt) }
#else
void LOGINF(const char * fmt, ...) {  }
#endif
#ifdef DO_LOGDEBUG
void LOGDBG(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "D", fmt) }
#else
void LOGDBG(const char * fmt, ...) {  }
#endif
#ifdef DO_LOGTRACE
void LOGTRC(const char * fmt, ...) { TUILOGBOX_LOG_INNER(&logbox, "T", fmt) }
#else
void LOGTRC(const char * fmt, ...) {  }
#endif

int logging_init() {
  int result = tui_logbox_init(&logbox, MAX_LOGMESSAGES, MAX_LOGMSGLENGTH);
  logbox_lasthead = logbox.head;
  return result;
}

void logging_try_render(void (*render)(tui_logbox *), int force) {
  if(logbox.head != logbox_lasthead || force) {
    render(&logbox);
    logbox_lasthead = logbox.head;
  }
}

void logging_free() {
  tui_logbox_free(&logbox);
}
