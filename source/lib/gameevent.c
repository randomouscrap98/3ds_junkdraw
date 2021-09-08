#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gameevent.h"
#include "myutils.h"

void initialize_gamestate(struct GameState * state)
{
   memset(state, 0, sizeof(struct GameState));
}

u32 insert_event_globalid = 0;

void reset_gameevent_globalid() { insert_event_globalid = 0; }

void initialize_gameevent(struct GameEvent * event)
{
   event->id = ++insert_event_globalid;
   event->next_event = NULL;
}

//Insert an event into the given queue. NULL queue is empty queue. Events with
//the same priority will preserve insert temporal order (oldest first).
struct GameEvent * insert_gameevent(struct GameEvent ** event_queue, game_event_handler handler, u8 priority)
{
   //Always create a new event
   struct GameEvent * new_event = malloc(sizeof(struct GameEvent));

   if(new_event == NULL)
   {
      LOGDBG("COULD NOT ALLOCATE GAME EVENT!");
      return NULL;
   }

   initialize_gameevent(new_event);
   new_event->handler = handler;
   new_event->priority = priority;

   struct GameEvent ** insert_here;

   int pos = 0;
   //This loop finds us the perfect spot to insert. We start by inspecting the
   //head of the list. We stop if we ever reach the "end" of the list (NULL),
   //OR if the priority of the element we're pointing at is strictly less than
   //ours. For instance, consider these situations:
   //1) if there are no elements, this loop ends immediately and insert_here
   //   points to the list head. The new element becomes the list head
   //2) if there is one element and it has a lower priority, the loop will
   //   immediately halt, because our priority is higher.
   //3) if there's elements 50, 40, 30, inserting 50 will skip the first and
   //   stop on the second. position 2 is where we are inserted.
   for(insert_here = event_queue; 
       (*insert_here) != NULL && (*insert_here)->priority >= priority; 
       insert_here = &((*insert_here)->next_event))
   {
      pos++;
   }

   //printf("p%d;ih%p;", pos, *insert_here);

   //Insert ourselves wherever we're needed. First, attach the current insert
   //element to ourselves (if it exists). Then, just dump ourselves in the spot.
   if((*insert_here) != NULL) new_event->next_event = (*insert_here);
   (*insert_here) = new_event;

   return new_event;
}

//FULLY remove the given event, which means DESTROYING the event too
s32 remove_gameevent(struct GameEvent ** event_queue, u32 id)
{
   //Find the element. If there is none, do nothing.
   struct GameEvent ** remove_event;
   
   s32 index = 0;

   for(remove_event = event_queue; 
       (*remove_event) != NULL && (*remove_event)->id != id;
       remove_event = &((*remove_event)->next_event))
   {
      index++;
   }

   //At this point, if remove_event points to null, just quit.
   if(*remove_event == NULL)
      return -1;

   //The pointer we're pointing to is NOT the event itself, it's the pointer
   //from the PREVIOUS event. To remove, we take our current next and assign
   //it to the base pointer.
   struct GameEvent * free_this = (*remove_event);
   (*remove_event) = free_this->next_event;
   free(free_this);

   return index;
}

//Free ALL the game events from the given queue
u32 free_gameevent_queue(struct GameEvent ** event_queue)
{
   struct GameEvent* cur = (*event_queue); 
   u32 count = 0;

   while(cur != NULL)
   {
      struct GameEvent * next = cur->next_event;
      free(cur);
      cur = next;
      count++;
   } 

   (*event_queue) = NULL;

   return count;
}

u32 gameevent_queue_count(struct GameEvent ** event_queue)
{
   struct GameEvent* cur = (*event_queue); 
   u32 count = 0;

   while(cur != NULL)
   {
      count++;
      cur = cur->next_event;
   } 

   return count;
}
