#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/myutils.h"
#include "lib/gameevent.h"

#include "gamemain.h"


void initialize_gamestate(struct GameState * state)
{
   memset(state, 0, sizeof(struct GameState));
}

struct DefaultEventPriority default_event_priorities[] = { 
   { set_gstate_inputs, CRITICAL_EVENT_PRIORITY + 20 },
   { draw_controls, HIGH_EVENT_PRIORITY }
};


void set_gstate_inputs(gs gstate)
{
   gstate->k_down = hidKeysDown();
   gstate->k_repeat = hidKeysDownRepeat();
   gstate->k_up = hidKeysUp();
   gstate->k_held = hidKeysHeld();
   hidTouchRead(&gstate->touch_position);
   hidCircleRead(&gstate->circle_position);
}

void draw_controls(gs gstate)
{

}
