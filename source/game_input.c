
#include "game_input.h"

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

u32 input_to_action(struct InputSet * input)
{
   u32 result = 0;

   if(input->k_repeat & KEY_DUP) 
      result |= IUA_PAGEUP;
   if(input->k_repeat & KEY_DDOWN) 
      result |= IUA_PAGEDOWN;
   if(input->k_repeat & KEY_DRIGHT) 
      result |= (input->k_held & KEY_R ? IUA_WIDTHUPBIG : IUA_WIDTHUP);
   if(input->k_repeat & KEY_DLEFT) 
      result |= (input->k_held & KEY_R ? IUA_WIDTHDOWNBIG : IUA_WIDTHDOWN);
   if(input->k_down & KEY_A) 
      result |= IUA_TOOLA;
   if(input->k_down & KEY_B) 
      result |= IUA_TOOLB;
   if(input->k_down & KEY_START) 
      result |= IUA_MENU;

   if(input->k_down & KEY_L)
   {
      if(input->k_held & KEY_R)
         result |= IUA_PALETTESWAP;
      else
         result |= IUA_PALETTETOGGLE;
   }

   if(input->k_down & KEY_SELECT)
   {
      if(input->k_held & KEY_R)
         result |= IUA_EXPORTPAGE;
      else
         result |= IUA_NEXTLAYER;
   }

   return result;
}

////This is too big to put on the stack, and I want it all in one place, so here
////we are doing it this way. This is like... 150 bytes max, come on.
//struct InputAction[] game_input_default_actions {
//   { , }
//};
//
//u32 malloc_inputactions_default(struct InputAction * result)
//{
//
//}
