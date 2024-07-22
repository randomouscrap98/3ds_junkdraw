#ifndef __HEADER_BUFFER
#define __HEADER_BUFFER

#include <3ds.h>
#include "draw.h"

#define MAX_STROKE_DATA MAX_STROKE_LINES << 3

// At 100_000, will take nearly 1 full second just to scan to end. But, we 
// don't want to have the system hitching when you move to a new page, so it
// has to be low enough to not do too much work per frame. Remember that scanning
// requires parsing a variable width integer to get the page... it's not trivial.
#define MAX_DRAWDATA_SCAN 100000

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