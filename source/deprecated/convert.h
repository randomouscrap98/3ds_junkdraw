#ifndef __HEADER_CONVERT__
#define __HEADER_CONVERT__

#include <stdlib.h>

// Convert a FULL file from version 00 to 01 (including inserting the header).
// Return the new data end (data size may increase). Null is error
char * convert_00_01(char * data_begin, char * data_end, size_t max_size);

#endif
