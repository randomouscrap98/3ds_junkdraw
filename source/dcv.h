
#ifndef __HEADER_DCV
#define __HEADER_DCV

#include <3ds.h>

#define DCV_START 48
#define DCV_BITSPER 6
#define DCV_VARIBITSPER 5
#define DCV_MAXVAL(x) ((1 << (x * DCV_BITSPER)) - 1)
#define DCV_VARIMAXVAL(x) ((1 << (x * DCV_VARIBITSPER)) - 1)
#define DCV_VARISTEP (1 << DCV_VARIBITSPER) 
#define DCV_VARIMAXSCAN 7

char * int_to_chars(u32 num, const u8 chars, char * container);
u32 chars_to_int(const char * container, const u8 count);
s32 special_to_signed(u32 special);
u32 signed_to_special(s32 value);
char * int_to_varwidth(u32 value, char * container);
u32 varwidth_to_int(char * container, u8 * read_count);

#endif
