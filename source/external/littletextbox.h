#ifndef __HEADER_LITTLETUI_TEXTBOX__
#define __HEADER_LITTLETUI_TEXTBOX__

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#ifndef TUITEXTBOX_UNIT_TYPE
#define TUITEXTBOX_UNIT_TYPE int
#endif
#ifndef TUITEXTBOX_UNIT_MAX
#define TUITEXTBOX_UNIT_MAX INT_MAX
#endif
#ifndef TUITEXTBOX_WRAPDELIM
#define TUITEXTBOX_WRAPDELIM ' '
#endif
#ifndef TUITEXTBOX_WRAPLONG
#define TUITEXTBOX_WRAPLONG 0
#endif

typedef TUITEXTBOX_UNIT_TYPE tui_textbox_unit_t;

typedef struct {
  tui_textbox_unit_t width;
  tui_textbox_unit_t line;
  uint8_t wraplong;
  char delim;
} tui_textwrap_config;

#define TUITEXTWRAP_CONFIG(_width, _line) (tui_textwrap_config){ \
  .width = (tui_textbox_unit_t)_width, \
  .line = (tui_textbox_unit_t)_line, \
  .delim = TUITEXTBOX_WRAPDELIM, \
  .wraplong = TUITEXTBOX_WRAPLONG, \
}

typedef struct {
  char * last_start;
  char * last_next;
  char * str;
  tui_textbox_unit_t last_len;
  tui_textbox_unit_t full_height;
  tui_textwrap_config last_config;
  size_t scan_total;
} tui_textwrap;

void tui_textwrap_init(tui_textwrap * tw, char * str);
void tui_textwrap_reset(tui_textwrap * tw);
int tui_textwrap_renderline(tui_textwrap * tw, char * out, tui_textwrap_config config);
tui_textbox_unit_t tui_textwrap_height(tui_textwrap * tw, tui_textwrap_config config);

#endif
