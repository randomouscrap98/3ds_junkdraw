#include "render_palette.h"
#include "color.h"

void draw_colorpicker_collapsed(struct ColorSystem *cs) {
  C2D_DrawRectSolid(0, 0, 0.5f, PALETTE_MINISIZE, PALETTE_MINISIZE, PALETTE_BG);
  C2D_DrawRectSolid(PALETTE_MINIPADDING, PALETTE_MINIPADDING, 0.5f,
                    PALETTE_MINISIZE - PALETTE_MINIPADDING * 2,
                    PALETTE_MINISIZE - PALETTE_MINIPADDING * 2,
                    rgba16_to_rgba32c(colorsystem_getcolor(cs)));
}

// Draw (JUST draw) the entire color picker area, which may include other
// stateful controls
void draw_colorpicker(struct ColorSystem *cs, bool collapsed) {
  // TODO: Maybe split this out?
  if (collapsed) {
    draw_colorpicker_collapsed(cs);
    return;
  }

  u16 shift = PALETTE_SHIFT;
  u16 total_width = PALETTE_INNERW + (PALETTE_OFSX << 1);
  u16 total_height = PALETTE_INNERH + (PALETTE_OFSY << 1);
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
  } else if (cs->mode == COLORSYSMODE_RGB) {
    u8 rgbarr[3] = {cs->r, cs->g, cs->b};

    // The sliders
    for (int i = 0; i < 3; i++) {
      // Slightly off from center to make it closer to the color list thing
      C2D_DrawRectSolid(PALETTE_OFSX + (2.5 + 2 * i) * shift -
                            PALETTE_SWATCHMARGIN * 0.5 - 1,
                        PALETTE_RGBCLICKSTART, 0.5f, PALETTE_SWATCHMARGIN,
                        PALETTE_RGBCLICKH * 32 - 1, PALETTE_SLIDERBG);
      C2D_DrawRectSolid(
          PALETTE_OFSX + (2 + 2 * i) * shift + PALETTE_SWATCHMARGIN - 1,
          PALETTE_RGBCLICKSTART + PALETTE_RGBCLICKH * (31 - rgbarr[i]), 0.5f,
          PALETTE_SWATCHWIDTH, PALETTE_RGBCLICKH - 1, PALETTE_SLIDERBG);
    }

    for (u16 i = 0; i < 32; i++) {
      u16 cols[3] = {rgb_to_rgba16(31 - i, cs->g, cs->b),
                     rgb_to_rgba16(cs->r, 31 - i, cs->b),
                     rgb_to_rgba16(cs->r, cs->g, 31 - i)};

      for (int j = 0; j < 3; j++) {
        C2D_DrawRectSolid(PALETTE_OFSX + (1 + 2 * j) * shift +
                              PALETTE_SWATCHMARGIN,
                          PALETTE_RGBCLICKSTART + i * PALETTE_RGBCLICKH, 0.5f,
                          PALETTE_SWATCHWIDTH, PALETTE_RGBCLICKH - 1,
                          rgba16_to_rgba32c(cols[j]));
      }
    }
  } else if (cs->mode == COLORSYSMODE_AUTOPALETTE) {
    // u16 col = 0x8000;
    for (u16 i = 0; i < 32768; i++) {
      // TODO: an implicit 32 wide thing
      u16 x = i % 181; //& 255;
      u16 y = i / 181; //>> 3;

      // if ((i & 0xFFF) == 0) {
      //   C3D_FrameEnd(0);
      //   C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      // }

      // if (i == selected_index) {
      //   C2D_DrawRectSolid(PALETTE_OFSX + x * shift, PALETTE_OFSY + y * shift,
      //                     0.5f, shift, shift, PALETTE_SELECTED_COLOR);
      // }

      C2D_DrawRectSolid(PALETTE_OFSX + x, PALETTE_OFSY + y, 0.5f, 1, 1,
                        rgba16_to_rgba32c(0x8000 + i));
      // C2D_DrawRectSolid(PALETTE_RGBCLICKSTART + x * PALETTE_RGBCLICKH,
      //                   PALETTE_RGBCLICKSTART + y * PALETTE_RGBCLICKH, 0.5f,
      //                   PALETTE_RGBCLICKH - 1, PALETTE_RGBCLICKH - 1,
      //                   rgba16_to_rgba32c(col));
      // // col = 0x8000 + ((col + 273) & 32767);
      // col = 0x8000 + ((col + 273) & 32767);
    }
  }

  // History section, always in the same place
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

// Given a touch position (presumably on the color palette), update the selected
// palette index within the color system. Includes history
int update_colorpicker(struct ColorSystem *cs, const touchPosition *pos) {
  u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
  u16 xind = (pos->px - PALETTE_OFSX) / shift;
  u16 yind = (pos->py - PALETTE_OFSY) / shift;
  if (yind < 8) {
    if (xind < 8) {
      if (cs->mode == COLORSYSMODE_PALETTE) {
        u16 new_index = (yind << 3) + xind;
        if (new_index >= 0 && new_index < cs->palette_size) {
          colorsystem_setpaletteoffset(cs, new_index);
          return 1;
        }
      } else if (cs->mode == COLORSYSMODE_RGB) {
        // THIS WILL OVERFLOW, THAT'S OK!
        xind = (xind - 1) / 2; // THere's 1 unused slot plus each takes up 2
        if (xind < 3) {
          int newval =
              31 - ((pos->py - PALETTE_RGBCLICKSTART) / PALETTE_RGBCLICKH);
          if (newval >= 0 && newval < 32) {
            if (xind == 0)
              cs->r = newval;
            if (xind == 1)
              cs->g = newval;
            if (xind == 2)
              cs->b = newval;
            return 1;
          }
        }
      }
    } else if (xind >= 9 && xind < 9 + (COLORSYS_HISTORY >> 3)) {
      u16 setcol = cs->history[(yind * (COLORSYS_HISTORY >> 3)) + xind - 9];
      if (colorsystem_trysetcolor(cs, setcol))
        return 2;
    }
  }
  return 0;
}
