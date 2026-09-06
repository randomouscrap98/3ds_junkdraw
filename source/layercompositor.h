#ifndef __HEADER_3DSJUNKDRAW_LAYERCOMPOSITOR__
#define __HEADER_3DSJUNKDRAW_LAYERCOMPOSITOR__

#include <3ds.h>
#include <citro3d.h>

#include "layer.h"

#define JDLCM_SCREEN_COLOR C2D_Color32(90, 90, 90, 255)
#define JDLCM_CANVAS_COLOR C2D_Color32(255, 255, 255, 255)
#define JDLCM_SCROLL_WIDTH 3
#define JDLCM_SCROLL_COLOR_BG  C2D_Color32f(0.8, 0.8, 0.8, 1)
#define JDLCM_SCROLL_COLOR_BAR C2D_Color32f(0.5, 0.5, 0.5, 1)

#define JDLCM_MODTYPE_NONE          0
#define JDLCM_MODTYPE_ONIONOPACITY  1

typedef union {
  float opacity;
} LayerDrawMod;

typedef struct {
  Layer * layer;        // Just a pointer to somewhere
  LayerDrawMod mod;     // What kind of modification to give the layer
  int modtype;          // Which mod it is
} LayerDraw;

#define LAYERDRAW_NOMOD(_layer) (LayerDraw) { \
  .layer = _layer, \
  .modtype = JDLCM_MODTYPE_NONE, \
}

#define LAYERDRAW_ONIONOPACITY(_layer, _op) (LayerDraw) { \
  .layer = _layer, \
  .mod.opacity = _op, \
  .modtype = JDLCM_MODTYPE_ONIONOPACITY, \
}

typedef struct {
  float zoom;
  float offset_x;
  float offset_y;
  u32 canvas_color;
  u32 screen_color;
  u32 scroll_color_bg;
  u32 scroll_color_bar;
  u16 scroll_width;
  C3D_RenderTarget * screen;
} LayerCompositor;

// Initialize compositor to point to the given screen (GFX_BOTTOM, etc);
int layercompositor_init_screen(LayerCompositor * c, int screen);
void layercompositor_free(LayerCompositor * c);
// Reset compositor view (position, zoom, etc);
void layercompositor_reset_view(LayerCompositor * c);
// Reset compositor visuals (colors, etc)
void layercompositor_reset_visuals(LayerCompositor * c);
// Reset the compositor entirely
void layercompositor_reset(LayerCompositor * c);

void layercompositor_draw(LayerCompositor * c, LayerDraw * layers, size_t layer_count);

#endif
