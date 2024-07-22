#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <dirent.h>

//#define DEBUG_COORD
//#define DEBUG_DATAPRINT

#include "filesys.h"
#include "console.h"
#include "input.h"
#include "color.h"

#include "setup.h"
#include "draw.h"

#include "system.h"

// TODO: Figure out these weirdness things:
// - Can't draw on the first 8 pixels along the edge of a target, system crashes
// - Fill color works perfectly fine using line/rect calls, but ClearTarget
//   MUST have the proper 16 bit format....
// - ClearTarget with a transparent color seems to make the color stick using
//   DrawLine unless a DrawRect (or perhaps other) call is performed.
//    THIS IS FIXED IN A LATER REVISION

//Some globals for the um... idk.
u8 _db_prnt_row = 0;


#define MY_C2DOBJLIMIT 8192
#define MY_C2DOBJLIMITSAFETY MY_C2DOBJLIMIT - 100
#define PSX1BLEN 30

#define DEFAULT_PALETTE_SPLIT 64 
#define DEFAULT_START_LAYER 1
#define DEFAULT_PALETTE_STARTINDEX 1

#define TOOL_PENCIL 0
#define TOOL_ERASER 1
#define TOOL_COUNT 2
#define TOOL_CHARS "pe"

// Glitch in citro2d (or so we assume) prevents us from writing into the first 8
// pixels in the texture. As such, we simply shift the texture over by this amount
// when drawing. HOWEVER: for extra safety, we just avoid double
#define LAYER_EDGEERROR 8
#define LAYER_EDGEBUF LAYER_EDGEERROR * 2

typedef u16 page_num;
typedef u8 layer_num;

#pragma region INIT

// ----------- INIT -------------

u32 default_palette[] = { 
   //Endesga 64
   0xff0040, 0x131313, 0x1b1b1b, 0x272727, 0x3d3d3d, 0x5d5d5d, 0x858585, 0xb4b4b4,
   0xffffff, 0xc7cfdd, 0x92a1b9, 0x657392, 0x424c6e, 0x2a2f4e, 0x1a1932, 0x0e071b,
   0x1c121c, 0x391f21, 0x5d2c28, 0x8a4836, 0xbf6f4a, 0xe69c69, 0xf6ca9f, 0xf9e6cf,
   0xedab50, 0xe07438, 0xc64524, 0x8e251d, 0xff5000, 0xed7614, 0xffa214, 0xffc825,
   0xffeb57, 0xd3fc7e, 0x99e65f, 0x5ac54f, 0x33984b, 0x1e6f50, 0x134c4c, 0x0c2e44,
   0x00396d, 0x0069aa, 0x0098dc, 0x00cdf9, 0x0cf1ff, 0x94fdff, 0xfdd2ed, 0xf389f5,
   0xdb3ffd, 0x7a09fa, 0x3003d9, 0x0c0293, 0x03193f, 0x3b1443, 0x622461, 0x93388f,
   0xca52c9, 0xc85086, 0xf68187, 0xf5555d, 0xea323c, 0xc42430, 0x891e2b, 0x571c27,

   //Resurrect
   0x2e222f, 0x3e3546, 0x625565, 0x966c6c, 0xab947a, 0x694f62, 0x7f708a, 0x9babb2,
   0xc7dcd0, 0xffffff, 0x6e2727, 0xb33831, 0xea4f36, 0xf57d4a, 0xae2334, 0xe83b3b,
   0xfb6b1d, 0xf79617, 0xf9c22b, 0x7a3045, 0x9e4539, 0xcd683d, 0xe6904e, 0xfbb954,
   0x4c3e24, 0x676633, 0xa2a947, 0xd5e04b, 0xfbff86, 0x165a4c, 0x239063, 0x1ebc73,
   0x91db69, 0xcddf6c, 0x313638, 0x374e4a, 0x547e64, 0x92a984, 0xb2ba90, 0x0b5e65,
   0x0b8a8f, 0x0eaf9b, 0x30e1b9, 0x8ff8e2, 0x323353, 0x484a77, 0x4d65b4, 0x4d9be6,
   0x8fd3ff, 0x45293f, 0x6b3e75, 0x905ea9, 0xa884f3, 0xeaaded, 0x753c54, 0xa24b6f,
   0xcf657f, 0xed8099, 0x831c5d, 0xc32454, 0xf04f78, 0xf68181, 0xfca790, 0xfdcbb0,

   //AAP-64
   0x060608, 0x141013, 0x3b1725, 0x73172d, 0xb4202a, 0xdf3e23, 0xfa6a0a, 0xf9a31b,
   0xffd541, 0xfffc40, 0xd6f264, 0x9cdb43, 0x59c135, 0x14a02e, 0x1a7a3e, 0x24523b,
   0x122020, 0x143464, 0x285cc4, 0x249fde, 0x20d6c7, 0xa6fcdb, 0xffffff, 0xfef3c0,
   0xfad6b8, 0xf5a097, 0xe86a73, 0xbc4a9b, 0x793a80, 0x403353, 0x242234, 0x221c1a,
   0x322b28, 0x71413b, 0xbb7547, 0xdba463, 0xf4d29c, 0xdae0ea, 0xb3b9d1, 0x8b93af,
   0x6d758d, 0x4a5462, 0x333941, 0x422433, 0x5b3138, 0x8e5252, 0xba756a, 0xe9b5a3,
   0xe3e6ff, 0xb9bffb, 0x849be4, 0x588dbe, 0x477d85, 0x23674e, 0x328464, 0x5daf8d,
   0x92dcba, 0xcdf7e2, 0xe4d2aa, 0xc7b08b, 0xa08662, 0x796755, 0x5a4e44, 0x423934,

   //Famicube
   0x000000, 0xe03c28, 0xffffff, 0xd7d7d7, 0xa8a8a8, 0x7b7b7b, 0x343434, 0x151515,
   0x0d2030, 0x415d66, 0x71a6a1, 0xbdffca, 0x25e2cd, 0x0a98ac, 0x005280, 0x00604b,
   0x20b562, 0x58d332, 0x139d08, 0x004e00, 0x172808, 0x376d03, 0x6ab417, 0x8cd612,
   0xbeeb71, 0xeeffa9, 0xb6c121, 0x939717, 0xcc8f15, 0xffbb31, 0xffe737, 0xf68f37,
   0xad4e1a, 0x231712, 0x5c3c0d, 0xae6c37, 0xc59782, 0xe2d7b5, 0x4f1507, 0x823c3d,
   0xda655e, 0xe18289, 0xf5b784, 0xffe9c5, 0xff82ce, 0xcf3c71, 0x871646, 0xa328b3,
   0xcc69e4, 0xd59cfc, 0xfec9ed, 0xe2c9ff, 0xa675fe, 0x6a31ca, 0x5a1991, 0x211640,
   0x3d34a5, 0x6264dc, 0x9ba0ef, 0x98dcff, 0x5ba8ff, 0x0a89ff, 0x024aca, 0x00177d,

   //Sweet Canyon 64
   0x0f0e11, 0x2d2c33, 0x40404a, 0x51545c, 0x6b7179, 0x7c8389, 0xa8b2b6, 0xd5d5d5,
   0xeeebe0, 0xf1dbb1, 0xeec99f, 0xe1a17e, 0xcc9562, 0xab7b49, 0x9a643a, 0x86482f,
   0x783a29, 0x6a3328, 0x541d29, 0x42192c, 0x512240, 0x782349, 0x8b2e5d, 0xa93e89,
   0xd062c8, 0xec94ea, 0xf2bdfc, 0xeaebff, 0xa2fafa, 0x64e7e7, 0x54cfd8, 0x2fb6c3,
   0x2c89af, 0x25739d, 0x2a5684, 0x214574, 0x1f2966, 0x101445, 0x3c0d3b, 0x66164c,
   0x901f3d, 0xbb3030, 0xdc473c, 0xec6a45, 0xfb9b41, 0xf0c04c, 0xf4d66e, 0xfffb76,
   0xccf17a, 0x97d948, 0x6fba3b, 0x229443, 0x1d7e45, 0x116548, 0x0c4f3f, 0x0a3639,
   0x251746, 0x48246d, 0x69189c, 0x9f20c0, 0xe527d2, 0xff51cf, 0xff7ada, 0xff9edb,

   //Rewild 64
   0x172038, 0x253a5e, 0x3c5e8b, 0x4f8fba, 0x73bed3, 0xa4dddb, 0x193024, 0x245938,
   0x2b8435, 0x62ac4c, 0xa2dc6e, 0xc5e49b, 0x19332d, 0x25562e, 0x468232, 0x75a743,
   0xa8ca58, 0xd0da91, 0x5f6d43, 0x97933a, 0xa9b74c, 0xcfd467, 0xd5dc97, 0xd6dea6,
   0x382a28, 0x43322f, 0x564238, 0x715a42, 0x867150, 0xb1a282, 0x4d2b32, 0x7a4841,
   0xad7757, 0xc09473, 0xd7b594, 0xe7d5b3, 0x341c27, 0x602c2c, 0x884b2b, 0xbe772b,
   0xde9e41, 0xe8c170, 0x241527, 0x411d31, 0x752438, 0xa53030, 0xcf573c, 0xda863e,
   0x1e1d39, 0x402751, 0x7a367b, 0xa23e8c, 0xc65197, 0xdf84a5, 0x090a14, 0x10141f,
   0x151d28, 0x202e37, 0x394a50, 0x577277, 0x819796, 0xa8b5b2, 0xc7cfcc, 0xebede9

};

struct ToolData default_tooldata[] = {
   //Pencil
   { 2, LINESTYLE_STROKE, false },
   //Eraser
   { 4, LINESTYLE_STROKE, true, 0 }
};


void set_screenstate_defaults(struct ScreenState * state)
{
   state->offset_x = 0;
   state->offset_y = 0;
   state->zoom = 1;
   state->layer_width = LAYER_WIDTH; //DEFAULT_LAYER_WIDTH;
   state->layer_height = LAYER_HEIGHT; //DEFAULT_LAYER_HEIGHT;
   state->screen_width = 320;  //These two should literally never change 
   state->screen_height = 240; //GSP_SCREEN_WIDTH; //GSP_SCREEN_HEIGHT_BOTTOM;
   state->screen_color = SCREEN_COLOR;
   state->bg_color = CANVAS_BG_COLOR;
}

void init_default_drawstate(struct DrawState * state)
{
   state->zoom_power = 0;
   state->page = 0;
   state->layer = DEFAULT_START_LAYER; 

   state->palette_count = sizeof(default_palette) / sizeof(u32);
   state->palette = malloc(state->palette_count * sizeof(u16));
   convert_palette(default_palette, state->palette, state->palette_count);
   state->current_color = state->palette + DEFAULT_PALETTE_STARTINDEX;

   state->tools = malloc(sizeof(default_tooldata));
   memcpy(state->tools, default_tooldata, sizeof(default_tooldata));
   state->current_tool = state->tools + TOOL_PENCIL;

   state->min_width = MIN_WIDTH;
   state->max_width = MAX_WIDTH;
}

//Only really applies to default initialized states
void free_default_drawstate(struct DrawState * state)
{
   free(state->tools);
   free(state->palette);
}

void set_cpadprofile_canvas(struct CpadProfile * profile)
{
   profile->deadzone = 40;
   profile->mod_constant = 1;
   profile->mod_multiplier = 0.02f;
   profile->mod_curve = 3.2f;
   profile->mod_general = 1;
}

#pragma endregion


// -- DRAWING UTILS --

//Some (hopefully temporary) globals to overcome some unforeseen limits
u32 _drw_cmd_cnt = 0;

#define MY_FLUSH() { C3D_FrameEnd(0); C3D_FrameBegin(C3D_FRAME_SYNCDRAW); _drw_cmd_cnt = 0; }
#define MY_FLUSHCHECK() if(_drw_cmd_cnt > MY_C2DOBJLIMITSAFETY) { \
   LOGDBG("FLUSHING %ld DRAW CMDS PREMATURELY\n", _drw_cmd_cnt); \
   MY_FLUSH(); }

void MY_SOLIDRECT(float x, float y, u16 width, u32 color)
{
   x += LAYER_EDGEBUF;
   y += LAYER_EDGEBUF;
   if(x < LAYER_EDGEERROR || y < LAYER_EDGEERROR || x >= LAYER_WIDTH + LAYER_EDGEBUF || y >= LAYER_WIDTH + LAYER_EDGEBUF)
   {
      LOGDBG("IGNORING RECT AT (%f, %f)", x, y);
      return;
   }
   C2D_DrawRectSolid(x, y, 0.5, width, width, color);
   _drw_cmd_cnt++; 
   MY_FLUSHCHECK();
}

//Draw the scrollbars on the sides of the screen for the given screen
//modification (translation AND zoom affect the scrollbars)
void draw_scrollbars(const struct ScreenState * mod)
{
   //Need to draw an n-thickness scrollbar on the right and bottom. Assumes
   //standard page size for screen modifier.

   //Bottom and right scrollbar bg
   C2D_DrawRectSolid(0, mod->screen_height - SCROLL_WIDTH, 0.5f, 
         mod->screen_width, SCROLL_WIDTH, SCROLL_BG);
   C2D_DrawRectSolid(mod->screen_width - SCROLL_WIDTH, 0, 0.5f, 
         SCROLL_WIDTH, mod->screen_height, SCROLL_BG);

   u16 sofs_x = (float)mod->offset_x / LAYER_WIDTH / mod->zoom * mod->screen_width;
   u16 sofs_y = (float)mod->offset_y / LAYER_HEIGHT / mod->zoom * mod->screen_height;

   //bottom and right scrollbar bar
   C2D_DrawRectSolid(sofs_x, mod->screen_height - SCROLL_WIDTH, 0.5f, 
         mod->screen_width * mod->screen_width / (float)mod->layer_width / mod->zoom, SCROLL_WIDTH, SCROLL_BAR);
   C2D_DrawRectSolid(mod->screen_width - SCROLL_WIDTH, sofs_y, 0.5f, 
         SCROLL_WIDTH, mod->screen_height * mod->screen_height / (float)mod->layer_height / mod->zoom, SCROLL_BAR);
}

//Draw (JUST draw) the entire color picker area, which may include other
//stateful controls
void draw_colorpicker(u16 * palette, u16 palette_size, u16 selected_index, 
      bool collapsed)
{
   //TODO: Maybe split this out?
   if(collapsed)
   {
      C2D_DrawRectSolid(0, 0, 0.5f, PALETTE_MINISIZE, PALETTE_MINISIZE, PALETTE_BG);
      C2D_DrawRectSolid(PALETTE_MINIPADDING, PALETTE_MINIPADDING, 0.5f, 
            PALETTE_MINISIZE - PALETTE_MINIPADDING * 2, 
            PALETTE_MINISIZE - PALETTE_MINIPADDING * 2, 
            rgba16_to_rgba32c(palette[selected_index])); 

      return;
   }

   u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
   C2D_DrawRectSolid(0, 0, 0.5f, 
         8 * shift + (PALETTE_OFSX << 1) + PALETTE_SWATCHMARGIN, 
         8 * shift + (PALETTE_OFSY << 1) + PALETTE_SWATCHMARGIN, 
         PALETTE_BG);

   for(u16 i = 0; i < palette_size; i++)
   {
      //TODO: an implicit 8 wide thing
      u16 x = i & 7;
      u16 y = i >> 3;

      if(i == selected_index)
      {
         C2D_DrawRectSolid(
            PALETTE_OFSX + x * shift, 
            PALETTE_OFSY + y * shift, 0.5f, 
            shift, shift, PALETTE_SELECTED_COLOR);
      }

      C2D_DrawRectSolid(
         PALETTE_OFSX + x * shift + PALETTE_SWATCHMARGIN, 
         PALETTE_OFSY + y * shift + PALETTE_SWATCHMARGIN, 0.5f, 
         PALETTE_SWATCHWIDTH, PALETTE_SWATCHWIDTH, rgba16_to_rgba32c(palette[i]));
   }
}

void draw_layers(const struct LayerData * layers, layer_num layer_count, 
      const struct ScreenState * mod, u32 bg_color)
{
   C2D_DrawRectSolid(-mod->offset_x, -mod->offset_y, 0.5f,
         mod->layer_width * mod->zoom, mod->layer_height * mod->zoom, bg_color); //The bg color
   for(layer_num i = 0; i < layer_count; i++)
   {
      C2D_DrawImageAt(layers[i].image, -mod->offset_x - LAYER_EDGEBUF * mod->zoom, -mod->offset_y - LAYER_EDGEBUF * mod->zoom, 0.5f, 
            NULL, mod->zoom, mod->zoom);
   }
}

struct SimpleLine * add_point_to_stroke(struct LinePackage * pending, 
      const touchPosition * pos, const struct ScreenState * mod)
{
   //This is for a stroke, do different things if we have different tools!
   struct SimpleLine * line = pending->lines + pending->line_count;

   line->x2 = pos->px / mod->zoom + mod->offset_x / mod->zoom;
   line->y2 = pos->py / mod->zoom + mod->offset_y / mod->zoom;

   if(pending->line_count == 0)
   {
      line->x1 = line->x2;
      line->y1 = line->y2;
   }
   else
   {
      line->x1 = pending->lines[pending->line_count - 1].x2;
      line->y1 = pending->lines[pending->line_count - 1].y2;
   }

   //Added a line
   pending->line_count++;

   return line;
}


// -- CONTROL UTILS --


//Given a touch position (presumably on the color palette), update the selected
//palette index. 
void update_paletteindex(const touchPosition * pos, u8 * index)
{
   u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
   u16 xind = (pos->px - PALETTE_OFSX) / shift;
   u16 yind = (pos->py - PALETTE_OFSY) / shift;
   u16 new_index = (yind << 3) + xind;
   if(new_index >= 0 && new_index < DEFAULT_PALETTE_SPLIT) //PALETTE_COLORS)
      *index = new_index;
}



// -- BIG SCAN DRAW SYSTEM --

u32 tdugh = 0;

// Draw as much as possible from the given ring buffer, with as little context switching as possible. 
// WARN: MAKES A LOT OF ASSUMPTIONS IN ORDER TO PREVENT COSTLY MALLOCS PER FRAME
void draw_from_buffer(struct LineRingBuffer * scandata, struct LayerData * layers)
{
   u16 lineCount = 0;
   struct FullLine * lines[MAX_FRAMELINES];
   struct FullLine * next = NULL;

   // Repeat while there's something in the buffer and we haven't reached the limit. Essentially, just pull as much
   // as possible out of the ring buffer so we can later draw it per-layer
   while((next = lineringbuffer_shrink(scandata)) && lineCount < MAX_FRAMELINES) { //line_drawcount) {
      lines[lineCount] = next;
      lineCount++;
   }

   if(lineCount == 0) {
      return;
   }
   tdugh += lineCount;
   LOGDBG("DRAWING %d LINES (%d)", lineCount, tdugh);

   for(u8 i = 0; i < LAYER_COUNT; i++)
   {
      //Don't want to call this too often, so do as much as possible PER
      //layer instead of jumping around
      C2D_SceneBegin(layers[i].target);

      // Now loop over our lines
      for(u16 li = 0; li < lineCount; li++)
      {
         //Just entirely skip data for layers we're not focusing on yet.
         if(lines[li]->layer != i) continue;

         pixaligned_fulllinefunc(lines[li], MY_SOLIDRECT);
      }
   }
}


// -- MENU/PRINT STUFF --

#define PRINTCLEAR() { printf_flush("\x1b[%d;2H%-150s", MAINMENU_TOP, ""); }

#define PRINTGENERAL(x, col, ...) { \
   printf_flush("\x1b[%d;1H%-150s\x1b[%d;2H\x1b[%dm", MAINMENU_TOP, "", MAINMENU_TOP, col); \
   printf_flush(x, ## __VA_ARGS__); } \

#define PRINTERR(x, ...) PRINTGENERAL(x, 31, ## __VA_ARGS__)
#define PRINTWARN(x, ...) PRINTGENERAL(x, 33, ## __VA_ARGS__)
#define PRINTINFO(x, ...) PRINTGENERAL(x, 37, ## __VA_ARGS__)


void print_controls()
{
   printf("\x1b[0m");
   printf("     L - change color        R - general modifier\n");
   printf(" LF/RT - line width     UP/DWN - zoom (+R - page)\n");
   printf("SELECT - change layers   START - menu\n");
   printf("  ABXY - change tools    C-PAD - scroll canvas\n");
   printf("--------------------------------------------------");
}

void print_framing()
{
   printf("\x1b[0m");
   printf("\x1b[29;1H--------------------------------------------------");
   printf("\x1b[28;43H%7s", VERSION);
}

void get_printmods(char * status_x1b, char * active_x1b, char * statusbg_x1b, char * activebg_x1b)
{
   //First is background, second is foreground (within string)
   if(status_x1b != NULL) sprintf(status_x1b, "\x1b[40m\x1b[%dm", STATUS_MAINCOLOR);
   if(active_x1b != NULL) sprintf(active_x1b, "\x1b[40m\x1b[%dm", STATUS_ACTIVECOLOR);
   if(statusbg_x1b != NULL) sprintf(statusbg_x1b, "\x1b[%dm\x1b[30m", 10 + STATUS_MAINCOLOR);
   if(activebg_x1b != NULL) sprintf(activebg_x1b, "\x1b[%dm\x1b[30m", 10 + STATUS_ACTIVECOLOR);
}

void print_data(char * data, char * dataptr, char * saveptr)
{
   char status_x1b[PSX1BLEN];
   char active_x1b[PSX1BLEN];
   get_printmods(status_x1b, active_x1b, NULL, NULL);

   u32 datasize = dataptr - data;
   u32 unsaved = dataptr - saveptr;
   float percent = 100.0 * (float)datasize / MAX_DRAW_DATA;

   char numbers[51];
   sprintf(numbers, "%ld %ld  %s(%05.2f%%)", unsaved, datasize, active_x1b, percent);
   printf("\x1b[28;1H%s %s%*s", status_x1b, numbers, 
         PRINTDATA_WIDTH - (strlen(numbers) - strlen(active_x1b)),"");
}

void print_status(u8 width, u8 layer, s8 zoom_power, u8 tool, u16 color, u16 page)
{
   char tool_chars[TOOL_COUNT + 1];
   strcpy(tool_chars, TOOL_CHARS);

   char status_x1b[PSX1BLEN];
   char active_x1b[PSX1BLEN];
   char statusbg_x1b[PSX1BLEN];
   char activebg_x1b[PSX1BLEN];
   get_printmods(status_x1b, active_x1b, statusbg_x1b, activebg_x1b);

   printf("\x1b[30;1H%s W:%s%02d%s L:", status_x1b, active_x1b, width, status_x1b);
   for(s8 i = LAYER_COUNT - 1; i >= 0; i--) // NOTE: the second input is an optional character to display on current layer
      printf("%s%c", i == layer ? activebg_x1b : statusbg_x1b, i == layer ? ' ' : ' ');
   printf("%s Z:", status_x1b);
   for(s8 i = MIN_ZOOMPOWER; i <= MAX_ZOOMPOWER; i++)
      printf("%s%c", 
            i == zoom_power ? activebg_x1b : statusbg_x1b,
            i == 0 ? '|' : ' ');
   printf("%s T:", status_x1b);
   for(u8 i = 0; i < TOOL_COUNT; i++)
      printf("%s%c", i == tool ? activebg_x1b : active_x1b, tool_chars[i]);
   printf("%s P:%s%03d", status_x1b, active_x1b, page + 1);
   printf("%s C:%s%#06x", status_x1b, active_x1b, color);
}

void print_time(bool showcolon)
{
   char status_x1b[PSX1BLEN];
   get_printmods(status_x1b, NULL, NULL, NULL);

   time_t rawtime = time(NULL);
   struct tm * timeinfo = localtime(&rawtime);

   printf("\x1b[30;45H%s%02d%c%02d", 
         status_x1b, timeinfo->tm_hour, showcolon ? ':' : ' ', timeinfo->tm_min);
}


// -- FILESYSTEM --

void get_save_location(char * savename, char * container)
{
   container[0] = 0;
   sprintf(container, "%s%s/", SAVE_BASE, savename);
}

void get_rawfile_location(char * savename, char * container)
{
   get_save_location(savename, container);
   strcpy(container + strlen(container), "raw");
}

int save_drawing(char * filename, char * data)
{
   char savefolder[MAX_FILEPATH];
   char fullpath[MAX_FILEPATH];
   char temp_msg[MAX_FILEPATH];

   //LOGDBG("SAVEFILENAME: %s", filename);

   if(enter_text_fixed("Enter a filename: ", MAINMENU_TOP, filename, 
            MAX_FILENAMESHOW, !strlen(filename), KEY_B | KEY_START))
   {
      //Go get the full path
      get_save_location(filename, savefolder);
      get_rawfile_location(filename, fullpath);

      //Prepare the warning message
      snprintf(temp_msg, MAX_FILEPATH - 1, "WARN: OVERWRITE %s", filename);

      //We only save if it's new or if... idk.
      if(!file_exists(fullpath) || easy_warn(temp_msg,
               "Save already exists, definitely overwrite?", MAINMENU_TOP))
      {
         PRINTINFO("Saving file %s...", filename);
         int result = mkdir_p(savefolder);
         if(!result) result = write_file(fullpath, data);
         PRINTCLEAR();
         return result;
      }
   }

   return -1;
}

char * load_drawing(char * data_container, char * final_filename)
{
   char fullpath[MAX_FILEPATH];
   char temp_msg[MAX_FILEPATH]; //Find another constant for this I guess
   char * result = NULL;

   char * all_files = malloc(sizeof(char) * MAX_ALLFILENAMES);
   if(!all_files) {
      PRINTERR("Couldn't allocate memory");
      goto TRUEEND;
   }

   PRINTINFO("Searching for saves...");
   u32 dircount = get_directories(SAVE_BASE, all_files, MAX_ALLFILENAMES);

   if(dircount < 0) { PRINTERR("Something went wrong while searching!"); goto END; }
   else if(dircount <= 0) { PRINTINFO("No saves found"); goto END; }

   sprintf(temp_msg, "Found %ld saves:", dircount);
   s32 sel = easy_menu(temp_msg, all_files, MAINMENU_TOP, FILELOAD_MENUCOUNT, KEY_START | KEY_B);

   if(sel < 0) goto END;

   final_filename[0] = 0;
   strcpy(final_filename, get_menu_item(all_files, MAX_ALLFILENAMES, sel));
   //LOGDBG("LOADFILENAME: %s", final_filename);

   PRINTINFO("Loading file %s...", final_filename)
   get_rawfile_location(final_filename, fullpath);
   result = read_file(fullpath, data_container, MAX_DRAW_DATA);
   PRINTCLEAR();

END:
   free(all_files);
TRUEEND:
   return result;
}

//This is a funny little system. Custom line drawing, passing the function,
//idk. There are better ways, but I'm lazy
u32 * _exp_layer_dt = NULL;

void _exp_layer_dt_func(float x, float y, u16 width, u32 color)
{
   //pre-calculating can save tons of time in the critical loop down there. We
   //don't want ANY long operations (modulus, my goodness) and we want it to
   //use cache as much as possible. 
   u32 minx = x < 0 ? 0 : ((u32)x % LAYER_WIDTH);
   u32 maxx = minx + width;
   u32 minyi = y < 0 ? 0 : (y * LAYER_WIDTH);
   u32 maxyi = minyi + LAYER_WIDTH * width;
   if(maxx >= LAYER_WIDTH) maxx = LAYER_WIDTH - 1;
   if(maxyi >= LAYER_WIDTH * LAYER_HEIGHT) maxyi = LAYER_WIDTH * LAYER_HEIGHT - 1;

   for(u32 yi = minyi; yi < maxyi; yi += LAYER_WIDTH)
      for(u32 xi = minx; xi < maxx; xi++)
         _exp_layer_dt[yi + xi] = color;
}

//Export the given page from the given data into a default named file in some
//default image format (bitmap? png? who knows)
void export_page(page_num page, char * data, char * data_end, char * basename)
{
   //NOTE: this entire function is going to use Citro2D u32 formatted colors, all
   //the way until the final bitmap is written (at which point it can be
   //converted as needed)

   PRINTINFO("Exporting page %d: building...", page);

   if(mkdir_p(SCREENSHOTS_BASE))
   {
      PRINTERR("Couldn't create screenshots folder: %s", SCREENSHOTS_BASE);
      return;
   }

   u32 * layerdata[LAYER_COUNT + 1];
   u32 size_bytes = sizeof(u32) * LAYER_WIDTH * LAYER_HEIGHT;

   for(int i = 0; i < LAYER_COUNT + 1; i++)
   {
      layerdata[i] = malloc(size_bytes);

      if(layerdata[i] == NULL)
      {
         LOGDBG("ERR: COULDN'T ALLOCATE MEMORY FOR EXPORT");

         for(int j = 0; j < i; j++)
            free(layerdata[j]);

         return;
      }

      //TODO: This assumes the bg is white
      memset(layerdata[i], (i == LAYER_COUNT) ? 0xFF : 0, size_bytes);
   }

   char savepath[MAX_FILEPATH];
   time_t now = time(NULL);
   sprintf(savepath, "%s%s_%d_%jd.png", SCREENSHOTS_BASE, 
         strlen(basename) ? basename : "new", page, now);

   //Now just parse and parse and parse until we reach the end!
   u32 data_length = data_end - data;
   char * current_data = data;
   char * stroke_start = NULL;
   struct LinePackage package;
   init_linepackage(&package); // WARN: initialization could fail, no checks performed!

   while(current_data < data_end)
   {
      current_data = datamem_scanstroke(current_data, data_end, data_length, 
              page, &stroke_start);

      //Is this the normal way to do this? idk
      if(stroke_start == NULL) continue;

      convert_data_to_linepack(&package, stroke_start, current_data);
      _exp_layer_dt = layerdata[package.layer];

      //At this point, we draw the line.
      pixaligned_linepackfunc_all(&package, &_exp_layer_dt_func);
   }

   //Now merge arrays together; our alpha blending is dead simple. 
   //Will this take a while?
   for(int i = 0; i < LAYER_HEIGHT * LAYER_WIDTH; i++)
   {
      //Loop over arrays, the topmost (layer) value persists
      for(int j = LAYER_COUNT - 1; j >= 0; j--)
      {
         if(layerdata[j][i])
         {
            layerdata[LAYER_COUNT][i] = layerdata[j][i];
            break;
         }
      }
   }

   PRINTINFO("Exporting page %d: converting to png...", page);

   if(write_citropng(layerdata[LAYER_COUNT], LAYER_WIDTH, LAYER_HEIGHT, savepath) == 0) {
      PRINTINFO("Exported page to: %s", savepath);
   }
   else {
      PRINTERR("FAILED to export: %s", savepath);
   }

   free_linepackage(&package);
   for(int i = 0; i < LAYER_COUNT + 1; i++)
      free(layerdata[i]);
}

// -- MAIN, OFC --

#define UTILS_CLAMP(x, mn, mx) (x <= mn ? mn : x >= mx ? mx : x)

// Some macros used ONLY for main (think lambdas)
#define MAIN_UPDOWN(x) {   \
   if(kHeld & KEY_R) {     \
      drwst.page = UTILS_CLAMP(drwst.page + x * ((kHeld&KEY_L)?10:1), 0, MAX_PAGE);    \
      draw_pointer = draw_data;     \
      reset_lineringbuffer(&scandata); \
      flush_layers = true;          \
   } else {                \
      drwst.zoom_power = UTILS_CLAMP(drwst.zoom_power + x, MIN_ZOOMPOWER, MAX_ZOOMPOWER);    \
   } }

#define PRINT_DATAUSAGE() print_data(draw_data, draw_data_end, saved_last);

#define MAIN_NEWDRAW() { \
   draw_data_end = draw_pointer = saved_last = draw_data; \
   reset_lineringbuffer(&scandata); \
   free_default_drawstate(&drwst); \
   init_default_drawstate(&drwst); \
   flush_layers = true;       \
   save_filename[0] = '\0';   \
   pending.line_count = 0;        \
   current_frame = end_frame = 0; \
   printf("\x1b[1;1H");       \
   print_controls();          \
   print_framing();           \
   PRINT_DATAUSAGE();         \
}

#define MAIN_UNSAVEDCHECK(x) \
   (saved_last == draw_data_end || easy_warn("WARN: UNSAVED DATA", x, MAINMENU_TOP))


int main(int argc, char** argv)
{
   gfxInitDefault();
   C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
   C2D_Init(MY_C2DOBJLIMIT);
   C2D_Prepare();

   consoleInit(GFX_TOP, NULL);
   C3D_RenderTarget* screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

   LOGDBG("INITIALIZED")

   struct DrawState drwst;
   struct ScreenState scrst;
   struct CpadProfile cpdpr;

   init_default_drawstate(&drwst); // Just in case
   set_screenstate_defaults(&scrst);
   set_cpadprofile_canvas(&cpdpr);

   LOGDBG("SET SCREENSTATE/CANVAS");

   //weird byte order? 16 bits of color are at top
   const u32 layer_color = rgba32c_to_rgba16c_32(CANVAS_LAYER_COLOR);

   const Tex3DS_SubTexture subtex = {
      TEXTURE_WIDTH, TEXTURE_HEIGHT,
      0.0f, 1.0f, 1.0f, 0.0f
   };

   struct LayerData layers[LAYER_COUNT];

   for(int i = 0; i < LAYER_COUNT; i++)
      create_layer(layers + i, subtex);

   LOGDBG("CREATED LAYERS");

   bool touching = false;
   bool palette_active = false;
   bool flush_layers = true;

   u32 current_frame = 0;
   u32 end_frame = 0;
   s8 last_zoom_power = 0;

   struct LinePackage pending;
   init_linepackage(&pending);
   struct SimpleLine * pending_lines = malloc(MAX_STROKE_LINES * sizeof(struct SimpleLine));
   pending.lines = pending_lines;

   struct LineRingBuffer scandata;
   init_lineringbuffer(&scandata, MAX_FRAMELINES);
   LOGDBG("RINGBUFFER CAPACITY: %d", scandata.capacity);
   if(!scandata.lines) {
      LOGDBG("ERROR: COULD NOT INIT LINERINGBUFFER");
   } 
   if(!scandata.pending.lines) {
      LOGDBG("ERROR: COULD NOT INIT LRB PENDING");
   } 

   char * save_filename = malloc(MAX_FILENAME * sizeof(char));
   char * draw_data = malloc(MAX_DRAW_DATA * sizeof(char));
   char * stroke_data = malloc(MAX_STROKE_DATA * sizeof(char));
   char * draw_data_end; // NOTE: this is exclusive: it points one past the end. draw_data_end - draw_data = length
   char * draw_pointer;
   char * saved_last;

   LOGDBG("SYSTEM MALLOC");

   MAIN_NEWDRAW();

   hidSetRepeatParameters(BREPEAT_DELAY, BREPEAT_INTERVAL);

   mkdir_p(SAVE_BASE);

   LOGDBG("STARTING MAIN LOOP");

   while(aptMainLoop())
   {
      hidScanInput();

      u32 kDown = hidKeysDown();
      u32 kUp = hidKeysUp();
      u32 kRepeat = hidKeysDownRepeat();
      u32 kHeld = hidKeysHeld();
      circlePosition pos;
      touchPosition current_touch;
      hidTouchRead(&current_touch);
      hidCircleRead(&pos);

      u16 po = (drwst.current_color - drwst.palette) / DEFAULT_PALETTE_SPLIT;
      u8 pi = (drwst.current_color - drwst.palette) % DEFAULT_PALETTE_SPLIT;

      // Respond to user input
      if(kDown & KEY_L && !(kHeld & KEY_R)) palette_active = !palette_active;
      if(kRepeat & KEY_DUP) MAIN_UPDOWN(1)
      if(kRepeat & KEY_DDOWN) MAIN_UPDOWN(-1)
      if(kRepeat & KEY_DRIGHT) shift_drawstate_width(&drwst, (kHeld & KEY_R ? 5 : 1));
      if(kRepeat & KEY_DLEFT) shift_drawstate_width(&drwst, -(kHeld & KEY_R ? 5 : 1));
      if(kDown & KEY_A) set_drawstate_tool(&drwst, TOOL_PENCIL); 
      if(kDown & KEY_B) set_drawstate_tool(&drwst, TOOL_ERASER);
      if(kHeld & KEY_L && kDown & KEY_R && palette_active) {
         shift_drawstate_color(&drwst, DEFAULT_PALETTE_SPLIT);
      }
      if(kDown & KEY_SELECT) {
         drwst.layer = (drwst.layer + 1) % LAYER_COUNT;
      }
      if(kDown & KEY_START) 
      {
         s32 selected = easy_menu(MAINMENU_TITLE, MAINMENU_ITEMS, MAINMENU_TOP, 0, KEY_B | KEY_START);
         if(selected == MAINMENU_EXIT) {
            if(MAIN_UNSAVEDCHECK("Really quit?")) break;
         }
         else if(selected == MAINMENU_NEW) {
            if(MAIN_UNSAVEDCHECK("Are you sure you want to start anew?")) MAIN_NEWDRAW();
         }
         else if(selected == MAINMENU_SAVE)
         {
            *draw_data_end = 0;
            if(save_drawing(save_filename, draw_data) == 0)
            {
               saved_last = draw_data_end;
               PRINT_DATAUSAGE(); //Should this be out in the main loop?
            }
         }
         else if (selected == MAINMENU_LOAD)
         {
            if(MAIN_UNSAVEDCHECK("Are you sure you want to load and lose changes?"))
            {
               MAIN_NEWDRAW();
               draw_data_end = load_drawing(draw_data, save_filename);

               if(draw_data_end == NULL)
               {
                  PRINTERR("LOAD FAILED!");
                  MAIN_NEWDRAW();
               }
               else
               {
                  saved_last = draw_data_end;
                  // Find last page, set it.
                  drwst.page = last_used_page(draw_data, draw_data_end - draw_data);
                  PRINT_DATAUSAGE();
               }
            }
         }
         else if (selected == MAINMENU_EXPORT)
         {
            export_page(drwst.page, draw_data, draw_data_end, save_filename);
         }
      }
      if(kDown & KEY_TOUCH) {
         pending.color = get_drawstate_color(&drwst);
         pending.style = drwst.current_tool->style;
         pending.width = drwst.current_tool->width;
         pending.layer = drwst.layer;
      }
      if(kUp & KEY_TOUCH) {
         end_frame = current_frame;
         palette_active = false; // Something of a hack: might get rid of it later. This makes the palette disappear on selection
      }

      //Update zoom separately, since the update is always the same
      if(drwst.zoom_power != last_zoom_power) set_screenstate_zoom(&scrst, pow(2, drwst.zoom_power));

      if(kRepeat & ~(KEY_TOUCH) || !current_frame)
      {
         print_status(drwst.current_tool->width, drwst.layer, drwst.zoom_power, 
               drwst.current_tool - drwst.tools, *drwst.current_color, drwst.page);
      }

      touching = (kHeld & KEY_TOUCH) > 0;

      set_screenstate_offset(&scrst, 
            cpad_translate(&cpdpr, pos.dx, scrst.offset_x), 
            cpad_translate(&cpdpr, -pos.dy, scrst.offset_y));


      // Render the scene
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

      // -- LAYER DRAW SECTION --
      C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);


      //Apparently (not sure), all clearing should be done within our main loop?
      if(flush_layers)
      {
         for(int i = 0; i < LAYER_COUNT; i++)
            C2D_TargetClear(layers[i].target, layer_color); 
         flush_layers = false;
      }
      //Ignore first frame touches
      else if(touching)
      {
         if(palette_active)
         {
            update_paletteindex(&current_touch, &pi);
            drwst.current_color = drwst.palette + po * DEFAULT_PALETTE_SPLIT + pi;
         }
         else
         {
            //Keep this outside the if statement below so it can be used for
            //background drawing too (draw commands from other people)
            C2D_SceneBegin(layers[drwst.layer].target);

            if(pending.line_count < MAX_STROKE_LINES)
            {
               //This is for a stroke, do different things if we have different tools!
               add_point_to_stroke(&pending, &current_touch, &scrst);
               //Draw ONLY the current line
               pixaligned_linepackfunc(&pending, pending.line_count - 1, pending.line_count, MY_SOLIDRECT);
            }
         }
      }

      //Just always try to draw whatever is leftover in the buffer
      if(draw_pointer < draw_data_end)
      {
         // Fill the buffer as much as possible to start
         char * init_pointer = draw_pointer;
         draw_pointer = scan_lines(&scandata, draw_pointer, draw_data_end, drwst.page);
         //LOGDBG("SCANNED: %d\n", draw_pointer - init_pointer);
         // Then just pull as many lines as possible out, UP TO the maximum per frame
         //draw_from_buffer(&scandata, MAX_FRAMELINES, layers, LAYER_COUNT);
      }
      draw_from_buffer(&scandata, layers);

      C2D_Flush();
      _drw_cmd_cnt = 0;

      // -- OTHER DRAW SECTION --
      C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, 
            GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);

      if(!(current_frame % 30)) print_time(current_frame % 60);

      //TODO: Eventually, change this to put the data in different places?
      if(end_frame == current_frame && pending.line_count > 0)
      {
         char * cvl_end = convert_linepack_to_data(&pending, stroke_data, MAX_STROKE_DATA);

         if(cvl_end == NULL)
         {
            LOGDBG("ERR: Couldn't convert lines!\n");
         }
         else
         {
            char * previous_end = draw_data_end;
            draw_data_end = write_to_datamem(stroke_data, cvl_end, drwst.page, draw_data, draw_data_end);
            //An optimization: if draw_pointer was already at the end, don't
            //need to RE-draw what we've already drawn, move it forward with
            //the mem write. Note: there are instances where we WILL be drawing
            //twice, but it's difficult to determine what has or has not been
            //drawn when the pointer isn't at the end.
            if(previous_end == draw_pointer) draw_pointer = draw_data_end;

            //TODO: need to do this in more places (like when you get lines)
            PRINT_DATAUSAGE();
         }

         pending.line_count = 0;
      }

      C2D_TargetClear(screen, scrst.screen_color);
      C2D_SceneBegin(screen);

      draw_layers(layers, LAYER_COUNT, &scrst, scrst.bg_color);
      draw_scrollbars(&scrst);
      draw_colorpicker(drwst.palette + po * DEFAULT_PALETTE_SPLIT, DEFAULT_PALETTE_SPLIT, 
            drwst.current_tool->has_static_color ? DEFAULT_PALETTE_SPLIT + 1 : pi, !palette_active);

      C3D_FrameEnd(0);

      last_zoom_power = drwst.zoom_power;
      current_frame++;
   }

   free_lineringbuffer(&scandata);
   free_linepackage(&pending);
   free(draw_data);
   free(stroke_data);

   for(int i = 0; i < LAYER_COUNT; i++)
      delete_layer(layers[i]);

   //scandata_free(&scandata);

   C2D_Fini();
   C3D_Fini();
   gfxExit();
   return 0;
}
