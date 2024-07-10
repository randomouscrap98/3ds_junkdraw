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
#include "draw.h"

#include "system.h"
#include "setup.h"

// #include "constants.h"
// #include "gamemain.h"
// #include "game_input.h"
// #include "game_drawctrl.h"
// #include "game_drawsys.h"
// #include "game_defaults.h"

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

#define LAYER_COUNT 2
#define TEXTURE_WIDTH 1024
#define TEXTURE_HEIGHT 1024

#define TOOL_PENCIL 0
#define TOOL_ERASER 1
#define TOOL_COUNT 2
#define TOOL_CHARS "pe"

#define LAYER_EDGEBUF 8

typedef u16 page_num;
typedef u8 layer_num;

#pragma region GAMESTATE

// --------- GAME STATE 0-------------

//Represents most of what you'd need to keep track of general state.
//Consider splitting out the various portions in preparation for a more
//"modular" approach... if that's what you need, anyway.
struct GameState
{
   u32 frame;   

   //Inputs
   circlePosition circle_position;
   touchPosition touch_position;
   u32 k_down;
   u32 k_repeat;
   u32 k_up; 
   u32 k_held; 

   // drawing state
   s8 zoom_power;
   s8 last_zoom_power;
   u16 draw_page;
   u8 draw_tool;
};

typedef struct GameState * gs;
typedef void (*game_event_handler)(gs);

void initialize_gamestate(struct GameState * state)
{
   memset(state, 0, sizeof(struct GameState));
}

void set_gstate_inputs(gs gstate)
{
   gstate->k_down = hidKeysDown();
   gstate->k_repeat = hidKeysDownRepeat();
   gstate->k_up = hidKeysUp();
   gstate->k_held = hidKeysHeld();
   hidTouchRead(&gstate->touch_position);
   hidCircleRead(&gstate->circle_position);
}

void draw_controls(gs gstate)
{

}
#pragma endregion

#pragma region INPUT

// ---------------- INPUT ---------------

// User Actions 
#define IUA_PAGEUP         0x00001
#define IUA_PAGEDOWN       0x00002
#define IUA_PALETTETOGGLE  0x00004
#define IUA_WIDTHUP        0x00008
#define IUA_WIDTHDOWN      0x00010
#define IUA_TOOLA          0x00020
#define IUA_TOOLB          0x00040
#define IUA_PALETTESWAP    0x00080
#define IUA_MENU           0x00100
#define IUA_EXPORTPAGE     0x00200
#define IUA_NEXTLAYER      0x00400
#define IUA_WIDTHUPBIG     0x00800
#define IUA_WIDTHDOWNBIG   0x01000
#define IUA_ZOOMIN         0x02000
#define IUA_ZOOMOUT        0x04000
#define IUA_PAGEUPBIG      0x08000
#define IUA_PAGEDOWNBIG    0x10000

//Returns a bitfield representing all "game actions" taken based on the given input.
u32 input_to_action(struct InputSet * input)
{
   u32 result = 0;
   bool rmod = input->k_held & KEY_R;
   bool lmod = input->k_held & KEY_L;

   if(input->k_repeat & KEY_DRIGHT) 
      result |= (rmod ? IUA_WIDTHUPBIG : IUA_WIDTHUP);
   if(input->k_repeat & KEY_DLEFT) 
      result |= (rmod ? IUA_WIDTHDOWNBIG : IUA_WIDTHDOWN);
   if(input->k_repeat & KEY_DUP) 
      result |= (rmod ? (lmod ? IUA_PAGEUPBIG : IUA_PAGEUP) : IUA_ZOOMIN);
   if(input->k_repeat & KEY_DDOWN) 
      result |= (rmod ? (lmod ? IUA_PAGEDOWNBIG : IUA_PAGEDOWN) : IUA_ZOOMOUT);

   if(input->k_down & KEY_A) 
      result |= IUA_TOOLA;
   if(input->k_down & KEY_B) 
      result |= IUA_TOOLB;
   if(input->k_down & KEY_START) 
      result |= IUA_MENU;
   if(input->k_down & KEY_L)
      result |= (rmod ? IUA_PALETTESWAP : IUA_PALETTETOGGLE);
   if(input->k_down & KEY_SELECT)
      result |= (rmod ? IUA_EXPORTPAGE : IUA_NEXTLAYER);

   return result;
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
   C2D_DrawRectSolid(x + LAYER_EDGEBUF, y + LAYER_EDGEBUF, 0.5, width, width, color);
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
      C2D_DrawImageAt(layers[i].image, -mod->offset_x - LAYER_EDGEBUF, -mod->offset_y - LAYER_EDGEBUF, 0.5f, 
            NULL, mod->zoom, mod->zoom);
   }
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

//Draw and track a certain number of lines from the given scandata.
u32 scandata_draw(struct ScanDrawData * scandata, u32 line_drawcount, struct LayerData * layers, u8 layer_count)
{
   u32 current_drawcount = 0;

   //Only draw if there's something to start the whole thing
   if(scandata->current_package != NULL && 
         scandata->current_package != scandata->last_package)
   {
      u8 last_layer = (scandata->last_package - 1)->layer;

      struct LinePackage * stopped_on = NULL;
       
      //Loop over layers
      for(u8 i = 0; i < layer_count; i++)
      {
         //This is the end of the line, we stopped somewhere
         if(stopped_on != NULL) break;

         //Calculate actual layer (last_layer produces shifted window)
         u8 layer_i = (i + last_layer + 1) % layer_count;

         //Don't want to call this too often, so do as much as possible PER
         //layer instead of jumping around
         C2D_SceneBegin(layers[layer_i].target);

         //Scan over every package
         for(struct LinePackage * p = scandata->current_package; 
               p < scandata->last_package; p++)
         {
            //Just entirely skip data for layers we're not focusing on yet.
            if(p->layer != layer_i) continue;

            u16 packagedrawlines = p->line_count;
            u32 leftover_drawcount = line_drawcount - current_drawcount;

            //If this is going to be the last package we're drawing, track
            //where we stopped and don't draw ALL the lines.
            if(packagedrawlines > leftover_drawcount)
            {
               //This is where we stopped. 
               stopped_on = p;
               packagedrawlines = leftover_drawcount;
            }

            pixaligned_linepackfunc(p, 0, packagedrawlines, MY_SOLIDRECT);
            line_drawcount += packagedrawlines;

            //If we didn't draw ALL the lines, move the line pointer forward.
            //We know that the line pointers in these packages points to a flat array
            if(packagedrawlines != p->line_count)
               p->lines += packagedrawlines;
         }
      }

      //At the end, only set package to NULL if we got all the way through.
      scandata->current_package = stopped_on;
   }

   return current_drawcount;
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
   printf(" LF/RT - line width     UP/DWN - zoom\n");
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
   for(s8 i = LAYER_COUNT - 1; i >= 0; i--)
      printf("%s ", i == layer ? activebg_x1b : statusbg_x1b);
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
   char * all_files = malloc(sizeof(char) * MAX_ALLFILENAMES);
   char fullpath[MAX_FILEPATH];
   char temp_msg[MAX_FILEPATH]; //Find another constant for this I guess
   char * result = NULL;

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
   package.lines = malloc(sizeof(struct SimpleLine) * MAX_STROKE_LINES);
   package.max_lines = MAX_STROKE_LINES;

   if(!package.lines)
   {
      LOGDBG("ERR: Couldn't allocate stroke lines");
      goto END;
   }

   //PRINTINFO("Exporting page %d: drawing", page);
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

END:
   for(int i = 0; i < LAYER_COUNT + 1; i++)
      free(layerdata[i]);
}



// -- TESTS --

/*#ifdef DEBUG_RUNTESTS
#include "tests.h" //Yes this is awful, don't care
#endif*/



// -- MAIN, OFC --

#define UTILS_CLAMP(x, mn, mx) (x <= mn ? mn : x >= mx ? mx : x)

// Some macros used ONLY for main (think lambdas)
#define MAIN_UPDOWN(x) {   \
   if(kHeld & KEY_R) {     \
      drwst.page = UTILS_CLAMP(drwst.page + x * ((kHeld&KEY_L)?10:1), 0, MAX_PAGE);    \
      draw_pointer = draw_data;     \
      flush_layers = true;          \
   } else {                \
      drwst.zoom_power = UTILS_CLAMP(drwst.zoom_power + x, MIN_ZOOMPOWER, MAX_ZOOMPOWER);    \
   } }

#define PRINT_DATAUSAGE() print_data(draw_data, draw_data_end, saved_last);

#define MAIN_NEWDRAW() { \
   draw_data_end = draw_pointer = saved_last = draw_data; \
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

   //TODO: eventually, I want the game state to include most of what you'd need
   //to reboot the system and still have the exact same setup.
   struct GameState gstate;

   struct DrawState drwst;
   struct ScreenState scrst;
   struct CpadProfile cpdpr;

   set_screenstate_defaults(&scrst);
   set_cpadprofile_canvas(&cpdpr);

   //weird byte order? 16 bits of color are at top
   const u32 layer_color = rgba32c_to_rgba16c_32(CANVAS_LAYER_COLOR);

   const Tex3DS_SubTexture subtex = {
      TEXTURE_WIDTH, TEXTURE_HEIGHT,
      0.0f, 1.0f, 1.0f, 0.0f
   };

   struct LayerData layers[LAYER_COUNT];

   for(int i = 0; i < LAYER_COUNT; i++)
      create_layer(layers + i, subtex);


   bool touching = false;
   bool palette_active = false;
   bool flush_layers = true;

   u32 current_frame = 0;
   u32 end_frame = 0;
   s8 last_zoom_power = 0;

   struct LinePackage pending;
   struct SimpleLine * pending_lines = malloc(MAX_STROKE_LINES * sizeof(struct SimpleLine));
   pending.lines = pending_lines;

   struct ScanDrawData scandata;
   scandata_initialize(&scandata, MAX_FRAMELINES);

   char * save_filename = malloc(MAX_FILENAME * sizeof(char));
   char * draw_data = malloc(MAX_DRAW_DATA * sizeof(char));
   char * stroke_data = malloc(MAX_STROKE_DATA * sizeof(char));
   char * draw_data_end;
   char * draw_pointer;
   char * saved_last;

   udsNetworkStruct network_struct;
   udsBindContext bind_ctx;
   u8 con_type = 0;
   //bool hosting_local = false;

   MAIN_NEWDRAW();

   hidSetRepeatParameters(BREPEAT_DELAY, BREPEAT_INTERVAL);

#ifdef DEBUG_RUNTESTS
   printf("\x1b[6;1H");
   run_tests();
#endif

   mkdir_p(SAVE_BASE);

   LOGDBG("STARTING MAIN LOOP");

   while(aptMainLoop())
   {
      hidScanInput();

      set_gstate_inputs(&gstate);
      u32 kDown = gstate.k_down;
      u32 kUp = gstate.k_up;
      u32 kRepeat = gstate.k_repeat;
      u32 kHeld = gstate.k_held;
      circlePosition pos = gstate.circle_position;
      touchPosition current_touch = gstate.touch_position;

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
         if(kHeld & KEY_R) { export_page(drwst.page, draw_data, draw_data_end, save_filename); } 
         else { drwst.layer = (drwst.layer + 1) % LAYER_COUNT; }
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

               if(draw_data_end == NULL) //!draw_data_end)
               {
                  PRINTERR("LOAD FAILED!");
                  MAIN_NEWDRAW();
               }
               else
               {
                  saved_last = draw_data_end;
                  PRINT_DATAUSAGE();
               }
            }
         }
      }
      if(kDown & KEY_TOUCH) {
         pending.color = get_drawstate_color(&drwst);
         pending.style = drwst.current_tool->style;
         pending.width = drwst.current_tool->width;
         pending.layer = drwst.layer;
      }
      if(kUp & KEY_TOUCH) end_frame = current_frame;

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
         u32 maxdraw = MAX_FRAMELINES;
         maxdraw -= scandata_draw(&scandata, maxdraw, layers, LAYER_COUNT);
         draw_pointer = scandata_parse(&scandata, draw_pointer, draw_data_end,
               maxdraw, drwst.page);
         scandata_draw(&scandata, maxdraw, layers, LAYER_COUNT); 
      }

      C2D_Flush();
      _drw_cmd_cnt = 0;
      //MY_FLUSH();

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

   MAIN_NETWORK_DC(con_type);

   free(pending_lines);
   free(draw_data);
   free(stroke_data);

   for(int i = 0; i < LAYER_COUNT; i++)
      delete_layer(layers[i]);

   scandata_free(&scandata);

   C2D_Fini();
   C3D_Fini();
   gfxExit();
   return 0;
}
