
#ifndef __HEADER_GAMEEVENT
#define __HEADER_GAMEEVENT

#include <3ds.h>

//This may be a very poor estimate, but we'll see.
//#define MAX_GAME_EVENTS 1000
#define LOW_EVENT_PRIORITY 50
#define DEFAULT_EVENT_PRIORITY 100
#define HIGH_EVENT_PRIORITY 150
#define CRITICAL_EVENT_PRIORITY 200

struct GameEvent
{
   void * handler;
   u8 priority;
   u32 id;

   struct GameEvent * next_event;
};

void reset_gameevent_globalid();
struct GameEvent * insert_gameevent(struct GameEvent ** event_queue, void * handler, u8 priority);
s32 remove_gameevent(struct GameEvent ** event_queue, u32 id);
u32 free_gameevent_queue(struct GameEvent ** event_queue);
u32 gameevent_queue_count(struct GameEvent ** event_queue);

#endif
