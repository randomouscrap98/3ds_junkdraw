#include <3ds.h>

u32 __stacksize__ = 512 * 1024;

#include <citro2d.h>
#include <citro3d.h>

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MSF_GIF_NO_SSE2
#define MSF_GIF_IMPL
#include <msf_gif.h>

// #define DEBUG_COORD
// #define DEBUG_DATAPRINT
// #define DEBUG_IGNORERECT

#include "buffer.h"
#include "color.h"
#include "console.h"
#include "draw.h"
#include "filesys.h"
#include "input.h"
// #include "my3ds.h"

#include "setup.h"
#include "system.h"

// TODO: Figure out these weirdness things:
// - Can't draw on the first 8 pixels along the edge of a target, system crashes
// - Fill color works perfectly fine using line/rect calls, but ClearTarget
//   MUST have the proper 16 bit format....
// - ClearTarget with a transparent color seems to make the color stick using
//   DrawLine unless a DrawRect (or perhaps other) call is performed.
//    THIS IS FIXED IN A LATER REVISION

// Some globals for the um... idk.
u8 _db_prnt_row = 0;

#define PRINTCLEAR()                                                           \
  {                                                                            \
    printf_flush("\x1b[%d;2H%-150s", MAINMENU_TOP, "");                        \
  }

#define PRINTGENERAL(x, col, ...)                                              \
  {                                                                            \
    printf_flush("\x1b[%d;1H%-150s\x1b[%d;2H\x1b[%dm", MAINMENU_TOP, "",       \
                 MAINMENU_TOP, col);                                           \
    printf_flush(x, ##__VA_ARGS__);                                            \
  }

#define PRINTERR(x, ...) PRINTGENERAL(x, 31, ##__VA_ARGS__)
#define PRINTWARN(x, ...) PRINTGENERAL(x, 33, ##__VA_ARGS__)
#define PRINTINFO(x, ...) PRINTGENERAL(x, 37, ##__VA_ARGS__)

#define MY_C2DOBJLIMIT 8192
#define MY_C2DOBJLIMITSAFETY MY_C2DOBJLIMIT - 100
#define PSX1BLEN 30

#define TOOL_PENCIL 0
#define TOOL_ERASER 1
#define TOOL_SLOW 2
#define TOOL_COUNT 3
#define TOOL_CHARS "pes"
#define LAYER_CHARS "bt"

// Glitch in citro2d (or so we assume) prevents us from writing into the first 8
// pixels in the texture. As such, we simply shift the texture over by this
// amount when drawing. HOWEVER: for extra safety, we just avoid double
#define LAYER_EDGEERROR 8
#define LAYER_EDGEBUF LAYER_EDGEERROR * 2

typedef u16 page_num;
typedef u8 layer_num;

#pragma region INIT

// ----------- INIT -------------

u32 default_palette[] = {
    // Endesga 64
    0xff0040, 0x131313, 0x1b1b1b, 0x272727, 0x3d3d3d, 0x5d5d5d, 0x858585,
    0xb4b4b4, 0xffffff, 0xc7cfdd, 0x92a1b9, 0x657392, 0x424c6e, 0x2a2f4e,
    0x1a1932, 0x0e071b, 0x1c121c, 0x391f21, 0x5d2c28, 0x8a4836, 0xbf6f4a,
    0xe69c69, 0xf6ca9f, 0xf9e6cf, 0xedab50, 0xe07438, 0xc64524, 0x8e251d,
    0xff5000, 0xed7614, 0xffa214, 0xffc825, 0xffeb57, 0xd3fc7e, 0x99e65f,
    0x5ac54f, 0x33984b, 0x1e6f50, 0x134c4c, 0x0c2e44, 0x00396d, 0x0069aa,
    0x0098dc, 0x00cdf9, 0x0cf1ff, 0x94fdff, 0xfdd2ed, 0xf389f5, 0xdb3ffd,
    0x7a09fa, 0x3003d9, 0x0c0293, 0x03193f, 0x3b1443, 0x622461, 0x93388f,
    0xca52c9, 0xc85086, 0xf68187, 0xf5555d, 0xea323c, 0xc42430, 0x891e2b,
    0x571c27,

    // Resurrect
    0x2e222f, 0x3e3546, 0x625565, 0x966c6c, 0xab947a, 0x694f62, 0x7f708a,
    0x9babb2, 0xc7dcd0, 0xffffff, 0x6e2727, 0xb33831, 0xea4f36, 0xf57d4a,
    0xae2334, 0xe83b3b, 0xfb6b1d, 0xf79617, 0xf9c22b, 0x7a3045, 0x9e4539,
    0xcd683d, 0xe6904e, 0xfbb954, 0x4c3e24, 0x676633, 0xa2a947, 0xd5e04b,
    0xfbff86, 0x165a4c, 0x239063, 0x1ebc73, 0x91db69, 0xcddf6c, 0x313638,
    0x374e4a, 0x547e64, 0x92a984, 0xb2ba90, 0x0b5e65, 0x0b8a8f, 0x0eaf9b,
    0x30e1b9, 0x8ff8e2, 0x323353, 0x484a77, 0x4d65b4, 0x4d9be6, 0x8fd3ff,
    0x45293f, 0x6b3e75, 0x905ea9, 0xa884f3, 0xeaaded, 0x753c54, 0xa24b6f,
    0xcf657f, 0xed8099, 0x831c5d, 0xc32454, 0xf04f78, 0xf68181, 0xfca790,
    0xfdcbb0,

    // AAP-64
    0x060608, 0x141013, 0x3b1725, 0x73172d, 0xb4202a, 0xdf3e23, 0xfa6a0a,
    0xf9a31b, 0xffd541, 0xfffc40, 0xd6f264, 0x9cdb43, 0x59c135, 0x14a02e,
    0x1a7a3e, 0x24523b, 0x122020, 0x143464, 0x285cc4, 0x249fde, 0x20d6c7,
    0xa6fcdb, 0xffffff, 0xfef3c0, 0xfad6b8, 0xf5a097, 0xe86a73, 0xbc4a9b,
    0x793a80, 0x403353, 0x242234, 0x221c1a, 0x322b28, 0x71413b, 0xbb7547,
    0xdba463, 0xf4d29c, 0xdae0ea, 0xb3b9d1, 0x8b93af, 0x6d758d, 0x4a5462,
    0x333941, 0x422433, 0x5b3138, 0x8e5252, 0xba756a, 0xe9b5a3, 0xe3e6ff,
    0xb9bffb, 0x849be4, 0x588dbe, 0x477d85, 0x23674e, 0x328464, 0x5daf8d,
    0x92dcba, 0xcdf7e2, 0xe4d2aa, 0xc7b08b, 0xa08662, 0x796755, 0x5a4e44,
    0x423934,

    // Famicube
    0x000000, 0xe03c28, 0xffffff, 0xd7d7d7, 0xa8a8a8, 0x7b7b7b, 0x343434,
    0x151515, 0x0d2030, 0x415d66, 0x71a6a1, 0xbdffca, 0x25e2cd, 0x0a98ac,
    0x005280, 0x00604b, 0x20b562, 0x58d332, 0x139d08, 0x004e00, 0x172808,
    0x376d03, 0x6ab417, 0x8cd612, 0xbeeb71, 0xeeffa9, 0xb6c121, 0x939717,
    0xcc8f15, 0xffbb31, 0xffe737, 0xf68f37, 0xad4e1a, 0x231712, 0x5c3c0d,
    0xae6c37, 0xc59782, 0xe2d7b5, 0x4f1507, 0x823c3d, 0xda655e, 0xe18289,
    0xf5b784, 0xffe9c5, 0xff82ce, 0xcf3c71, 0x871646, 0xa328b3, 0xcc69e4,
    0xd59cfc, 0xfec9ed, 0xe2c9ff, 0xa675fe, 0x6a31ca, 0x5a1991, 0x211640,
    0x3d34a5, 0x6264dc, 0x9ba0ef, 0x98dcff, 0x5ba8ff, 0x0a89ff, 0x024aca,
    0x00177d,

    // Sweet Canyon 64
    0x0f0e11, 0x2d2c33, 0x40404a, 0x51545c, 0x6b7179, 0x7c8389, 0xa8b2b6,
    0xd5d5d5, 0xeeebe0, 0xf1dbb1, 0xeec99f, 0xe1a17e, 0xcc9562, 0xab7b49,
    0x9a643a, 0x86482f, 0x783a29, 0x6a3328, 0x541d29, 0x42192c, 0x512240,
    0x782349, 0x8b2e5d, 0xa93e89, 0xd062c8, 0xec94ea, 0xf2bdfc, 0xeaebff,
    0xa2fafa, 0x64e7e7, 0x54cfd8, 0x2fb6c3, 0x2c89af, 0x25739d, 0x2a5684,
    0x214574, 0x1f2966, 0x101445, 0x3c0d3b, 0x66164c, 0x901f3d, 0xbb3030,
    0xdc473c, 0xec6a45, 0xfb9b41, 0xf0c04c, 0xf4d66e, 0xfffb76, 0xccf17a,
    0x97d948, 0x6fba3b, 0x229443, 0x1d7e45, 0x116548, 0x0c4f3f, 0x0a3639,
    0x251746, 0x48246d, 0x69189c, 0x9f20c0, 0xe527d2, 0xff51cf, 0xff7ada,
    0xff9edb,

    // Rewild 64
    0x172038, 0x253a5e, 0x3c5e8b, 0x4f8fba, 0x73bed3, 0xa4dddb, 0x193024,
    0x245938, 0x2b8435, 0x62ac4c, 0xa2dc6e, 0xc5e49b, 0x19332d, 0x25562e,
    0x468232, 0x75a743, 0xa8ca58, 0xd0da91, 0x5f6d43, 0x97933a, 0xa9b74c,
    0xcfd467, 0xd5dc97, 0xd6dea6, 0x382a28, 0x43322f, 0x564238, 0x715a42,
    0x867150, 0xb1a282, 0x4d2b32, 0x7a4841, 0xad7757, 0xc09473, 0xd7b594,
    0xe7d5b3, 0x341c27, 0x602c2c, 0x884b2b, 0xbe772b, 0xde9e41, 0xe8c170,
    0x241527, 0x411d31, 0x752438, 0xa53030, 0xcf573c, 0xda863e, 0x1e1d39,
    0x402751, 0x7a367b, 0xa23e8c, 0xc65197, 0xdf84a5, 0x090a14, 0x10141f,
    0x151d28, 0x202e37, 0x394a50, 0x577277, 0x819796, 0xa8b5b2, 0xc7cfcc,
    0xebede9

};

struct ToolData default_tooldata[] = {
    // Pencil
    {2, LINESTYLE_STROKE, false},
    // Eraser
    {4, LINESTYLE_STROKE, true, 0},
    // Slow pen
    {2, LINESTYLE_STROKE, false}};

// Set some default values and malloc anything needed in the SystemState
void create_defaultsystemstate(struct SystemState *state) {
  state->slow_avg = 0.15;
  state->power_saver = false;
  state->onion_count = DEFAULT_ONIONCOUNT;
  state->onion_blendstart = DEFAULT_ONIONBLENDSTART;
  state->onion_blendend = DEFAULT_ONIONBLENDEND;
  memset(&state->colors, 0, sizeof(struct ColorSystem));
  state->colors.palette_size = DEFAULT_PALETTE_SPLIT;
  colorsystem_setcolors(&state->colors, default_palette,
                        sizeof(default_palette) / sizeof(u32));
  state->draw_state.tools = malloc(sizeof(default_tooldata));
}

void set_screenstate_defaults(struct ScreenState *state) {
  state->offset_x = 0;
  state->offset_y = 0;
  state->zoom = 1;
  state->layer_width = LAYER_WIDTH;
  state->layer_height = LAYER_HEIGHT;
  state->screen_width = 320; // These two should literally never change
  state->screen_height = 240;
  state->screen_color = SCREEN_COLOR;
  state->bg_color = CANVAS_BG_COLOR;

  state->layer_visibility = (1 << LAYER_COUNT) - 1; // All visible
}

void set_default_drawstate(struct DrawState *state) {
  state->zoom_power = 0;
  state->page = 0;
  state->layer = DEFAULT_START_LAYER;
  state->mode = DRAWMODE_NORMAL;

  memcpy(state->tools, default_tooldata, sizeof(default_tooldata));
  state->current_tool = state->tools + TOOL_PENCIL;

  state->min_width = MIN_WIDTH;
  state->max_width = MAX_WIDTH;
}

// Only really applies to default initialized states
void free_defaultsystemstate(struct SystemState *state) {
  free(state->draw_state.tools);
  colorsystem_free(&state->colors);
}

void set_cpadprofile_canvas(struct CpadProfile *profile) {
  profile->deadzone = 40;
  profile->mod_constant = 1;
  profile->mod_multiplier = 0.02f;
  profile->mod_curve = 3.2f;
  profile->mod_general = 1;
}

#pragma endregion

// -- DRAWING UTILS --

// Some (hopefully temporary) globals to overcome some unforeseen limits
u32 _drw_cmd_cnt = 0;

#define MY_FLUSH()                                                             \
  {                                                                            \
    C3D_FrameEnd(0);                                                           \
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);                                        \
    _drw_cmd_cnt = 0;                                                          \
  }
#define MY_FLUSHCHECK()                                                        \
  if (_drw_cmd_cnt > MY_C2DOBJLIMITSAFETY) {                                   \
    LOGDBG("FLUSHING %ld DRAW CMDS PREMATURELY\n", _drw_cmd_cnt);              \
    MY_FLUSH();                                                                \
  }

// These "_drawrect" globals apply to ALL rectangle functions
struct ScreenState *_drawrect_scrst = NULL;
int _msr_ofsx = 0, _msr_ofsy = 0;

void MY_SOLIDRECT(float x, float y, u16 width, u32 color) {
  if (x < 0 || y < 0 || x >= _drawrect_scrst->layer_width ||
      y >= _drawrect_scrst->layer_height) {
#ifdef DEBUG_IGNORERECT
    LOGDBG("IGNORING RECT AT (%f, %f)", x, y);
#endif
    return;
  }
  C2D_DrawRectSolid(x + _msr_ofsx + LAYER_EDGEBUF,
                    y + _msr_ofsy + LAYER_EDGEBUF, 0.5, width, width, color);
  _drw_cmd_cnt++;
  MY_FLUSHCHECK();
}

// This is a funny little system. Custom line drawing, passing the function,
// idk. There are better ways, but I'm lazy
u32 *_exp_layer_dt = NULL;

void _exp_layer_dt_func(float x, float y, u16 width, u32 color) {
  // pre-calculating can save tons of time in the critical loop down there. We
  // don't want ANY long operations (modulus, my goodness) and we want it to
  // use cache as much as possible.
  //  NOTE: we changed how this worked in 0.4/0.5, this WILL change how exports
  //  look but it's only for the weird case of drawing right on the edge...
  //  hopefully that's ok??
  u32 minx = x;
  u32 maxx = x + width;
  if (minx < 0)
    minx = 0;
  if (maxx >= _drawrect_scrst->layer_width)
    maxx = _drawrect_scrst->layer_width - 1;
  u32 miny = y;
  u32 maxy = y + width;
  if (miny < 0)
    miny = 0;
  if (maxy >= _drawrect_scrst->layer_height)
    maxy = _drawrect_scrst->layer_height - 1;

  for (u32 yi = miny; yi < maxy; yi++) //= _drawrect_scrst->layer_width)
    for (u32 xi = minx; xi < maxx; xi++)
      _exp_layer_dt[yi * _drawrect_scrst->layer_width + xi] = color;
}

// Draw the scrollbars on the sides of the screen for the given screen
// modification (translation AND zoom affect the scrollbars)
void draw_scrollbars(const struct ScreenState *mod) {
  // Need to draw an n-thickness scrollbar on the right and bottom. Assumes
  // standard page size for screen modifier.

  // Bottom and right scrollbar bg
  C2D_DrawRectSolid(0, mod->screen_height - SCROLL_WIDTH, 0.5f,
                    mod->screen_width, SCROLL_WIDTH, SCROLL_BG);
  C2D_DrawRectSolid(mod->screen_width - SCROLL_WIDTH, 0, 0.5f, SCROLL_WIDTH,
                    mod->screen_height, SCROLL_BG);

  u16 sofs_x =
      (float)mod->offset_x / mod->layer_width / mod->zoom * mod->screen_width;
  u16 sofs_y =
      (float)mod->offset_y / mod->layer_height / mod->zoom * mod->screen_height;

  // bottom and right scrollbar bar
  C2D_DrawRectSolid(sofs_x, mod->screen_height - SCROLL_WIDTH, 0.5f,
                    mod->screen_width * mod->screen_width /
                        (float)mod->layer_width / mod->zoom,
                    SCROLL_WIDTH, SCROLL_BAR);
  C2D_DrawRectSolid(mod->screen_width - SCROLL_WIDTH, sofs_y, 0.5f,
                    SCROLL_WIDTH,
                    mod->screen_height * mod->screen_height /
                        (float)mod->layer_height / mod->zoom,
                    SCROLL_BAR);
}

// Draw (JUST draw) the entire color picker area, which may include other
// stateful controls
void draw_colorpicker(struct ColorSystem *cs, bool collapsed) {
  // TODO: Maybe split this out?
  if (collapsed) {
    C2D_DrawRectSolid(0, 0, 0.5f, PALETTE_MINISIZE, PALETTE_MINISIZE,
                      PALETTE_BG);
    C2D_DrawRectSolid(PALETTE_MINIPADDING, PALETTE_MINIPADDING, 0.5f,
                      PALETTE_MINISIZE - PALETTE_MINIPADDING * 2,
                      PALETTE_MINISIZE - PALETTE_MINIPADDING * 2,
                      rgba16_to_rgba32c(colorsystem_getcolor(cs)));

    return;
  }

  // const int LASTCOLSHIFT = 8;
  u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
  C2D_DrawRectSolid(0, 0, 0.5f,
                    (9 + (COLORSYS_HISTORY >> 3)) * shift +
                        (PALETTE_OFSX << 1) + PALETTE_SWATCHMARGIN,
                    8 * shift + (PALETTE_OFSY << 1) + PALETTE_SWATCHMARGIN,
                    PALETTE_BG);

  u16 selected_index = colorsystem_getpaletteoffset(cs);
  u16 *palette = colorsystem_getcurrentpalette(cs);

  for (u16 i = 0; i < cs->palette_size; i++) {
    // TODO: an implicit 8 wide thing
    u16 x = i & 7;
    u16 y = i >> 3;

    if (i == selected_index) {
      C2D_DrawRectSolid(PALETTE_OFSX + x * shift, PALETTE_OFSY + y * shift,
                        0.5f, shift, shift, PALETTE_SELECTED_COLOR);
    }

    C2D_DrawRectSolid(PALETTE_OFSX + x * shift + PALETTE_SWATCHMARGIN,
                      PALETTE_OFSY + y * shift + PALETTE_SWATCHMARGIN, 0.5f,
                      PALETTE_SWATCHWIDTH, PALETTE_SWATCHWIDTH,
                      rgba16_to_rgba32c(palette[i]));
  }

  for (u16 i = 0; i < COLORSYS_HISTORY; i++) {
    // REALLY hope the optimizing compiler fixes this...
    u16 x = i % (COLORSYS_HISTORY >> 3);
    u16 y = i / (COLORSYS_HISTORY >> 3);

    C2D_DrawRectSolid(PALETTE_OFSX + (x + 9) * shift + PALETTE_SWATCHMARGIN,
                      PALETTE_OFSY + y * shift + PALETTE_SWATCHMARGIN, 0.5f,
                      PALETTE_SWATCHWIDTH, PALETTE_SWATCHWIDTH,
                      rgba16_to_rgba32c(cs->history[i]));
  }
}

// Calculate the x and y offset into some layer for an onion layer based on an
// offset from current page. So, 0 would be current page, but you should never
// give that as a value. The offset is given WITHOUT the edgebuf
void onion_offset(const struct DrawState *dstate, int ofs, int *x, int *y) {
  if (ofs >= 0) {
    *x = 0;
    *y = 0;
    return;
  }
  int region = -ofs; // The images go back from closest to furthest
  // IF we go back to the more optimized onion top thing, this is what we do:
  // int otop = (dstate->page % MAXONION); // First actual onion page
  // // Plus one to skip player region
  // int region = 1 + (MAXONION + otop + ofs + 1) % MAXONION;
  // CAREFUL: TODO: When we get more modes, this will misbehave!
  *x = (region & 1) * (LAYER_WIDTH >> dstate->mode); // offset without edgebuf
  *y = (region >> 1) * (LAYER_WIDTH >> dstate->mode);
}

void draw_layers(const struct LayerData *layers, layer_num layer_count,
                 const struct SystemState *sys) {
  C2D_DrawRectSolid(-sys->screen_state.offset_x, -sys->screen_state.offset_y,
                    0.5f,
                    sys->screen_state.layer_width * sys->screen_state.zoom,
                    sys->screen_state.layer_height * sys->screen_state.zoom,
                    sys->screen_state.bg_color); // The bg color

  C2D_ImageTint tint;
  // We draw the onion skin stuff first
  if (sys->draw_state.mode == DRAWMODE_ANIMATION ||
      sys->draw_state.mode == DRAWMODE_ANIMATION2) {
    // The offset from current page for onion skin
    for (int o = -DCV_MIN(sys->onion_count, sys->draw_state.page); o <= -1;
         o++) {
      for (int i = 0; i < 4; i++) {
        tint.corners[i].color = 0xFFFFFFFF; // Setable sometime?
        tint.corners[i].blend =
            1 - DCV_LERP(sys->onion_blendstart, sys->onion_blendend,
                         fabs((o + 1.0) / (MAXONION - 1.0)));
      }
      int x, y;
      onion_offset(&sys->draw_state, o, &x, &y);
      for (layer_num i = 0; i < layer_count; i++) {
        if (sys->screen_state.layer_visibility & (1 << i)) {
          C2D_DrawImageAt(layers[i].image,
                          -sys->screen_state.offset_x -
                              (LAYER_EDGEBUF + x) * sys->screen_state.zoom,
                          -sys->screen_state.offset_y -
                              (LAYER_EDGEBUF + y) * sys->screen_state.zoom,
                          0.5f, &tint, sys->screen_state.zoom,
                          sys->screen_state.zoom);
        }
      }
    }
  }

  for (layer_num i = 0; i < layer_count; i++) {
    if (sys->screen_state.layer_visibility & (1 << i)) {
      C2D_DrawImageAt(
          layers[i].image,
          -sys->screen_state.offset_x - LAYER_EDGEBUF * sys->screen_state.zoom,
          -sys->screen_state.offset_y - LAYER_EDGEBUF * sys->screen_state.zoom,
          0.5f, NULL, sys->screen_state.zoom, sys->screen_state.zoom);
    }
  }

  float canvas_x = sys->screen_state.layer_width * sys->screen_state.zoom -
                   sys->screen_state.offset_x;
  float canvas_y = sys->screen_state.layer_height * sys->screen_state.zoom -
                   sys->screen_state.offset_y;

  // This is rather wasteful but eh...
  C2D_DrawRectSolid(
      canvas_x, 0, 0.5f, sys->screen_state.screen_width - canvas_x,
      sys->screen_state.screen_height, sys->screen_state.screen_color);
  C2D_DrawRectSolid(0, canvas_y, 0.5f, canvas_x,
                    sys->screen_state.screen_height,
                    sys->screen_state.screen_color);
  // sys->screen_state.layer_width * sys->screen_state.zoom,
  // sys->screen_state.layer_height * sys->screen_state.zoom, bg_color); //The
  // bg color
}

// Draw as much as possible from the given ring buffer, with as little context
// switching as possible. WARN: MAKES A LOT OF ASSUMPTIONS IN ORDER TO PREVENT
// COSTLY MALLOCS PER FRAME
void draw_from_buffer(struct LineRingBuffer *scandata, struct LayerData *layers,
                      struct ScreenState *scrst) {
  u16 lineCount = 0;
  struct FullLine *lines[MAX_FRAMELINES];
  struct FullLine *next = NULL;

  // Repeat while there's something in the buffer and we haven't reached the
  // limit. Essentially, just pull as much as possible out of the ring buffer so
  // we can later draw it per-layer
  while ((next = lineringbuffer_shrink(scandata)) &&
         lineCount < MAX_FRAMELINES) {
    lines[lineCount] = next;
    lineCount++;
  }

  if (lineCount == 0) {
    return;
  }

  for (u8 i = 0; i < LAYER_COUNT; i++) {
    // Skip expensive drawing for layers that aren't visible
    if ((scrst->layer_visibility & (1 << i)) == 0) {
      continue;
    }

    // Don't want to call this too often, so do as much as possible PER
    // layer instead of jumping around
    C2D_SceneBegin(layers[i].target);

    // Now loop over our lines
    for (u16 li = 0; li < lineCount; li++) {
      // Just entirely skip data for layers we're not focusing on yet.
      if (lines[li]->layer != i)
        continue;

      // NOTE: It's up to you to set _msr_ofsx + _msr_ofsy appropriately before
      // calling "draw_from_buffer"
      pixaligned_linefunc(lines[li], MY_SOLIDRECT);
    }
  }
}

// -------- Data helpers ------------

struct SimpleLine *add_point_to_stroke(struct LinePackage *pending,
                                       const touchPosition *pos,
                                       struct SystemState *sys) {
  struct ScreenState *mod = &sys->screen_state;

  // Stroke is overfull, can't do anything
  if (pending->line_count >= pending->max_lines) {
    return NULL;
  }

  // This is for a stroke, do different things if we have different tools!
  struct SimpleLine *line = pending->lines + pending->line_count;

  line->x2 = pos->px / mod->zoom + mod->offset_x / mod->zoom;
  line->y2 = pos->py / mod->zoom + mod->offset_y / mod->zoom;

  if (pending->line_count == 0) {
    line->x1 = line->x2;
    line->y1 = line->y2;
    sys->slowx = line->x1;
    sys->slowy = line->y1;
  } else {
    line->x1 = pending->lines[pending->line_count - 1].x2;
    line->y1 = pending->lines[pending->line_count - 1].y2;
  }

  // No matter what, this SHOULD work... I think.
  if (get_drawstate_tool(&sys->draw_state) == TOOL_SLOW) {
    sys->slowx += ((float)line->x2 - line->x1) * sys->slow_avg;
    sys->slowy += ((float)line->y2 - line->y1) * sys->slow_avg;
    line->x2 = round(sys->slowx);
    line->y2 = round(sys->slowy);
  }

  // Added a line
  pending->line_count++;

  return line;
}

// Given a touch position (presumably on the color palette), update the selected
// palette index within the color system. Includes history
int update_paletteselect(const touchPosition *pos, struct ColorSystem *cs) {
  u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
  u16 xind = (pos->px - PALETTE_OFSX) / shift;
  u16 yind = (pos->py - PALETTE_OFSY) / shift;
  if (yind < 8) {
    if (xind < 8) {
      u16 new_index = (yind << 3) + xind;
      if (new_index >= 0 && new_index < cs->palette_size) {
        colorsystem_setpaletteoffset(cs, new_index);
        return 1;
      }
    } else if (xind >= 9 && xind < 9 + (COLORSYS_HISTORY >> 3)) {
      colorsystem_trysetcolor(
          cs, cs->history[(yind * (COLORSYS_HISTORY >> 3)) + xind - 9]);
      return 2;
    }
  }
  return 0;
}

// Fill the historical colors in colorsystem with colors from the given page.
void fill_colorhistory(struct ColorSystem *cs, char *draw_start, char *draw_end,
                       u16 page) {
  char *coldat = draw_start;
  char *colstroke = NULL;
  for (int lci = 0; lci < COLORSYS_HISTORY; lci++) {
    cs->history[lci] = 0xFFFF;
  }
  int lchead = 0;
  while (coldat && coldat < draw_end) {
    coldat =
        datamem_scanstroke(coldat, draw_end, MAX_DRAW_DATA, page, &colstroke);
    if (!colstroke) // No more to read
      break;
    // TODO: put this in draw sys as a function
    u16 hcol = chars_to_int(colstroke + 2, 3);
    if (hcol == 0) // Skip eraser
      continue;
    for (int lci = 0; lci < COLORSYS_HISTORY; lci++) {
      if (cs->history[lci] == hcol)
        goto colscanloopend;
    }
    cs->history[lchead] = hcol;
    lchead = (lchead + 1) & (COLORSYS_HISTORY - 1);
  colscanloopend:;
  }
}

// Export a page as a raw buffer. YOU MUST FREE THE BUFFER! Returns null if
// error
u32 *export_page_raw(struct ScreenState *scrst, page_num page, char *data,
                     char *data_end) {
  u32 *layerdata[LAYER_COUNT + 1];
  u32 size_bytes = sizeof(u32) * scrst->layer_width * scrst->layer_height;

  for (int i = 0; i < LAYER_COUNT + 1; i++) {
    layerdata[i] = malloc(size_bytes);

    if (layerdata[i] == NULL) {
      LOGDBG("ERR: COULDN'T ALLOCATE MEMORY FOR EXPORT");

      for (int j = 0; j < i; j++)
        free(layerdata[j]);

      return NULL;
    }

    // TODO: This assumes the bg is white
    memset(layerdata[i], (i == LAYER_COUNT) ? 0xFF : 0, size_bytes);
  }

  // Now just parse and parse and parse until we reach the end!
  u32 data_length = data_end - data;
  char *current_data = data;
  char *stroke_start = NULL;
  struct LinePackage package;
  init_linepackage(
      &package); // WARN: initialization could fail, no checks performed!

  while (current_data < data_end) {
    current_data = datamem_scanstroke(current_data, data_end, data_length, page,
                                      &stroke_start);

    // Is this the normal way to do this? idk
    if (stroke_start == NULL)
      continue;

    convert_data_to_linepack(&package, stroke_start, current_data);
    _exp_layer_dt = layerdata[package.layer];

    // At this point, we draw the whole stroke
    pixaligned_linepackfunc(&package, 0, package.line_count,
                            &_exp_layer_dt_func);
  }

  // Now merge arrays together; our alpha blending is dead simple.
  // Will this take a while?
  for (int i = 0; i < scrst->layer_width * scrst->layer_height; i++) {
    // Loop over arrays, the topmost (layer) value persists
    for (int j = LAYER_COUNT - 1; j >= 0; j--) {
      if (layerdata[j][i]) {
        layerdata[LAYER_COUNT][i] = layerdata[j][i];
        break;
      }
    }
  }

  free_linepackage(&package);

  // Remember, don't free the last layer, as that's our merged
  for (int i = 0; i < LAYER_COUNT; i++)
    free(layerdata[i]);

  return layerdata[LAYER_COUNT];
}

struct GifSettings {
  u16 bitdepth;
  u16 csecsperframe;
};

int export_gif(struct ScreenState *scrst, struct GifSettings *settings,
               char *data, char *data_end, char *basename) {
  aptSetHomeAllowed(false);
  aptSetSleepAllowed(false);
  int ret = 0;

  PRINTINFO("Beginning gif export...");
  if (mkdir_p(SCREENSHOTS_BASE)) {
    PRINTERR("Couldn't create screenshots folder: %s", SCREENSHOTS_BASE);
    ret = 1;
    goto EXPORTGIFEND;
  }

  char savepath[MAX_FILEPATH];
  time_t now = time(NULL);
  sprintf(savepath, "%s%s_%jd.gif", SCREENSHOTS_BASE,
          strlen(basename) ? basename : "new", now);

  MsfGifState gifState = {};
  FILE *fp = fopen(savepath, "wb");
  if (fp == NULL) {
    PRINTERR("ERR: Couldn't open gif for writing: %s\n", savepath);
    ret = 1;
    goto EXPORTGIFEND;
  }
  msf_gif_begin_to_file(&gifState, scrst->layer_width, scrst->layer_height,
                        (MsfGifFileWriteFunc)fwrite, (void *)fp);
  int lastpageused = last_used_page(data, data_end - data);
  for (int i = 0; i <= lastpageused; i++) {
    PRINTINFO("Exporting page %d / %d...", i + 1, lastpageused + 1);
    u32 *page = export_page_raw(scrst, i, data, data_end);
    PRINTINFO("Packing gif page %d / %d...", i + 1, lastpageused + 1);
    if (!msf_gif_frame_to_file(&gifState, (uint8_t *)page,
                               settings->csecsperframe, settings->bitdepth,
                               scrst->layer_width * 4)) {
      PRINTERR("ERROR ON PAGE %d\n", i + 1);
      free(page);
      break;
    }
    free(page);
  }
  if (!msf_gif_end_to_file(&gifState)) {
    PRINTERR("ERR: Couldn't finalize gif\n");
    ret = 1;
    goto EXPORTGIFEND;
  }
  fclose(fp);
  PRINTINFO("Exported gif to: %s", savepath);
EXPORTGIFEND:;
  aptSetHomeAllowed(true);
  aptSetSleepAllowed(true);
  aptCheckHomePressRejected();
  return ret;
}

// -- MENU/PRINT STUFF --

void print_controls() {
  printf("\x1b[0m");
  printf("     L - change color        R - general modifier\n");
  printf(" LF/RT - line width     UP/DWN - zoom (+R - page)\n");
  printf("SELECT - change layers   START - menu\n");
  printf("  ABXY - change tools    C-PAD - scroll canvas\n");
  printf("--------------------------------------------------");
}

void print_framing() {
  printf("\x1b[0m");
  printf("\x1b[29;1H--------------------------------------------------");
  printf("\x1b[28;43H%7s", VERSION);
}

void get_printmods(char *status_x1b, char *active_x1b, char *statusbg_x1b,
                   char *activebg_x1b) {
  // First is background, second is foreground (within string)
  if (status_x1b != NULL)
    sprintf(status_x1b, "\x1b[40m\x1b[%dm", STATUS_MAINCOLOR);
  if (active_x1b != NULL)
    sprintf(active_x1b, "\x1b[40m\x1b[%dm", STATUS_ACTIVECOLOR);
  if (statusbg_x1b != NULL)
    sprintf(statusbg_x1b, "\x1b[%dm\x1b[30m", 10 + STATUS_MAINCOLOR);
  if (activebg_x1b != NULL)
    sprintf(activebg_x1b, "\x1b[%dm\x1b[30m", 10 + STATUS_ACTIVECOLOR);
}

void print_data(char *data, char *dataptr, char *saveptr) {
  char status_x1b[PSX1BLEN];
  char active_x1b[PSX1BLEN];
  get_printmods(status_x1b, active_x1b, NULL, NULL);

  u32 datasize = dataptr - data;
  u32 unsaved = dataptr - saveptr;
  float percent = 100.0 * (float)datasize / MAX_DRAW_DATA;

  char numbers[51];
  sprintf(numbers, "%ld %ld  %s(%05.2f%%)", unsaved, datasize, active_x1b,
          percent);
  printf("\x1b[28;1H%s %s%*s", status_x1b, numbers,
         PRINTDATA_WIDTH - (strlen(numbers) - strlen(active_x1b)), "");
}

void print_status(u8 width, u8 layer, s8 zoom_power, u8 tool, u16 color,
                  u16 page) {
  char tool_chars[TOOL_COUNT + 1];
  strcpy(tool_chars, TOOL_CHARS);

  char status_x1b[PSX1BLEN];
  char active_x1b[PSX1BLEN];
  char statusbg_x1b[PSX1BLEN];
  char activebg_x1b[PSX1BLEN];
  get_printmods(status_x1b, active_x1b, statusbg_x1b, activebg_x1b);

  printf("\x1b[30;1H%s W:%s%02d%s L:", status_x1b, active_x1b, width,
         status_x1b);
  char layernames[] =
      LAYER_CHARS; // apparently layer 1 is the top, but we display it backwards
                   // (layer 1 is first, then 0)
  for (s8 i = LAYER_COUNT - 1; i >= 0;
       i--) // NOTE: the second input is an optional character to display on
            // current layer
  {
    printf("%s%c", i == layer ? activebg_x1b : statusbg_x1b,
           i == layer ? layernames[i] : ' ');
  }
  printf("%s Z:", status_x1b);
  for (s8 i = MIN_ZOOMPOWER; i <= MAX_ZOOMPOWER; i++)
    printf("%s%c", i == zoom_power ? activebg_x1b : statusbg_x1b,
           i == 0 ? '|' : ' ');
  printf("%s T:", status_x1b);
  for (u8 i = 0; i < TOOL_COUNT; i++)
    printf("%s%c", i == tool ? activebg_x1b : active_x1b, tool_chars[i]);
  printf("%s P:%s%03d", status_x1b, active_x1b, page + 1);
  printf("%s C:%s%#06x", status_x1b, active_x1b, color);
}

void print_time(bool showcolon) {
  char status_x1b[PSX1BLEN];
  get_printmods(status_x1b, NULL, NULL, NULL);

  time_t rawtime = time(NULL);
  struct tm *timeinfo = localtime(&rawtime);

  printf("\x1b[30;45H%s%02d%c%02d", status_x1b, timeinfo->tm_hour,
         showcolon ? ':' : ' ', timeinfo->tm_min);
}

void inc_drawstate_mode(struct ScreenState *scrst, struct DrawState *drwst) {
  int newmode = (drwst->mode + 1) % DRAWMODE_COUNT;
  if (newmode == DRAWMODE_ANIMATION) {
    if (easy_warn("Switching to animation mode",
                  "This will shrink your canvas by half for the\n"
                  " duration of animation mode. You will not lose\n"
                  " strokes made outside this area, they will\n"
                  " just not be seen.\n\n Switch to animation mode?",
                  MAINMENU_TOP)) {
      drwst->mode = newmode;
      // Entering animation mode, make the screen smaller
      scrst->layer_height >>= 1;
      scrst->layer_width >>= 1;
    }
  } else if (newmode == DRAWMODE_ANIMATION2) {
    drwst->mode = newmode;
    // Entering animation mode, make the screen smaller
    scrst->layer_height >>= 1;
    scrst->layer_width >>= 1;
  } else // Going back to the beginning (careful with how this works!)
  {
    drwst->mode = newmode;
    // Exiting animation mode, make the screen larger again
    scrst->layer_height <<= 2;
    scrst->layer_width <<= 2;
  }
}

void run_options_menu(struct SystemState *sys) {
  char menu[256];
  char visibility[4][3] = {"", "b", "t", "bt"};
  char modes[3][16] = {"Normal", "Animation", "Small Animation"};
  s32 menuopt = 0;
  while (1) {
    // Recreate menu every time, since we have dynamic values. To make life
    // easier, we just sprintf everything into the array with newlines, then
    // replace newlines with 0
    sprintf(menu,
            "Mode: %s\nOnion layers: %d\nOnion darkness: %.1f\nLayer "
            "visibility: %s\nSlow pen friction: %.2f\nPower saving: %s\nExit\n",
            modes[sys->draw_state.mode], sys->onion_count,
            sys->onion_blendstart,
            visibility[sys->screen_state.layer_visibility], 1 - sys->slow_avg,
            sys->power_saver ? "on" : "off");
    for (int x = strlen(menu); x >= 0; x--) {
      if (menu[x] == '\n')
        menu[x] = 0;
    }
    menuopt =
        easy_menu("Options", menu, MAINMENU_TOP, 0, menuopt, KEY_B | KEY_START);
    switch (menuopt) {
    case 0:
      inc_drawstate_mode(&sys->screen_state, &sys->draw_state);
      break;
    case 1: // onion layers
      sys->onion_count = (sys->onion_count + 1) % (MAXONION + 1);
      break;
    case 2: // onion darkness
      sys->onion_blendstart += 0.1;
      if (sys->onion_blendstart > 0.91) {
        sys->onion_blendstart = 0.1;
      }
      sys->onion_blendend = DCV_MAX(0.01, sys->onion_blendstart - 0.25);
      break;
    case 3: // layer visibility
      sys->screen_state.layer_visibility =
          (sys->screen_state.layer_visibility + 1) & ((1 << LAYER_COUNT) - 1);
      break;
    case 4: // slow avg
      sys->slow_avg += 0.05;
      if (sys->slow_avg > 0.51) {
        sys->slow_avg = 0.05;
      }
      break;
    case 5: // power saver
      sys->power_saver = !sys->power_saver;
      osSetSpeedupEnable(!sys->power_saver);
      break;
    default:
      return;
    }
  }
}

void run_gif_menu(struct SystemState *sys, char *draw_data, char *draw_data_end,
                  char *filename) {
  char menu[256];
  s32 menuopt = 0;
  struct GifSettings settings;
  settings.bitdepth = 16;
  settings.csecsperframe = 5;
  while (1) {
    // Recreate menu every time, since we have dynamic values. To make life
    // easier, we just sprintf everything into the array with newlines, then
    // replace newlines with 0
    sprintf(menu,
            "Colors: %d\nFrame time: %dms\nCrop: %dx%d\nExport now!\nExit\n",
            1 << settings.bitdepth, settings.csecsperframe * 10,
            sys->screen_state.layer_width, sys->screen_state.layer_height);
    for (int x = strlen(menu); x >= 0; x--) {
      if (menu[x] == '\n')
        menu[x] = 0;
    }
    menuopt = easy_menu("Export gif options", menu, MAINMENU_TOP, 0, menuopt,
                        KEY_B | KEY_START);
    switch (menuopt) {
    case 0: // Colors
      settings.bitdepth <<= 1;
      if (settings.bitdepth > 16)
        settings.bitdepth = 2;
      break;
    case 1: // Frame time
      settings.csecsperframe++;
      if (settings.csecsperframe > 20)
        settings.csecsperframe = 1;
      break;
    case 2: // shortcut to fix animation size
      inc_drawstate_mode(&sys->screen_state, &sys->draw_state);
      break;
    case 3: // export
      export_gif(&sys->screen_state, &settings, draw_data, draw_data_end,
                 filename);
      return;
    default:
      return;
    }
  }
}

// -- FILESYSTEM --

void get_save_location(char *savename, char *container) {
  container[0] = 0;
  sprintf(container, "%s%s/", SAVE_BASE, savename);
}

void get_rawfile_location(char *savename, char *container) {
  get_save_location(savename, container);
  strcpy(container + strlen(container), "raw");
}

int save_drawing(char *filename, char *data) {
  char savefolder[MAX_FILEPATH];
  char fullpath[MAX_FILEPATH];
  char temp_msg[MAX_FILEPATH];

  // LOGDBG("SAVEFILENAME: %s", filename);

  if (enter_text_fixed("Enter a filename: ", MAINMENU_TOP, filename,
                       MAX_FILENAMESHOW, !strlen(filename),
                       KEY_B | KEY_START)) {
    // Go get the full path
    get_save_location(filename, savefolder);
    get_rawfile_location(filename, fullpath);

    // Prepare the warning message
    snprintf(temp_msg, MAX_FILEPATH - 1, "WARN: OVERWRITE %s", filename);

    // We only save if it's new or if... idk.
    if (!file_exists(fullpath) ||
        easy_warn(temp_msg, "Save already exists, definitely overwrite?",
                  MAINMENU_TOP)) {
      aptSetHomeAllowed(false);
      aptSetSleepAllowed(false);
      PRINTINFO("Saving file %s...", filename);
      int result = mkdir_p(savefolder);
      if (!result)
        result = write_file(fullpath, data);
      PRINTCLEAR();
      aptSetHomeAllowed(true);
      aptSetSleepAllowed(true);
      aptCheckHomePressRejected();
      return result;
    }
  }

  return -1;
}

char *load_drawing(char *data_container, char *final_filename) {
  char fullpath[MAX_FILEPATH];
  char temp_msg[MAX_FILEPATH]; // Find another constant for this I guess
  char *result = NULL;

  char *all_files = malloc(sizeof(char) * MAX_ALLFILENAMES);
  if (!all_files) {
    PRINTERR("Couldn't allocate memory");
    goto TRUEEND;
  }

  PRINTINFO("Searching for saves...");
  u32 dircount = get_directories(SAVE_BASE, all_files, MAX_ALLFILENAMES);

  if (dircount < 0) {
    PRINTERR("Something went wrong while searching!");
    goto END;
  } else if (dircount <= 0) {
    PRINTINFO("No saves found");
    goto END;
  }

  sprintf(temp_msg, "Found %ld saves:", dircount);
  s32 sel = easy_menu(temp_msg, all_files, MAINMENU_TOP, FILELOAD_MENUCOUNT, 0,
                      KEY_START | KEY_B);

  if (sel < 0)
    goto END;

  final_filename[0] = 0;
  strcpy(final_filename, get_menu_item(all_files, MAX_ALLFILENAMES, sel));
  // LOGDBG("LOADFILENAME: %s", final_filename);

  PRINTINFO("Loading file %s...", final_filename)
  aptSetHomeAllowed(false);
  aptSetSleepAllowed(false);
  get_rawfile_location(final_filename, fullpath);
  result = read_file(fullpath, data_container, MAX_DRAW_DATA);
  PRINTCLEAR();
  aptSetHomeAllowed(true);
  aptSetSleepAllowed(true);
  aptCheckHomePressRejected();

END:
  free(all_files);
TRUEEND:
  return result;
}

// Export the given page from the given data into a default named file in some
// default image format (bitmap? png? who knows)
int export_page(struct ScreenState *scrst, page_num page, char *data,
                char *data_end, char *basename) {
  // NOTE: this entire function is going to use Citro2D u32 formatted colors,
  // all the way until the final bitmap is written (at which point it can be
  // converted as needed)
  aptSetHomeAllowed(false);
  aptSetSleepAllowed(false);
  int ret = 0;

  PRINTINFO("Exporting page %d: building...", page);

  if (mkdir_p(SCREENSHOTS_BASE)) {
    PRINTERR("Couldn't create screenshots folder: %s", SCREENSHOTS_BASE);
    ret = 1;
    goto EXPORTPAGEEND;
  }

  char savepath[MAX_FILEPATH];
  time_t now = time(NULL);
  sprintf(savepath, "%s%s_%d_%jd.png", SCREENSHOTS_BASE,
          strlen(basename) ? basename : "new", page, now);

  u32 *exported = export_page_raw(scrst, page, data, data_end);
  if (exported == NULL) {
    PRINTERR("Couldn't export page %d (unknown error)", page);
    ret = 1;
    goto EXPORTPAGEEND;
  }

  PRINTINFO("Exporting page %d: converting to png...", page);

  if (write_citropng(exported, scrst->layer_width, scrst->layer_height,
                     savepath) == 0) {
    PRINTINFO("Exported page to: %s", savepath);
  } else {
    PRINTERR("FAILED to export: %s", savepath);
    ret = 1;
    goto EXPORTPAGEEND;
  }
EXPORTPAGEEND:;
  aptSetHomeAllowed(true);
  aptSetSleepAllowed(true);
  aptCheckHomePressRejected();
  return ret;
}

// --------------------- MAIN -------------------------------

#define UTILS_CLAMP(x, mn, mx) (x <= mn ? mn : x >= mx ? mx : x)
#define FLUSH_LAYERS()                                                         \
  for (int __fli = 0; __fli < MAXONION + 1; __fli++) {                         \
    draw_pointers[__fli] = draw_data;                                          \
  }                                                                            \
  reset_lineringbuffer(&scandata);                                             \
  flush_layers = true;

// Some macros used ONLY for main (think lambdas)
#define MAIN_UPDOWN(x)                                                         \
  {                                                                            \
    if (kHeld & KEY_R) {                                                       \
      sys.draw_state.page = UTILS_CLAMP(                                       \
          sys.draw_state.page + x * ((kHeld & KEY_L) ? 10 : 1), 0, MAX_PAGE);  \
      FLUSH_LAYERS();                                                          \
    } else {                                                                   \
      sys.draw_state.zoom_power = UTILS_CLAMP(sys.draw_state.zoom_power + x,   \
                                              MIN_ZOOMPOWER, MAX_ZOOMPOWER);   \
    }                                                                          \
  }

#define PRINT_DATAUSAGE() print_data(draw_data, draw_data_end, saved_last);

#define MAIN_NEWDRAW()                                                         \
  {                                                                            \
    draw_data_end = saved_last = draw_data;                                    \
    set_default_drawstate(&sys.draw_state);                                    \
    sys.colors.index = PALETTE_STARTINDEX;                                     \
    set_screenstate_defaults(&sys.screen_state);                               \
    FLUSH_LAYERS();                                                            \
    save_filename[0] = '\0';                                                   \
    pending.line_count = 0;                                                    \
    current_frame = end_frame = 0;                                             \
    printf("\x1b[1;1H");                                                       \
    print_controls();                                                          \
    print_framing();                                                           \
    PRINT_DATAUSAGE();                                                         \
  }

#define MAIN_UNSAVEDCHECK(x)                                                   \
  (saved_last == draw_data_end ||                                              \
   easy_warn("WARN: UNSAVED DATA", x, MAINMENU_TOP))

int main(int argc, char **argv) {
  gfxInitDefault();
  hidSetRepeatParameters(BREPEAT_DELAY, BREPEAT_INTERVAL);

  // Enable the higher clock speed on New 3DS
  osSetSpeedupEnable(true);

  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(MY_C2DOBJLIMIT);
  C2D_Prepare();

  consoleInit(GFX_TOP, NULL);
  C3D_RenderTarget *screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  LOGDBG("INITIALIZED")

  struct SystemState sys;

  create_defaultsystemstate(&sys);
  set_default_drawstate(&sys.draw_state);
  set_screenstate_defaults(&sys.screen_state);
  set_cpadprofile_canvas(&sys.cpad);

  // Very silly global so rect drawing functions know screen dimensions and such
  _drawrect_scrst = &sys.screen_state;

  LOGDBG("SET SCREENSTATE/CANVAS");

  // weird byte order? 16 bits of color are at top
  const u32 layer_color = rgba32c_to_rgba16c_32(CANVAS_LAYER_COLOR);

  const Tex3DS_SubTexture subtex = {TEXTURE_WIDTH, TEXTURE_HEIGHT, 0.0f,
                                    1.0f,          1.0f,           0.0f};

  struct LayerData layers[LAYER_COUNT];

  for (int i = 0; i < LAYER_COUNT; i++)
    create_layer(layers + i, subtex);

  LOGDBG("CREATED LAYERS");

  bool touching = false;
  bool palette_active = false;
  bool flush_layers = true;
  bool close_palette = false;

  u32 current_frame = 0;
  u32 end_frame = 0;
  s8 last_zoom_power = 0;

  struct LinePackage pending;
  init_linepackage(&pending);
  if (!pending.lines) {
    LOGDBG("ERR: Couldn't allocate stroke lines");
  }

  struct LineRingBuffer scandata;
  init_lineringbuffer(&scandata, MAX_FRAMELINES);
  if (!scandata.lines) {
    LOGDBG("ERR: COULD NOT INIT LINERINGBUFFER");
  }
  if (!scandata.pending.lines) {
    LOGDBG("ERR: COULD NOT INIT LRB PENDING");
  }

  char *save_filename = malloc(MAX_FILENAME * sizeof(char));
  char *draw_data = malloc(MAX_DRAW_DATA * sizeof(char));
  char *stroke_data = malloc(MAX_STROKE_DATA * sizeof(char));
  char *draw_data_end; // NOTE: this is exclusive: it points one past the end.
                       // draw_data_end - draw_data = length
  char *saved_last;

  // Draw pointers for us and all our onion friends
  char *draw_pointers[1 + MAXONION];

  if (!save_filename || !draw_data || !stroke_data) {
    LOGDBG("ERR: COULD NOT INIT MAIN BUFFER");
  }

  LOGDBG("SYSTEM MALLOC");

  MAIN_NEWDRAW();
  mkdir_p(SAVE_BASE);

  LOGDBG("STARTING MAIN LOOP");

  while (aptMainLoop()) {
    hidScanInput();

    u32 kDown = hidKeysDown();
    u32 kUp = hidKeysUp();
    u32 kRepeat = hidKeysDownRepeat();
    u32 kHeld = hidKeysHeld();
    circlePosition pos;
    touchPosition current_touch;
    hidTouchRead(&current_touch);
    hidCircleRead(&pos);

    // Respond to user input
    if (kDown & KEY_L && !(kHeld & KEY_R)) {
      palette_active = !palette_active;
      if (palette_active) { // Only calculate the last cols on button press
        fill_colorhistory(&sys.colors, draw_data, draw_data_end,
                          sys.draw_state.page);
      }
    }
    if (kRepeat & KEY_DUP)
      MAIN_UPDOWN(1)
    if (kRepeat & KEY_DDOWN)
      MAIN_UPDOWN(-1)
    if (kRepeat & KEY_DRIGHT)
      shift_drawstate_width(&sys.draw_state, (kHeld & KEY_R ? 5 : 1));
    if (kRepeat & KEY_DLEFT)
      shift_drawstate_width(&sys.draw_state, -(kHeld & KEY_R ? 5 : 1));
    if (kDown & KEY_A)
      set_drawstate_tool(&sys.draw_state, TOOL_PENCIL);
    if (kDown & KEY_B)
      set_drawstate_tool(&sys.draw_state, TOOL_ERASER);
    if (kDown & KEY_Y)
      set_drawstate_tool(&sys.draw_state, TOOL_SLOW);
    if (kHeld & KEY_L && kDown & KEY_R && palette_active)
      colorsystem_nextpalette(&sys.colors, 1);
    if (kDown & KEY_SELECT) {
      sys.draw_state.layer = (sys.draw_state.layer + 1) % LAYER_COUNT;
    }
    if (kDown & KEY_START) {
      switch (easy_menu(MAINMENU_TITLE, MAINMENU_ITEMS, MAINMENU_TOP, 0, 0,
                        KEY_B | KEY_START)) {
      case MAINMENU_EXIT:
        if (MAIN_UNSAVEDCHECK("Really quit?"))
          goto ENDMAINLOOP;
        break;
      case MAINMENU_NEW:
        if (MAIN_UNSAVEDCHECK("Are you sure you want to start anew?"))
          MAIN_NEWDRAW();
        break;
      case MAINMENU_SAVE:
        *draw_data_end = 0;
        if (save_drawing(save_filename, draw_data) == 0) {
          saved_last = draw_data_end;
          PRINT_DATAUSAGE(); // Should this be out in the main loop?
        }
        break;
      case MAINMENU_LOAD:
        if (MAIN_UNSAVEDCHECK(
                "Are you sure you want to load and lose changes?")) {
          MAIN_NEWDRAW();
          draw_data_end = load_drawing(draw_data, save_filename);

          if (draw_data_end == NULL) {
            PRINTERR("LOAD FAILED!");
            MAIN_NEWDRAW();
          } else {
            saved_last = draw_data_end;
            // Find last page, set it.
            sys.draw_state.page =
                last_used_page(draw_data, draw_data_end - draw_data);
            PRINT_DATAUSAGE();
          }
        }
        break;
      case MAINMENU_OPTIONS:
        // Run options system
        run_options_menu(&sys);
        FLUSH_LAYERS(); // Why not...
        break;
      case MAINMENU_EXPORT:
        export_page(&sys.screen_state, sys.draw_state.page, draw_data,
                    draw_data_end, save_filename);
        break;
      case MAINMENU_EXPORTGIF:
        run_gif_menu(&sys, draw_data, draw_data_end, save_filename);
        FLUSH_LAYERS(); // Why not...
        break;
      }
    }

    u16 curcol = colorsystem_getcolor(&sys.colors);
    if (kDown & KEY_TOUCH) {
      pending.color = sys.draw_state.current_tool->has_static_color
                          ? sys.draw_state.current_tool->static_color
                          : curcol;
      pending.style = sys.draw_state.current_tool->style;
      pending.width = sys.draw_state.current_tool->width;
      pending.layer = sys.draw_state.layer;
    }
    if (kUp & KEY_TOUCH) {
      end_frame = current_frame;
      // Something of a hack: might get rid of it later.
      // This makes the palette disappear on selection (if desired)
      if (close_palette) {
        close_palette = false;
        palette_active = false;
      }
    }

    // Update zoom separately, since the update is always the same
    if (sys.draw_state.zoom_power != last_zoom_power)
      set_screenstate_zoom(&sys.screen_state,
                           pow(2, sys.draw_state.zoom_power));

    if (kRepeat & ~(KEY_TOUCH) || !current_frame) {
      print_status(sys.draw_state.current_tool->width, sys.draw_state.layer,
                   sys.draw_state.zoom_power,
                   sys.draw_state.current_tool - sys.draw_state.tools, curcol,
                   sys.draw_state.page);
    }

    touching = (kHeld & KEY_TOUCH) > 0;

    set_screenstate_offset(
        &sys.screen_state,
        cpad_translate(&sys.cpad, pos.dx, sys.screen_state.offset_x),
        cpad_translate(&sys.cpad, -pos.dy, sys.screen_state.offset_y));

    // Render the scene
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    // -- LAYER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE,
                   GPU_ZERO);

    // Apparently (not sure), all clearing should be done within our main loop?
    if (flush_layers) {
      for (int i = 0; i < LAYER_COUNT; i++)
        C2D_TargetClear(layers[i].target, layer_color);
      flush_layers = false;
    }
    // Ignore first frame touches
    else if (touching) {
      if (palette_active) {
        if (update_paletteselect(&current_touch, &sys.colors))
          close_palette = true;
      } else {
        // Keep this outside the if statement below so it can be used for
        // background drawing too (draw commands from other people)
        C2D_SceneBegin(layers[sys.draw_state.layer].target);

        if (pending.line_count < MAX_STROKE_LINES) {
          onion_offset(&sys.draw_state, 0, &_msr_ofsx,
                       &_msr_ofsy); // This works for 0 (returns 0,0)
          // This is for a stroke, do different things if we have different
          // tools!
          add_point_to_stroke(&pending, &current_touch, &sys);
          // Draw ONLY the current line
          pixaligned_linepackfunc(&pending, pending.line_count - 1,
                                  pending.line_count, MY_SOLIDRECT);
        }
      }
    }

    int oend = DCV_MIN(sys.onion_count, sys.draw_state.page);
    int dp_ofs = 0;

    // Find the place to stop looking for the appropriate draw pointer to work
    // on. This has complicated rules
    for (dp_ofs = 0; dp_ofs <= oend; dp_ofs++) {
      onion_offset(&sys.draw_state, -dp_ofs, &_msr_ofsx,
                   &_msr_ofsy); // This works for 0 (returns 0,0)
      if (dp_ofs ==
          oend) // Don't bother with any logic below, this is the last slot.
        break;
      if (sys.draw_state.mode == DRAWMODE_ANIMATION ||
          sys.draw_state.mode == DRAWMODE_ANIMATION2) {
        // In animation mode, we want to fully complete the current "slot"
        // before continuing to the next. This is functionally equivalent to no
        // animation for onion_count = 0. So, if we're not done scanning this
        // data, obviously continue this. But if it's complete AND the next
        // HASN'T STARTED AND there's draw data left, this is where to break.
        if ((draw_pointers[dp_ofs] != draw_data_end) ||
            ((draw_pointers[dp_ofs + 1] == draw_data) &&
             lineringbuffer_size(&scandata)))
          break;
      } else {
        // This one is simple: if we're not in animation mode, stop immediately
        break;
      }
    }

    draw_pointers[dp_ofs] =
        scan_lines(&scandata, draw_pointers[dp_ofs], draw_data_end,
                   sys.draw_state.page - dp_ofs);
    draw_from_buffer(&scandata, layers, &sys.screen_state);

    C2D_Flush();
    _drw_cmd_cnt = 0;

    // -- OTHER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA);

    if (!(current_frame % 30))
      print_time(current_frame % 60);

    // TODO: Eventually, change this to put the data in different places?
    if (end_frame == current_frame && pending.line_count > 0) {
      char *cvl_end =
          convert_linepack_to_data(&pending, stroke_data, MAX_STROKE_DATA);

      if (cvl_end == NULL) {
        LOGDBG("ERR: Couldn't convert lines!\n");
      } else {
        char *previous_end = draw_data_end;
        draw_data_end =
            write_to_datamem(stroke_data, cvl_end, sys.draw_state.page,
                             draw_data, draw_data_end);
        // An optimization: if draw_pointer was already at the end, don't
        // need to RE-draw what we've already drawn, move it forward with
        // the mem write. Note: there are instances where we WILL be drawing
        // twice, but it's difficult to determine what has or has not been
        // drawn when the pointer isn't at the end.
        if (previous_end == draw_pointers[0])
          draw_pointers[0] = draw_data_end;

        // TODO: need to do this in more places (like when you get lines)
        PRINT_DATAUSAGE();
      }

      pending.line_count = 0;
    }

    C2D_TargetClear(screen, sys.screen_state.screen_color);
    C2D_SceneBegin(screen);

    draw_layers(layers, LAYER_COUNT, &sys);
    draw_scrollbars(&sys.screen_state);
    draw_colorpicker(&sys.colors, !palette_active);

    C3D_FrameEnd(0);

    last_zoom_power = sys.draw_state.zoom_power;
    current_frame++;
  }
ENDMAINLOOP:;

  free_defaultsystemstate(&sys);
  free_lineringbuffer(&scandata);
  free_linepackage(&pending);
  free(save_filename);
  free(draw_data);
  free(stroke_data);

  for (int i = 0; i < LAYER_COUNT; i++)
    delete_layer(layers[i]);

  C3D_RenderTargetDelete(screen);

  C2D_Fini();
  C3D_Fini();
  gfxExit();
  return 0;
}
