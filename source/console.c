#include "console.h"

#include <stdarg.h>
#include <citro3d.h>

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