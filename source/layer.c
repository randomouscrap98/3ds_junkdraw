#include "layer.h"

#include <stdlib.h>

// Create a LAYER based on the information for the subtexture
void layer_create(LayerData *result, Tex3DS_SubTexture subtex) {
  result->subtex = subtex;
  C3D_TexInitVRAM(&(result->texture), subtex.width, subtex.height, LAYER_FORMAT);
  result->target = C3D_RenderTargetCreateFromTex(&(result->texture), GPU_TEXFACE_2D, 0, -1);
  result->image.tex = &(result->texture);
  result->image.subtex = &(result->subtex);
}

void layer_create_wh(LayerData *result, int width, int height) {
  Tex3DS_SubTexture subtex = { width, height, 0.0f, 1.0f, 1.0f, 0.0f };
  layer_create(result, subtex);
}

// Clean up a layer created by create_page
void layer_free(LayerData * layer) {
  C3D_RenderTargetDelete(layer->target);
  C3D_TexDelete(&layer->texture);
}

int layerpackwindow_init(LayerPackWindow * window, MaxLayerInfo layer_info, int layer_count, 
                         char * start, char ** end) {
  window->slot_count = layer_info.max_layers / layer_count;
  window->total_layers = window->slot_count * layer_count;
  window->window_head = 0;
  window->slots = malloc(sizeof(LayerPackItem) * window->slot_count);
  if(!window->slots) {
    return 1;
  }
  window->master_layers = malloc(sizeof(LayerData) * window->total_layers);
  if(!window->master_layers) {
    free(window->slots);
    return 1;
  }
  // All textures are the same
  for(int li = 0; li < window->total_layers; li++) {
    layer_create_wh(window->master_layers + li, layer_info.texture_width, layer_info.texture_height);
  }
  //window->slot_layers = layer_count;
  for(int si = 0; si < window->slot_count; si++) {
    LayerPackItem * slot = window->slots + si;
    slot->layer_count = layer_count;
    slot->layers = window->master_layers + (si * layer_count);
    // All scanners start in an invalid state (page invalid)
    linescanner_init(&slot->scanner, start, end);
  }
  return 0;
}
  
void layerpackwindow_free(LayerPackWindow * window) {
  for(int li = 0; li < window->total_layers; li++) {
    layer_free(window->master_layers + li);
  }
  for(int si = 0; si < window->slot_count; si++) {
    linescanner_free(&window->slots[si].scanner);
  }
  free(window->master_layers);
  free(window->slots);
}

void layerpackwindow_next(LayerPackWindow * window, int increment) {
  window->window_head = (window->window_head + window->slot_count * 100 + increment) % window->slot_count;
  // NOTE: don't need to change the page in the window slot, as the redraw handles that...
}

void layerpackwindow_invalidate_head(LayerPackWindow * window) {
  window->slots[window->window_head].scanner.page = -1;
}

LayerPackItem * layerpackwindow_at(LayerPackWindow * window, int offset) {
  return window->slots + ((window->window_head + offset + 100 * window->slot_count) % window->slot_count);
}

void citrotracking_init(CitroTracking * ct) {
  ct->draw_cmd_count = 0;
  // I tried many things to increase this limit but it seems pretty set...
  ct->obj_limit = 8192;
  ct->obj_safety = ct->obj_limit - 100;
  // Those may change if I can figure stuff out but for now, those are the defaults...
}

u32 citrotracking_flush(CitroTracking * ct, bool force) {
  u32 result = 0;
  if(force || ct->draw_cmd_count > ct->obj_safety) {
    //LOGTRACE("FLUSHING %ld DRAW CMDS PREMATURELY\n", ct->draw_cmd_count); 
    result = ct->draw_cmd_count;
    C3D_FrameEnd(0);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    ct->draw_cmd_count = 0;
  }
  return result;
}

void solidrectstate_init(SolidRectState * srs) {
  srs->layer_info = NULL;
  citrotracking_init(&srs->ct);
}

MaxLayerInfo query_layer_maximums(int resolution) {
  MaxLayerInfo result;
  switch(resolution) {
    // Unfortunately, both the 500 and 250 versions have the same limitations
    // because the library needs that 8 pixel buffer around the edge of the texture...
    case 1:
      result.max_layers = 8;
      result.layer_width = 500;
      result.layer_height = 500;
      result.texture_width = 512;
      result.texture_height = 512;
      break;
    case 2:
      result.max_layers = 16;
      result.layer_width = 320;
      result.layer_height = 240;
      result.texture_width = 512;
      result.texture_height = 256;
      break;
    // case 3 reserved
    default: // also case 0
      result.max_layers = 2;
      result.layer_width = 1000;
      result.layer_height = 1000;
      result.texture_width = 1024;
      result.texture_height = 1024;
  }
  return result;
}
