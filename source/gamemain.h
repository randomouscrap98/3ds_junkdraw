
#ifndef __HEADER_GAMEMAIN
#define __HEADER_GAMEMAIN

#include <3ds.h>

//Represents most of what you'd need to keep track of general state.
//Consider splitting out the various portions in preparation for a more
//"modular" approach... if that's what you need, anyway.
struct GameState
{
   u32 frame;   

   //Inputs
   circlePosition circle_position;
   touchPosition touch_position;
   u32 k_down;
   u32 k_repeat;
   u32 k_up; 
   u32 k_held; 

   // drawing state
   s8 zoom_power;
   s8 last_zoom_power;
   u16 draw_page;
   u8 draw_tool;
};
void initialize_gamestate(struct GameState * state);

typedef void * (*game_event_handler)(struct GameState *);


#endif
