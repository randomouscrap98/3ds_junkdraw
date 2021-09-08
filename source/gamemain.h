
#ifndef __HEADER_GAMEMAIN
#define __HEADER_GAMEMAIN

#include <3ds.h>

#include "lib/gameevent.h"

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

typedef struct GameState * gs;
typedef void (*game_event_handler)(gs);

//ALL the events, gosh is this even OK??
void set_gstate_inputs(gs gstate);
void draw_controls(gs gstate);


struct DefaultEventPriority
{
   game_event_handler handler;
   u8 priority;
};  /* C's shortcut struct define/initialization */

extern struct DefaultEventPriority default_event_priorities[];

#endif
