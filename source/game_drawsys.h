
#ifndef __HEADER_GAMEDRAWSYS
#define __HEADER_GAMEDRAWSYS

#include <3ds.h>
#include "lib/myutils.h"
#include "game_drawctrl.h"

// A basic representation of a collection of lines. Doesn't understand "tools"
// or any of that, it JUST has the information required to draw the lines.
struct LinePackage {
   u8 style;
   u8 layer;
   u16 color;
   u8 width;
   struct SimpleLine * lines;
   u16 line_count;
   u16 max_lines;
};

struct SimpleLine * add_point_to_stroke(struct LinePackage * pending, 
      const touchPosition * pos, const struct ScreenState * mod);

char * convert_lines_to_data(struct LinePackage * lines, char * container, u32 container_size);
char * convert_data_to_lines(struct LinePackage * package, char * data, char * data_end);

void pixaligned_linepackfunc(
   const struct LinePackage * linepack, u16 pack_start, u16 pack_end, rectangle_func rect_f);
void pixaligned_linepackfunc_all(const struct LinePackage * linepack, rectangle_func rect_f);

char * convert_linepack_to_data(struct LinePackage * lines, char * container, u32 container_size);
char * convert_data_to_linepack(struct LinePackage * package, char * data, char * data_end);

#endif
