#include "undo.h"

#include <stdlib.h>

// C lets you redefine stuff... right?
#ifndef LOGDBG
#define LOGDBG(f_, ...)
#endif

void init_undoringstack(struct UndoRingStack *buffer, u16 capacity) {
  buffer->capacity = capacity;
  buffer->positions = malloc(sizeof(char *) * buffer->capacity);
  if (!buffer->positions) {
    LOGDBG("ERROR: COULD NOT INIT LINERINGBUFFER");
  }
  reset_undoringstack(buffer);
}

void reset_undoringstack(struct UndoRingStack *buffer) {
  buffer->size = 0;
  buffer->end = 0;
}

void free_undoringstack(struct UndoRingStack *buffer) {
  free(buffer->positions);
  buffer->capacity = 0;
}

// u8 undoringbuffer_size(struct UndoRingBuffer *buffer) {
//   return (buffer->end + buffer->capacity - buffer->start) % buffer->capacity;
// }

void undoringstack_add(struct UndoRingStack *buffer, char *item) {
  buffer->positions[buffer->end] = item;
  // char ** result = buffer->positions + buffer->end;
  buffer->end = (buffer->end + 1) % buffer->capacity;
  if (buffer->size < buffer->capacity)
    buffer->size++;
  // return result;
}

// Shrink the ring buffer, returning the slot you just popped. Returns null if
// the buffer is empty.
char *undoringstack_remove(struct UndoRingStack *buffer) {
  if (buffer->size == 0) {
    return NULL;
  }
  // This COULD overflow if the capacity is too great
  buffer->end = (buffer->end - 1 + buffer->capacity) % buffer->capacity;
  return buffer->positions[buffer->end];
  // char ** result = buffer->positions + buffer->end;
  // return result;
}
