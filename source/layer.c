#include "layer.h"


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

void solidrectstate_init(SolidRectState * srs, MaxLayerInfo layer_info) {
  srs->ofsx = 0;
  srs->ofsy = 0;
  srs->layer_info = layer_info;
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
