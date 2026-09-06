#ifndef __HEADER_INPUT
#define __HEADER_INPUT

#include <3ds.h>

struct InputSet
{
   circlePosition circle_position;
   touchPosition touch_position;
   u32 k_down;
   u32 k_repeat;
   u32 k_up; 
   u32 k_held; 
};

//Retrieve the standard inputs and fill the given struct
void input_std_get(struct InputSet * input);
u32 input_mod_lefty_single(u32 input);
void input_mod_lefty(struct InputSet * input);

struct CpadProfile
{
   u16 deadzone;
   float mod_constant;
   float mod_multiplier;
   float mod_curve;
   float mod_general;
};

float cpad_translate(struct CpadProfile * profile, s16 cpad_magnitude, float existing_pos);

#endif