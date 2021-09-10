
#include <citro2d.h>

#include "game_drawstate.h"


void set_screenstate_offset(struct ScreenState * state, u16 offset_x, u16 offset_y)
{
   float maxofsx = state->layer_width * state->zoom - state->screen_width;
   float maxofsy = state->layer_height * state->zoom - state->screen_height;
   state->offset_x = C2D_Clamp(offset_x, 0, maxofsx < 0 ? 0 : maxofsx);
   state->offset_y = C2D_Clamp(offset_y, 0, maxofsy < 0 ? 0 : maxofsy);
}

void set_screenstate_zoom(struct ScreenState * state, float zoom)
{
   float zoom_ratio = zoom / state->zoom;
   u16 center_x = state->screen_width >> 1;
   u16 center_y = state->screen_height >> 1;
   u16 new_ofsx = zoom_ratio * (state->offset_x + center_x) - center_x;
   u16 new_ofsy = zoom_ratio * (state->offset_y + center_y) - center_y;
   state->zoom = zoom;
   set_screenstate_ofs(state, new_ofsx, new_ofsy);
}

