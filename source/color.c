#include "color.h"
#include <stdlib.h> // To make colors:
// dos2unix colors.txt
// awk '{printf "0x%s", substr($0, 3); printf (NR % 8 == 0 ? "\n" : ", ")}'
// colors.txt

u32 default_palette[] = {
    // Endesga 64
    0xff0040, 0x131313, 0x1b1b1b, 0x272727, 0x3d3d3d, 0x5d5d5d, 0x858585,
    0xb4b4b4, 0xffffff, 0xc7cfdd, 0x92a1b9, 0x657392, 0x424c6e, 0x2a2f4e,
    0x1a1932, 0x0e071b, 0x1c121c, 0x391f21, 0x5d2c28, 0x8a4836, 0xbf6f4a,
    0xe69c69, 0xf6ca9f, 0xf9e6cf, 0xedab50, 0xe07438, 0xc64524, 0x8e251d,
    0xff5000, 0xed7614, 0xffa214, 0xffc825, 0xffeb57, 0xd3fc7e, 0x99e65f,
    0x5ac54f, 0x33984b, 0x1e6f50, 0x134c4c, 0x0c2e44, 0x00396d, 0x0069aa,
    0x0098dc, 0x00cdf9, 0x0cf1ff, 0x94fdff, 0xfdd2ed, 0xf389f5, 0xdb3ffd,
    0x7a09fa, 0x3003d9, 0x0c0293, 0x03193f, 0x3b1443, 0x622461, 0x93388f,
    0xca52c9, 0xc85086, 0xf68187, 0xf5555d, 0xea323c, 0xc42430, 0x891e2b,
    0x571c27,

    // Resurrect
    0x2e222f, 0x3e3546, 0x625565, 0x966c6c, 0xab947a, 0x694f62, 0x7f708a,
    0x9babb2, 0xc7dcd0, 0xffffff, 0x6e2727, 0xb33831, 0xea4f36, 0xf57d4a,
    0xae2334, 0xe83b3b, 0xfb6b1d, 0xf79617, 0xf9c22b, 0x7a3045, 0x9e4539,
    0xcd683d, 0xe6904e, 0xfbb954, 0x4c3e24, 0x676633, 0xa2a947, 0xd5e04b,
    0xfbff86, 0x165a4c, 0x239063, 0x1ebc73, 0x91db69, 0xcddf6c, 0x313638,
    0x374e4a, 0x547e64, 0x92a984, 0xb2ba90, 0x0b5e65, 0x0b8a8f, 0x0eaf9b,
    0x30e1b9, 0x8ff8e2, 0x323353, 0x484a77, 0x4d65b4, 0x4d9be6, 0x8fd3ff,
    0x45293f, 0x6b3e75, 0x905ea9, 0xa884f3, 0xeaaded, 0x753c54, 0xa24b6f,
    0xcf657f, 0xed8099, 0x831c5d, 0xc32454, 0xf04f78, 0xf68181, 0xfca790,
    0xfdcbb0,

    // AAP-64
    0x060608, 0x141013, 0x3b1725, 0x73172d, 0xb4202a, 0xdf3e23, 0xfa6a0a,
    0xf9a31b, 0xffd541, 0xfffc40, 0xd6f264, 0x9cdb43, 0x59c135, 0x14a02e,
    0x1a7a3e, 0x24523b, 0x122020, 0x143464, 0x285cc4, 0x249fde, 0x20d6c7,
    0xa6fcdb, 0xffffff, 0xfef3c0, 0xfad6b8, 0xf5a097, 0xe86a73, 0xbc4a9b,
    0x793a80, 0x403353, 0x242234, 0x221c1a, 0x322b28, 0x71413b, 0xbb7547,
    0xdba463, 0xf4d29c, 0xdae0ea, 0xb3b9d1, 0x8b93af, 0x6d758d, 0x4a5462,
    0x333941, 0x422433, 0x5b3138, 0x8e5252, 0xba756a, 0xe9b5a3, 0xe3e6ff,
    0xb9bffb, 0x849be4, 0x588dbe, 0x477d85, 0x23674e, 0x328464, 0x5daf8d,
    0x92dcba, 0xcdf7e2, 0xe4d2aa, 0xc7b08b, 0xa08662, 0x796755, 0x5a4e44,
    0x423934,

    // Famicube
    0x000000, 0xe03c28, 0xffffff, 0xd7d7d7, 0xa8a8a8, 0x7b7b7b, 0x343434,
    0x151515, 0x0d2030, 0x415d66, 0x71a6a1, 0xbdffca, 0x25e2cd, 0x0a98ac,
    0x005280, 0x00604b, 0x20b562, 0x58d332, 0x139d08, 0x004e00, 0x172808,
    0x376d03, 0x6ab417, 0x8cd612, 0xbeeb71, 0xeeffa9, 0xb6c121, 0x939717,
    0xcc8f15, 0xffbb31, 0xffe737, 0xf68f37, 0xad4e1a, 0x231712, 0x5c3c0d,
    0xae6c37, 0xc59782, 0xe2d7b5, 0x4f1507, 0x823c3d, 0xda655e, 0xe18289,
    0xf5b784, 0xffe9c5, 0xff82ce, 0xcf3c71, 0x871646, 0xa328b3, 0xcc69e4,
    0xd59cfc, 0xfec9ed, 0xe2c9ff, 0xa675fe, 0x6a31ca, 0x5a1991, 0x211640,
    0x3d34a5, 0x6264dc, 0x9ba0ef, 0x98dcff, 0x5ba8ff, 0x0a89ff, 0x024aca,
    0x00177d,

    // Sweet Canyon 64
    0x0f0e11, 0x2d2c33, 0x40404a, 0x51545c, 0x6b7179, 0x7c8389, 0xa8b2b6,
    0xd5d5d5, 0xeeebe0, 0xf1dbb1, 0xeec99f, 0xe1a17e, 0xcc9562, 0xab7b49,
    0x9a643a, 0x86482f, 0x783a29, 0x6a3328, 0x541d29, 0x42192c, 0x512240,
    0x782349, 0x8b2e5d, 0xa93e89, 0xd062c8, 0xec94ea, 0xf2bdfc, 0xeaebff,
    0xa2fafa, 0x64e7e7, 0x54cfd8, 0x2fb6c3, 0x2c89af, 0x25739d, 0x2a5684,
    0x214574, 0x1f2966, 0x101445, 0x3c0d3b, 0x66164c, 0x901f3d, 0xbb3030,
    0xdc473c, 0xec6a45, 0xfb9b41, 0xf0c04c, 0xf4d66e, 0xfffb76, 0xccf17a,
    0x97d948, 0x6fba3b, 0x229443, 0x1d7e45, 0x116548, 0x0c4f3f, 0x0a3639,
    0x251746, 0x48246d, 0x69189c, 0x9f20c0, 0xe527d2, 0xff51cf, 0xff7ada,
    0xff9edb,

    // Rewild 64
    0x172038, 0x253a5e, 0x3c5e8b, 0x4f8fba, 0x73bed3, 0xa4dddb, 0x193024,
    0x245938, 0x2b8435, 0x62ac4c, 0xa2dc6e, 0xc5e49b, 0x19332d, 0x25562e,
    0x468232, 0x75a743, 0xa8ca58, 0xd0da91, 0x5f6d43, 0x97933a, 0xa9b74c,
    0xcfd467, 0xd5dc97, 0xd6dea6, 0x382a28, 0x43322f, 0x564238, 0x715a42,
    0x867150, 0xb1a282, 0x4d2b32, 0x7a4841, 0xad7757, 0xc09473, 0xd7b594,
    0xe7d5b3, 0x341c27, 0x602c2c, 0x884b2b, 0xbe772b, 0xde9e41, 0xe8c170,
    0x241527, 0x411d31, 0x752438, 0xa53030, 0xcf573c, 0xda863e, 0x1e1d39,
    0x402751, 0x7a367b, 0xa23e8c, 0xc65197, 0xdf84a5, 0x090a14, 0x10141f,
    0x151d28, 0x202e37, 0x394a50, 0x577277, 0x819796, 0xa8b5b2, 0xc7cfcc,
    0xebede9,

    // Fluffytron Plus
    0x000000, 0x273449, 0x945276, 0x508d74, 0xaf847b, 0x5f574f, 0xc2c3c7,
    0xfff1e8, 0xd2859f, 0xe3b174, 0xe2dc83, 0x73c97f, 0x98bfda, 0xaea3bd,
    0xe5c7d1, 0xedd8cd, 0x588ac0, 0x41aeab, 0x9673b7, 0x3e8482, 0xa15252,
    0x8c5671, 0xc4b1aa, 0xf1dce1, 0xb86489, 0xd5946b, 0xbddea1, 0x42bf77,
    0xb1dae1, 0xddcee8, 0xc97cb7, 0xe8bab7, 0x171717, 0x1d2b53, 0x7e2553,
    0x008751, 0xab5236, 0x423c37, 0x7c8487, 0xf3fefc, 0xff004d, 0xffa300,
    0xffec27, 0x00e436, 0x29adff, 0x83769c, 0xff77a8, 0xffccaa, 0x1c5eac,
    0x00a5a1, 0x754e97, 0x125359, 0x742f29, 0x492d38, 0xa28879, 0xffacc5,
    0xc3004c, 0xeb6b00, 0x90ec42, 0x00b251, 0x64dff6, 0xbd9adf, 0xe40dab,
    0xff856d,

    // Jehkoba 64
    0x000000, 0xfabbaf, 0xeb758f, 0xd94c8e, 0xb32d7d, 0xfa9891, 0xff7070,
    0xf53141, 0xc40c2e, 0x852264, 0xfaa032, 0xf58122, 0xf2621f, 0xdb4b16,
    0x9e4c4c, 0xfad937, 0xffb938, 0xe69b22, 0xcc8029, 0xad6a45, 0xccc73d,
    0xb3b02d, 0x989c27, 0x8c8024, 0x7a5e37, 0x94bf30, 0x55b33b, 0x179c43,
    0x068051, 0x116061, 0xa0eba8, 0x7ccf9a, 0x5cb888, 0x3da17e, 0x20806c,
    0x49c2f2, 0x25acf5, 0x1793e6, 0x1c75bd, 0x195ba6, 0xae88e3, 0x7e7ef2,
    0x586ac4, 0x3553a6, 0x243966, 0xe29bfa, 0xca7ef2, 0xa35dd9, 0x773bbf,
    0x4e278c, 0xb58c7f, 0x9e7767, 0x875d58, 0x6e4250, 0x472e3e, 0xa69a9c,
    0x807980, 0x696570, 0x495169, 0x0d2140, 0x050e1a, 0xd9a798, 0xc4bbb3,
    0xf2f2da,

    // Personal grayscale + hilda32
    0x000000, 0x080808, 0x101010, 0x181818, 0x202020, 0x282828, 0x303030,
    0x383838, 0x404040, 0x484848, 0x505050, 0x585858, 0x606060, 0x686868,
    0x707070, 0x787878, 0x808080, 0x888888, 0x909090, 0x989898, 0xa0a0a0,
    0xa8a8a8, 0xb0b0b0, 0xb8b8b8, 0xc0c0c0, 0xc8c8c8, 0xd0d0d0, 0xd8d8d8,
    0xe0e0e0, 0xe8e8e8, 0xf0f0f0, 0xf8f8f8, 0xb0766a, 0x98584c, 0x673536, 
    0x47272f, 0x27141d, 0x543128, 0x854a37, 0xac6545, 0xc98459, 0xe49f63, 
    0xf9c287, 0xddac81, 0xeccfa8, 0xf9e4ca, 0xffffff, 0xdedbd2, 0xb3a49f, 
    0x9d7766, 0xd4b25b, 0xc4974d, 0xa7723f, 0x775939, 0x553e28, 0x3c2e1d, 
    0x475f5c, 0x54858c, 0x77aba7, 0xf27947, 0xd35b2b, 0x9c392d, 0xc64639, 
    0xe2654f,

    //0xFF0000, 0xFFFF00, 0x00FF00, 0x00FFFF, 0x0000FF, 0xFF00FF
};


u16 colorsystem_getcolor(struct ColorSystem *cs) {
  struct ColorSelect * css = cs->selected + cs->selected_index;
  if (cs->mode == COLORSYSMODE_PALETTE) {
    if (css->index < cs->num_colors)
      return cs->colors[css->index];
    else
      return css->forcecolor;
  } else if (cs->mode == COLORSYSMODE_RGB) {
    return rgb_to_rgba16(css->r, css->g, css->b);
  } else if (cs->mode == COLORSYSMODE_AUTOPALETTE) {
    return css->forcecolor;
  }
  return 0;
}

void colorsystem_setcolors(struct ColorSystem *cs, u32 *colors, u16 numcolors) {
  if (cs->colors)
    free(cs->colors);
  cs->colors = malloc(numcolors * sizeof(u16));
  cs->num_colors = numcolors;
  convert_palette(colors, cs->colors, numcolors);
}

void colorsystem_setcolors_default(struct ColorSystem *cs) {
  colorsystem_setcolors(cs, default_palette, sizeof(default_palette) / sizeof(u32));
}

void colorsystem_free(struct ColorSystem *cs) {
  if (cs->colors)
    free(cs->colors);
  cs->colors = NULL;
}

void colorsystem_reset(struct ColorSystem *cs) {
  cs->selected_index = 0;
  for(int i = 0; i < COLORSYS_SELECTIONS; i++) {
    cs->selected[i].index = DEFAULT_PALETTE_STARTINDEX + i * 
      (DEFAULT_PALETTE_STARTINDEX_2 - DEFAULT_PALETTE_STARTINDEX);
  }
}

u16 colorsystem_nextpalette(struct ColorSystem *cs, s8 ofs) {
  struct ColorSelect * css = cs->selected + cs->selected_index;
  css->index =
      (css->index + ofs * cs->palette_size + cs->num_colors) % (cs->num_colors);
  return css->index;
}

int colorsystem_trysetcolor(struct ColorSystem *cs, u16 color) {
  struct ColorSelect * css = cs->selected + cs->selected_index;
  if (cs->mode == COLORSYSMODE_PALETTE) {
    int start = css->index;
    if (cs->palette_size > 0) { // Start at beginning of given block size
      start -= (start % cs->palette_size);
    }
    for (int i = 0; i < cs->num_colors; i++) {
      u16 coli = (i + start) % cs->num_colors;
      if (cs->colors[coli] == color) {
        css->index = coli;
        return 1;
      }
    }
    // FORCE the setting of the color if not found
    css->forcecolor = color;
    css->index = 65535;
    return 1;
  } else if (cs->mode == COLORSYSMODE_RGB) {
    css->r = (color >> 10) & 0b11111;
    css->g = (color >> 5) & 0b11111;
    css->b = (color) & 0b11111;
    return 1;
  } else if (cs->mode == COLORSYSMODE_AUTOPALETTE) {
    css->forcecolor = color;
    return 1;
  }
  return 0;
}

/* clang-format off */

//Take an rgb WITHOUT alpha and convert to 3ds full color (with alpha)
u32 rgb24_to_rgba32c(u32 rgb)
{
   return 0xFF000000 | ((rgb >> 16) & 0x0000FF) | (rgb & 0x00FF00) | ((rgb << 16) & 0xFF0000);
}

//Convert a citro2D rgba to a weird shifted 16bit thing that Citro2D needed for proper 
//clearing. I assume it's because of byte ordering in the 3DS and Citro not
//properly converting the color for that one instance. TODO: ask about that bug
//if anyone ever gets back to you on that forum.
u32 rgba32c_to_rgba16c_32(u32 val)
{
   //Format: 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   //Half  : 0b GGBBBBBA RRRRRGGG 00000000 00000000
   u8 green = (val & 0x0000FF00) >> 11; //crush last three bits

   return 
      (
         (val & 0xFF000000 ? 0x0100 : 0) |   //Alpha is lowest bit on upper byte
         (val & 0x000000F8) |                //Red is slightly nice because it's already in the right position
         ((green & 0x1c) >> 2) | ((green & 0x03) << 14) | //Green is split between bytes
         ((val & 0x00F80000) >> 10)          //Blue is just shifted and crushed
      ) << 16; //first 2 bytes are trash
}

//Convert a citro2D rgba to a proper 16 bit color (but with the opposite byte
//ordering preserved)
u16 rgba32c_to_rgba16c(u32 val)
{
   return rgba32c_to_rgba16c_32(val) >> 16;
}

//Convert a proper 16 bit citro2d color (with the opposite byte ordering
//preserved) to a citro2D rgba
u32 rgba16c_to_rgba32c(u16 val)
{
   //Half : 0b                   GGBBBBBA RRRRRGGG
   //Full : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return
      (
         (val & 0x0100 ? 0xFF000000 : 0)  |  //Alpha always 255 or 0
         ((val & 0x3E00) << 10) |            //Blue just shift a lot
         ((val & 0x0007) << 13) | ((val & 0xc000) >> 3) |  //Green is split
         (val & 0x00f8)                      //Red is easy, already in the right place
      );
}

u16 rgba32c_to_rgba16(u32 val)
{
   //16 : 0b                   ARRRRRGG GGGBBBBB
   //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return 
      ((val & 0x80000000) >> 16) |
      ((val & 0x00F80000) >> 19) | //Blue?
      ((val & 0x0000F800) >> 6) | //green?
      ((val & 0x000000F8) << 7)  //red?
      ;
}

u32 rgba16_to_rgba32c(u16 val)
{
   //16 : 0b                   ARRRRRGG GGGBBBBB
   //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return 
      ((val & 0x8000) ? 0xFF000000 : 0) |
      ((val & 0x001F) << 19) | //Blue?
      ((val & 0x03E0) << 6) | //green?
      ((val & 0x7C00) >> 7)  //red?
      ;
}

//Convert a whole palette from regular RGB (no alpha) to true 16 bit values
//used for in-memory palettes (and written drawing data)
void convert_palette(u32 * original, u16 * destination, u16 size)
{
   for(int i = 0; i < size; i++)
      destination[i] = rgba32c_to_rgba16(rgb24_to_rgba32c(original[i]));
}

// u32 hsv_to_rgb24(float h, float s, float v){
//     float r, g, b;
// 
//     int i = floor(h * 6);
//     float f = h * 6 - i;
//     float p = v * (1 - s);
//     float q = v * (1 - f * s);
//     float t = v * (1 - (1 - f) * s);
// 
//     switch(i % 6){
//         case 0: r = v, g = t, b = p; break;
//         case 1: r = q, g = v, b = p; break;
//         case 2: r = p, g = v, b = t; break;
//         case 3: r = p, g = q, b = v; break;
//         case 4: r = t, g = p, b = v; break;
//         case 5: r = v, g = p, b = q; break;
//     }
// 
//     return (((u32)(r * 255) & 255) << 16) | (((u32)(g * 255) & 255) << 8) | ((u32)(b * 255) & 255);
// }
