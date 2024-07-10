#include "input.h"

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

