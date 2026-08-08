#ifndef __HEADER_SYSTEM
#define __HEADER_SYSTEM

#include "color.h"
#include "input.h"
#include "layer.h"
#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

// ---------- LAYERS -----------

// #define LINESTYLE_STROKE 0

// -------------- SCREEN --------------

// #define MAXONION 3
// #define DEFAULT_ONIONCOUNT 3
// #define DEFAULT_ONIONBLENDSTART 0.3
// #define DEFAULT_ONIONBLENDEND 0.05

// Variables for modifying display of drawing
struct ScreenState {
  float offset_x;
  float offset_y;
  float zoom;

  MaxLayerInfo layer_info;
  u8 layer_count;
  u8 resolution_id;
  u8 onion_count;
  //u8 layer_visibility;

  // These are pretty standard, but included just in case..
  u16 screen_width;
  u16 screen_height;

  // Some configurable display stuff related to drawing
  u32 screen_color;
  u16 bg_color;
};

int get_screenstate_total_layers(struct ScreenState * state);
int screenstate_layers_overloaded(struct ScreenState * state);

// Safely adjust the screen offset given new desired offsets (doesn't let you
// set to unsafe values, etc)
void set_screenstate_offset(struct ScreenState *state, u16 offset_x,
                            u16 offset_y);
// Adjust the zoom while preserving subjective location in some manner.
void set_screenstate_zoom(struct ScreenState *state, float zoom);

// --------- TOOLS -------------

struct ToolData {
  s8 width;
  u8 style;
  bool has_static_color;
  u16 static_color;
};

// #define DRAWMODE_NORMAL 0
// #define DRAWMODE_ANIMATION 1
// #define DRAWMODE_ANIMATION2 2
// 
// #define DRAWMODE_COUNT 3

struct DrawState {
  s8 zoom_power;
  u16 page;
  u8 layer;

  // u8 mode;

  // The tool states; each tool can have its own modifiable state
  struct ToolData *tools;
  struct ToolData *current_tool; // the current selected tool

  // Some configurable limits and such. This is the limit on width of lines
  u8 min_width;
  u8 max_width;
};

// void shift_drawstate_color(struct DrawState *state, s16 ofs);
void shift_drawstate_width(struct DrawState *state, s16 ofs);
void set_drawstate_tool(struct DrawState *state, u8 tool);
u8 get_drawstate_tool(const struct DrawState *state);

// Variables for system stuff, probably shouldn't change between loads etc
struct SystemState {
  //u8 onion_count;
  u16 anim_loop;

  // This is the target blend for the last onion layer. The other layers are
  // calculated based on this blend target.
  float onion_blendend;
  // The target blend for the first onion layer. The middle layer(s) are
  // calculated based on this and blendend
  float onion_blendstart;

  bool power_saver;
  u8 datestamp;
  u8 datestamp_color;
  u16 control_scheme;

  float slow_avg;
  float slowx;
  float slowy;

  struct ScreenState screen_state;
  struct DrawState draw_state;
  struct CpadProfile cpad;
  struct ColorSystem colors;
};

static inline void set_systemstate_onionstart(struct SystemState *sys, float start) {
  // TODO: some magic numbers, what are they?
  sys->onion_blendstart = start;
  if (sys->onion_blendstart > 0.91)
    sys->onion_blendstart = 0.91;
  sys->onion_blendend = sys->onion_blendstart - 0.25;
  if (sys->onion_blendend < 0.01)
    sys->onion_blendend = 0.01;
}

// Calculate the maximum onionlayers we're going to show
static inline int get_systemstate_max_onionlayers(const struct SystemState * sys) {
  int max_layers = sys->screen_state.onion_count; // Assume it's the actual count
  int max_actual = sys->screen_state.layer_info.max_layers - sys->screen_state.layer_count;
  if(max_actual < max_layers) max_layers = max_actual;
  int cut = sys->anim_loop; // figure out if either anim_loop or the page cuts it
  if(cut == 0)
    cut = sys->draw_state.page;
  if(cut < max_layers) // return the lesser of the two
    return cut;
  else
    return max_layers;
}

static inline s16 get_systemstate_onion_offset(const struct SystemState * sys, int offset) {
  s16 realpage = sys->draw_state.page + offset;
  if(sys->anim_loop > 0 && sys->anim_loop > sys->draw_state.page) {
    realpage = (realpage + sys->anim_loop) % sys->anim_loop;
  } else {
    realpage = DCV_MAX(realpage, 0);
  }
  return realpage;
}


#endif
