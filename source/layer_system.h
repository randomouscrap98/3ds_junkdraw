#ifndef __HEADER_3DSJUNKDRAW_LAYER__
#define __HEADER_3DSJUNKDRAW_LAYER__

#include "draw.h"

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

// Glitch in citro2d (or so we assume) prevents us from writing into the first 8
// pixels in the texture. As such, we simply shift the texture over by this
// amount when drawing.
#define LAYER_EDGEERROR 8
#define LAYER_EDGEBUF   10
#define LAYER_FORMAT GPU_RGBA5551

#define RESOLUTIONCOUNT 3
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
void layer_delete(LayerData * layer);

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
} LayerPackWindow;

int layerpackwindow_init(LayerPackWindow * window, MaxLayerInfo layer_info, int layer_count, char * start, char ** end);
void layerpackwindow_free(LayerPackWindow * window);
void layerpackwindow_next(LayerPackWindow * window, int increment);
// Force the head slot to become invalidated, which SHOULD trigger a reset. We separate reset from
// invalidation because reset requires us to be in a drawing state (Citro etc)
void layerpackwindow_invalidate_head(LayerPackWindow * window);
LayerPackItem * layerpackwindow_at(LayerPackWindow * window, int offset);

typedef struct {
  u32 draw_cmd_count;
  u32 obj_limit;
  u32 obj_safety;
} CitroTracking;

void citrotracking_init(CitroTracking * ct);
u32 citrotracking_flush(CitroTracking * ct, bool force);

typedef struct {
  MaxLayerInfo * layer_info;
  CitroTracking ct;
  u32 * export_buffer; // a buffer of pixels to fill potentially
} SolidRectState;

void solidrectstate_init(SolidRectState * srs);

#endif
