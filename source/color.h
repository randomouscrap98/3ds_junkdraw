#ifndef __HEADER_COLOR
#define __HEADER_COLOR

#include <3ds.h>

u32 rgb24_to_rgba32c(u32 rgb);
u32 rgba32c_to_rgba16c_32(u32 val);
u16 rgba32c_to_rgba16c(u32 val);
u32 rgba16c_to_rgba32c(u16 val);
u16 rgba32c_to_rgba16(u32 val); //This is proper rgbaa16
u32 rgba16_to_rgba32c(u16 val);
void convert_palette(u32 * original, u16 * destination, u16 size);

#endif