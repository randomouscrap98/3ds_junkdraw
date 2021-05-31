#include "myutils.h"



// -------------------
// -- GENERAL UTILS --
// -------------------

u32 u32_clamp(u32 x, u32 mn, u32 mx)
{
   return x <= mn ? mn : x >= mx ? mx : x;
}

float float_clamp(float x, float mn, float mx)
{
   return x <= mn ? mn : x >= mx ? mx : x;
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

//Convert a whole palette from regular RGB (no alpha) to true 16 bit values
//used for in-memory palettes (and written drawing data)
void convert_palette(u32 * original, u16 * destination, u16 size)
{
   for(int i = 0; i < size; i++)
      destination[i] = rgba32c_to_rgba16c(rgb24_to_rgba32c(original[i]));
}



