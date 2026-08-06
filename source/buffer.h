#ifndef __HEADER_BUFFER
#define __HEADER_BUFFER

#include <3ds.h>
#include "draw.h"

#define MAX_STROKE_DATA MAX_STROKE_LINES << 3

#define MAX_DRAWDATA_SCAN 500000

// A circular buffer which is able to "pack" lines from disparate strokes together
// into one buffer, useful for drawing later. Use out of band split for layers
struct LineRingBuffer {
   struct FullLine * lines;
   u16 start;
   u16 end;
   u16 capacity;
   struct LinePackage pending;
};

void init_lineringbuffer(struct LineRingBuffer * buffer, u16 capacity);
void reset_lineringbuffer(struct LineRingBuffer * buffer);
void free_lineringbuffer(struct LineRingBuffer * buffer);

u16 lineringbuffer_size(struct LineRingBuffer * buffer);
struct FullLine * lineringbuffer_grow(struct LineRingBuffer * buffer);
struct FullLine * lineringbuffer_shrink(struct LineRingBuffer * buffer);

// Scan the maximum safe amount of strokes into the given buffer. Size the buffer appropriately
char * scan_lines(struct LineRingBuffer * buffer, char * drawdata, char * drawdata_end, const u16 page);

#endif
