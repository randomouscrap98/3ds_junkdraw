#ifndef __HEADER_3DSJUNKDRAW_LAYER__
#define __HEADER_3DSJUNKDRAW_LAYER__

#include "draw.h"
#include "vector.h"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

// Glitch in citro2d (or so we assume) prevents us from writing into the first 8
// pixels in the texture. As such, we simply shift the texture over by this
// amount when drawing.
#define LAYER_EDGEERROR 8
#define LAYER_EDGEBUF   10
#define LAYER_FORMAT GPU_RGBA5551

#define MAXLAYERS   8
#define MAXONION    4
#define MAXWINDOWLAYERS (MAXLAYERS * (MAXONION + 1))


typedef struct {
  int layer_width;
  int layer_height;
  int texture_width;
  int texture_height;
  int max_layers;
} MaxLayerInfo;

MaxLayerInfo query_layer_maximums(int resolution);


typedef struct {
  Tex3DS_SubTexture subtex; // Simple structures
  C3D_Tex texture;
  C2D_Image image;
  C3D_RenderTarget *target; // Actual data?
} LayerData;

void layer_create(LayerData * layer, Tex3DS_SubTexture subtex);
void layer_create_wh(LayerData * layer, int width, int height);
void layer_free(LayerData * layer);

// typedef union {
//   u32 clear;
//   struct FullLine line;
// } LayerDrawCommand;

//VECTOR_DECLARE(LayerDrawCommand);

// A collection of layers you can draw into. Should represent one "page".
// The only use for this is to create a whole window of them
// typedef struct {
//   LayerData layers[MAXLAYERS]; // Wastes memory but whatever
//   int layer_count;
//   s16 page;
//   //char * draw_pointer;
//   //vector_LayerDrawCommand pending_commands;
// } LayerPack;

// void layerpack_init(LayerPack * lpack, MaxLayerInfo layer_info, int layer_count);
// void layerpack_schedule_clear(LayerPack * lpack, u32 clear_color);
// void layerpack_free(LayerPack * lpack);

typedef struct {
  LayerData * layers;
  int layer_count;
  LineScanner scanner;
} LayerPackItem;

// A sliding window of layer packs, each of which can represent a page
typedef struct {
  LayerData * master_layers;
  LayerPackItem * slots;
  int slot_count;   // You may not use all the slots (texture mem)
  int window_head;
  int total_layers;
  //LayerData   raw_layers[MAXWINDOWLAYERS];
  //u16         raw_pages[MAXWINDOWLAYERS]; // The page per layer, though we only care about per-slot
  //LineScanner scanners[MAXWINDOWLAYERS];
  //int         slot_layers;  // number of layers per slot
  //LayerPack * slots;
  //int slots_count;
} LayerPackWindow;

int layerpackwindow_init(LayerPackWindow * window, MaxLayerInfo layer_info, int layer_count, char * start, char ** end);
void layerpackwindow_free(LayerPackWindow * window);
void layerpackwindow_next(LayerPackWindow * window, int increment);
// Force the head slot to become invalidated, which SHOULD trigger a reset. We separate reset from
// invalidation because reset requires us to be in a drawing state (Citro etc)
void layerpackwindow_invalidate_head(LayerPackWindow * window);
LayerPackItem * layerpackwindow_at(LayerPackWindow * window, int offset);
// void layerpackwindow_resetpointers(LayerPackWindow * window, char * pointer);
// void layerpackwindow_schedule_clear(LayerPackWindow * window, u32 clear_color);

typedef struct {
  u32 draw_cmd_count;
  u32 obj_limit;
  u32 obj_safety;
} CitroTracking;

void citrotracking_init(CitroTracking * ct);
u32 citrotracking_flush(CitroTracking * ct, bool force);

typedef struct {
  MaxLayerInfo * layer_info;
  //struct ScreenState * screen;
  CitroTracking ct;
  //int ofsx;
  //int ofsy;
  u32 * export_buffer; // a buffer of pixels to fill potentially
} SolidRectState;

void solidrectstate_init(SolidRectState * srs);

// `// The settings pulled from the save file
// `typedef struct {
// `  int layer_count;
// `  int resolution_id;
// `} LayerSettings;

#endif
