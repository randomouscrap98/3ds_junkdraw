#ifndef __HEADER_LITTLETUI_LOGBOX__
#define __HEADER_LITTLETUI_LOGBOX__

// NOTE: this is a very simple implementation! In the future, there
// may be more features, such as log format and scrolling through logs etc!

#include <stdint.h>
#include <stdarg.h>

// #define TUILOGBOX_INCLUDESECONDS

#ifndef TUILOGBOX_UI_EMPTY
#define TUILOGBOX_UI_EMPTY ' ' // Difficult to make this not a single char
#endif

typedef int32_t tui_logbox_unit_t;

// Easy wrapper to generate a log function like LOG(const char * fmt, ...)
#define TUILOGBOX_LOG_INNER(lb, prefix, fmt) { \
  va_list args; \
  va_start(args, fmt); \
  tui_logbox_logargs(lb, prefix, fmt, args); \
  va_end(args); \
}
// Some suggestions
#define TUILOGBOX_ERR_INNER(lb, fmt) TUILOGBOX_LOG_INNER(lb, "ERR ", fmt)
#define TUILOGBOX_WRN_INNER(lb, fmt) TUILOGBOX_LOG_INNER(lb, "WRN ", fmt)
#define TUILOGBOX_INF_INNER(lb, fmt) TUILOGBOX_LOG_INNER(lb, "INF ", fmt)
#define TUILOGBOX_DBG_INNER(lb, fmt) TUILOGBOX_LOG_INNER(lb, "DBG ", fmt)
#define TUILOGBOX_TRC_INNER(lb, fmt) TUILOGBOX_LOG_INNER(lb, "TRC ", fmt)

// Represents a rotating log you can dump messages into and later
// display. Logs roll automatically, and display is automatically
// pegged to the newest message at the bottom (auto-scroll).
typedef struct {
  char * log;
  tui_logbox_unit_t head;
  tui_logbox_unit_t msglength;    // Max length of each message, INCLUDES the null terminator!
  tui_logbox_unit_t maxmessages;  // Amount of messages allowed
  char ui_empty;
  uint8_t include_seconds;
} tui_logbox;

int tui_logbox_init(tui_logbox * lb, tui_logbox_unit_t maxmessages, 
    tui_logbox_unit_t msglength);
void tui_logbox_free(tui_logbox * lb);

// Get a pointer to the log message of given index. Remember log is circula buffer.
// Any value outside range is wrapped (negative is fine)
char * tui_logbox_msg(tui_logbox * lb, tui_logbox_unit_t idx);
void tui_logbox_log(tui_logbox * lb, const char * prefix, const char * fmt, ...);
void tui_logbox_logargs(tui_logbox * lb, const char * prefix, 
    const char * fmt, va_list args);
// NOTE: width is the visible width of the box! out needs to be width + 1 size at least!
void tui_logbox_renderline(tui_logbox * lb, char * out, 
    tui_logbox_unit_t width, tui_logbox_unit_t height, tui_logbox_unit_t line);

#endif
