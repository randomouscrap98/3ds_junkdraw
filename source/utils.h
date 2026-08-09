#ifndef __HEADER_JUNKDRAW_UTILS__
#define __HEADER_JUNKDRAW_UTILS__

#include <3ds.h>

#define JD_CLAMP(x, mn, mx) (x <= mn ? mn : x >= mx ? mx : x)
#define JD_LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#define JD_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define JD_MAX(a, b) ((a) >= (b) ? (a) : (b))

static inline u32 next_power_of_2(u32 v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

// Logging is a utility I guess...
// void LOGERR(const char *, ...);
void LOGDBG(const char *, ...);
void LOGTRACE(const char *, ...);


#endif
