#include "undo.h"
#include "log.h"

#include <stdlib.h>

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
