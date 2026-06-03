#ifndef __HEADER_METADATA__
#define __HEADER_METADATA__

#include <3ds.h>

#define METAKEY_LOAD "LOAD"
#define METAKEY_SAVE "SAVE"
#define METAKEY_NEW  "NEW"

typedef struct {
  char * raw;
  u32 size;
} metacontainer;

// Get ISO datetime as a string into buf, but without timezone information...
int get_iso_datetime(char * buf, u32 size);
// Get current date as yyyymmdd (make sure buf has 9 free slots, 8 chars + null)
int get_yyyymmdd(char * buf);

int metacontainer_init(metacontainer * mc, u32 size);
void metacontainer_free(metacontainer * mc);
// Add a basic key with date to the metacontainer, IF it will fit
int metacontainer_addsimple(metacontainer * mc, const char * key);
// Scan backwards, returning the pointer to the next metadata entry.
// Pass NULL to scan from end
char * metacontainer_scanback(metacontainer * mc, char * from);
// Same as metacontainer_scanback but only returns when key is given.
// Basically a rudimentary search
char * metacontainer_scanback_key(metacontainer * mc, char * from, const char * key);
// Given a pointer to a specific metadata, return the pointer
// to the metadata past the key
char * metacontainer_skip_key(metacontainer * mc, char * pos);
// A very specific check: are the last two loads a different date. Returns
// yes if there aren't enough loads to check. Add loads before calling this
// function for maximum effect
int metacontainer_lastloads_differentdate(metacontainer * mc);

// Return a pointer to the actual metadata associated with the next meta of type given.
// Pass NULL for from to scan from back. So, if the 2nd to last line is LOAD ABCDF,
// then result will point to ABCDF
// char * metacontainer_scanback(metacontainer * mc, char * from);


#endif
