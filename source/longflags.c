#include "longflags.h"

#include <stdlib.h>
#include <string.h>

static int longflags_expand(longflags * flags, u32 length) {
  //printf("LONGFLAGS_EXPAND %d\n", length);
  if(length <= flags->length) return 0;
  u32 olength = flags->length;
  flags->length = length;
  flags->flags = realloc(flags->flags, sizeof(u32) * flags->length);
  //printf("EXPANDED %d\n", length);
  if(flags->flags == NULL) {
    return 1;
  }
  for(; olength < flags->length; olength++) {
    flags->flags[olength] = 0;
  }
  return 0;
}

int longflags_init(longflags * flags) {
  flags->length = 0;
  flags->flags = NULL;
  return longflags_expand(flags, 1);
}

// Make sure dest is initialized!! Only VALUES are copied; does not make a 
// necessarily perfect copy (you can do that yourself with a basic memcpy).
// EG the length might be different etc.
int longflags_copy(longflags * dest, longflags * src) {
  // Some optimizations: if memory is already allocated for dest,
  // don't bother reallocating
  int result = longflags_expand(dest, src->length);
  if(result) return result;
  memcpy(dest->flags, src->flags, sizeof(u32) * src->length);
  // Zero out any remaining data, as missing data is always 0
  if(dest->length > src->length) {
    memset(dest->flags + src->length, 0, 
           sizeof(u32) * (dest->length - src->length));
  }
  return 0;
}

int longflags_clear(longflags * flags) {
  if(flags->flags) {free(flags->flags); }
  return longflags_init(flags);
}

void longflags_zero(longflags * flags) {
  memset(flags->flags, 0, sizeof(u32) * flags->length);
}

int longflags_rawset(longflags * flags, u32 flag, int set) {
  u32 pos = flag >> 5;
  int result = longflags_expand(flags, pos + 1);
  if(result) return result;
  switch(set) {
    case 0:
      flags->flags[pos] &= ~(1 << (flag & 0x1F));
      break;
    case 1:
      flags->flags[pos] |= (1 << (flag & 0x1F));
      break;
    case 2:
      flags->flags[pos] ^= (1 << (flag & 0x1F));
      break;
  }
  return 0;
}

int longflags_set(longflags * flags, u32 flag) {
  return longflags_rawset(flags, flag, 1);
}
int longflags_unset(longflags * flags, u32 flag) {
  return longflags_rawset(flags, flag, 0);
}
int longflags_toggle(longflags * flags, u32 flag) {
  return longflags_rawset(flags, flag, 2);
}

int longflags_isset(longflags * flags, u32 flag) {
  u32 pos = flag >> 5;
  if(pos >= flags->length) return 0;
  return flags->flags[pos] & (1 << (flag & 0x1F));
}

void longflags_free(longflags * flags) {
  if(flags->flags) {free(flags->flags); }
  flags->length = 0;
}
