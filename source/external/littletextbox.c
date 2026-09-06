#include "littletextbox.h"

#include <string.h>
#include <stdio.h>

void tui_textwrap_init(tui_textwrap * tw, char * str) {
  tw->str = str;
  tw->scan_total = 0;
  tui_textwrap_reset(tw);
}

void tui_textwrap_reset(tui_textwrap * tw) {
  tw->last_start = tw->str;
  tw->last_next = NULL;
  tw->last_len = 0;
  tw->last_config.width = 0;
  tw->last_config.line = 0;
  tw->last_config.delim = 0;
  tw->full_height = 0;
}

static inline void tui_textwrap_reset_withconfig(tui_textwrap * tw, tui_textwrap_config config) {
  tui_textwrap_reset(tw); 
  tw->last_config = config;
  tw->last_config.line = 0;
}

static inline int tui_textwrap_config_should_reset(tui_textwrap_config orig, 
    tui_textwrap_config new) {
  return orig.delim != new.delim || orig.width != new.width ||
    new.line < orig.line;
}

static inline void tui_textwrap_scanchunk(tui_textwrap * tw, char delim) {
  while(*tw->last_next != delim && *tw->last_next != 0) { tw->last_next++; }
  tw->last_len = tw->last_next - tw->last_start;
  while(*tw->last_next == delim && *tw->last_next != 0) { tw->last_next++; }
}

// Wherever we are in the tracking, find len and next. Does NOT advance last_line, only
// finds length and next
static inline int tui_textwrap_findnext(tui_textwrap * tw, tui_textwrap_config config) {
  // quick exit if past end
  if(tw->last_start[0] == 0) return -1;
  // Reset scanner end (just in case)
  tw->last_next = tw->last_start;
  while(1) {
    // Consume the next word + space chunk. Save some stuff before probing forward
    char * prevnext = tw->last_next;
    tui_textbox_unit_t prevlen = tw->last_len;
    tui_textwrap_scanchunk(tw, config.delim);
    // scanned past end during WORD (very specifically PAST). This is always
    // an exit condition: either we thought we could scan another word and
    // couldn't, or we are still on the first word of the line.
    if(tw->last_len > config.width) {
      if(prevnext == tw->last_start) {
        tw->last_len = config.width;
        // Special case means next is at a different position than expected
        if(config.wraplong) {
          tw->last_next = tw->last_start + tw->last_len;
        } 
      } else {
        // Scanned too far, turn around. This should only happen once
        // per line, hence allowing going forward too far
        tw->last_len = prevlen;
        tw->last_next = prevnext;
      }
      break;
    }
    // Early exit: we're at a perfect location (or at the end)
    if(tw->last_next - tw->last_start >= config.width || *tw->last_next == 0) {
      break;
    }
  }
  (tw)->scan_total++;
  return 0;
}

static inline int tui_textwrap_walk(tui_textwrap * tw, tui_textwrap_config config) {
  if(config.width < 1) { return -1; }
  if(tui_textwrap_config_should_reset((tw)->last_config, config)) { 
    tui_textwrap_reset_withconfig(tw, config); 
  }
  /* If SPECIFICALLY reset, pre-emptive scan for next */
  if((tw)->last_next == NULL) { tui_textwrap_findnext(tw, config); }
  /* Move forward through lines, wrapping each one */
  for(; (tw)->last_config.line < config.line; (tw)->last_config.line++) {
    /* If we're trying to scan forward and we're at the end before */
    /* going to next, this is an error. You can't render lines past the end */
    if(*(tw)->last_next == 0) { 
      tw->full_height = tw->last_config.line + 1;
      return 99;  // Special: want to know when this happens specifically
    } 
    (tw)->last_start = (tw)->last_next; 
    tui_textwrap_findnext(tw, config); 
  }
  return 0;
}

tui_textbox_unit_t tui_textwrap_height(tui_textwrap * tw, tui_textwrap_config config) {
  config.line = TUITEXTBOX_UNIT_MAX;
  int err = tui_textwrap_walk(tw, config);
  if(err != 99) return err;
  return tw->full_height;
}

int tui_textwrap_renderline(tui_textwrap * tw, char * out, tui_textwrap_config config) {
  out[0] = 0;
  // Because walk is special, we consume any error and produce a flat -1
  if(tui_textwrap_walk(tw, config)) { return -1; }
  // Now we should be at the appropriate spot. Render out the line, plus padding
  snprintf(out, tw->last_len + 1, "%s", tw->last_start);
  for(tui_textbox_unit_t i = strlen(out); i < config.width; i++) {
    out[i] = config.delim;
  }
  out[config.width] = 0;
  return 0;
}
