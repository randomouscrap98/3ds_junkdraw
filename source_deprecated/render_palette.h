#ifndef __HEADER_RENDER
#define __HEADER_RENDER

#include "color.h"
#include "system.h"

#define PALETTE_MINISIZE 10
#define PALETTE_MINIPADDING 1
#define PALETTE_OFSX 12
#define PALETTE_OFSY 12
#define PALETTE_SELECTED_COLOR C2D_Color32f(1, 0, 0, 1)
#define PALETTE_BG C2D_Color32f(0.3, 0.3, 0.3, 1)

#define PALETTE_SLIDERBG C2D_Color32f(0.5, 0.5, 0.5, 1)

#define PALETTE_SWATCHWIDTH 18
#define PALETTE_SWATCHMARGIN 2
#define PALETTE_SHIFT (PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN)
#define PALETTE_INNERW ((9 + (COLORSYS_HISTORY >> 3)) * PALETTE_SHIFT)
#define PALETTE_INNERH (8 * PALETTE_SHIFT)

#define PALETTE_RGBCLICKH (PALETTE_INNERH >> 5)
#define PALETTE_RGBCLICKSTART                                                  \
  (PALETTE_OFSY + ((PALETTE_INNERH - 32 * PALETTE_RGBCLICKH) >> 1))

#define PALETTE_RGBSWATCHWIDTH 18
#define PALETTE_RGBSWATCHMARGIN 1
#define PALETTE_RGBSLIDERWIDTH 18
// #define PALETTE_

void draw_colorpicker(struct ColorSystem *cs, bool collapsed);

// Update color in colorpicker based on touch position. 0 = no action,
// 1 = update main color, 2 = update history color
int update_colorpicker(struct ColorSystem *cs, const touchPosition *pos);

#endif
