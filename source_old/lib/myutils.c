#include <citro3d.h>

#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <png.h>

#include "myutils.h"


// -------------------
// -- GENERAL UTILS --
// -------------------

u32 char_occurrences(const char * string, char c)
{
   u32 count = 0;
   char *pch = strchr(string,c);
   while (pch != NULL) {
      count++;
      pch = strchr(pch+1,c);
   }
   return count;
}

u32 swap_bits(u32 x, u8 p1, u8 p2)
{
   u8 swapflag = ((x >> p1) ^ (x >> p2)) & 1;
   return x ^ ((swapflag << p1) | (swapflag << p2));
}

u32 swap_bits_mask(u32 x, u32 m1, u32 m2)
{
   u32 f = (m1 | m2);
   u32 q = (f & x);
   return (q == 0 || q == f) ? x : x ^ f;
}

// ----------------
// -- DRAW UTILS --
// ----------------

//Draw a line using a custom line drawing system (required like this because of
//javascript's general inability to draw non anti-aliased lines, and I want the
//strokes saved by this program to be 100% accurately reproducible on javascript)
void pixaligned_linefunc(const struct SimpleLine * line, u16 width, u32 color,
      rectangle_func rect_f)
{
   float xdiff = line->x2 - line->x1;
   float ydiff = line->y2 - line->y1;
   float dist = sqrt(xdiff * xdiff + ydiff * ydiff);
   float ang = atan(ydiff/(xdiff?xdiff:0.0001))+(xdiff<0?M_PI:0);
   float xang = cos(ang);
   float yang = sin(ang);

   float x, y; 
   float ox = -1;
   float oy = -1;

   //Where the "edge" of each rectangle to be drawn is
   float ofs = (width / 2.0) - 0.5;

   //Iterate over an acceptable amount of points on the line and draw
   //rectangles to create a line.
   for(float i = 0; i <= dist; i+=0.5)
   {
      x = floor(line->x1+xang*i - ofs);
      y = floor(line->y1+yang*i - ofs);

      //If we align to the same pixel as before, no need to draw again.
      if(ox == x && oy == y) continue;

      //Actually draw a centered rect... however you want to.
      (*rect_f)(x,y,width,color);

      ox = x; oy = y;
   }
}


// -----------------
// -- COLOR UTILS --
// -----------------

//Take an rgb WITHOUT alpha and convert to 3ds full color (with alpha)
u32 rgb24_to_rgba32c(u32 rgb)
{
   return 0xFF000000 | ((rgb >> 16) & 0x0000FF) | (rgb & 0x00FF00) | ((rgb << 16) & 0xFF0000);
}

//Convert a citro2D rgba to a weird shifted 16bit thing that Citro2D needed for proper 
//clearing. I assume it's because of byte ordering in the 3DS and Citro not
//properly converting the color for that one instance. TODO: ask about that bug
//if anyone ever gets back to you on that forum.
u32 rgba32c_to_rgba16c_32(u32 val)
{
   //Format: 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   //Half  : 0b GGBBBBBA RRRRRGGG 00000000 00000000
   u8 green = (val & 0x0000FF00) >> 11; //crush last three bits

   return 
      (
         (val & 0xFF000000 ? 0x0100 : 0) |   //Alpha is lowest bit on upper byte
         (val & 0x000000F8) |                //Red is slightly nice because it's already in the right position
         ((green & 0x1c) >> 2) | ((green & 0x03) << 14) | //Green is split between bytes
         ((val & 0x00F80000) >> 10)          //Blue is just shifted and crushed
      ) << 16; //first 2 bytes are trash
}

//Convert a citro2D rgba to a proper 16 bit color (but with the opposite byte
//ordering preserved)
u16 rgba32c_to_rgba16c(u32 val)
{
   return rgba32c_to_rgba16c_32(val) >> 16;
}

//Convert a proper 16 bit citro2d color (with the opposite byte ordering
//preserved) to a citro2D rgba
u32 rgba16c_to_rgba32c(u16 val)
{
   //Half : 0b                   GGBBBBBA RRRRRGGG
   //Full : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return
      (
         (val & 0x0100 ? 0xFF000000 : 0)  |  //Alpha always 255 or 0
         ((val & 0x3E00) << 10) |            //Blue just shift a lot
         ((val & 0x0007) << 13) | ((val & 0xc000) >> 3) |  //Green is split
         (val & 0x00f8)                      //Red is easy, already in the right place
      );
}

u16 rgba32c_to_rgba16(u32 val)
{
   //16 : 0b                   ARRRRRGG GGGBBBBB
   //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return 
      ((val & 0x80000000) >> 16) |
      ((val & 0x00F80000) >> 19) | //Blue?
      ((val & 0x0000F800) >> 6) | //green?
      ((val & 0x000000F8) << 7)  //red?
      ;
}

u32 rgba16_to_rgba32c(u16 val)
{
   //16 : 0b                   ARRRRRGG GGGBBBBB
   //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return 
      ((val & 0x8000) ? 0xFF000000 : 0) |
      ((val & 0x001F) << 19) | //Blue?
      ((val & 0x03E0) << 6) | //green?
      ((val & 0x7C00) >> 7)  //red?
      ;
}

//Convert a whole palette from regular RGB (no alpha) to true 16 bit values
//used for in-memory palettes (and written drawing data)
void convert_palette(u32 * original, u16 * destination, u16 size)
{
   for(int i = 0; i < size; i++)
      destination[i] = rgba32c_to_rgba16(rgb24_to_rgba32c(original[i]));
}



// ----------------
// -- MENU UTILS --
// ----------------

//WARN: THIS MALLOCS DATA, YOU MUST CLEAN UP THE DATA!
void initialize_easy_menu_state(struct EasyMenuState * state)
{
   //Need at LEAST menu_items and title
   state->_has_title = (state->title != NULL && strlen(state->title));
   state->_title_height = state->_has_title ? (1 + char_occurrences(state->title, '\n')) : 0;

   //Setup for menu item calculation
   state->_menu_strings = malloc(MAX_MENU_ITEMS * sizeof(char *));
   state->_menu_num = 0;
   state->_menu_strings[0] = state->menu_items;

   //Find the locations of each menu option within the items string
   while(state->_menu_num < MAX_MENU_ITEMS && *state->_menu_strings[state->_menu_num] != 0)
   {
      state->_menu_strings[state->_menu_num + 1] = state->_menu_strings[state->_menu_num] + 
         strlen(state->_menu_strings[state->_menu_num]) + 1;
      state->_menu_num++;
   }

   //Pure madness. But really, if no max height is set, the max height is as
   //big as possible (makes the math... easier? idk).
   if(state->max_height == 0)
      state->max_height = 0xFF;

   //The 2 is for the dots; we make room even if they're not used
   u8 min_totalheight = state->_title_height + 2 + MIN_MENU_DISPLAY; 

   if(state->max_height < min_totalheight)
   {
      LOGDBG("MENU TOO SMALL, INCREASING FROM %d TO %d", state->max_height, min_totalheight);
      state->max_height = min_totalheight;
   }

   //Assume they won't all fit (saves a duplicate calculation in code)
   state->_display_items = state->max_height - state->_title_height - 2; 

   //Oops, they actually would've fit lol
   if(state->_display_items > state->_menu_num) 
      state->_display_items = state->_menu_num;

   state->selected_index = 0;
   state->menu_ofs = 0;
}

//Cleans up any weird data initialized in menu state
void free_easy_menu_state(struct EasyMenuState * state) {
   free(state->_menu_strings);
}

//Clears the drawn easy menu
void clear_easy_menu(struct EasyMenuState * state) {
   printf("\x1b[0m\x1b[%d;1H%*s", state->top, (state->_title_height + state->_display_items + 2) * 50, "");
}

//Draws the easy menu. Does NOT assume the background has been cleared!
void draw_easy_menu(struct EasyMenuState * state)
{
   if(state->_has_title)
      printf("\x1b[0m\x1b[%d;1H %s\n", state->top, state->title); //Still keeping that space for some reason.

   //Show if there's more to the menu 
   printf("\x1b[0m\x1b[%d;3H%s", state->top + state->_title_height, 
         (state->menu_ofs > 0) ? "..." : "   ");
   printf("\x1b[%d;3H%s", state->top + state->_title_height + state->_display_items + 1, 
         (state->_menu_num > state->menu_ofs + state->_display_items) ? "..." : "   ");

   //Print menu. When you get to the selected item, do a different bg
   for(u8 i = 0; i < state->_display_items; i++)
   {
      u32 im = i + state->menu_ofs;
      printf("\x1b[%d;1H\x1b[%s  %-48s", 
         state->top + state->_title_height + 1 + i,
         state->selected_index == im ? "47;30m" : "0m", 
         state->_menu_strings[im]);
   }
}

//Given key presses, modify the given menu state.
bool modify_easy_menu_state(struct EasyMenuState * state, u32 kDown, u32 kRepeat)
{
   if(kDown & state->accept_buttons)
      return true;
   if(kDown & state->cancel_buttons) 
      { state->selected_index= -1; return true; }
   if(kRepeat & KEY_UP) 
      state->selected_index = (state->selected_index - 1 + state->_menu_num) % state->_menu_num;
   if(kRepeat & KEY_DOWN) 
      state->selected_index = (state->selected_index + 1) % state->_menu_num;

   //Move the offset if the selected index goes outside visibility
   if(state->selected_index < state->menu_ofs) 
      state->menu_ofs = state->selected_index;
   if(state->selected_index >= state->menu_ofs + state->_display_items) 
      state->menu_ofs = state->selected_index - state->_display_items + 1;

   return false;
}

//Get the text for the given menu item from a menu item blob
const char * get_menu_item(const char * menu_items, u32 length, u32 item)
{
   u32 strings = 0;

   for(u32 i = 0; i < length; i++)
   {
      //This will always be true on the NEXT character, which is after the \0
      //and is good, keep it first.
      if(strings == item)
         return menu_items + i;

      if(!menu_items[i])
         strings++;
   }

   return NULL;
}



//Menu items must be packed together, separated by \0. Last item needs two \0
//after. CONTROL WILL BE GIVEN FULLY TO THIS MENU UNTIL IT FINISHES!
s32 easy_menu(const char * title, const char * menu_items, u8 top, u8 display, u32 exit_buttons)
{
   struct EasyMenuState state;
   state.title = title;
   state.menu_items = menu_items;
   state.top = top;
   state.max_height = display; //Just for now; but this will severely limit the menus!
   state.cancel_buttons = exit_buttons;
   state.accept_buttons = KEY_A;

   initialize_easy_menu_state(&state);
   clear_easy_menu(&state);

   while(aptMainLoop())
   {
      hidScanInput();

      if(modify_easy_menu_state(&state, hidKeysDown(), hidKeysDownRepeat())) 
         break;

      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      draw_easy_menu(&state);
      C3D_FrameEnd(0);
   }

   clear_easy_menu(&state);
   free_easy_menu_state(&state);

   return state.selected_index;
}

//Set up the menu to be a simple confirmation
bool easy_confirm(const char * title, u8 top) {
   return 1 == easy_menu(title, "No\0Yes\0", top, 0, KEY_B);
}

void easy_ok(const char * title, u8 top) {
   easy_menu(title, "OK\0", top, 0, KEY_B);
}

//Set up the menu to be a warning (still yes/no confirmation though)
bool easy_warn(const char * warn, const char * title, u8 top)
{
   printf("\x1b[%d;1H\x1b[31;40m %-49s", top, warn);
   bool value = easy_confirm(title, top + 1);
   printf("\x1b[%d;1H%-50s",top, "");
   return value;
}


// -- enter text stuff --

void initialize_enter_text_state(struct EnterTextState * state)
{
   state->_has_title = (state->title != NULL && strlen(state->title));
   state->_title_height = state->_has_title ? (1 + char_occurrences(state->title, '\n')) : 0;
   state->_charset_count = strlen(state->charset);

   state->text = malloc(sizeof(char) * (state->entry_max + 1));
   memset(state->text, state->charset[0], state->entry_max); 
   state->text[state->entry_max] = '\0';
   state->selected_index = 0;
   state->confirmed = false;
}

void free_enter_text_state(struct EnterTextState * state) {
   free(state->text);
   state->text = NULL;
}

//3 is: 1 for text entry, 2 for cursor (above and below)
void clear_enter_text(struct EnterTextState * state) {
   printf("\x1b[0m\x1b[%d;1H%*s", state->top, (state->_title_height + 3) * 50, "");
}

void draw_enter_text(struct EnterTextState * state)
{
   if(state->_has_title)
      printf("\x1b[%d;1H\x1b[0m %-49s", state->top, state->title);

   u8 y = state->top + state->_title_height + 1;

   for(u8 i = 0; i < state->entry_max; i++)
   {
      u8 x = 3 + i;
      bool selected = (i == state->selected_index);
      printf("\x1b[%d;%dH%c\x1b[%d;%dH%c\x1b[%d;%dH%c", 
            y, x, state->text[i], //The char data
            y - 1, x, selected ? '-' : ' ',  //The up/down
            y + 1, x, selected ? '-' : ' ');
   }
}

bool modify_enter_text_state(struct EnterTextState * state, u32 kDown, u32 kRepeat)
{
   u8 current_index = (strchr(state->charset, state->text[state->selected_index]) - state->charset);

   if(kDown & state->accept_buttons) { state->confirmed = true; return true; }
   if(kDown & state->cancel_buttons) { state->confirmed = false; return true; }
   if(kRepeat & KEY_RIGHT && state->selected_index < state->entry_max - 1) state->selected_index++;
   if(kRepeat & KEY_LEFT && state->selected_index > 0) state->selected_index--;
   if(kRepeat & KEY_UP) {
      state->text[state->selected_index] = 
         state->charset[(current_index + 1) % state->_charset_count];
   }
   if(kRepeat & KEY_DOWN) {
      state->text[state->selected_index] = 
         state->charset[(current_index - 1 + state->_charset_count) % state->_charset_count];
   }

   return false;
}


//Allows user to submit a fixed length text using the dpad. HIGHLY limited characters
bool enter_text_fixed(const char * title, u8 top, char * container, u8 fixed_length,
      bool clear, u32 exit_buttons)
{
   char chars[ENTERTEXT_CHARARRSIZE];
   strcpy(chars, ENTERTEXT_CHAR);

   struct EnterTextState state;
   state.title = title;
   state.top = top;
   state.charset = chars;
   state.entry_max = fixed_length;
   state.cancel_buttons = exit_buttons;
   state.accept_buttons = KEY_A;

   initialize_enter_text_state(&state);

   //Weird code, but this will all go away anyway
   if(!clear) strcpy(state.text, container);

   clear_enter_text(&state);

   //I want to see how inefficient printf is, so I'm doing this awful on purpose 
   while(aptMainLoop())
   {
      hidScanInput();

      if(modify_enter_text_state(&state, hidKeysDown(), hidKeysDownRepeat())) 
         break;

      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      draw_enter_text(&state);
      C3D_FrameEnd(0);
   }

   //Get the data out BEFORE we free it.
   strcpy(container, state.text);

   clear_enter_text(&state);
   free_enter_text_state(&state);

   return state.confirmed;
}


// -- PRINT STUFF --

//So apparently printf doesn't work unless you do the standard Citro3D frame
//stuff. It only SOMETIMES works, IDK. So, this will wait a full frame to show
//the given message, basically forcing it to be shown even if you're about to
//do a long running task.
void printf_flush(const char * format, ...)
{
   C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);
   C3D_FrameEnd(0);
}

//Taken verbatim from https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p; 

    errno = 0;

    /* Copy string so its mutable */
    if (len > PATH_MAX-1) {
        errno = ENAMETOOLONG;
        LOGDBG("MKDIR_P: DIRECTORY TOO LONG: %d, %s\n", PATH_MAX, path);
        return -1; 
    }   
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0 && errno != EEXIST) {
               LOGDBG("MKDIR_P: Couldn't make inner path: %s\n", _path);
               return -1; 
            }

            *p = '/';
        }
    }   

    if (mkdir(_path, S_IRWXU) != 0 && errno != EEXIST) {
       LOGDBG("MKDIR_P: Couldn't make final path: %s\n", _path);
       return -1; 
    }   

    LOGDBG("Created directory: %s\n", path);

    return 0;
}

//Taken from https://stackoverflow.com/a/230070/1066474
bool file_exists (char * filename)
{
   struct stat buffer;
   return (stat (filename, &buffer) == 0);
}

//Get directories in menu format (separated by \0, last item has 2 \0)
s32 get_directories(char * directory, char * container, u32 c_size)
{
   s32 count = 0;
   char * current_file = container;
   DIR * dir = opendir(directory);

   if(!dir) 
   {
      LOGDBG("ERR: Couldn't open dir %s\n", directory);
      return -1;
   }

   struct dirent * entry = readdir(dir);

   while(entry != NULL)
   {
      if(entry->d_type == DT_DIR)
      {
         u32 len = strlen(entry->d_name);

         //No more files will fit
         if(current_file - container + len + 2 > c_size)
         {
            LOGDBG("WARN: No more room in directory container for %s", directory);
            break;
         }

         //Copy entry into just past the last slot (where the 0 is)
         memcpy(current_file, entry->d_name, len);
         current_file += len;
         *current_file = 0;
         current_file++;

         count++;
      }

      entry = readdir(dir);
   }

   *current_file = 0;

   return count;
}

int write_file(const char * filename, const char * data)
{
   int result = 0;
   FILE * savefile = fopen(filename, "w");
   if(!savefile)
   {
      LOGDBG("ERR: Couldn't open file %s", filename);
      return -1;
   }
   else if(fputs(data, savefile) == EOF)
   {
      LOGDBG("ERR: Couldn't write data to %s", filename);
      result = -2;
   }
   fclose(savefile);
   return result;
}

char * read_file(const char * filename, char * container, u32 maxread)
{
   char * result = NULL;
   FILE * loadfile = fopen(filename, "r");
   if(loadfile == NULL)
   {
      LOGDBG("ERR: Couldn't open file %s", filename);
      return NULL;
   }
   else
   {
      result = fgets(container, maxread, loadfile);
      if(result == NULL)
      {
         LOGDBG("ERR: Couldn't read file %s", filename);
      }
      else
      {
         result = container + strlen(container);
      }
   }

   fclose(loadfile);
   return result;
}

#define MYPNG_FORMAT PNG_COLOR_TYPE_RGBA
#define MYPNG_BYTESPER 4

//Given a citro-formatted array of u32 colors, write a png to the given 
//location. Rawdata needs to be linear and row first. The 

//No, the data must be in a linear array, row first, of PNG formatted RGBA
//bytes. Ironically, citro-formatted colors on the 3ds are already like this,
//as the 3ds is little endian just like png
int write_citropng(u32 * rawdata, u16 width, u16 height, char * filepath)
{
   //A lot of this is taken from http://www.labbookpages.co.uk/software/imgProc/libPNG.html
   int code = 0;
   FILE *fp = NULL;
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;

   // Open file for writing (binary mode)
   fp = fopen(filepath, "wb");
   if (fp == NULL) {
      LOGDBG("ERR: Couldn't open png for writing: %s\n", filepath);
      code = 1;
      goto finalise;
   }

   // Initialize write structure
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (png_ptr == NULL) {
      LOGDBG("ERR: Could not allocate png write struct\n");
      code = 1;
      goto finalise;
   }

   // Initialize info structure
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      LOGDBG("ERR: Could not allocate png info struct\n");
      code = 1;
      goto finalise;
   }

   // Setup Exception handling
   if (setjmp(png_jmpbuf(png_ptr))) {
      LOGDBG("General error during png creation\n");
      code = 1;
      goto finalise;
   }

   //Use the filepointer we got before for png io
   png_init_io(png_ptr, fp);

   // Write header (8 bit colour depth)
   png_set_IHDR(png_ptr, info_ptr, width, height,
         8, MYPNG_FORMAT, PNG_INTERLACE_NONE,
         PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   png_write_info(png_ptr, info_ptr);

   // Write image data
   for (u32 y=0 ; y<height ; y++) {
      png_write_row(png_ptr, (png_bytep)(rawdata + y * width));
   }

   // End write
   png_write_end(png_ptr, NULL);

finalise:
   if (fp != NULL) fclose(fp);
   if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
   if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

   return code;

}


// -----------------
// -- INPUT UTILS --
// -----------------

float cpad_translate(struct CpadProfile * profile, s16 cpad_magnitude, float existing_pos)
{
   u16 cpadmag = abs(cpad_magnitude);

   if(cpadmag > profile->deadzone)
   {
      return existing_pos + (cpad_magnitude < 0 ? -profile->mod_general : profile->mod_general) * 
            (profile->mod_constant + pow(cpadmag * profile->mod_multiplier, profile->mod_curve));
   }
   else
   {
      return existing_pos;
   }
}

//NOTE: assumes input has already been prepared for retrieval
void input_std_get(struct InputSet * input)
{
   input->k_down = hidKeysDown();
   input->k_up = hidKeysUp();
   input->k_held = hidKeysHeld();
   input->k_repeat = hidKeysDownRepeat();
   hidTouchRead(&input->touch_position);
   hidCircleRead(&input->circle_position);
}

u32 input_mod_lefty_single(u32 input)
{
   input = swap_bits_mask(input, KEY_L, KEY_R);
   input = swap_bits_mask(input, KEY_UP, KEY_X);
   input = swap_bits_mask(input, KEY_RIGHT, KEY_A);
   input = swap_bits_mask(input, KEY_LEFT, KEY_Y);
   input = swap_bits_mask(input, KEY_DOWN, KEY_B);
   return input;
}

void input_mod_lefty(struct InputSet * input)
{
   input->k_down = input_mod_lefty_single(input->k_down);
   input->k_up = input_mod_lefty_single(input->k_up);
   input->k_held = input_mod_lefty_single(input->k_held);
   input->k_repeat = input_mod_lefty_single(input->k_repeat);
}

