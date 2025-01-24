#include "render_palette.h"
#include "color.h"

void draw_colorpicker_collapsed(struct ColorSystem *cs) {
  C2D_DrawRectSolid(0, 0, 0.5f, PALETTE_MINISIZE, PALETTE_MINISIZE, PALETTE_BG);
  C2D_DrawRectSolid(PALETTE_MINIPADDING, PALETTE_MINIPADDING, 0.5f,
                    PALETTE_MINISIZE - PALETTE_MINIPADDING * 2,
                    PALETTE_MINISIZE - PALETTE_MINIPADDING * 2,
                    rgba16_to_rgba32c(colorsystem_getcolor(cs)));
}

// void draw_colorpicker_palettemode(struct ColorSystem *cs) {
//
// }

// Draw (JUST draw) the entire color picker area, which may include other
// stateful controls
void draw_colorpicker(struct ColorSystem *cs, bool collapsed) {
  // TODO: Maybe split this out?
  if (collapsed) {
    draw_colorpicker_collapsed(cs);
    return;
  } else if (cs->mode == COLORSYSMODE_PALETTE) {
  }

  u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
  u16 total_width = (9 + (COLORSYS_HISTORY >> 3)) * shift +
                    (PALETTE_OFSX << 1) + PALETTE_SWATCHMARGIN;
  u16 total_height = 8 * shift + (PALETTE_OFSY << 1) + PALETTE_SWATCHMARGIN;
  C2D_DrawRectSolid(0, 0, 0.5f, total_width, total_height, PALETTE_BG);

  if (cs->mode == COLORSYSMODE_PALETTE) {
    u16 selected_index = colorsystem_getpaletteoffset(cs);
    u16 *palette = colorsystem_getcurrentpalette(cs);

    for (u16 i = 0; i < cs->palette_size; i++) {
      // TODO: an implicit 8 wide thing
      u16 x = i & 7;
      u16 y = i >> 3;

      if (i == selected_index) {
        C2D_DrawRectSolid(PALETTE_OFSX + x * shift, PALETTE_OFSY + y * shift,
                          0.5f, shift, shift, PALETTE_SELECTED_COLOR);
      }

      C2D_DrawRectSolid(PALETTE_OFSX + x * shift + PALETTE_SWATCHMARGIN,
                        PALETTE_OFSY + y * shift + PALETTE_SWATCHMARGIN, 0.5f,
                        PALETTE_SWATCHWIDTH, PALETTE_SWATCHWIDTH,
                        rgba16_to_rgba32c(palette[i]));
    }
  }

  for (u16 i = 0; i < COLORSYS_HISTORY; i++) {
    // REALLY hope the optimizing compiler fixes this...
    u16 x = i % (COLORSYS_HISTORY >> 3);
    u16 y = i / (COLORSYS_HISTORY >> 3);

    C2D_DrawRectSolid(PALETTE_OFSX + (x + 9) * shift + PALETTE_SWATCHMARGIN,
                      PALETTE_OFSY + y * shift + PALETTE_SWATCHMARGIN, 0.5f,
                      PALETTE_SWATCHWIDTH, PALETTE_SWATCHWIDTH,
                      rgba16_to_rgba32c(cs->history[i]));
  }
}
