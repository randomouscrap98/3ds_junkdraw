
#include <citro2d.h>
#include <stdlib.h>

#include "lib/myutils.h"
#include "game_drawctrl.h"



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
   set_screenstate_offset(state, new_ofsx, new_ofsy);
}

void shift_drawstate_color(struct DrawState * state, s16 ofs)
{
   u16 i = state->current_color - state->palette;
   state->current_color = state->palette +
      ((i + ofs + state->palette_count) % (state->palette_count));
}

void shift_drawstate_width(struct DrawState * state, s16 ofs)
{
   //instead of rolling, this one clamps
   state->current_tool->width =
      C2D_Clamp(state->current_tool->width + ofs, state->min_width, state->max_width);
}

u16 get_drawstate_color(struct DrawState * state)
{
   return state->current_tool->has_static_color ? 
      state->current_tool->static_color : *state->current_color;
}

void set_drawstate_tool(struct DrawState * state, u8 tool)
{
   state->current_tool = state->tools + tool;
}
