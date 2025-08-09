#ifndef __HEADER_UNDO
#define __HEADER_UNDO

#include <3ds.h>
// #include "draw.h"

// Used for both undo and redo buffer. When you add a stroke, add
// the PREVIOUS position to the undo buffer. When you undo, add
// current pos to redo buffer.
struct RingStack {
  char **positions;
  u16 next; // ALWAYS exclusive
  u16 size;
  u16 capacity;
};

void init_ringstack(struct RingStack *buffer, u16 capacity);
void reset_ringstack(struct RingStack *buffer);
void free_ringstack(struct RingStack *buffer);

// u8 undoring_size(struct UndoRingBuffer *buffer);
void ringstack_push(struct RingStack *buffer, char *);
char *ringstack_pop(struct RingStack *buffer);
// char * undoringstack_redo(struct UndoRingStack *buffer);

// struct UndoRingStack {
//   char **positions;
//   u16 next; // ALWAYS exclusive
//   u16 undos;
//   u16 redos;
//   u16 capacity;
// };

// void init_undoringstack(struct UndoRingStack *buffer, u16 capacity);
// void reset_undoringstack(struct UndoRingStack *buffer);
// void free_undoringstack(struct UndoRingStack *buffer);
//
// // u8 undoring_size(struct UndoRingBuffer *buffer);
// void undoringstack_add(struct UndoRingStack *buffer, char *);
// char *undoringstack_undo(struct UndoRingStack *buffer);
// char *undoringstack_redo(struct UndoRingStack *buffer);

#endif
