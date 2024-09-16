#ifndef __HEADER_CONSOLE
#define __HEADER_CONSOLE

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_MENU_ITEMS 10000
#define MIN_MENU_DISPLAY 1
#define ENTERTEXT_CHAR "_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define ENTERTEXT_CHARARRSIZE 50

// C lets you redefine stuff... right?
#define LOGDBG(f_, ...)

// General utils (is this really console?)
u32 char_occurrences(const char * string, char c);

// All the information needed to draw a menu in a given state
struct EasyMenuState
{
   //You need to set these for the menu to function all right
   const char * title;
   const char * menu_items;
   u8 top;
   u8 max_height; //set to 0 for unlimited height
   u32 accept_buttons;
   u32 cancel_buttons;
   
   //Cached data (you don't need to set these)
   const char ** _menu_strings;
   u32 _menu_num;
   bool _has_title;
   u8 _title_height;
   u8 _display_items; 

   //Menu tracking
   u32 selected_index;
   u32 menu_ofs;
   bool home_rejected;
};

//Generic menu operations; can be used without context
void initialize_easy_menu_state(struct EasyMenuState * state);
void free_easy_menu_state(struct EasyMenuState * state);
void draw_easy_menu(struct EasyMenuState * state);
void clear_easy_menu(struct EasyMenuState * state);
bool modify_easy_menu_state(struct EasyMenuState * state, u32 kdown, u32 krepeat);

const char * get_menu_item(const char * menu_items, u32 length, u32 item);

//Actual menu implementations. A little more exact in use case;
//might get replaced
s32 easy_menu(const char * title, const char * menu_items, u8 top, u8 display, u32 index, u32 exit_btns);
void easy_ok(const char * title, u8 top);
bool easy_confirm(const char * title, u8 top); 
bool easy_warn(const char * warn, const char * title, u8 top);

struct EnterTextState
{
   //You need to set these for the input to function
   const char * title;
   u8 top;
   char * charset;
   u8 entry_max;
   u32 accept_buttons;
   u32 cancel_buttons;

   bool _has_title;
   u8 _title_height;
   u8 _charset_count;

   //State tracking
   bool confirmed;
   u32 selected_index;
   char * text;
   bool home_rejected;
};

//Generic text input operations.
void initialize_enter_text_state(struct EnterTextState * state);
void free_enter_text_state(struct EnterTextState * state);
void draw_enter_text(struct EnterTextState * state);
void clear_enter_text(struct EnterTextState * state);
bool modify_enter_text_state(struct EnterTextState * state, u32 kdown, u32 krepeat);

bool enter_text_fixed(const char * title, u8 top, char * container, 
      u8 max_entry, bool clear, u32 exit_btns);

// PRINT STUFF
void printf_flush(const char * format, ...);

#endif