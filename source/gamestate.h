
#ifndef __HEADER_GAMESTATE
#define __HEADER_GAMESTATE

#include <3ds.h>

//This may be a very poor estimate, but we'll see.
#define MAX_GAME_EVENTS 1000
#define DEFAULT_EVENT_PRIORITY 20
#define HIGH_EVENT_PRIORITY 40
#define CRITICAL_EVENT_PRIORITY 60

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

   // drawing state
   s8 zoom_power;
   s8 last_zoom_power;
   u16 draw_page;
   u8 draw_tool;
};

typedef void * (*game_event_handler)(struct GameState *);

struct GameEvent
{
   game_event_handler handler;
   u8 priority;
   u32 id;

   struct GameEvent * next_event;
};

void initialize_gamestate(struct GameState * state);
struct GameEvent * insert_gameevent(struct GameEvent ** event_queue, game_event_handler handler, u8 priority);
s32 remove_gameevent(struct GameEvent ** event_queue, u32 id);

#endif
