
#ifndef __HEADER_GAMEINPUT
#define __HEADER_GAMEINPUT

#include <3ds.h>

#include "lib/myutils.h"


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
u32 input_to_action(struct InputSet * input); //, struct InputAction * mapping);



#endif
