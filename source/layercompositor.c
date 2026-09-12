#include "layercompositor.h"
#include "layer.h"
#include "utils.h"

#include <citro2d.h>

static inline void layercompositor_dims(LayerCompositor * c, u16 * width, u16 * height) {
  // It's flipped remember
  *width = c->screen->frameBuf.height;
  *height = c->screen->frameBuf.width;
}

int layercompositor_init_screen(LayerCompositor * c, int screen) {
  c->screen = C2D_CreateScreenTarget(screen, GFX_LEFT);
  if(!c->screen) { return 1; }
  layercompositor_reset(c);
  u16 width, height;
  layercompositor_dims(c, &width, &height);
  LOGTRC("Screen: %dx%d", width, height);
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

static inline void layercompositor_draw_scrollbars(LayerCompositor * c, const Layer * linfo) {
  u16 swidth, sheight;
  layercompositor_dims(c, &swidth, &sheight);
  float fill_w = swidth / (float)linfo->width / c->zoom;
  float fill_h = sheight / (float)linfo->height / c->zoom;
  u16 sofs_x = fill_w * (float)c->offset_x;
  u16 sofs_y = fill_h * (float)c->offset_y;

  // Bottom and right scrollbar bg
  if(fill_w < 1.0f) {
    C2D_DrawRectSolid(0, sheight - c->scroll_width, 0.5f,
                      swidth, c->scroll_width, c->scroll_color_bg);
  }
  if(fill_h < 1.0f) {
    C2D_DrawRectSolid(swidth - c->scroll_width, 0, 0.5f, c->scroll_width,
                      sheight, c->scroll_color_bg);
  }
  // bottom and right scrollbar bar
  if(fill_w < 1.0f) {
    C2D_DrawRectSolid(sofs_x, sheight - c->scroll_width, 0.5f,
                      swidth * fill_w, c->scroll_width, c->scroll_color_bar);
  }
  if(fill_h < 1.0f) {
    C2D_DrawRectSolid(swidth - c->scroll_width, sofs_y, 0.5f,
                    c->scroll_width, sheight * fill_h, c->scroll_color_bar);
  }
}

void layercompositor_draw(LayerCompositor * c, LayerDraw * layers, size_t layer_count) {
  // IDK, this was from the original junkdraw code.
  C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA,
                 GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
  C2D_SceneBegin(c->screen);
  // -- The empty background color --
  C2D_TargetClear(c->screen, c->screen_color);
  if(layer_count <= 0) { goto DRAWEND; }
  // -- The canvas background before layers (they should be transparent) --
  // ASSUME the first layer has the info; draw the canvas background itself
  C2D_DrawRectSolid(-c->offset_x, -c->offset_y, 0.5f,
                    layers[0].layer->width * c->zoom, layers[0].layer->height * c->zoom,
                    c->canvas_color);
  u16 swidth, sheight;
  layercompositor_dims(c, &swidth, &sheight);
  // -- Draw the layers -- 
  C2D_ImageTint tint;
  for (int l = 0; l < layer_count; l++) {
    C2D_ImageTint * tintptr;
    // Layers can be "modified" by various things before drawing.
    switch(layers[l].modtype) {
      case JDLCM_MODTYPE_ONIONOPACITY:
        // Onion Opacity is "fake" because I'm just blending the colors with the canvas color. this
        // doesn't actually work for "normal" opacity but it's exactly what these should be
        for (int i = 0; i < 4; i++) {
          tint.corners[i].color = c->canvas_color;
          tint.corners[i].blend = 1.0f - layers[l].mod.opacity;
          // NOTE: opacity is 0 for full transparent, which for fake opacity should be 1 for
          // full color blend, hence 1 - opacity
        }
        tintptr = &tint;
        break;
      default:
        tintptr = NULL;
        break;
    }
    // TODO: Right now we ONLY composite hardware layers...
    if(layers[l].layer->type == JDL_TYPE_HARDWARE) {
      S32Bounds bounds;
      layer_mapped_area(layers[l].layer, &bounds);
      C2D_DrawImageAt(layers[l].layer->texture.hw.image,
                      -c->offset_x - bounds.x1 * c->zoom,
                      -c->offset_y - bounds.y1 * c->zoom,
                      0.5f, tintptr, c->zoom, c->zoom);
    }
  }
  // TODO: I don't actually know what this was for, but old junkdraw did it. I may remove it later
  // and see what happens. Might be because texture is power of 2 and there's no cropping on the
  // thing above (that's probably what it was... yeah)
  float canvas_x = layers[0].layer->width * c->zoom - c->offset_x;
  float canvas_y = layers[0].layer->height * c->zoom - c->offset_y;
  // This is rather wasteful but eh...
  C2D_DrawRectSolid(canvas_x, 0, 0.5f,  // pos
                    swidth - canvas_x, sheight, c->screen_color);
  C2D_DrawRectSolid(0, canvas_y, 0.5f,  // pos
                    canvas_x, sheight, c->screen_color);
  // -- Draw the scrollbars --
  layercompositor_draw_scrollbars(c, layers[0].layer);
DRAWEND:
  C2D_Flush();
}

// Move the offset for the compositor against the given layer. Can be any of the layers you'd
// normally send to the draw function (use the first one for perfect compatibility)
void layercompositor_offset(LayerCompositor * lc, Layer * layer, u16 offset_x, u16 offset_y) {
  u16 width, height;
  layercompositor_dims(lc, &width, &height);
  float maxofsx = layer->width * lc->zoom - width;
  float maxofsy = layer->height * lc->zoom - height;
  lc->offset_x = C2D_Clamp(offset_x, 0, maxofsx < 0 ? 0 : maxofsx);
  lc->offset_y = C2D_Clamp(offset_y, 0, maxofsy < 0 ? 0 : maxofsy);
}

// Move the zoom for the compositor against the given layer. Can be any of the layers you'd
// normally send to the draw function (use the first one for perfect compatibility)
void layercompositor_zoom(LayerCompositor * lc, Layer * layer, float zoom) {
  u16 width, height;
  layercompositor_dims(lc, &width, &height);
  float zoom_ratio = zoom / lc->zoom;
  u16 center_x = width >> 1;
  u16 center_y = height >> 1;
  u16 new_ofsx = zoom_ratio * (lc->offset_x + center_x) - center_x;
  u16 new_ofsy = zoom_ratio * (lc->offset_y + center_y) - center_y;
  lc->zoom = zoom;
  layercompositor_offset(lc, layer, new_ofsx, new_ofsy);
}
