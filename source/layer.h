#ifndef __HEADER_3DSJUNKDRAW_LAYER__
#define __HEADER_3DSJUNKDRAW_LAYER__

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#define JDL_EDGEERROR 8
#define JDL_EDGEBUF   10
#define JDL_FORMAT GPU_RGBA5551
// TODO: see if this system is even needed anymore...
#define JDL_MAXOBJECTS 8192 // Default is 4096, and 8192 is extremely pushing it
#define JDL_FLUSHOBJECTS (JDL_MAXOBJECTS - 100)

typedef struct {
  Tex3DS_SubTexture subtex; // Simple structures
  C3D_Tex texture;
  C2D_Image image;
  C3D_RenderTarget *target; // Actual data?
} Layer_Hardware;

typedef struct {
  u32 * buf;
  u16 width;
  u16 height;
} Layer_Software;

typedef union {
  Layer_Hardware hw;
  Layer_Software sf;
} LayerTexture;

#define JDL_TYPE_HARDWARE 0
#define JDL_TYPE_SOFTWARE 1

// A layer is a single texture that can accept lines (and maybe other things) to draw.
typedef struct {
  LayerTexture texture;
  u16 width;    // APPARENT width (requested)
  u16 height;   // APPARENT height (requested)
  u8 type;
} Layer;

typedef struct {
  u32 color; // Must be pre-converted.
  u16 x1, y1, x2, y2;
  u8 width;
} RenderLine;

//void layer_create_wh(Layer * layer, int width, int height);
int layer_init(Layer * layer, u16 width, u16 height, u8 type); //Tex3DS_SubTexture subtex);
void layer_free(Layer * layer);
void layer_realsize(Layer * layer, u16 * width, u16 * height);

// WARN: sets target if hardware rendering, which is slow and flushes!
void layer_drawlines(Layer * layer, RenderLine * lines, size_t count);
// WARN: make sure you're in a render scene before clearing! Target not set...
void layer_clear(Layer * layer, u32 color);


#endif
