
#ifndef __HEADER_GAMEDEFAULTS
#define __HEADER_GAMEDEFAULTS

#include <3ds.h>

#include "lib/myutils.h"
#include "game_drawctrl.h"


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
//(which probably contains many subdivided palettes)
extern u32 default_palette[];

extern struct ToolData default_tooldata[];


void set_screenstate_defaults(struct ScreenState * state);

void init_default_drawstate(struct DrawState * state);
void free_default_drawstate(struct DrawState * state);

//This gives a profile that's good for moving the canvas around
void set_cpadprofile_canvas(struct CpadProfile * profile);


#endif
