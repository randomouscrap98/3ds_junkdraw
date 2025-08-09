#include "undo.h"

#include <stdlib.h>

// C lets you redefine stuff... right?
#ifndef LOGDBG
#define LOGDBG(f_, ...)
#endif

void init_ringstack(struct RingStack *buffer, u16 capacity) {
  buffer->capacity = capacity;
  buffer->positions = malloc(sizeof(char *) * buffer->capacity);
  if (!buffer->positions) {
    LOGDBG("ERROR: COULD NOT INIT LINERINGBUFFER");
  }
  reset_ringstack(buffer);
}

void reset_ringstack(struct RingStack *buffer) {
  buffer->size = 0;
  buffer->next = 0;
}

void free_ringstack(struct RingStack *buffer) {
  free(buffer->positions);
  buffer->capacity = 0;
}

void ringstack_push(struct RingStack *buffer, char *item) {
  buffer->positions[buffer->next] = item;
  buffer->next = (buffer->next + 1) % buffer->capacity;
  if (buffer->size < buffer->capacity)
    buffer->size++;
}

char *ringstack_pop(struct RingStack *buffer) {
  if (buffer->size == 0) {
    return NULL;
  }
  // This COULD overflow if the capacity is too great
  buffer->next = (buffer->next - 1 + buffer->capacity) % buffer->capacity;
  buffer->size--;
  return buffer->positions[buffer->next];
}

// void init_undoringstack(struct UndoRingStack *buffer, u16 capacity) {
//   buffer->capacity = capacity;
//   buffer->positions = malloc(sizeof(char *) * buffer->capacity);
//   if (!buffer->positions) {
//     LOGDBG("ERROR: COULD NOT INIT LINERINGBUFFER");
//   }
//   reset_undoringstack(buffer);
// }
//
// void reset_undoringstack(struct UndoRingStack *buffer) {
//   buffer->undos = 0;
//   buffer->next = 0;
//   buffer->redos = 0;
// }
//
// void free_undoringstack(struct UndoRingStack *buffer) {
//   free(buffer->positions);
//   buffer->capacity = 0;
// }
//
// void undoringstack_add(struct UndoRingStack *buffer, char *item) {
//   buffer->positions[buffer->next] = item;
//   buffer->next = (buffer->next + 1) % buffer->capacity;
//   buffer->redos = 0;
//   if (buffer->undos < buffer->capacity)
//     buffer->undos++;
// }
//
// char *undoringstack_undo(struct UndoRingStack *buffer) {
//   if (buffer->undos == 0) {
//     return NULL;
//   }
//   // This COULD overflow if the capacity is too great
//   buffer->next = (buffer->next - 1 + buffer->capacity) % buffer->capacity;
//   buffer->redos++;
//   buffer->undos--;
//   return buffer->positions[buffer->next];
// }
//
// char *undoringstack_redo(struct UndoRingStack *buffer) {
//   if (buffer->redos == 0) {
//     return NULL;
//   }
//   // This COULD overflow if the capacity is too great
//   buffer->next = (buffer->next - 1 + buffer->capacity) % buffer->capacity;
//   return buffer->positions[buffer->next];
// }
