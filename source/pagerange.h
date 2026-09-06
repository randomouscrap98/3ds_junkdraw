#ifndef __HEADER_JUNKDRAW_PAGERANGE__
#define __HEADER_JUNKDRAW_PAGERANGE__

// Future self: if you think this can be removed or simplified, the complication
// comes from the fact that the looping is OPTIONAL, ruining basic mathematical
// looping/etc.

// Just for the types
#include "datacontainer.h"

// A page range represents some (optionally looping) range of pages, allowing for
// easy and reproducible iteration over a potentially complicated set. This is useful
// for onion skinning, where you may want to stop at 0 when scanning backwards, or
// optionally loop to some other point.
typedef struct {
  page_t page;
  page_t offset;
  page_t loop_point;
  page_t _end;        // internal; you don't need to set this
  page_t _increment;  // internal; you don't need to set this
  page_t _iter;       // internal; you don't need to set this
} PageRange;

// Precalculate some internal values for iterating over page range. Can set the
// maximum total range size (useful for eg LayerWindow)
void pagerange_begin(PageRange * range, size_t max_range);
// Call in a while loop, out has next page in range.
int pagerange_next(PageRange * range, page_t * out);

#endif
