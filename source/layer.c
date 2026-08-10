#include "layer.h"
#include "3ds/gpu/gx.h"
#include "utils.h"

#include <stdlib.h>

static Tex3DS_SubTexture layer_new_subtexture(layerdim_t width, layerdim_t height) {
  return (Tex3DS_SubTexture) { 
    next_power_of_2(width + JDL_EDGEBUF), 
    next_power_of_2(height + JDL_EDGEBUF), 
    0.0f, 1.0f, 1.0f, 0.0f 
  };
}

int layer_init(Layer * layer, layerdim_t width, layerdim_t height, u8 type) {
  layer->type = type;
  layer->width = width;
  layer->height = height;
  if(type == JDL_TYPE_HARDWARE) {
    layer->texture.hw.subtex = layer_new_subtexture(width, height);
    if(layer->texture.hw.subtex.width > JDL_MAXHWLAYERDIM || 
       layer->texture.hw.subtex.height > JDL_MAXHWLAYERDIM) {
      LOGDBG("ERR: HW LAYER TOO LARGE! MAX: %d", JDL_MAXHWLAYERDIM);
      return 1;
    }
    C3D_TexInitVRAM(&(layer->texture.hw.texture), 
                    layer->texture.hw.subtex.width, 
                    layer->texture.hw.subtex.height, 
                    JDL_FORMAT);
    layer->texture.hw.target = C3D_RenderTargetCreateFromTex(
      &(layer->texture.hw.texture), GPU_TEXFACE_2D, 0, -1);
    layer->texture.hw.image.tex = &(layer->texture.hw.texture);
    layer->texture.hw.image.subtex = &(layer->texture.hw.subtex);
  } else if(type == JDL_TYPE_SOFTWARE) {
    layer->texture.sf.width = width;
    layer->texture.sf.height = height;
    // WARN: will linearAlloc become a problem? Something something fragmented...
    layer->texture.sf.buf = linearAlloc(sizeof(u32) * width * height);
    if(!layer->texture.sf.buf) {
      return 1;
    }
  } else {
    LOGDBG("ERR: UNKNOWN LAYER TYPE %d", type);
    return 1;
  }
  return 0;
}

void layer_realsize(Layer * layer, layerdim_t * width, layerdim_t * height) {
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

size_t layer_pixelcount(Layer * layer) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    return layer->texture.hw.subtex.width * layer->texture.hw.subtex.height;
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    return layer->texture.sf.width * layer->texture.sf.height;
  } 
  return 0;
}

size_t layer_estimate_pixelcount(u8 type, layerdim_t width, layerdim_t height) {
  if(type == JDL_TYPE_HARDWARE) {
    Tex3DS_SubTexture subtex = layer_new_subtexture(width, height);
    return subtex.width * subtex.height;
  } else if(type == JDL_TYPE_SOFTWARE) {
    return width * height;
  } 
  return 0;
}

void layer_clear(Layer * layer, u32 color) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    C2D_TargetClear(layer->texture.hw.target, color);
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    size_t max = layer_pixelcount(layer);
    for(size_t i = 0; i < max; i++) {
      layer->texture.sf.buf[i] = color;
    }
  }
}

void layer_free(Layer * layer) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    C3D_RenderTargetDelete(layer->texture.hw.target);
    C3D_TexDelete(&layer->texture.hw.texture);
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    linearFree(layer->texture.sf.buf);
  } 
}

// Copy from one layer into another. Can perform conversions while copying.
// CAREFUL: will begin a scene / flush etc to copy vram!
int layer_copy(Layer * dest, Layer * source, u8 type) {
  // Simple init the dest texture
  layer_init(dest, source->width, source->height, type);
  if(dest->type == source->type) {
    if(dest->type == JDL_TYPE_HARDWARE) {
      // VRAM to VRAM Copy
      C2D_SceneBegin(dest->texture.hw.target);
      C2D_DrawImageAt(source->texture.hw.image, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);
    } else if(dest->type == JDL_TYPE_SOFTWARE) {
      // We can straight copy the whole pix buffer
      memcpy(dest->texture.sf.buf, source->texture.sf.buf, 
             layer_pixelcount(source) * sizeof(u32));
    } else {
      LOGDBG("ERR: Unsupported texture type!");
      return 1;
    }
  } else {
    if(dest->type == JDL_TYPE_HARDWARE && source->type == JDL_TYPE_SOFTWARE) {
      size_t sourcepixelcount = layer_pixelcount(source);
      // Flush cache for the source buffer in FCRAM
      GSPGPU_FlushDataCache(source->texture.sf.buf, sourcepixelcount * sizeof(u32));

      for(size_t i = 0; i < sourcepixelcount; i++) {
        source->texture.sf.buf[i] = JD_REVERSE32(source->texture.sf.buf[i]);
      }

      // Perform Display Transfer from FCRAM (RGBA8) to VRAM (RGBA5551)
      C3D_SyncDisplayTransfer(
          (u32*)source->texture.sf.buf, GX_BUFFER_DIM(source->texture.sf.width, source->texture.sf.height),
          (u32*)dest->texture.hw.texture.data,  GX_BUFFER_DIM(
            dest->texture.hw.subtex.width, dest->texture.hw.subtex.height),
          (GX_TRANSFER_OUT_TILED(1) | 
           GX_TRANSFER_RAW_COPY(0) |
           GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | 
           GX_TRANSFER_OUT_FORMAT(JDL_FORMAT_TRANSFER) |
           GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
      );

      for(size_t i = 0; i < sourcepixelcount; i++) {
        source->texture.sf.buf[i] = JD_REVERSE32(source->texture.sf.buf[i]);
      }
    } else if(dest->type == JDL_TYPE_SOFTWARE && source->type == JDL_TYPE_HARDWARE) {
      // TODO: pull texture data out of vram into software buffer
      size_t destpixelcount = layer_pixelcount(dest);
      // Perform Display Transfer from FCRAM (RGBA8) to VRAM (RGBA5551)
      C3D_SyncDisplayTransfer(
          (u32*)source->texture.hw.texture.data,  GX_BUFFER_DIM(
            source->texture.hw.subtex.width, source->texture.hw.subtex.height),
          (u32*)dest->texture.sf.buf, GX_BUFFER_DIM(
            dest->texture.sf.width, dest->texture.sf.height),
          (GX_TRANSFER_OUT_TILED(0) | 
           GX_TRANSFER_RAW_COPY(0) |
           GX_TRANSFER_IN_FORMAT(JDL_FORMAT_TRANSFER) | 
           GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
           GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
      );
      // Flush cache for the source buffer in FCRAM
      GSPGPU_FlushDataCache(dest->texture.sf.buf, destpixelcount * sizeof(u32));
    } else {
      LOGDBG("ERR: Unsupported layer conversion type!");
      return 1;
    }
  }
  return 0;
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
        // ALTERNATIVE: Try C2D_Flush()? That might be why it flashes all weird...
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
    size_t max = layer_pixelcount(dest);
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

