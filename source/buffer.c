#include "buffer.h"

#include <stdlib.h>

// C lets you redefine stuff... right?
#ifndef LOGDBG
#define LOGDBG(f_, ...)
#endif

void init_lineringbuffer(struct LineRingBuffer * buffer, u16 capacity) {
   // To safely read a stroke with unknown amount of lines, add extra padding equal to the max stroke lines
   buffer->capacity = capacity + MAX_STROKE_LINES; 
   buffer->lines = malloc(sizeof(struct FullLine) * buffer->capacity); 
   if(!buffer->lines) {
      LOGDBG("ERROR: COULD NOT INIT LINERINGBUFFER");
   } 
   init_linepackage(&buffer->pending);
   reset_lineringbuffer(buffer);
}

void reset_lineringbuffer(struct LineRingBuffer * buffer) {
   buffer->start = 0;
   buffer->end = 0;
}

void free_lineringbuffer(struct LineRingBuffer * buffer) {
   free_linepackage(&buffer->pending);
   free(buffer->lines);
}

u16 lineringbuffer_size(struct LineRingBuffer * buffer) {
   return (buffer->end + buffer->capacity - buffer->start) % buffer->capacity;
}

// Grow the ring buffer, returning the slot you want to write to as the "grown" item
struct FullLine * lineringbuffer_grow(struct LineRingBuffer * buffer) {
   struct FullLine * result = buffer->lines + buffer->end;
   buffer->end = (buffer->end + 1) % buffer->capacity;
   return result;
}

// Shrink the ring buffer, returning the slot you just popped. Returns null if the
// buffer is empty.
struct FullLine * lineringbuffer_shrink(struct LineRingBuffer * buffer) {
   if(buffer->start == buffer->end) {
      return NULL;
   }
   struct FullLine * result = buffer->lines + buffer->start;
   buffer->start= (buffer->start + 1) % buffer->capacity;
   return result;
}

// Fill up ring buffer as much as safely possible with pre-parsed full lines, ready for drawing at any time.
char * scan_lines(struct LineRingBuffer * buffer, char * drawdata, char * drawdata_end, const u16 page) {
   char * scandata = drawdata;
   char * stroke_pointer = NULL;
   u32 scan_remaining = MAX_DRAWDATA_SCAN; // Some arbitrarily high number. This may require tweaking!
   while(scan_remaining > 0 && scandata < drawdata_end && 
         buffer->capacity - 1 - lineringbuffer_size(buffer) > MAX_STROKE_LINES) {
      // Find and parse the next stroke in our page.
      scan_remaining = MAX_DRAWDATA_SCAN - (scandata - drawdata);
      scandata = datamem_scanstroke(scandata, drawdata_end, scan_remaining, page, &stroke_pointer);
      // There's no more data, or so it seems...
      if(stroke_pointer == NULL) {
         break;
      }
      // scandata is already pointing at the end of the current stroke, so we don't need to 
      // reassign it to the output of this conversion.
      convert_data_to_linepack(&buffer->pending, stroke_pointer, scandata);
      // Now, for each line, we're going to create a special "full" line and add it
      // to the circular buffer.
      for(u16 i = 0; i < buffer->pending.line_count; i++) {
         struct FullLine * line = lineringbuffer_grow(buffer);
         convert_to_fullline(&buffer->pending, i, line);
      }
   }
   return scandata;
}
