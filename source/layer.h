#ifndef __HEADER_3DSJUNKDRAW_LAYER__
#define __HEADER_3DSJUNKDRAW_LAYER__

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

typedef struct {
  Tex3DS_SubTexture subtex; // Simple structures
  C3D_Tex texture;
  C2D_Image image;
  C3D_RenderTarget *target; // Actual data?
} LayerData;

void layer_create(LayerData * layer, Tex3DS_SubTexture subtex);
void layer_create_wh(LayerData * layer, int width, int height);
void layer_free(LayerData * layer);

typedef struct {
  int layer_width;
  int layer_height;
  int texture_width;
  int texture_height;
  int max_layers;
} MaxLayerInfo;

MaxLayerInfo query_layer_maximums(int resolution);

typedef struct {
  u32 draw_cmd_count;
  u32 obj_limit;
  u32 obj_safety;
} CitroTracking;

void citrotracking_init(CitroTracking * ct);
u32 citrotracking_flush(CitroTracking * ct, bool force);

typedef struct {
  MaxLayerInfo layer_info;
  //struct ScreenState * screen;
  CitroTracking ct;
  int ofsx;
  int ofsy;
  u32 * export_buffer; // a buffer of pixels to fill potentially
} SolidRectState;

void solidrectstate_init(SolidRectState * srs, MaxLayerInfo layer_info);


#endif
