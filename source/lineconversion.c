#include "lineconversion.h"
#include "datacontainer.h"
#include "utils.h"

VECTOR_DEFINE(RenderLine);
VECTOR_DEFINE(vector_RenderLine);

int lineconverter_init(LineConverter * lc) {
  lc->pending_next = 0;
  int err = vector_vector_RenderLine_init(&lc->lines);
  if(err) return err;
  err = linecontainer_init_stroke(&lc->pending);
  if(err) {
    vector_vector_RenderLine_free(&lc->lines);
  }
  return err;
}

static inline void lineconverter_delete_lines(LineConverter * lc) {
  for(size_t i = 0; i < lc->lines.length; i++) {
    vector_RenderLine_free(lc->lines.array + i);
  }
  vector_vector_RenderLine_clear(&lc->lines);
}

void lineconverter_free(LineConverter * lc) {
  // Get rid of inner lines first
  lineconverter_delete_lines(lc);
  vector_vector_RenderLine_free(&lc->lines);
}

int lineconverter_done(LineConverter * lc) {
  return lc->pending_next >= lc->pending.length;
}

void lineconverter_reset_converted(LineConverter * lc) {
  // NOTE: let the vectors get left behind from previous runs, it's fine.
  // We expect this thing to get reused a lot (at least our own usage of it)
  for(size_t i = 0; i < lc->lines.length; i++) {
    vector_RenderLine_clear(lc->lines.array + i);
  }
}

void lineconverter_reset_pending(LineConverter * lc) {
  lc->pending_next = 0;
  lc->pending.length = 0;
}

void lineconverter_reset(LineConverter * lc) {
  lineconverter_reset_converted(lc);
  lineconverter_reset_pending(lc);
}

// Convert a region of lines from the line container into the internal renderline vector.
// TODO: If this function is too expensive, consider redoing a lot of these function calls
// into raw vector manip (bad) plus assuming pre-fab vector_vector construction?
size_t lineconverter_convert(LineConverter * lc, size_t count) {
  // TODO: is this the correct conversion?
  u32 color = rgba16_to_abgr8(lc->pending.color);
  vector_vector_RenderLine_reserve(&lc->lines, lc->pending.layer + 1);
  while(lc->lines.length <= lc->pending.layer) {
    size_t next;
    vector_vector_RenderLine_increment(&lc->lines, &next);
    vector_RenderLine_init(lc->lines.array + next);
  }
  size_t end = JD_MIN(lc->pending_next + count, lc->pending.length);
  count = end - lc->pending_next;
  vector_RenderLine * rl = lc->lines.array + lc->pending.layer;
  // Reserve some space in the vector (for speed)
  vector_RenderLine_reserve(rl, rl->length + count);
  for(; lc->pending_next < end; lc->pending_next++) {
    size_t rlidx;
    vector_RenderLine_increment(rl, &rlidx);
    rl->array[rlidx].width = lc->pending.width;
    rl->array[rlidx].color = color;
    rl->array[rlidx].x1 = lc->pending.lines[lc->pending_next].x1;
    rl->array[rlidx].x2 = lc->pending.lines[lc->pending_next].x2;
    rl->array[rlidx].y1 = lc->pending.lines[lc->pending_next].y1;
    rl->array[rlidx].y2 = lc->pending.lines[lc->pending_next].y2;
  }
  return count;
}

