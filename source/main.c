#include "3ds/os.h"
#include "3ds/result.h"
#include "3ds/services/apt.h"
#include "digits.h"
#include "metadata.h"
#include "settings.h"
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

// Networking stuff added this 2026-05-18
#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MSF_GIF_NO_SSE2
#define MSF_GIF_IMPL
#include <msf_gif.h>

#define INI_IMPLEMENTATION
#include <ini.h>

// #define DEBUG_COORD
// #define DEBUG_DATAPRINT
// #define DEBUG_IGNORERECT

#include "buffer.h"
#include "color.h"
#include "console.h"
#include "draw.h"
#include "filesys.h"
#include "input.h"
#include "render_palette.h"
#include "settings.h"
#include "undo.h"

#include "log.h"
#include "setup.h"
#include "system.h"
#include "convert.h"
#include "version.h"

// TODO: Figure out these weirdness things:
// - Can't draw on the first 8 pixels along the edge of a target, system crashes
// - Fill color works perfectly fine using line/rect calls, but ClearTarget
//   MUST have the proper 16 bit format....
// - ClearTarget with a transparent color seems to make the color stick using
//   DrawLine unless a DrawRect (or perhaps other) call is performed.
//    THIS IS FIXED IN A LATER REVISION

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

// #define O3DS_C2DOBJLIMIT 8192
// #define O3DS_C2DOBJLIMITSAFETY O3DS_C2DOBJLIMIT - 100
// #define O3DS_MAXDRAWLINES 1000

// #define N3DS_C2DOBJLIMIT (O3DS_C2DOBJLIMIT * 3)
// #define N3DS_C2DOBJLIMITSAFETY (O3DS_C2DOBJLIMITSAFETY * 3)
// #define N3DS_MAXDRAWLINES (O3DS_MAXDRAWLINES * 3 / 2)

// #define N3DS_C2DOBJLIMIT (8192 * 3)
// #define N3DS_C2DOBJLIMITSAFETY ((8192 - 100) * 3)
// #define N3DS_MAXDRAWLINES (1000 * 3 / 2)

#define MAX_FRAMELINES 1000 // Having trouble with this...

// These can be overridden. But can't seem to figure out balance...
u32 _OBJLIMIT = 8192; //N3DS_C2DOBJLIMIT;
u32 _OBJSAFETY = 8192 - 100; //N3DS_C2DOBJLIMITSAFETY;
u32 _MAXDRAWLINES = MAX_FRAMELINES; //N3DS_MAXDRAWLINES;

#define PSX1BLEN 30
#define MAX_FILE_DATA (MAX_DRAW_DATA + CUR_FHEADER_LEN)
#define MAX_META_DATA 200000

#define TOOL_PENCIL 0
#define TOOL_ERASER 1
#define TOOL_SLOW 2
#define TOOL_COUNT 3
#define TOOL_CHARS "pes"
#define LAYER_CHARS "bt"

#define CONTROLSCHEME_COUNT 2

// Glitch in citro2d (or so we assume) prevents us from writing into the first 8
// pixels in the texture. As such, we simply shift the texture over by this
// amount when drawing. HOWEVER: for extra safety, we just avoid double
#define LAYER_EDGEERROR 8
#define LAYER_EDGEBUF LAYER_EDGEERROR * 2

typedef u16 page_num;
typedef u8 layer_num;

// ----------- INIT -------------

struct ToolData default_tooldata[] = {
    // Pencil
    {2, LINESTYLE_STROKE, false},
    // Eraser
    {4, LINESTYLE_STROKE, true, 0},
    // Slow pen
    {2, LINESTYLE_STROKE, false}};

// Set some default values and malloc anything needed in the SystemState
void create_defaultsystemstate(struct SystemState *state) {
  colorsystem_init(&state->colors);
  state->colors.palette_size = DEFAULT_PALETTE_SPLIT;
  state->anim_loop = 0;
  colorsystem_setcolors_default(&state->colors);
  colorsystem_reset(&state->colors);
  state->draw_state.tools = malloc(sizeof(default_tooldata));

  // ALWAYS set default settings in case something doesn't load. We don't
  // necessarily know if load_settings does everything on the tin
  set_default_settings(state); // Just in case, load default settings

  PRINTINFO("Loading settings...");
  if (load_settings(state, SETTINGS_PATH) != 0) {
    LOGDBG("No settings.ini; using defaults");
    // Just in case, load default settings again (shouldn't hurt?)
    set_default_settings(state); 
  }
  PRINTCLEAR();
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
  if (_drw_cmd_cnt > _OBJSAFETY) {                                             \
    LOGTRACE("FLUSHING %ld DRAW CMDS PREMATURELY\n", _drw_cmd_cnt);            \
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
    // The offset from current page for onion skin. WARN: This used to max out
    // at the page but now it maxes out at animation loop
    int max_layers = get_systemstate_max_onionlayers(sys);
    for (int o = -max_layers; o <= -1; o++) {
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
}

// Draw as much as possible from the given ring buffer, with as little context
// switching as possible. WARN: MAKES A LOT OF ASSUMPTIONS IN ORDER TO PREVENT
// COSTLY MALLOCS PER FRAME
void draw_from_buffer(struct LineRingBuffer *scandata, struct LayerData *layers,
                      struct ScreenState *scrst) {
  u16 lineCount = 0;
  struct FullLine *lines[MAX_FRAMELINES]; // The largest available
  struct FullLine *next = NULL;

  // Repeat while there's something in the buffer and we haven't reached the
  // limit. Essentially, just pull as much as possible out of the ring buffer so
  // we can later draw it per-layer
  while ((next = lineringbuffer_shrink(scandata)) &&
         lineCount < _MAXDRAWLINES) {
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

// Fill the historical colors in colorsystem with colors from the given page.
void fill_colorhistory(struct ColorSystem *cs, char *draw_start, char *draw_end,
                       u16 page) {
  char *coldat = draw_start;
  char *colstroke = NULL;
  u32 history[COLORSYS_HISTORY];
  for (int lci = 0; lci < COLORSYS_HISTORY; lci++) {
    history[lci] = 0xFFFF;
  }
  u16 ihistory = 1; // id counter so newer colors overwrite older
  u8 histmax = 0;   // maximum color slot filled
  while (coldat && coldat < draw_end) {
    coldat =
        datamem_scanstroke(coldat, draw_end, MAX_DRAW_DATA, page, &colstroke);
    if (!colstroke) // No more to read
      break;
    // TODO: put this in draw sys as a function
    u16 hcol = chars_to_int(colstroke + 2, 3);
    if (hcol == 0) // Skip eraser
      continue;
    u32 hval = hcol | (ihistory << 16);
    for (int lci = 0; lci < histmax; lci++) {
      if ((history[lci] & 0xFFFF) == hcol) {
        history[lci] = hval;
        goto colscanloopend;
      }
    }
    // Need to add a new color.
    if (histmax < COLORSYS_HISTORY) { // Simple, have a slot
      history[histmax++] = hval;
    } else { // Have to find the oldest color ugh
      u32 mincol = UINT32_MAX;
      int imin = 0;
      for (int lci = 0; lci < COLORSYS_HISTORY; lci++) {
        if (history[lci] < mincol) {
          mincol = history[lci];
          imin = lci;
        }
      }
      history[imin] = hval;
    }
  colscanloopend:;
    // No more scanning if too much (don't want to lag)
    if (ihistory++ == 0xFFFF)
      break;
  }
  // Now we can sort the colors since top 16 bits is recent id
  for (int i = 1; i < COLORSYS_HISTORY; i++) {
    u32 x = history[i];
    int j = i;
    while (j > 0 && history[j - 1] < x) {
      history[j] = history[j - 1];
      j--;
    }
    history[j] = x;
  }
  // Then history is just lower 16 bits
  for (int lci = 0; lci < COLORSYS_HISTORY; lci++) {
    cs->history[lci] = history[lci] & 0xFFFF;
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

// A global to store the last savepath of any export
char last_savepath[MAX_FILEPATH] = {0};

int export_gif(struct ScreenState *scrst, struct GifSettings *settings,
               char *data, char *data_end, char *basename, int lastpage) {
  aptSetHomeAllowed(false);
  aptSetSleepAllowed(false);
  int ret = 0;

  PRINTINFO("Beginning gif export...");
  if (mkdir_p(SCREENSHOTS_BASE)) {
    PRINTERR("Couldn't create screenshots folder: %s", SCREENSHOTS_BASE);
    ret = 1;
    goto EXPORTGIFEND;
  }

  time_t now = time(NULL);
  sprintf(last_savepath, "%s%s_%jd.gif", SCREENSHOTS_BASE,
          strlen(basename) ? basename : "new", now);

  MsfGifState gifState = {};
  FILE *fp = fopen(last_savepath, "wb");
  if (fp == NULL) {
    PRINTERR("ERR: Couldn't open gif for writing: %s\n", last_savepath);
    ret = 1;
    goto EXPORTGIFEND;
  }
  msf_gif_begin_to_file(&gifState, scrst->layer_width, scrst->layer_height,
                        (MsfGifFileWriteFunc)fwrite, (void *)fp);
  if(lastpage == 0) {
    lastpage = last_total_page(data, data_end);
  }
  for (int i = 0; i <= lastpage; i++) {
    PRINTINFO("Exporting page %d / %d...", i + 1, lastpage + 1);
    u32 *page = export_page_raw(scrst, i, data, data_end);
    PRINTINFO("Packing gif page %d / %d...", i + 1, lastpage + 1);
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
  PRINTINFO("Exported gif to: %s", last_savepath);
EXPORTGIFEND:;
  aptSetHomeAllowed(true);
  aptSetSleepAllowed(true);
  aptCheckHomePressRejected();
  return ret;
}

#define _SOCKBLOCK(sock, block) { \
	int socflags = fcntl(sock, F_GETFL, 0); \
  if(socflags < 0) { \
	  PRINTERR("fcntl get: %d %s\n", errno, strerror(errno)); \
    goto SERVEEND; \
  } \
  socflags = block ? (socflags & ~O_NONBLOCK) : (socflags | O_NONBLOCK); \
	if(fcntl(sock, F_SETFL, socflags) < 0) { \
	  PRINTERR("fcntl set: %d %s\n", errno, strerror(errno)); \
    goto SERVEEND; \
  } \
}

#define _SOCKCHECK(result, name) { \
  if(result < 0) { \
    PRINTERR(name": %d %s\n", errno, strerror(errno)); \
    goto SERVEEND; \
  } \
}

void serveFileHttp() {
  PRINTCLEAR();

  if(strlen(last_savepath) == 0) {
    PRINTERR("No file exported this session!");
    return;
  }

  FILE *loadfile = fopen(last_savepath, "rb");
  if (loadfile == NULL) {
    PRINTERR("Couldn't open file %s", last_savepath);
    return; // No need to cleanup, haven't done anything yet
  }

  // Extension is everything past last dot
  char * extension = last_savepath + strlen(last_savepath);
  while(--extension != last_savepath) {
    if(*extension == '.') {
      extension++;
      break;
    }
  }

  static u32 *SOC_buffer = NULL;
  bool socInitialized = false;
  s32 sock = -1, csock = -1;
	struct sockaddr_in client;
	struct sockaddr_in server;
  u32 clientlen;
  char temp[4098];
  char http_200[] = "HTTP/1.1 200 OK\r\n";

  // We only allocate the SOC buffer once and leave it there...
  if(SOC_buffer == NULL) {
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
  }
  if(SOC_buffer == NULL) {
    PRINTERR("Can't allocate SOC buf");
    goto SERVEEND;
  }
  int ret;
  if((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
    PRINTERR("Can't init SOC");
    goto SERVEEND;
  }
  socInitialized = true;

  clientlen = sizeof(client);
	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sock < 0) {
    PRINTERR("socket: %d %s", errno, strerror(errno));
    goto SERVEEND;
	}

	memset (&server, 0, sizeof (server));
	memset (&client, 0, sizeof (client));

	server.sin_family = AF_INET;
	server.sin_port = htons (80);
	server.sin_addr.s_addr = gethostid();

  // char * filename = last_savepath + strlen(last_savepath);
  // while(--filename != last_savepath) {
  //   if(*filename == '/') {
  //     filename++;
  //     break;
  //   }
  // }
  PRINTINFO("BROWSER: http://%s/\n\n Press any key to stop", inet_ntoa(server.sin_addr));
  //PRINTINFO("FILE: %s\n BROWSER: http://%s/\n\n Press any key to stop", 
            //filename, inet_ntoa(server.sin_addr));

  if ( (ret = bind (sock, (struct sockaddr *) &server, sizeof (server))) ) {
		PRINTERR("bind: %d %s\n", errno, strerror(errno));
    goto SERVEEND;
	}

  // Set socket non blocking so we can still read input to exit
  _SOCKBLOCK(sock, 0);

  // Begin listen, let N clients connect at once
	if ( (ret = listen( sock, SOC_MAXCLIENTS)) ) {
		PRINTERR("listen: %d %s\n", errno, strerror(errno));
    goto SERVEEND;
	}

  aptSetHomeAllowed(false);

  while (aptMainLoop()) {
    hidScanInput();

    csock = accept (sock, (struct sockaddr *) &client, &clientlen);

		if (csock<0) {
			if(errno != EAGAIN) {
				PRINTERR("accept: %d %s\n", errno, strerror(errno));
        goto SERVEEND;
			}
		} else {
			// set client socket to blocking to simplify sending data back
      _SOCKBLOCK(csock, 1);
			LOGDBG("Connecting port %d from %s\n", client.sin_port, inet_ntoa(client.sin_addr));
			memset (temp, 0, sizeof(temp));

			ret = recv (csock, temp, sizeof(temp) - 2, 0);
      LOGDBG("RECV: %d bytes", ret);

      _SOCKCHECK(send(csock, http_200, strlen(http_200),0), "send");
      sprintf(temp, "Content-type: image/%s\r\n\r\n", extension);
      _SOCKCHECK(send(csock, temp, strlen(temp), 0), "send");

      // Now read from the given file into a small buffer and send it over and over.
      if(fseek(loadfile, 0, SEEK_SET)) {
        PRINTERR("Failed to seek to start: %s", last_savepath);
        goto SERVEEND;
      }
      while (1) {
        unsigned long count = fread(temp, 1, sizeof(temp) - 2, loadfile);
        if(count) {
          _SOCKCHECK(send(csock, temp, count, 0), "send");
        }
        if(count != sizeof(temp) - 2) {
          if (!feof(loadfile)) {
            PRINTERR("Failed to read file %s", last_savepath);
            goto SERVEEND;
          }
          break;
        } 
      }

			close(csock);
      LOGDBG("Closed %s", inet_ntoa(client.sin_addr));
			csock = -1;
		}

    if (aptCheckHomePressRejected()) {
      LOGDBG("HOME REJECTED");
    }

    if(hidKeysDownRepeat()) {
      break;
    }

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    // Draw here?
    // draw_easy_menu(&state);
    C3D_FrameEnd(0);
  }

  PRINTINFO("SHUTTING DOWN...");
  PRINTCLEAR();

  aptSetHomeAllowed(true);

SERVEEND:
  if(loadfile) {
    fclose(loadfile);
  }
  if(sock>0) { 
    LOGDBG("Closing socket %d", sock);
    close(sock); 
  }
  if(csock>0) { 
    LOGDBG("Closing csocket %d", sock);
    close(csock); 
  }
  if(socInitialized) {
    LOGDBG("Shutting down SOC");
    socExit();
  }
}

// -- MENU/PRINT STUFF --

void print_controls() {
  printf("\x1b[0m");
  printf("     L - color picker        R - general modifier\n");
  printf("LFT/RT - line width     UP/DWN - zoom (+R - page)\n");
  printf("SELECT - change layers   START - menu\n");
  printf("  ABXY - change tools    C-PAD - scroll canvas\n");
  printf(" R+B/A - undo/redo    COLP+L+R - change palette\n");
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
  sprintf(numbers, "%zu %zu  %s(%05.2f%%)", (size_t)unsaved, (size_t)datasize, active_x1b,
          percent);
  printf("\x1b[28;1H%s%s%*s", status_x1b, numbers,
         (int)(PRINTDATA_WIDTH - (strlen(numbers) - strlen(active_x1b))), "");
}

void print_status(u8 width, u8 layer, s8 zoom_power, u8 tool, u16 color,
                  u16 page, u16 undosize) {
  char tool_chars[TOOL_COUNT + 1];
  strcpy(tool_chars, TOOL_CHARS);

  char status_x1b[PSX1BLEN];
  char active_x1b[PSX1BLEN];
  char statusbg_x1b[PSX1BLEN];
  char activebg_x1b[PSX1BLEN];
  get_printmods(status_x1b, active_x1b, statusbg_x1b, activebg_x1b);

  printf("\x1b[30;1H%sW:%s%02d%s L:", status_x1b, active_x1b, width,
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
  printf("%s C:%s#%04x", status_x1b, active_x1b, color);
  printf("%s U:%s%02d", status_x1b, active_x1b, undosize);
}

void print_time(bool showcolon) {
  char status_x1b[PSX1BLEN];
  get_printmods(status_x1b, NULL, NULL, NULL);

  time_t rawtime = time(NULL);
  struct tm *timeinfo = localtime(&rawtime);

  printf("\x1b[30;46H%s%02d%c%02d", status_x1b, timeinfo->tm_hour,
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

// -- FILESYSTEM --

void get_save_location(char *savename, char *container) {
  container[0] = 0;
  sprintf(container, "%s%s/", SAVE_BASE, savename);
}

void get_rawfile_location(char *savename, char *container) {
  get_save_location(savename, container);
  strcpy(container + strlen(container), "raw");
}

void get_metafile_location(char *savename, char *container) {
  get_save_location(savename, container);
  strcpy(container + strlen(container), "meta");
}

int save_drawing(char *filename, char *data, metacontainer * mc) {
  char savefolder[MAX_FILEPATH];
  char fullpath[MAX_FILEPATH];
  char temp_msg[MAX_FILEPATH];

  LOGTRACE("SAVEFILENAME: %s", filename);

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
      if(result) {
        LOGDBG("Can't create save directory!");
        goto SAVEDRAWEND;
      } 
      result = write_file(fullpath, data);
      if(result) {
        LOGDBG("Can't create/write save file!");
        goto SAVEDRAWEND;
      } 
      PRINTINFO("Saving meta %s...", filename);
      metacontainer_addsimple(mc, METAKEY_SAVE);
      get_metafile_location(filename, fullpath);
      result = write_file(fullpath, mc->raw);
      if(result) {
        LOGDBG("Can't create meta file!");
        goto SAVEDRAWEND;
      } 
      LOGDBG("Save complete: %s", filename);
    SAVEDRAWEND:
      PRINTCLEAR();
      aptSetHomeAllowed(true);
      aptSetSleepAllowed(true);
      aptCheckHomePressRejected();
      return result;
    }
  }

  return -1;
}

char *load_drawing(char *data_container, char *final_filename, metacontainer * mc) {
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

  sprintf(temp_msg, "Found %zu saves:", (size_t)dircount);
  s32 sel = easy_menu(temp_msg, all_files, MAINMENU_TOP, MAX_MENULIST, 0,
                      KEY_START | KEY_B);

  if (sel < 0)
    goto END;

  final_filename[0] = 0;
  strcpy(final_filename, get_menu_item(all_files, MAX_ALLFILENAMES, sel));
  LOGTRACE("LOADFILENAME: %s", final_filename);

  PRINTINFO("Loading file %s...", final_filename)
  aptSetHomeAllowed(false);
  aptSetSleepAllowed(false);
  get_rawfile_location(final_filename, fullpath);
  result = read_file(fullpath, data_container, MAX_FILE_DATA);
  if(!result) {
    LOGDBG("Error loading main file!");
    goto LOADEND;
  }
  get_metafile_location(final_filename, fullpath);
  PRINTINFO("Loading meta %s...", final_filename)
  if(!read_file(fullpath, mc->raw, MAX_META_DATA)) {
    LOGDBG("Metadata file read error; skipping");
    mc->raw[0] = 0;
  }
  metacontainer_addsimple(mc, METAKEY_LOAD);
  if(strncmp(data_container, MAGICSTRING, 8) != 0) {
    // Try to convert file
    PRINTINFO("Converting to version 01...");
    result = convert_00_01(data_container, result, MAX_FILE_DATA);
    if(result == NULL) {
      LOGDBG("Conversion failure!");
    } else {
      LOGDBG("Converted file: v00->v01");
    }
  }
LOADEND:
  // TODO: will need to check version here in the future (but we only have one so...)
  PRINTCLEAR();
  aptSetHomeAllowed(true);
  aptSetSleepAllowed(true);
  aptCheckHomePressRejected();
  if(result) {
    LOGDBG("Load complete: %s", final_filename);
  }

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

  PRINTINFO("Exporting page %d: building...", page + 1);

  if (mkdir_p(SCREENSHOTS_BASE)) {
    PRINTERR("Couldn't create screenshots folder: %s", SCREENSHOTS_BASE);
    ret = 1;
    goto EXPORTPAGEEND;
  }

  //char savepath[MAX_FILEPATH];
  time_t now = time(NULL);
  sprintf(last_savepath, "%s%s_%d_%jd.png", SCREENSHOTS_BASE,
          strlen(basename) ? basename : "new", page + 1, now);

  u32 *exported = export_page_raw(scrst, page, data, data_end);
  if (exported == NULL) {
    PRINTERR("Couldn't export page %d (unknown error)", page + 1);
    ret = 1;
    goto EXPORTPAGEEND;
  }

  PRINTINFO("Exporting page %d: converting to png...", page + 1);

  if (write_citropng(exported, scrst->layer_width, scrst->layer_height,
                     last_savepath) == 0) {
    PRINTINFO("Exported page to: %s", last_savepath);
  } else {
    PRINTERR("FAILED to export: %s", last_savepath);
    ret = 1;
    goto EXPORTPAGEEND;
  }
EXPORTPAGEEND:;
  aptSetHomeAllowed(true);
  aptSetSleepAllowed(true);
  aptCheckHomePressRejected();
  return ret;
}

// ---------------- MENUS ---------------------

bool run_edit_menu(struct SystemState *sys, char * draw_data, char ** draw_end) {
  char menu[256];
  s32 menuopt = 0;
  static u16 source_page = 0;
  u16 dest_page = sys->draw_state.page;
  while (1) {
    // Recreate menu every time, since we have dynamic values. To make life
    // easier, we just sprintf everything into the array with newlines, then
    // replace newlines with 0
            //"Set source page: %d\nPaste  p%d -> p%d\nSwap   p%d -> p%d\nDelete p%d\n"
    sprintf(menu,
            "Set source page: %d\nPaste  (%d->%d)\nSwap   (%d->%d)\n"
            "Shift  (%d->%d)\nDelete (%d)\n"
            "Exit\n", 
            source_page + 1, 
            source_page + 1, dest_page + 1,
            source_page + 1, dest_page + 1, 
            source_page + 1, dest_page + 1,
            dest_page + 1);
    for (int x = strlen(menu); x >= 0; x--) {
      if (menu[x] == '\n')
        menu[x] = 0;
    }
    menuopt =
        easy_menu("Edit", menu, MAINMENU_TOP, 0, menuopt, KEY_B | KEY_START);
    switch (menuopt) {
    case 0: // color picker mode
      source_page = sys->draw_state.page;
      break;
    case 1:
      if(easy_warn("WARN: PASTE OVER PAGE", 
                   "This action cannot be undone!!\n\n Really paste page here?", MAINMENU_TOP)) {
        *draw_end = copy_page(draw_data, *draw_end, source_page, dest_page);
        return true;
      }
      break;
    case 2:
      if(easy_warn("WARN: SWAP PAGES", 
                   "This action cannot be undone!!\n\n Really swap pages?", MAINMENU_TOP)) {
        swap_pages(draw_data, *draw_end, source_page, dest_page);
        return true;
      }
      break;
    case 3:
      if(easy_warn("WARN: MOVE PAGE", 
                   "This action cannot be undone!!\n\n Really move page?", MAINMENU_TOP)) {
        move_page(draw_data, *draw_end, source_page, dest_page);
        return true;
      }
      break;
    case 4:
      if(easy_warn("WARN: DELETE PAGE", 
                   "This action CANNOT BE UNDONE!!\n\n Really DELETE current page?", MAINMENU_TOP)) {
        *draw_end = delete_page(draw_data, *draw_end, dest_page);
        return true;
      }
      break;
    default:
      return false;
    }
  }
}

void run_options_menu(struct SystemState *sys) {
  char menu[256];
  char colpickers[][16] = {"Palette", "RGB", "Auto Palette"};
  char controlschemes[][16] = {"Default", "Toggle"};
  char datesettings[][16] = {"off", "small", "large", "extra"};
  char datecolorsettings[][16] = {"pen", "black", "white", "red"};
  float newonionstart;
  s32 menuopt = 0;
  bool settings_changed = false;
  while (1) {
    // Recreate menu every time, since we have dynamic values. To make life
    // easier, we just sprintf everything into the array with newlines, then
    // replace newlines with 0
    sprintf(menu,
            "Color Picker: %s\nOnion layers: %d\nOnion darkness: "
            "%.1f\nSlow pen friction: %.2f\nPower saving: %s\n"
            "Date stamp: %s\nDate color: %s\n"
            "Control Scheme: %s\nReset to defaults\nExit\n",
            colpickers[sys->colors.mode], 
            sys->onion_count, sys->onion_blendstart,
            1 - sys->slow_avg, sys->power_saver ? "on" : "off",
            datesettings[sys->datestamp],
            datecolorsettings[sys->datestamp_color],
            controlschemes[sys->control_scheme]);
    for (int x = strlen(menu); x >= 0; x--) {
      if (menu[x] == '\n')
        menu[x] = 0;
    }
    menuopt =
        easy_menu("Options", menu, MAINMENU_TOP, MAX_MENULIST, menuopt, KEY_B | KEY_START);
    switch (menuopt) {
    case 0: // color picker mode
      settings_changed = true;
      sys->colors.mode = (sys->colors.mode + 1) % COLORSYSMODE_COUNT;
      break;
    case 1: // onion layers
      settings_changed = true;
      sys->onion_count = (sys->onion_count + 1) % (MAXONION + 1);
      break;
    case 2: // onion darkness
      settings_changed = true;
      newonionstart = sys->onion_blendstart + 0.1;
      set_systemstate_onionstart(sys, newonionstart);
      // We hit some preset cap, so reset to bottom
      if (fabs(newonionstart - sys->onion_blendstart) > 0.01) {
        set_systemstate_onionstart(sys, 0.1);
      }
      break;
    case 3: // slow avg
      settings_changed = true;
      sys->slow_avg += 0.05;
      if (sys->slow_avg > 0.51) {
        sys->slow_avg = 0.05;
      }
      break;
    case 4: // power saver
      settings_changed = true;
      sys->power_saver = !sys->power_saver;
      osSetSpeedupEnable(!sys->power_saver);
      break;
    case 5: // date stamp
      settings_changed = true;
      sys->datestamp = (sys->datestamp + 1) % 4;
      break;
    case 6: // date color stamp
      settings_changed = true;
      sys->datestamp_color = (sys->datestamp_color + 1) % 4;
      break;
    case 7: // control scheme
      settings_changed = true;
      sys->control_scheme = (sys->control_scheme + 1) % CONTROLSCHEME_COUNT;
      break;
    case 8: // defaults
      settings_changed = true;
      set_default_settings(sys);
      break;
    default:
      if (settings_changed) {
        PRINTINFO("Saving settings...");
        if (save_settings(sys, SETTINGS_PATH) != 0) {
          LOGDBG("ERR: Can't save settings!");
        }
        PRINTCLEAR();
      }
      return;
    }
  }
}

void run_runtime_options_menu(struct SystemState *sys, int lastpage) {
  char menu[256];
  char visibility[][3] = {"", "b", "t", "bt"};
  char modes[][16] = {"Normal", "Animation", "Small Animation"};
  s32 menuopt = 0;
  while (1) {
    // Recreate menu every time, since we have dynamic values. To make life
    // easier, we just sprintf everything into the array with newlines, then
    // replace newlines with 0
    sprintf(menu,
            "Draw Mode: %s\nLayer visibility: %s\n"
            "Page Loop+: %d\nPage Loop-: %d\n"
            "Page Loop Clear\nPage Loop Last Pg\nExit\n",
            modes[sys->draw_state.mode], 
            visibility[sys->screen_state.layer_visibility],
            sys->anim_loop, sys->anim_loop);
    for (int x = strlen(menu); x >= 0; x--) {
      if (menu[x] == '\n')
        menu[x] = 0;
    }
    menuopt =
        easy_menu("Runtime Options", menu, MAINMENU_TOP, 0, menuopt, KEY_B | KEY_START);
    switch (menuopt) {
    case 0:
      inc_drawstate_mode(&sys->screen_state, &sys->draw_state);
      break;
    case 1: // layer visibility
      sys->screen_state.layer_visibility =
          (sys->screen_state.layer_visibility + 1) & ((1 << LAYER_COUNT) - 1);
      break;
    case 2: // anim loop up
      sys->anim_loop++;
      break;
    case 3: // anim loop down
      if(sys->anim_loop > 0) { sys->anim_loop--; }
      break;
    case 4: // anim loop 0
      sys->anim_loop = 0;
      break;
    case 5: // anim loop 0
      sys->anim_loop = lastpage + 1;
      break;
    default:
      return;
    }
  }
}

void run_export_menu(struct SystemState *sys, char *draw_data, char *draw_data_end,
                  char *filename) {
  char menu[256];
  s32 menuopt = 0;
  static struct GifSettings settings;
  static bool setup = false;
  if(!setup) {
    settings.bitdepth = 16;
    settings.csecsperframe = 5;
    setup = true;
  }

  while (1) {
    // Recreate menu every time, since we have dynamic values. To make life
    // easier, we just sprintf everything into the array with newlines, then
    // replace newlines with 0
    sprintf(menu,
            "Export PNG\nExport GIF\nDownload Export\nGIF Colors: %d\n"
            "GIF Frame time +100: %dms\nGIF Frame time +10: %dms\nExit\n",
            1 << settings.bitdepth, settings.csecsperframe * 10,
            settings.csecsperframe * 10);
    for (int x = strlen(menu); x >= 0; x--) {
      if (menu[x] == '\n')
        menu[x] = 0;
    }
    menuopt = easy_menu("Export options", menu, MAINMENU_TOP, 0, menuopt,
                        KEY_B | KEY_START);
    switch (menuopt) {
    case 0:
      export_page(&sys->screen_state, sys->draw_state.page, draw_data,
                  draw_data_end, filename);
      return;
    case 1:
      export_gif(&sys->screen_state, &settings, draw_data, draw_data_end,
                 filename, sys->anim_loop ? sys->anim_loop - 1 : 0);
      return;
    case 2:
      serveFileHttp();
      return;
    case 3: // Colors
      settings.bitdepth <<= 1;
      if (settings.bitdepth > 16)
        settings.bitdepth = 2;
      break;
    case 4: // Frame time +100
      settings.csecsperframe+=9;
    case 5: // Frame time +10
      settings.csecsperframe++;
      if (settings.csecsperframe > 200)
        settings.csecsperframe -= 200;
      break;
    // case 2: // shortcut to fix animation size
    //   inc_drawstate_mode(&sys->screen_state, &sys->draw_state);
    //   break;
    default:
      return;
    }
  }
}

#define CTRL_UNDO     (1 << 1)
#define CTRL_REDO     (1 << 2)
#define CTRL_MENU     (1 << 3)
#define CTRL_PALETTE  (1 << 4)
#define CTRL_ZOOMIN   (1 << 5)
#define CTRL_ZOOMOUT  (1 << 6)
#define CTRL_PAGEUP   (1 << 7)
#define CTRL_PAGEDOWN (1 << 8)
#define CTRL_WIDTHUP  (1 << 9)
#define CTRL_WIDTHDOWN (1 << 10)
#define CTRL_PENCIL   (1 << 11)
#define CTRL_ERASER   (1 << 12)
#define CTRL_SLOWPEN  (1 << 13)
#define CTRL_NEXTPALETTE (1 << 14)
#define CTRL_LAYER    (1 << 15)
#define CTRL_PREVCOLOR (1 << 16)

// Return a constant representing the action the user took 
u32 get_controls(struct SystemState * state, u32 kDown, u32 kUp, u32 kRepeat, u32 kHeld) {
  u32 ctrl = 0;
  // These change with control schemes
  if(state->control_scheme == 0) {
    if (kDown & KEY_A) {
      if (kHeld & (KEY_R | KEY_ZR)) {
        ctrl |= CTRL_REDO;
      } else {
        ctrl |= CTRL_PENCIL;
      }
    }
    if (kDown & KEY_B) {
      if (kHeld & (KEY_R | KEY_ZR)) {
        ctrl |= CTRL_UNDO;
      } else {
        ctrl |= CTRL_ERASER;
      }
    }
    if (kDown & KEY_Y) {
      ctrl |= CTRL_SLOWPEN;
    }
  } else if(state->control_scheme == 1) {
    if(kDown & KEY_X) { 
      ctrl |= CTRL_UNDO;
    }
    if(kDown & KEY_Y) { 
      ctrl |= CTRL_REDO;
    }
    if(kDown & KEY_B) { 
      ctrl |= CTRL_PREVCOLOR;
    }
    if(kDown & KEY_A) {
      switch(state->draw_state.current_tool - state->draw_state.tools) {
      case TOOL_PENCIL:
        ctrl |= CTRL_ERASER;
        break;
      case TOOL_ERASER:
        ctrl |= CTRL_SLOWPEN;
        break;
      case TOOL_SLOW:
        ctrl |= CTRL_PENCIL;
        break;
      }
    }
  }
  // These don't change with control schemes (yet)
  if (kDown & (KEY_L | KEY_ZL) && !(kHeld & (KEY_R | KEY_ZR))) {
    ctrl |= CTRL_PALETTE;
  }
  if (kRepeat & KEY_DUP) {
    if (kHeld & (KEY_R | KEY_ZR)) {
      ctrl |= CTRL_PAGEUP;
    } else {
      ctrl |= CTRL_ZOOMIN;
    }
  }
  if (kRepeat & KEY_DDOWN) {
    if (kHeld & (KEY_R | KEY_ZR)) {
      ctrl |= CTRL_PAGEDOWN;
    } else {
      ctrl |= CTRL_ZOOMOUT;
    }
  }
  if (kRepeat & KEY_DRIGHT) {
    ctrl |= CTRL_WIDTHUP;
  }
  if (kRepeat & KEY_DLEFT) {
    ctrl |= CTRL_WIDTHDOWN;
  }
  if (kHeld & (KEY_L | KEY_ZL) && kDown & (KEY_R | KEY_ZR)) {
    ctrl |= CTRL_NEXTPALETTE;
  }
  if (kDown & KEY_SELECT) {
    ctrl |= CTRL_LAYER;
  }
  if (kDown & KEY_START) {
    ctrl |= CTRL_MENU;
  }
  return ctrl;
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
#define MAIN_UPDOWN(x, dopage)                                                 \
  {                                                                            \
    if (dopage) {                                                              \
      int diff = x * ((kHeld & KEY_L) ? 10 : 1);                               \
      if(sys.anim_loop > 0) {                                                  \
        sys.draw_state.page = (sys.draw_state.page + diff + sys.anim_loop) %   \
                               sys.anim_loop;                                  \
      } else {                                                                 \
        sys.draw_state.page = UTILS_CLAMP(                                     \
            sys.draw_state.page + diff, 0, MAX_PAGE);                          \
      }                                                                        \
      reset_ringstack(&undostack);                                             \
      reset_ringstack(&redostack);                                             \
      FLUSH_LAYERS();                                                          \
    } else {                                                                   \
      sys.draw_state.zoom_power = UTILS_CLAMP(sys.draw_state.zoom_power + x,   \
                                              MIN_ZOOMPOWER, MAX_ZOOMPOWER);   \
    }                                                                          \
  }

#define PRINT_DATAUSAGE() print_data(draw_data, draw_data_end, saved_last);

#define MAIN_NEWDRAW()                                                         \
  {                                                                            \
    reset_ringstack(&undostack);                                               \
    reset_ringstack(&redostack);                                               \
    /* Do this in here JUST IN CASE we support multiple formats */             \
    /* WARN: CAN'T USE 0 AS PADDING CHAR!!! */                                 \
    CUR_PUTFHEADER(data_container);                                            \
    draw_data = data_container + CUR_FHEADER_LEN;                              \
    draw_data_end = saved_last = draw_data;                                    \
    *draw_data = 0; /* Not strictly necessary */                               \
    meta.raw[0] = 0;                                                           \
    metacontainer_addsimple(&meta, METAKEY_NEW);                               \
    set_default_drawstate(&sys.draw_state);                                    \
    /*colorsystem_reset(&sys.colors);*/                                        \
    set_screenstate_defaults(&sys.screen_state);                               \
    FLUSH_LAYERS();                                                            \
    save_filename[0] = '\0';                                                   \
    pending.line_count = 0;                                                    \
    current_frame = end_frame = 0;                                             \
    is_new_date = true;                                                        \
    printf("\x1b[1;1H");                                                       \
    /*printf("--------------------- Start ----------------------");*/          \
    print_controls();                                                          \
    print_framing();                                                           \
    PRINT_DATAUSAGE();                                                         \
  }

#define MAIN_UNSAVEDCHECK(x)                                                   \
  (saved_last == draw_data_end ||                                              \
   easy_warn("WARN: UNSAVED DATA", x, MAINMENU_TOP))

// An optimization (inside): if draw_pointer was already at the end, don't
// need to RE-draw what we've already drawn, move it forward with
// the mem write. Note: there are instances where we WILL be drawing
// twice, but it's difficult to determine what has or has not been
// drawn when the pointer isn't at the end.
#define MAIN_WRITEPENDING(force) { \
  char *cvl_end = convert_linepack_to_data(&pending, stroke_data, MAX_STROKE_DATA); \
  if (cvl_end == NULL) { \
    LOGDBG("ERR: Couldn't convert lines!\n"); \
  } else { \
    char *previous_end = draw_data_end; \
    draw_data_end = write_to_datamem(stroke_data, cvl_end, sys.draw_state.page, draw_data, draw_data_end); \
    if (!force && previous_end == draw_pointers[0]) draw_pointers[0] = draw_data_end; \
    PRINT_DATAUSAGE(); \
  } \
  pending.line_count = 0; \
}

int main(int argc, char **argv) {
  gfxInitDefault();
  hidSetRepeatParameters(BREPEAT_DELAY, BREPEAT_INTERVAL);

  // Enable the higher clock speed on New 3DS
  osSetSpeedupEnable(true);

  bool isn3ds = false;
  bool isn3dssucceed = false;
  Result res = APT_CheckNew3DS(&isn3ds);

  if(R_SUCCEEDED(res)) {
    isn3dssucceed = true;
    //if(isn3ds) {
    //  _OBJLIMIT = N3DS_C2DOBJLIMIT;
    //  _OBJSAFETY = N3DS_C2DOBJLIMITSAFETY;
    //  _MAXDRAWLINES = N3DS_MAXDRAWLINES;
    //}
  } 

  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(_OBJLIMIT);
  C2D_Prepare();

  consoleInit(GFX_TOP, NULL);
  C3D_RenderTarget *screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  LOGTRACE("INITIALIZED");

  if(isn3ds) {
    LOGDBG("New 3ds detected");
  } else if(!isn3dssucceed) {
    LOGDBG("Failed to check 3ds version");
  }

  struct SystemState sys;

  create_defaultsystemstate(&sys);
  set_default_drawstate(&sys.draw_state);
  set_screenstate_defaults(&sys.screen_state);
  set_cpadprofile_canvas(&sys.cpad);

  // Very silly global so rect drawing functions know screen dimensions and such
  _drawrect_scrst = &sys.screen_state;

  LOGTRACE("SET SCREENSTATE/CANVAS");

  // weird byte order? 16 bits of color are at top
  const u32 layer_color = rgba32c_to_rgba16c_32(CANVAS_LAYER_COLOR);

  const Tex3DS_SubTexture subtex = {TEXTURE_WIDTH, TEXTURE_HEIGHT, 0.0f,
                                    1.0f,          1.0f,           0.0f};

  struct LayerData layers[LAYER_COUNT];

  for (int i = 0; i < LAYER_COUNT; i++)
    create_layer(layers + i, subtex);

  LOGTRACE("CREATED LAYERS");

  bool touching = false;
  bool palette_active = false;
  bool flush_layers = true;
  bool close_palette = false;
  bool is_new_date = false;

  u32 current_frame = 0;
  u32 end_frame = 0;
  s8 last_zoom_power = 0;
  char tempc; // Used for anything, very short life

  struct LinePackage pending;
  init_linepackage(&pending);
  if (!pending.lines) {
    LOGDBG("ERR: Couldn't allocate stroke lines");
  }

  struct LineRingBuffer scandata;
  init_lineringbuffer(&scandata, _MAXDRAWLINES);
  if (!scandata.lines) {
    LOGDBG("ERR: COULD NOT INIT LINERINGBUFFER");
  }
  if (!scandata.pending.lines) {
    LOGDBG("ERR: COULD NOT INIT LRB PENDING");
  }

  struct RingStack undostack, redostack;
  init_ringstack(&undostack, MAX_UNDO);
  init_ringstack(&redostack, MAX_UNDO);
  if (!undostack.positions) {
    LOGDBG("ERR: COULD NOT INIT UNDOBUFFER");
  }

  char *save_filename = malloc(MAX_FILENAME * sizeof(char));
  char *data_container = malloc(MAX_FILE_DATA * sizeof(char));
  char *stroke_data = malloc(MAX_STROKE_DATA * sizeof(char));
  char *draw_data;     // This goes to the start of actual drawing data, past file header
  char *draw_data_end; // NOTE: this is exclusive: it points one past the end.
                       // draw_data_end - draw_data = length
  char *saved_last;
  metacontainer meta;

  // Draw pointers for us and all our onion friends
  char *draw_pointers[1 + MAXONION];

  if (!save_filename || !data_container || !stroke_data || 
      metacontainer_init(&meta, MAX_META_DATA)) {
    LOGDBG("ERR: COULD NOT INIT MAIN BUFFER");
  }

  LOGTRACE("SYSTEM MALLOC");

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

    u32 control = get_controls(&sys, kDown, kUp, kRepeat, kHeld);

    // Respond to user input
    if (control & CTRL_PALETTE) {
      palette_active = !palette_active;
      if (palette_active) { // Only calculate the last cols on button press
        fill_colorhistory(&sys.colors, draw_data, draw_data_end,
                          sys.draw_state.page);
      }
    }
    if (control & CTRL_ZOOMIN) {
      MAIN_UPDOWN(1, 0)
    }
    if (control & CTRL_ZOOMOUT) {
      MAIN_UPDOWN(-1, 0)
    }
    if (control & CTRL_PAGEUP) {
      MAIN_UPDOWN(1, 1)
    }
    if (control & CTRL_PAGEDOWN) {
      MAIN_UPDOWN(-1, 1)
    }
    if (control & CTRL_PREVCOLOR) {
      sys.colors.selected_index = (sys.colors.selected_index + 1) % COLORSYS_SELECTIONS;
    }
    if (control & CTRL_WIDTHUP)
      shift_drawstate_width(&sys.draw_state, (kHeld & KEY_R ? 5 : 1));
    if (control & CTRL_WIDTHDOWN)
      shift_drawstate_width(&sys.draw_state, -(kHeld & KEY_R ? 5 : 1));
    if (control & CTRL_REDO) {
      char *redo = ringstack_pop(&redostack);
      if (redo != NULL) {
        ringstack_push(&undostack, draw_data_end);
        draw_data_end = redo;
        FLUSH_LAYERS();
        PRINT_DATAUSAGE();
      } else {
        LOGDBG("ERR: No redos in buffer!\n");
      }
    }
    if (control & CTRL_UNDO) {
      char *undo = ringstack_pop(&undostack);
      if (undo != NULL) {
        ringstack_push(&redostack, draw_data_end);
        draw_data_end = undo;
        FLUSH_LAYERS();
        PRINT_DATAUSAGE();
      } else {
        LOGDBG("ERR: No undos in buffer!\n");
      }
    }
    if (control & CTRL_PENCIL) {
      set_drawstate_tool(&sys.draw_state, TOOL_PENCIL);
    }
    if (control & CTRL_ERASER) {
      set_drawstate_tool(&sys.draw_state, TOOL_ERASER);
    }
    if (control & CTRL_SLOWPEN) {
      set_drawstate_tool(&sys.draw_state, TOOL_SLOW);
    }
    if ((control & CTRL_NEXTPALETTE) && palette_active) {
      colorsystem_nextpalette(&sys.colors, 1);
    }
    if (control & CTRL_LAYER) {
      sys.draw_state.layer = (sys.draw_state.layer + 1) % LAYER_COUNT;
    }
    if (control & CTRL_MENU) {
      switch (easy_menu(MAINMENU_TITLE, MAINMENU_ITEMS, MAINMENU_TOP, 0, 0,
                        KEY_B | KEY_START)) {
      case MAINMENU_EDIT:
        if(run_edit_menu(&sys, draw_data, &draw_data_end)) {
          // Full reset on these big edits...
          reset_ringstack(&undostack);
          reset_ringstack(&redostack);
          FLUSH_LAYERS();
          PRINT_DATAUSAGE();
        }
        break;
      case MAINMENU_NEW:
        if (MAIN_UNSAVEDCHECK("Are you sure you want to start anew?"))
          MAIN_NEWDRAW();
        break;
      case MAINMENU_SAVE:
        tempc = *draw_data_end;
        *draw_data_end = 0;
        if (save_drawing(save_filename, data_container, &meta) == 0) {
          saved_last = draw_data_end;
          PRINT_DATAUSAGE(); // Should this be out in the main loop?
        } else {
          PRINTERR("SAVE FAILED!");
        }
        *draw_data_end = tempc;
        break;
      case MAINMENU_LOAD:
        if (MAIN_UNSAVEDCHECK(
                "Are you sure you want to load and lose changes?")) {
          MAIN_NEWDRAW();
          draw_data_end = load_drawing(data_container, save_filename, &meta);

          if (draw_data_end == NULL) {
            PRINTERR("LOAD FAILED!");
            MAIN_NEWDRAW();
          } else {
            saved_last = draw_data_end;
            // Find last USED page (that the user touched), set it.
            sys.draw_state.page =
                last_used_page(draw_data, draw_data_end - draw_data);
            is_new_date = metacontainer_lastloads_differentdate(&meta);
            PRINT_DATAUSAGE();
          }
        }
        break;
      case MAINMENU_EXPORT:
        run_export_menu(&sys, draw_data, draw_data_end, save_filename);
        break;
      case MAINMENU_OPTIONS:
        // Run options system
        run_options_menu(&sys);
        FLUSH_LAYERS(); // Why not...
        break;
      case MAINMENU_RUNTIMEOPTIONS:
        // Run runtime options system (settings here not saved)
        run_runtime_options_menu(&sys, last_total_page(draw_data, draw_data_end));
        FLUSH_LAYERS(); // Why not...
        break;
      case MAINMENU_EXIT:
        if (MAIN_UNSAVEDCHECK("Really quit?"))
          goto ENDMAINLOOP;
        break;
      }
    }

    u16 curcol = colorsystem_getcolor(&sys.colors);
    if (kDown & KEY_TOUCH) {
      if (!palette_active) {
        reset_ringstack(&redostack);
        ringstack_push(&undostack, draw_data_end);
        u16 pcolor = sys.draw_state.current_tool->has_static_color
          ? sys.draw_state.current_tool->static_color : curcol;
        pending.layer = sys.draw_state.layer;
        // WARN: here is where we do date stamping! We reuse the pending
        // buffer for our one-time write into datamem, then undo it
        if(is_new_date && sys.datestamp) {
          LOGDBG("Stamping date into canvas");
          pending.width = 1;
          pending.style = LINESTYLE_COLLECTION;
          switch(sys.datestamp_color) {
            case 1: pending.color = rgb_to_rgba16(0, 0, 0); break;
            case 2: pending.color = rgb_to_rgba16(31,31,31); break;
            case 3: pending.color = rgb_to_rgba16(31,0,0); break;
            default: pending.color = pcolor; break;
          }
          char _tempdate[16];
          _tempdate[0] = 0;
          is_new_date = false;
          if(get_yyyymmdd(_tempdate)) {
            LOGDBG("Couldn't get date for stamping!");
          } else {
            push_digits(_tempdate, &pending, 
                        sys.screen_state.offset_x / sys.screen_state.zoom + 15, 
                        sys.screen_state.offset_y / sys.screen_state.zoom + 5, sys.datestamp);
            MAIN_WRITEPENDING(1);
            ringstack_push(&undostack, draw_data_end);
          }
        }
        pending.color = pcolor;
        pending.width = sys.draw_state.current_tool->width;
        pending.style = sys.draw_state.current_tool->style;
      }
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

    if (kRepeat & ~(KEY_TOUCH) || !current_frame || (kUp & KEY_TOUCH)) {
      print_status(sys.draw_state.current_tool->width, sys.draw_state.layer,
                   sys.draw_state.zoom_power,
                   sys.draw_state.current_tool - sys.draw_state.tools, curcol,
                   sys.draw_state.page, undostack.size);
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
        int action = update_colorpicker(&sys.colors, &current_touch);
        if (action == 2 ||
            (action == 1 && sys.colors.mode == COLORSYSMODE_PALETTE))
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

    int oend = get_systemstate_max_onionlayers(&sys);
    int dp_ofs = 0;

    // Find the place to stop looking for the appropriate draw pointer to work
    // on. This has complicated rules
    for (dp_ofs = 0; dp_ofs <= oend; dp_ofs++) {
      onion_offset(&sys.draw_state, -dp_ofs, &_msr_ofsx,
                   &_msr_ofsy); // This works for 0 (returns 0,0)
      if (dp_ofs == oend) // Don't bother with any logic below, this is the last slot.
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

    int drawpage = sys.draw_state.page - dp_ofs;
    if(sys.anim_loop > 0 && sys.anim_loop > sys.draw_state.page) {
      drawpage = (drawpage + sys.anim_loop) % sys.anim_loop;
    } else {
      drawpage = DCV_MAX(drawpage, 0);
    }
    //TickCounter timer;
    //osTickCounterStart(&timer);
    draw_pointers[dp_ofs] = scan_lines(
      &scandata, draw_pointers[dp_ofs], draw_data_end, drawpage);
    // osTickCounterUpdate(&timer);
    // bool saydraw = lineringbuffer_size(&scandata) > 0;
    // if(saydraw) { 
    //   LOGDBG("SCAN: %0.2fms", osTickCounterRead(&timer)); 
    // }
    //osTickCounterStart(&timer);
    draw_from_buffer(&scandata, layers, &sys.screen_state);
    // osTickCounterUpdate(&timer);
    // if(saydraw) {
    //   LOGDBG("DRAW: %0.2fms", osTickCounterRead(&timer));
    // }

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
      MAIN_WRITEPENDING(0);
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
  free_ringstack(&undostack);
  free_ringstack(&redostack);
  free_linepackage(&pending);
  free(save_filename);
  free(data_container);
  free(stroke_data);
  metacontainer_free(&meta);

  for (int i = 0; i < LAYER_COUNT; i++)
    delete_layer(layers[i]);

  C3D_RenderTargetDelete(screen);

  C2D_Fini();
  C3D_Fini();
  gfxExit();
  return 0;
}
