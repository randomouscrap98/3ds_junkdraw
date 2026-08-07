#ifndef __HEADER_LONGFLAGS_3DSJUNKDRAW__
#define __HEADER_LONGFLAGS_3DSJUNKDRAW__

#include <3ds.h>

// Storage structure for many many flag bits, beyond the usual limits
typedef struct {
  u32 length;
  u32 * flags;
} longflags;

int longflags_init(longflags * flags);
// Fully clear out the flags, freeing up the memory and setting all flags to 0.
// The flags are still fully usable after this.
int longflags_clear(longflags * flags);
// Set all existing flags to zero. Does not malloc or free
void longflags_zero(longflags * flags);
// Make a deep copy of src into dest. MAKE SURE DEST IS INITIALIZED!
int longflags_copy(longflags * dest, longflags * src);
int longflags_rawset(longflags * flags, u32 flag, int set);
int longflags_set(longflags * flags, u32 flag);
int longflags_unset(longflags * flags, u32 flag);
int longflags_toggle(longflags * flags, u32 flag);
int longflags_isset(longflags * flags, u32 flag);
void longflags_free(longflags * flags);

#endif
