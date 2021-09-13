
#ifndef __HEADER_GAMEDRAWSTATE
#define __HEADER_GAMEDRAWSTATE

#include <3ds.h>


#define TOOL_PENCIL 0
#define TOOL_ERASER 1
//#define TOOL_CHARS "pe"

#define LINESTYLE_STROKE 0

//Some default configuration (can probably be set during runtime)
#define DEFAULT_SCREEN_COLOR C2D_Color32(90,90,90,255)
#define DEFAULT_BG_COLOR C2D_Color32(255,255,255,255)

#define DEFAULT_LAYER_WIDTH 1024
#define DEFAULT_LAYER_HEIGHT 1024

#define DEFAULT_START_LAYER 1
#define DEFAULT_PALETTE_STARTINDEX 1
#define DEFAULT_MIN_WIDTH 1
#define DEFAULT_MAX_WIDTH 64

//palette split is arbitrary, but I personally have them in blocks of 64 colors
#define DEFAULT_PALETTE_SPLIT 64 


//An array of STANDARD rgb colors representing the full master palette
extern u32 default_palette[];

//Variables for modifying display of drawing
struct ScreenState
{
   float offset_x;
   float offset_y;
   float zoom;

   u16 layer_width;
   u16 layer_height;

   //These are pretty standard, but included just in case..
   u16 screen_width;
   u16 screen_height;

   //Some configurable display stuff related to drawing
   u32 screen_color;
   u32 bg_color;
};

void set_screenstate_defaults(struct ScreenState * state);

//Safely adjust the screen offset given new desired offsets (doesn't let you
//set to unsafe values, etc)
void set_screenstate_offset(struct ScreenState * state, u16 offset_x, u16 offset_y);

//Adjust the zoom while preserving subjective location in some manner.
void set_screenstate_zoom(struct ScreenState * state, float zoom);

struct ToolData {
   s8 width;
   u8 style;
   bool has_static_color;
   u16 static_color;
};

extern struct ToolData default_tooldata[];


struct DrawState
{
   s8 zoom_power;
   u16 page;
   u8 layer;

   u16 * palette;
   u16 * current_color; //This tells us the exact color for drawing
   u16 palette_count;

   //The tool states; each tool can have its own modifiable state
   struct ToolData * tools;
   struct ToolData * current_tool; //the current selected tool

   //Some configurable limits and such
   u8 min_width;
   u8 max_width;
};

void init_default_drawstate(struct DrawState * state);
void free_drawstate(struct DrawState * state);
void shift_drawstate_color(struct DrawState * state, s16 ofs);
void shift_drawstate_width(struct DrawState * state, s16 ofs);
u16 get_drawstate_color(struct DrawState * state);
void set_drawstate_tool(struct DrawState * state, u8 tool);

#endif
