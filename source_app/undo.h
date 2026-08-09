#ifndef __HEADER_UNDO
#define __HEADER_UNDO

#include <3ds.h>

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

void ringstack_push(struct RingStack *buffer, char *);
char *ringstack_pop(struct RingStack *buffer);

#endif
