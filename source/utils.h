#ifndef __HEADER_JUNKDRAW_UTILS__
#define __HEADER_JUNKDRAW_UTILS__

#include <3ds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// =====================
//       LOGGING
// =====================

// Logging is a utility I guess...
void LOGERR(const char *, ...);
void LOGWRN(const char *, ...);
void LOGINF(const char *, ...);
void LOGDBG(const char *, ...);
void LOGTRC(const char *, ...);

typedef struct {
  char * buffer;
  size_t head;
  size_t slot_count;
  size_t slot_size;
} LogBuffer;

int logbuffer_init(LogBuffer * lb, size_t slots, size_t slot_size);
void logbuffer_free(LogBuffer * lb);
// Get the real string within the global buffer for slot pos
char * logbuffer_str(LogBuffer * lb, size_t slot);

// Allows log with color and time
#define JDU_LOGBUFFER_STD(lb, color, printnow, fmt, ...) { \
  char * next = logbuffer_str((lb), (lb)->head); \
  time_t rawtime = time(NULL); \
  struct tm *timeinfo = localtime(&rawtime); \
  snprintf(next, (lb)->slot_size, "\x1b[%dm[%02d:%02d] ",  \
      color, timeinfo->tm_hour, timeinfo->tm_min); \
  size_t len = strlen(next); \
  if(len >= (lb)->slot_size) return; \
  va_list args; \
  va_start(args, fmt); \
  vsnprintf(next + len, (lb)->slot_size - len, fmt, args); \
  va_end(args); \
  if(printnow) printf("%s\n", next); \
  (lb)->head = ((lb)->head + 1) % (lb)->slot_count; \
}

// Some suggested colors for the messages
#define JDU_LOGCOLOR_TRC 35     // magenta?   //34     // blue?
#define JDU_LOGCOLOR_DBG 36     // teal?
#define JDU_LOGCOLOR_INF 37     // white?
#define JDU_LOGCOLOR_WRN 33     // yellow?
#define JDU_LOGCOLOR_ERR 31     // red?


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
#define COL5551TO8888(rs, gs, bs, as) \
  u32 r5 = (col >> rs) & 0x1F; \
  u32 g5 = (col >> gs) & 0x1F; \
  u32 b5 = (col >> bs) & 0x1F; \
  u32 a1 = (col >> as) & 0x01; \
  u32 r8 = (r5 << 3) | (r5 >> 2); \
  u32 g8 = (g5 << 3) | (g5 >> 2); \
  u32 b8 = (b5 << 3) | (b5 >> 2); \
  u32 a8 = a1 ? 0xFF : 0x00;

#define COL8888TO5551(rs, gs, bs, as) \
  u32 r8 = (col >> rs) & 0xFF; \
  u32 g8 = (col >> gs) & 0xFF; \
  u32 b8 = (col >> bs) & 0xFF; \
  u32 a8 = (col >> as) & 0xFF; \
  u16 r5 = r8 >> 3; \
  u16 g5 = g8 >> 3; \
  u16 b5 = b8 >> 3; \
  u16 a1 = a8 >> 7;

// Used to convert from SPECIFICALLY texture 16 bit format to 
// SPECIFICALLY the format used for PNG/DrawRect/etc
static inline u32 rgba5551_to_abgr8(u16 col) {
  //16 : 0b                   RRRRRGGG GGBBBBBA
  //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
  COL5551TO8888(11, 6, 1, 0);
  return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
}

// Used to convert from SPECIFICALLY my 16 bit format to
// SPECIFICALLY the format used for PNG/DrawRect/etc
static inline u32 rgba16_to_abgr8(u16 col) {
  //16 : 0b                   ARRRRRGG GGGBBBBB
  //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
  COL5551TO8888(10, 5, 0, 15);
  return (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
}

static inline u16 abgr8_to_rgba5551(u32 col) {
  //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
  //16 : 0b                   RRRRRGGG GGBBBBBA
  COL8888TO5551(0, 8, 16, 24);
  return (r5 << 11) | (g5 << 6) | (b5 << 1) | a1;
}

static inline u16 abgr8_to_rgba16(u32 col) {
  //32 : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
  //16 : 0b                   ARRRRRGG GGGBBBBB
  COL8888TO5551(0, 8, 16, 24);
  return (a1 << 15) | (r5 << 10) | (g5 << 5) | b5;
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

typedef struct {
  s32 x1;
  s32 y1;
  s32 x2; // Exclusive
  s32 y2; // Exclusive
} S32Bounds;

// Calculates the byte offset of pixel (x, y) inside an 8x8 tiled texture
static inline u32 get_tiled_pixel_offset(u32 x, u32 y, u32 width) {
  // Identify which 8x8 tile (tile_x, tile_y) contains (x, y)
  u32 tile_x = x / 8;
  u32 tile_y = y / 8;
  u32 tiles_per_row = width / 8;
  u32 tile_index = (tile_y * tiles_per_row) + tile_x;

  // Local coordinates within the 8x8 tile (0..7)
  u32 local_x = x % 8;
  u32 local_y = y % 8;

  // PICA200 Morton curve bit interleaving for an 8x8 tile:
  // Bit 0 = X0, Bit 1 = Y0, Bit 2 = X1, Bit 3 = Y1, Bit 4 = X2, Bit 5 = Y2
  u32 pixel_in_tile = ((local_x & 1) << 0) | ((local_y & 1) << 1) |
    ((local_x & 2) << 1) | ((local_y & 2) << 2) |
    ((local_x & 4) << 2) | ((local_y & 4) << 3);

  // Combine tile offset + intra-tile pixel offset
  u32 total_pixel_index = (tile_index * 64) + pixel_in_tile;
  return total_pixel_index;
}

#endif
