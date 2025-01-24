#ifndef __HEADER_RENDER
#define __HEADER_RENDER

#include "color.h"
#include "system.h"

#define PALETTE_MINISIZE 10
#define PALETTE_MINIPADDING 1
#define PALETTE_OFSX 10
#define PALETTE_OFSY 10
#define PALETTE_SELECTED_COLOR C2D_Color32f(1, 0, 0, 1)
#define PALETTE_BG C2D_Color32f(0.3, 0.3, 0.3, 1)

#define PALETTE_SWATCHWIDTH 18
#define PALETTE_SWATCHMARGIN 2

void draw_colorpicker(struct ColorSystem *cs, bool collapsed);

#endif
