
#ifndef __HEADER_GAMEDRAWSTATE
#define __HEADER_GAMEDRAWSTATE

#include <3ds.h>


#define TOOL_PENCIL 0
#define TOOL_ERASER 1
//#define TOOL_COUNT 2
//#define TOOL_CHARS "pe"

#define LINESTYLE_STROKE 0

//Some default configuration (can probably be set during runtime)
#define DEFAULT_SCREEN_COLOR C2D_Color32(90,90,90,255)
#define DEFAULT_BG_COLOR C2D_Color32(255,255,255,255)
#define DEFAULT_LAYER_WIDTH 1024
#define DEFAULT_LAYER_HEIGHT 1024

//An array of STANDARD rgb colors representing the full master palette
extern u32 default_master_palette[];

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
void set_screenstate_ofs(struct ScreenState * state, u16 offset_x, u16 offset_y);

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

   u16 * master_palette;
   u16 master_palette_index; //This tells us the exact color for drawing

   //The palette available currently? Why is this part of the draw state?
   //u16 * palette;
   //u16 pallete_count;
   //u16 palette_index; //Selected index

   //The tool states; each tool can have its own modifiable state
   struct ToolData * tools;
   u8 tool_index; //the current selected tool
};

#endif
