#ifndef __HEADER_JUNKDRAW_UTILS__
#define __HEADER_JUNKDRAW_UTILS__

#include <3ds.h>

// Logging is a utility I guess...
// void LOGERR(const char *, ...);
void LOGERR(const char *, ...);
void LOGWRN(const char *, ...);
void LOGINF(const char *, ...);
void LOGDBG(const char *, ...);
void LOGTRC(const char *, ...);

// =====================
//        MATH
// =====================

#define JD_CLAMP(x, mn, mx) (x <= mn ? mn : x >= mx ? mx : x)
#define JD_LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#define JD_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define JD_MAX(a, b) ((a) >= (b) ? (a) : (b))

#define JD_REVERSE32(val) ( \
    (((val) & 0x000000FFU) << 24) | \
    (((val) & 0x0000FF00U) << 8)  | \
    (((val) & 0x00FF0000U) >> 8)  | \
    (((val) & 0xFF000000U) >> 24)   \
)

static inline u32 next_power_of_2(u32 v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}


// =====================
//        COLOR 
// =====================

static inline u32 rgba5551_to_abgr8(u16 pixel) {
  u32 r5 = (pixel >> 11) & 0x1F;
  u32 g5 = (pixel >> 6)  & 0x1F;
  u32 b5 = (pixel >> 1)  & 0x1F;
  u32 a1 =  pixel        & 0x01;

  u32 r8 = (r5 << 3) | (r5 >> 2);
  u32 g8 = (g5 << 3) | (g5 >> 2);
  u32 b8 = (b5 << 3) | (b5 >> 2);
  u32 a8 = a1 ? 0xFF : 0x00;

  return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
}


// =====================
//        DRAWING
// =====================

// Two points representing the bounds of something. Could
// be a line or a rect or whatever.
typedef struct {
  u32 x1;
  u32 y1;
  u32 x2; // Exclusive
  u32 y2; // Exclusive
} U32Bounds;

// Calculates the byte offset of pixel (x, y) inside an 8x8 tiled texture
static inline u32 get_tiled_pixel_offset(u32 x, u32 y, u32 width) {
  // 1. Identify which 8x8 tile (tile_x, tile_y) contains (x, y)
  u32 tile_x = x / 8;
  u32 tile_y = y / 8;
  u32 tiles_per_row = width / 8;
  u32 tile_index = (tile_y * tiles_per_row) + tile_x;

  // 2. Local coordinates within the 8x8 tile (0..7)
  u32 local_x = x % 8;
  u32 local_y = y % 8;

  // 3. PICA200 Morton curve bit interleaving for an 8x8 tile:
  // Bit 0 = X0, Bit 1 = Y0, Bit 2 = X1, Bit 3 = Y1, Bit 4 = X2, Bit 5 = Y2
  u32 pixel_in_tile = ((local_x & 1) << 0) | ((local_y & 1) << 1) |
    ((local_x & 2) << 1) | ((local_y & 2) << 2) |
    ((local_x & 4) << 2) | ((local_y & 4) << 3);

  // 4. Combine tile offset + intra-tile pixel offset
  u32 total_pixel_index = (tile_index * 64) + pixel_in_tile;
  return total_pixel_index;
}

#endif
