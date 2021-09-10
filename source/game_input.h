
#ifndef __HEADER_GAMEINPUT
#define __HEADER_GAMEINPUT

#include <3ds.h>

// User Actions 
#define IUA_PAGEUP         0x0001
#define IUA_PAGEDOWN       0x0002
#define IUA_PALETTETOGGLE  0x0004
#define IUA_WIDTHUP        0x0008
#define IUA_WIDTHDOWN      0x0010
#define IUA_TOOLA          0x0020
#define IUA_TOOLB          0x0040
#define IUA_PALETTESWAP    0x0080
#define IUA_MENU           0x0100
#define IUA_EXPORTPAGE     0x0200
#define IUA_NEXTLAYER      0x0400
#define IUA_WIDTHUPBIG     0x0800
#define IUA_WIDTHDOWNBIG   0x1000

//#define U_KDOWN      0x01
//#define U_KUP        0x02
//#define U_KREPEAT    0x04
//#define U_KHELD      0x08

struct InputSet
{
   circlePosition circle_position;
   touchPosition touch_position;
   u32 k_down;
   u32 k_repeat;
   u32 k_up; 
   u32 k_held; 
};

//struct InputAction
//{
//   u32 k_held_combo;
//   u32 k_down_combo;
//   u32 k_repeat_combo;
//   u32 action;
//};

//Retrieve the standard inputs and fill the given struct
void input_std_get(struct InputSet * input);

//Returns a bitfield representing all "game actions" taken based on the given input.
u32 input_to_action(struct InputSet * input); //, struct InputAction * mapping);

//Set a pointer to point to a malloc'd array of the default input actions. You
//can manually free the result yourself if you want. The return value is the
//array size.
//u32 malloc_inputactions_default(struct InputAction * result);

#endif
