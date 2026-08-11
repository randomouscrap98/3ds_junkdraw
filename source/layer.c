#include "layer.h"
#include "3ds/gpu/gx.h"
#include "c2d/base.h"
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
      LOGERR("HW LAYER TOO LARGE! MAX: %d", JDL_MAXHWLAYERDIM);
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
    LOGERR("UNKNOWN LAYER TYPE %d", type);
    return 1;
  }
  return 0;
}

void layer_mapped_area(Layer * layer, U32Bounds * bounds) {
  bounds->x1 = 0;
  bounds->y1 = 0;
  bounds->x2 = layer->width;
  bounds->y2 = layer->height;
  if(layer->type == JDL_TYPE_HARDWARE) {
    bounds->x1 += JDL_EDGEBUF;
    bounds->y1 += JDL_EDGEBUF;
    bounds->x2 += JDL_EDGEBUF;
    bounds->y2 += JDL_EDGEBUF;
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

u32 layer_querycolor(Layer * layer, layerdim_t x, layerdim_t y) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    return rgba5551_to_abgr8(((u16*)layer->texture.hw.texture.data)[
      get_tiled_pixel_offset(x, y, layer->texture.hw.subtex.width)
    ]);
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    return JD_REVERSE32(layer->texture.sf.buf[x + y * layer->texture.sf.width]);
  } 
  return 0;
}

void layer_clear(Layer * layer, u32 color) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    u16 col16 = abgr8_to_rgba5551(color);
    C2D_TargetClear(layer->texture.hw.target, col16 | ((u32)col16 << 16));
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    size_t max = layer_pixelcount(layer);
    // reverse color for DMA transfer speed
    u32 revcolor = JD_REVERSE32(color);
    for(size_t i = 0; i < max; i++) {
      layer->texture.sf.buf[i] = revcolor;
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
// CAREFUL: will begin a scene / flush etc to copy vram! Does NOT init!!
int layer_copy(Layer * dest, Layer * source) {
  // Simple init the dest texture
  // layer_init(dest, source->width, source->height, type);
  if(dest->type == source->type) {
    if(dest->type == JDL_TYPE_HARDWARE) {
      // VRAM to VRAM Copy
      C2D_SceneBegin(dest->texture.hw.target);
      C2D_DrawImageAt(source->texture.hw.image, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);
      C2D_Flush();
    } else if(dest->type == JDL_TYPE_SOFTWARE) {
      // We can straight copy the whole pix buffer
      memcpy(dest->texture.sf.buf, source->texture.sf.buf, 
             layer_pixelcount(source) * sizeof(u32));
    } else {
      LOGERR("Unsupported texture type!");
      return 1;
    }
  } else {
    if(dest->type == JDL_TYPE_HARDWARE && source->type == JDL_TYPE_SOFTWARE) {
      size_t sourcepixelcount = layer_pixelcount(source);
      // Flush cache for the source buffer in FCRAM
      GSPGPU_FlushDataCache(source->texture.sf.buf, sourcepixelcount * sizeof(u32));
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
      LOGERR("Unsupported layer conversion type!");
      return 1;
    }
  }
  return 0;
}

#define BRESENHAM_PRE(rl, x1n, y1n, x2n, y2n, layer) { \
  float ofs = (rl.width / 2.0f) - 0.5f; \
  /* For speed, clip line... it deforms lines and looks bad but.. */ \
  int16_t x1n = ofs > rl.x1 ? ofs : rl.x1; \
  int16_t y1n = ofs > rl.y1 ? ofs : rl.y1; \
  int16_t x2n = ofs > rl.x2 ? ofs : rl.x2; \
  int16_t y2n = ofs > rl.y2 ? ofs : rl.y2; \
  /* The normal shift since points are in the middle */ \
  x1n = floor(x1n - ofs); \
  x2n = floor(x2n - ofs); \
  y1n = floor(y1n - ofs); \
  y2n = floor(y2n - ofs); \
  /* More line clipping */ \
  if(x1n + rl.width > layer->width) x1n = layer->width - rl.width; \
  if(x2n + rl.width > layer->width) x2n = layer->width - rl.width; \
  if(y1n + rl.width > layer->height) y1n = layer->height - rl.width; \
  if(y2n + rl.width > layer->height) y2n = layer->height - rl.width; \
  int16_t dx = abs(x2n - x1n); \
  int16_t sx = x1n < x2n ? 1 : -1; \
  int16_t dy = -abs(y2n - y1n); \
  int16_t sy = y1n < y2n ? 1 : -1; \
  int16_t error = dx + dy; \
  while(1) { \
    /* Your box drawing code or whatever goes here */

#define BRESENHAM_POST(rl, x1n, y1n, x2n, y2n) \
    int16_t e2 = 2 * error; \
    if (e2 >= dy) { \
      if (x1n == x2n) break; \
      error = error + dy; \
      x1n = x1n + sx; \
    } \
    if (e2 <= dx) { \
      if (y1n == y2n) break; \
      error = error + dx; \
      y1n = y1n + sy; \
    } \
  } \
}

static inline void layer_drawlines_hardware(Layer * layer, RenderLine * lines, size_t count) {
  //static u32 command_count = 0; // GLOBAL TRACKING, kind of scary but yeah...
  u32 command_count = 0;
  //  WARN: this flushes, be careful when calling!
  C2D_SceneBegin(layer->texture.hw.target);
  for(size_t i = 0; i < count; i++) {
    BRESENHAM_PRE(lines[i], x, y, x2, y2, layer) {
      C2D_DrawRectSolid(x + JDL_EDGEBUF, y + JDL_EDGEBUF, 
                        0.5, lines[i].width, lines[i].width, lines[i].color);
      if(++command_count >= (JDL_MAXOBJECTS - 1)) {
        LOGTRC("FLUSHING %ld DRAW CMDS PREMATURELY\n", command_count); 
        // ALTERNATIVE: Try C2D_Flush()? That might be why it flashes all weird...
        //C3D_FrameEnd(0);
        //C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_Flush();
        command_count = 0;
      }
    }
    BRESENHAM_POST(lines[i], x, y, x2, y2);
  }
  // Leave a buffer for other people (if we're too close to the max object). It's not a lot...
  if(command_count >= JDL_SAFETYFLUSH) {
    C2D_Flush();
  }
}

static inline void layer_drawlines_software(Layer * layer, RenderLine * lines, size_t count) {
  for(size_t i = 0; i < count; i++) {
    // Store reversed so DMA transfer is fast
    u32 color = JD_REVERSE32(lines[i].color);
    BRESENHAM_PRE(lines[i], x, y, x2, y2, layer) {
      for (u32 yi = 0; yi < lines[i].width; yi++)
        for (u32 xi = 0; xi < lines[i].width; xi++)
          layer->texture.sf.buf[(yi + y) * layer->texture.sf.width + xi + x] = color;
    }
    BRESENHAM_POST(lines[i], x, y, x2, y2);
  }
}

void layer_drawlines(Layer * layer, RenderLine * lines, size_t count) {
  if(layer->type == JDL_TYPE_HARDWARE) {
    layer_drawlines_hardware(layer, lines, count);
  } else if(layer->type == JDL_TYPE_SOFTWARE) {
    layer_drawlines_software(layer, lines, count);
  }
}

int layer_composite_onto(Layer * dest, Layer * source) {
  if(dest->type == JDL_TYPE_SOFTWARE && source->type == JDL_TYPE_SOFTWARE &&
    dest->width == source->width && dest->height == source->height) {
    size_t max = layer_pixelcount(dest);
    for (size_t i = 0; i < max; i++) {
      // TODO: CAREFUL! For speed, we check for full 0, when only alpha matters!
      if (source->texture.sf.buf[i]) {
        dest->texture.sf.buf[i] = source->texture.sf.buf[i];
      }
    }
    return 0;
  } else {
    LOGERR("UNSUPPORTED LAYER COMPOSITION");
    return 1;
  }
}

