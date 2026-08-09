#include "layer.h"
#include "utils.h"

#include <stdlib.h>

int layer_init(Layer * layer, u16 width, u16 height, u8 type) {
  layer->type = type;
  layer->width = width;
  layer->height = height;
  if(type == JDL_TYPE_HARDWARE) {
    Tex3DS_SubTexture subtex = { 
      next_power_of_2(width + JDL_EDGEBUF), 
      next_power_of_2(height + JDL_EDGEBUF), 
      0.0f, 1.0f, 1.0f, 0.0f 
    };
    layer->texture.hw.subtex = subtex;
    C3D_TexInitVRAM(&(layer->texture.hw.texture), subtex.width, subtex.height, JDL_FORMAT);
    layer->texture.hw.target = C3D_RenderTargetCreateFromTex(
      &(layer->texture.hw.texture), GPU_TEXFACE_2D, 0, -1);
    layer->texture.hw.image.tex = &(layer->texture.hw.texture);
    layer->texture.hw.image.subtex = &(layer->texture.hw.subtex);
  } else if(type == JDL_TYPE_SOFTWARE) {
    layer->texture.sf.width = width;
    layer->texture.sf.height = height;
    layer->texture.sf.buf = malloc(sizeof(u32) * width * height);
    if(!layer->texture.sf.buf) {
      return 1;
    }
  } else {
    LOGDBG("ERR: UNKNOWN LAYER TYPE %d", type);
    return 1;
  }
  return 0;
}

void layer_realsize(Layer * layer, u16 * width, u16 * height) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    *width = layer->texture.hw.subtex.width - JDL_EDGEBUF;
    *height = layer->texture.hw.subtex.height - JDL_EDGEBUF;
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    *width = layer->texture.sf.width;
    *height = layer->texture.sf.height;
  } else {
    *width = 0;
    *height = 0;
  }
}

void layer_clear(Layer * layer, u32 color) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    C2D_TargetClear(layer->texture.hw.target, color);
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    u32 max = layer->texture.sf.width * layer->texture.sf.height;
    for(u32 i = 0; i < max; i++) {
      layer->texture.sf.buf[i] = color;
    }
  }
}

void layer_free(Layer * layer) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    C3D_RenderTargetDelete(layer->texture.hw.target);
    C3D_TexDelete(&layer->texture.hw.texture);
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    free(layer->texture.sf.buf);
  } 
}

#define BRESENHAM_PRE(rl, layer) { \
  float ofs = (rl.width / 2.0f) - 0.5f; \
  rl.x1 = floor(rl.x1 - ofs); \
  rl.x2 = floor(rl.x2 - ofs); \
  rl.y1 = floor(rl.y1 - ofs); \
  rl.y2 = floor(rl.y2 - ofs); \
  int16_t dx = abs(rl.x2 - rl.x1); \
  int16_t sx = rl.x1 < rl.x2 ? 1 : -1; \
  int16_t dy = -abs(rl.y2 - rl.y1); \
  int16_t sy = rl.y1 < rl.y2 ? 1 : -1; \
  int16_t error = dx + dy; \
  while(1) { \
    /* Because of the safety buffer (a citro bug), we don't allow squares which even partially */ \
    /* go off the left side of the screen. Right side is ok (apparently), */ \
    /* but clamp to the "apparent" bounds of the texture (may be different in reality) */ \
    if (rl.x1 >= 0 && rl.y1 >= 0 && rl.x1 < layer->width && rl.y1 < layer->height) {

#define BRESENHAM_POST(rl) \
    } \
    int16_t e2 = 2 * error; \
    if (e2 >= dy) { \
      if (rl.x1 == rl.x2) break; \
      error = error + dy; \
      rl.x1 = rl.x1 + sx; \
    } \
    if (e2 <= dx) { \
      if (rl.y1 == rl.y2) break; \
      error = error + dx; \
      rl.y1 = rl.y1 + sy; \
    } \
  } \
}

static inline void layer_drawlines_hardware(Layer * layer, RenderLine * lines, size_t count) {
  static u32 command_count = 0; // GLOBAL TRACKING, kind of scary but yeah...
  //  WARN: this flushes, be careful when calling!
  C2D_SceneBegin(layer->texture.hw.target);
  for(size_t i = 0; i < count; i++) {
    BRESENHAM_PRE(lines[i], layer) {
      C2D_DrawRectSolid(lines[i].x1 + JDL_EDGEBUF, lines[i].y1 + JDL_EDGEBUF, 
                        0.5, lines[i].width, lines[i].width, lines[i].color);
      if(++command_count >= JDL_FLUSHOBJECTS) {
        //LOGTRACE("FLUSHING %ld DRAW CMDS PREMATURELY\n", ct->draw_cmd_count); 
        C3D_FrameEnd(0);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        command_count = 0;
      }
    }
    BRESENHAM_POST(lines[i]);
  }
}

static inline void layer_drawlines_software(Layer * layer, RenderLine * lines, size_t count) {
  for(size_t i = 0; i < count; i++) {
    BRESENHAM_PRE(lines[i], layer) {
      // We know we reject stuff on the left side for safety reasons, but the right
      // side is allowed to run off, so we must clamp
      u32 maxx = JD_MIN(lines[i].x1 + lines[i].width, layer->texture.sf.width);
      u32 maxy = JD_MIN(lines[i].y1 + lines[i].width, layer->texture.sf.height);
      for (u32 yi = lines[i].y1; yi < maxy; yi++)
        for (u32 xi = lines[i].x1; xi < maxx; xi++)
          layer->texture.sf.buf[yi * layer->texture.sf.width + xi] = lines[i].color;
    }
    BRESENHAM_POST(lines[i]);
  }
}

void layer_drawlines(Layer * layer, RenderLine * lines, size_t count) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    layer_drawlines_hardware(layer, lines, count);
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    layer_drawlines_software(layer, lines, count);
  }
}

void layer_composite_onto(Layer * dest, Layer * source) {
  if(dest->type == JDL_TYPE_SOFTWARE && source->type == JDL_TYPE_SOFTWARE &&
    dest->width == source->width && dest->height == source->height) {
    size_t max = dest->width * dest->height;
    for (size_t i = 0; i < max; i++) {
      // TODO: CAREFUL! For speed, we check for full 0, when only alpha matters!
      if (source->texture.sf.buf[i]) {
        dest->texture.sf.buf[i] = source->texture.sf.buf[i];
      }
    }
  } else {
    LOGDBG("UNSUPPORTED LAYER COMPOSITION");
  }
}

