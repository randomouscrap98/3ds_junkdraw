#ifndef __HEADER_JUNKDRAW_UTILS__
#define __HEADER_JUNKDRAW_UTILS__

#define JD_CLAMP(x, mn, mx) (x <= mn ? mn : x >= mx ? mx : x)
#define JD_LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#define JD_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define JD_MAX(a, b) ((a) >= (b) ? (a) : (b))

// Logging is a utility I guess...
// void LOGERR(const char *, ...);
void LOGDBG(const char *, ...);
void LOGTRACE(const char *, ...);

#endif
