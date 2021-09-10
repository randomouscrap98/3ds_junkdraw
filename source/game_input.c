
#include <stdlib.h>
#include <math.h>

#include "game_input.h"

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

void set_cpadprofile_canvas(struct CpadProfile * profile)
{
   profile->deadzone = 40;
   profile->mod_constant = 1;
   profile->mod_multiplier = 0.02f;
   profile->mod_curve = 3.2f;
   profile->mod_general = 1;
}

