#include "layercompositor.h"

#include <citro2d.h>

int layercompositor_init_screen(LayerCompositor * c, int screen) {
  c->screen = C2D_CreateScreenTarget(screen, GFX_LEFT);
  if(!c->screen) { return 1; }
  layercompositor_reset(c);
  return 0;
}

void layercompositor_free(LayerCompositor * c) {
  C3D_RenderTargetDelete(c->screen);
}

void layercompositor_reset_view(LayerCompositor * c) {
  c->zoom = 1;
  c->offset_x = 0;
  c->offset_y = 0;
}

void layercompositor_reset_visuals(LayerCompositor * c) {
  c->canvas_color = JDLCM_CANVAS_COLOR;
  c->screen_color = JDLCM_SCREEN_COLOR;
  c->scroll_width = JDLCM_SCROLL_WIDTH;
  c->scroll_color_bg = JDLCM_SCROLL_COLOR_BG;
  c->scroll_color_bar = JDLCM_SCROLL_COLOR_BAR;
}

void layercompositor_reset(LayerCompositor *c) {
  layercompositor_reset_view(c);
  layercompositor_reset_visuals(c);
}

static inline void layercompositor_draw_scrollbars(LayerCompositor * c) {

}

void layercompositor_draw(LayerCompositor * c, Layer ** layers, size_t layer_count) {
  C2D_SceneBegin(c->screen);
  // The empty background color
  C2D_TargetClear(c->screen, c->screen_color);
  if(layer_count <= 0) { goto DRAWEND; }
  // ASSUME the first layer has the info; draw the canvas background itself
  C2D_DrawRectSolid(-c->offset_x, -c->offset_y, 0.5f,
                    layers[0]->width * c->zoom, layers[0]->height * c->zoom,
                    c->canvas_color);
DRAWEND:
  C2D_Flush();
}

