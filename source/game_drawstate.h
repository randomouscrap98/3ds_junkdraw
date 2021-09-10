
#ifndef __HEADER_GAMEDRAWSTATE
#define __HEADER_GAMEDRAWSTATE

#include <3ds.h>


//Variables for modifying display of drawing
struct ScreenState
{
   float offset_x;
   float offset_y;
   float zoom;

   u16 layer_width;
   u16 layer_height;

   //These are pretty standard, but included just in case..
   u16 screen_width;
   u16 screen_height;
};

//Safely adjust the screen offset given new desired offsets (doesn't let you
//set to unsafe values, etc)
void set_screenstate_ofs(struct ScreenState * state, u16 offset_x, u16 offset_y);

//Adjust the zoom while preserving subjective location in some manner.
void set_screenstate_zoom(struct ScreenState * state, float zoom);


#endif
