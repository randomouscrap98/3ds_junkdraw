#ifndef __HEADER_COLOR
#define __HEADER_COLOR

#include <3ds.h>

#define COLORSYSMODE_PALETTE 0
#define COLORSYSMODE_RGB 1
#define COLORSYSMODE_AUTOPALETTE 2
#define COLORSYSMODE_COUNT 2 // 3 We aren't using the last one just yet

// THIS MUST BE A POWER OF 2!!
#define COLORSYS_HISTORY 32

// To initialize, memset 0
struct ColorSystem {
  u16 history[COLORSYS_HISTORY]; // A small buffer for historical color
  u16 *colors;      // All colors in system (all colors across all palettes)
  u16 num_colors;   // Amount of colors assigned
  u16 palette_size; // Numer of colors in a single "palette"
  u8 mode;          // Color picking mode
  u16 index;        // If in palette mode, index into colors
  u8 r;             // Red value if in rgb mode
  u8 g;             // green value in rgb mode
  u8 b;             // blue value in rgb mode
  u16 forcecolor;   // For anything that just has a color, no index
};

// Regardless of the mode, get the current color from the color system
u16 colorsystem_getcolor(struct ColorSystem *cs);
// Malloc some colors and convert them all
void colorsystem_setcolors(struct ColorSystem *cs, u32 *colors, u16 numcolors);
void colorsystem_free(struct ColorSystem *cs);
// Shift the palette up or down by given amount. Return the new index
u16 colorsystem_nextpalette(struct ColorSystem *cs, s8 ofs);
// Depending on the mode, attempt to set the color. May do vastly different
// things; for instance, setting the color in palette mode will scan for a
// matching color and only change things if color is found
int colorsystem_trysetcolor(struct ColorSystem *cs, u16 color);

// Get the current palette slot based on the global index.
static inline u16 colorsystem_getpaletteslot(struct ColorSystem *cs) {
  return cs->index / cs->palette_size;
}
// Get a pointer into the current palette
static inline u16 *colorsystem_getcurrentpalette(struct ColorSystem *cs) {
  if (cs->index < cs->num_colors)
    return cs->colors + colorsystem_getpaletteslot(cs) * cs->palette_size;
  else
    return cs->colors;
}
// Get the offset into the current palette slot, useful for drawing interfaces
static inline u16 colorsystem_getpaletteoffset(struct ColorSystem *cs) {
  if (cs->index < cs->num_colors)
    return cs->index % cs->palette_size;
  else // this is a forced color. This return might be stupid
    return 65535;
}
// Set the total color based on offset into current palette
static inline u16 colorsystem_setpaletteoffset(struct ColorSystem *cs, u16 i) {
  cs->index = colorsystem_getpaletteslot(cs) * cs->palette_size + i;
  return cs->index;
}

// Convert rgb (in the 0 to 31 range) to a true rgb
static inline u16 rgb_to_rgba16(u8 r, u8 g, u8 b) {
  return 0x8000 | ((r & 0b11111) << 10) | ((g & 0b11111) << 5) | (b & 0b11111);
}

u32 rgb24_to_rgba32c(u32 rgb);
u32 rgba32c_to_rgba16c_32(u32 val);
u16 rgba32c_to_rgba16c(u32 val);
u32 rgba16c_to_rgba32c(u16 val);
u16 rgba32c_to_rgba16(u32 val); // This is proper rgbaa16
u32 rgba16_to_rgba32c(u16 val);
void convert_palette(u32 *original, u16 *destination, u16 size);

// All values should be 0 to 1
// u32 hsv_to_rgb32(float h, float s, float v);

#endif
