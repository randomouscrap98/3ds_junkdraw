#ifndef __HEADER_DRAW
#define __HEADER_DRAW

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#define LAYER_FORMAT GPU_RGBA5551

struct LayerData {
   Tex3DS_SubTexture subtex;     //Simple structures
   C3D_Tex texture;
   C2D_Image image;
   C3D_RenderTarget * target;    //Actual data?
};

void create_layer(struct LayerData * result, Tex3DS_SubTexture subtex);
void delete_layer(struct LayerData page);

#endif