#include "input.h"

#include <math.h>
#include <stdlib.h>

// --------- utils ----------

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

// --------- input stuff -----------

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

