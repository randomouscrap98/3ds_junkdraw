
#ifndef __CCONFIG_HEADER_
#define __CCONFIG_HEADER_

//Release mode is a special mode
//#define RELEASE

// These are all compiler config things, which may be applied to any
// individually compiled portion of this app. Probably not the right way to do
// this.

#ifdef RELEASE
//Put anything that goes specifically for release in here
#else
//Put everything for debug mode in here
#define DEBUG_PRINT
#define DEBUG_PRINT_TIME
#define DEBUG_RUNTESTS

#define DEBUG_PRINT_MINROW 21
#define DEBUG_PRINT_ROWS 5

extern u8 _db_prnt_row;
#define DEBUG_PRINT_SPECIAL() { printf("\x1b[%d;1H\x1b[33m", _db_prnt_row + DEBUG_PRINT_MINROW); \
   _db_prnt_row = (_db_prnt_row + 1) % DEBUG_PRINT_ROWS; }

#endif

#endif
