#include "system.h"

#include <citro2d.h>

// ---------- lAYERS ----------

// Create a LAYER from an off-screen texture.
void create_layer(struct LayerData *result, Tex3DS_SubTexture subtex) {
  result->subtex = subtex;
  C3D_TexInitVRAM(&(result->texture), subtex.width, subtex.height,
                  LAYER_FORMAT);
  result->target =
      C3D_RenderTargetCreateFromTex(&(result->texture), GPU_TEXFACE_2D, 0, -1);
  result->image.tex = &(result->texture);
  result->image.subtex = &(result->subtex);
}

// Clean up a layer created by create_page
void delete_layer(struct LayerData page) {
  C3D_RenderTargetDelete(page.target);
  C3D_TexDelete(&page.texture);
}

// ---------- SCREEN STATE ------------

void set_screenstate_offset(struct ScreenState *state, u16 offset_x,
                            u16 offset_y) {
  float maxofsx = state->layer_width * state->zoom - state->screen_width;
  float maxofsy = state->layer_height * state->zoom - state->screen_height;
  state->offset_x = C2D_Clamp(offset_x, 0, maxofsx < 0 ? 0 : maxofsx);
  state->offset_y = C2D_Clamp(offset_y, 0, maxofsy < 0 ? 0 : maxofsy);
}

void set_screenstate_zoom(struct ScreenState *state, float zoom) {
  float zoom_ratio = zoom / state->zoom;
  u16 center_x = state->screen_width >> 1;
  u16 center_y = state->screen_height >> 1;
  u16 new_ofsx = zoom_ratio * (state->offset_x + center_x) - center_x;
  u16 new_ofsy = zoom_ratio * (state->offset_y + center_y) - center_y;
  state->zoom = zoom;
  set_screenstate_offset(state, new_ofsx, new_ofsy);
}

// ---------- DRAWSTATE --------------

// void shift_drawstate_color(struct DrawState *state, s16 ofs) {
//   u16 i = state->current_color - state->palette;
//   state->current_color = state->palette + ((i + ofs + state->palette_count) %
//                                            (state->palette_count));
// }

void shift_drawstate_width(struct DrawState *state, s16 ofs) {
  // instead of rolling, this one clamps
  state->current_tool->width = C2D_Clamp(state->current_tool->width + ofs,
                                         state->min_width, state->max_width);
}

// u16 get_drawstate_color(struct DrawState *state) {
//   return state->current_tool->has_static_color
//              ? state->current_tool->static_color
//              : *state->current_color;
// }

void set_drawstate_tool(struct DrawState *state, u8 tool) {
  state->current_tool = state->tools + tool;
}

// int try_set_drawstate_color(struct DrawState *state, u16 color, u16 block) {
//   int start = state->current_color - state->palette;
//   if (block > 0) { // Start at beginning of given block size
//     start -= (start % block);
//   }
//   for (int i = 0; i < state->palette_count; i++) {
//     u16 pali = (i + start) % state->palette_count;
//     if (state->palette[pali] == color) {
//       state->current_color = state->palette + pali;
//       return 1;
//     }
//   }
//   return 0;
// }

u8 get_drawstate_tool(const struct DrawState *state) {
  return state->current_tool - state->tools;
}
