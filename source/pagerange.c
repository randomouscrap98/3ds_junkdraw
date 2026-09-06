#include "pagerange.h"

#include <stdlib.h>

void pagerange_begin(PageRange * range, size_t max_range) {
  range->_increment = range->offset < 0 ? -1 : 1;
  if(max_range > 0 && abs(range->offset) >= max_range) {
    range->offset = range->_increment * (max_range - 1);
  }
  /* EXCLUSIVE (to make loops simpler) */
  range->_end = range->page + range->offset + range->_increment;
  /* Oops, don't go past the start! */
  if(-range->offset > range->page) {
    if(range->loop_point > 0) { /* looping */
      /* very non performant but only if you pass in stupid values (nobody will) */
      while(range->_end < 0) { range->_end += range->loop_point; }
    } else {
      range->_end = 0;  /* just clamp */
    }
  }
  range->_iter = range->page;
}

int pagerange_next(PageRange * range, page_t * out) {
  // We ASSUME the user has properly "began" the range system (for performance)
  if(range->_iter < 0) range->_iter += range->loop_point; 
  if(range->_iter == range->_end) return 0;
  *out = range->_iter;
  range->_iter += range->_increment;
  return 1;
}
