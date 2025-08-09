#ifndef __HEADER_UNDO
#define __HEADER_UNDO

#include <3ds.h>
// #include "draw.h"

struct UndoRingStack {
  char **positions;
  u16 end; // Always EXCLUSIVE
  u16 size;
  u16 capacity;
};

void init_undoringstack(struct UndoRingStack *buffer, u16 capacity);
void reset_undoringstack(struct UndoRingStack *buffer);
void free_undoringstack(struct UndoRingStack *buffer);

// u8 undoring_size(struct UndoRingBuffer *buffer);
void undoringstack_add(struct UndoRingStack *buffer, char *);
char *undoringstack_remove(struct UndoRingStack *buffer);

#endif
