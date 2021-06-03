#include "myutils.h"
#include "string.h"



// -------------------
// -- GENERAL UTILS --
// -------------------




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

//Menu items must be packed together, separated by \0. Last item needs two \0
//after. CONTROL WILL BE GIVEN FULLY TO THIS MENU UNTIL IT FINISHES!
s8 easy_menu(const char * title, const char * menu_items, u8 top, u32 exit_buttons)
{
   s8 menu_on = 0;
   u8 menu_num = 0;
   const char * menu_str[MAX_MENU_ITEMS];
   menu_str[0] = menu_items;
   bool has_title = (title != NULL && strlen(title));

   while(menu_num < MAX_MENU_ITEMS && *menu_str[menu_num] != 0)
   {
      menu_str[menu_num + 1] = menu_str[menu_num] + strlen(menu_str[menu_num]) + 1;
      menu_num++;
   }

   //I want to see how inefficient printf is, so I'm doing this awful on purpose 
   while(aptMainLoop())
   {
      gspWaitForVBlank();
      hidScanInput();
      u32 kDown = hidKeysDown();
      if(kDown & KEY_A) break;
      if(kDown & exit_buttons) { menu_on = -1; break; }
      if(kDown & KEY_UP) menu_on = (menu_on - 1 + menu_num) % menu_num;
      if(kDown & KEY_DOWN) menu_on = (menu_on + 1) % menu_num;

      //Print title, 1 over
      if(has_title)
      {
         printf("\x1b[%d;1H\x1b[0m %-49s", top, title);
         printf("%-50s","");
      }

      //Print menu. When you get to the selected item, do a different bg
      for(u8 i = 0; i < menu_num; i++)
      {
         u8 menutop = top + (has_title ? 2 : 0) + i;
         if(menu_on == i)
            printf("\x1b[%d;1H\x1b[47m\x1b[30m  %-48s", menutop, menu_str[i]);
         else
            printf("\x1b[%d;1H\x1b[0m  %-48s", menutop, menu_str[i]);
      }
   }

   //Clear the menu area
   for(u8 i = 0; i < (has_title ? 2 : 0) + menu_num; i++)
      printf("\x1b[%d;1H\x1b[0m%-50s", top + i, "");

   return menu_on;
}

bool easy_confirm(const char * title, u8 top)
{
   return 1 == easy_menu(title, "No\0Yes\0", top, KEY_B);
}

bool easy_warn(const char * warn, const char * title, u8 top)
{
   printf("\x1b[%d;1H\x1b[31m\x1b[40m %-49s", top, warn);
   bool value = easy_confirm(title, top + 1);
   printf("\x1b[%d;1H%-50s",top, "");
   return value;
}

