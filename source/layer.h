#ifndef __HEADER_3DSJUNKDRAW_LAYER__
#define __HEADER_3DSJUNKDRAW_LAYER__

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#define JDL_EDGEERROR 8
#define JDL_EDGEBUF   10
#define JDL_FORMAT GPU_RGBA5551
#define JDL_FORMAT_TRANSFER GX_TRANSFER_FMT_RGB5A1
#define JDL_MAXHWLAYERDIM 1024

// TODO: see if this system is even needed anymore...
#define JDL_MAXOBJECTS 8192 // Default is 4096, and 8192 is extremely pushing it
#define JDL_FLUSHOBJECTS (JDL_MAXOBJECTS - 100)


typedef u16 layerdim_t;

typedef struct {
  Tex3DS_SubTexture subtex; // Simple structures
  C3D_Tex texture;
  C2D_Image image;
  C3D_RenderTarget *target; // Actual data?
} Layer_Hardware;

typedef struct {
  u32 * buf;
  layerdim_t width;
  layerdim_t height;
} Layer_Software;

typedef union {
  Layer_Hardware hw;
  Layer_Software sf;
} LayerTexture;

#define JDL_TYPE_HARDWARE 0
#define JDL_TYPE_SOFTWARE 1

// A layer is a single texture that can accept lines (and maybe other things) to draw.
// You can initialize a layer to be hardware or software, so you can use the same
// data type for export (software) or for compositing a scene (hardware) and feed it
// the same lines, etc. 
typedef struct {
  LayerTexture texture;
  layerdim_t width;    // APPARENT width (requested)
  layerdim_t height;   // APPARENT height (requested)
  u8 type;
} Layer;

typedef struct {
  u32 color; // Must be pre-converted.
  u16 x1, y1, x2, y2;
  u8 width;
} RenderLine;

//void layer_create_wh(Layer * layer, int width, int height);
int layer_init(Layer * layer, layerdim_t width, layerdim_t height, u8 type); //Tex3DS_SubTexture subtex);
void layer_free(Layer * layer);
void layer_realsize(Layer * layer, layerdim_t * width, layerdim_t * height);
size_t layer_pixelcount(Layer * layer);
size_t layer_estimate_pixelcount(u8 type, layerdim_t width, layerdim_t height);

// Copy from one layer into another. Can perform conversions while copying.
// CAREFUL: will begin a scene / flush etc to copy vram!
int layer_copy(Layer * dest, Layer * source);

// WARN: sets target if hardware rendering, which is slow and flushes!
void layer_drawlines(Layer * layer, RenderLine * lines, size_t count);
// WARN: make sure you're in a render scene before clearing! Target not set...
void layer_clear(Layer * layer, u32 color);
// Put source layer onto dest layer, ONLY works for software layers!
void layer_composite_onto(Layer * dest, Layer * source);


#endif
