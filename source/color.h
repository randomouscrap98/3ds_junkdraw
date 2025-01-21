#ifndef __HEADER_COLOR
#define __HEADER_COLOR

#include <3ds.h>

#define COLORSYSMODE_PALETTE 0
#define COLORSYSMODE_RGB 1

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
  return cs->colors + colorsystem_getpaletteslot(cs) * cs->palette_size;
}
// Get the offset into the current palette slot, useful for drawing interfaces
static inline u16 colorsystem_getpaletteoffset(struct ColorSystem *cs) {
  return cs->index % cs->palette_size;
}
// Set the total color based on offset into current palette
static inline u16 colorsystem_setpaletteoffset(struct ColorSystem *cs, u16 i) {
  return colorsystem_getpaletteslot(cs) * cs->palette_size + i;
}

u32 rgb24_to_rgba32c(u32 rgb);
u32 rgba32c_to_rgba16c_32(u32 val);
u16 rgba32c_to_rgba16c(u32 val);
u32 rgba16c_to_rgba32c(u16 val);
u16 rgba32c_to_rgba16(u32 val); // This is proper rgbaa16
u32 rgba16_to_rgba32c(u16 val);
void convert_palette(u32 *original, u16 *destination, u16 size);

#endif
