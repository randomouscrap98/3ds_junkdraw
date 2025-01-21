#ifndef __HEADER_SYSTEM
#define __HEADER_SYSTEM

#include "color.h"
#include "input.h"
#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

// ---------- LAYERS -----------
#define LAYER_FORMAT GPU_RGBA5551

struct LayerData {
  Tex3DS_SubTexture subtex; // Simple structures
  C3D_Tex texture;
  C2D_Image image;
  C3D_RenderTarget *target; // Actual data?
};

void create_layer(struct LayerData *result, Tex3DS_SubTexture subtex);
void delete_layer(struct LayerData page);

#define LINESTYLE_STROKE 0

// -------------- SCREEN --------------

#define MAXONION 3
#define DEFAULT_ONIONCOUNT 3
#define DEFAULT_ONIONBLENDSTART 0.3
#define DEFAULT_ONIONBLENDEND 0.05

// Variables for modifying display of drawing
struct ScreenState {
  float offset_x;
  float offset_y;
  float zoom;

  u16 layer_width;
  u16 layer_height;
  u8 layer_visibility;

  // These are pretty standard, but included just in case..
  u16 screen_width;
  u16 screen_height;

  // Some configurable display stuff related to drawing
  u32 screen_color;
  u32 bg_color;
};

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

#define DRAWMODE_NORMAL 0
#define DRAWMODE_ANIMATION 1
#define DRAWMODE_ANIMATION2 2

#define DRAWMODE_COUNT 3

struct DrawState {
  s8 zoom_power;
  u16 page;
  u8 layer;
  // u16 *palette;
  // u16 *current_color; // This tells us the exact color for drawing
  // u16 palette_count;

  u8 mode;

  // The tool states; each tool can have its own modifiable state
  struct ToolData *tools;
  struct ToolData *current_tool; // the current selected tool

  // Some configurable limits and such. This is the limit on width of lines
  u8 min_width;
  u8 max_width;
};

// void shift_drawstate_color(struct DrawState *state, s16 ofs);
void shift_drawstate_width(struct DrawState *state, s16 ofs);
// u16 get_drawstate_color(struct DrawState *state);
// Try to set the color to the specified color by scanning all the palette
// colors. Does not set anything if not found. Begin scan at start of block
// size.
// int try_set_drawstate_color(struct DrawState *state, u16 color, u16 block);
void set_drawstate_tool(struct DrawState *state, u8 tool);
u8 get_drawstate_tool(const struct DrawState *state);

// Variables for system stuff, probably shouldn't change between loads etc
struct SystemState {
  u8 onion_count;

  // This is the target blend for the last onion layer. The other layers are
  // calculated based on this blend target.
  float onion_blendend;
  // The target blend for the first onion layer. The middle layer(s) are
  // calculated based on this and blendend
  float onion_blendstart;

  bool power_saver;

  float slow_avg;
  float slowx;
  float slowy;

  struct ScreenState screen_state;
  struct DrawState draw_state;
  struct CpadProfile cpad;
  struct ColorSystem colors;
};

#endif
