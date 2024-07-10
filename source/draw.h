#ifndef __HEADER_DRAW
#define __HEADER_DRAW

#include <3ds.h>

#define DRAWDATA_ALIGNMENT '.'
#define LAYER_WIDTH 1000
#define LAYER_HEIGHT 1000

// ---------- GENERAL UTILS ----------------

#define CENTER_RECT_ALIGNPIXEL(x,y,width) \
   float __c_r_ap_ofs = (width / 2.0) - 0.5; \
   x = floor(x - __c_r_ap_ofs); y = floor(y - __c_r_ap_ofs);

typedef void (* rectangle_func)(float, float, u16, u32); //X,Y,width,32-bit color

// ------- Draw data conversion -------------

#define DCV_CLAMP(x, mn, mx) (x <= mn ? mn : x >= mx ? mx : x)

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
u32 varwidth_to_int(const char * container, u8 * read_count);

// ---------- Line package system ----------------

#define LINESTYLE_STROKE 0

struct SimpleLine { u16 x1, y1, x2, y2; };

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

/* struct ScreenState
{
   float offset_x;
   float offset_y;
   float zoom;
};

struct SimpleLine * add_point_to_stroke(struct LinePackage * pending, 
      const touchPosition * pos, const struct ScreenState * mod);*/

char * convert_lines_to_data(struct LinePackage * lines, char * container, u32 container_size);
char * convert_data_to_lines(struct LinePackage * package, char * data, char * data_end);

void pixaligned_linefunc (const struct SimpleLine * line, u16 width, u32 color, rectangle_func rect_f);

void pixaligned_linepackfunc(const struct LinePackage * linepack, u16 pack_start, u16 pack_end, rectangle_func rect_f);
void pixaligned_linepackfunc_all(const struct LinePackage * linepack, rectangle_func rect_f);

char * convert_linepack_to_data(struct LinePackage * lines, char * container, u32 container_size);
char * convert_data_to_linepack(struct LinePackage * package, char * data, char * data_end);

// -------------- Scan draw system ------------

#define MAX_DRAW_DATA (u32)5000000
#define MAX_STROKE_LINES 5000
#define MAX_STROKE_DATA MAX_STROKE_LINES << 3

#define MAX_DRAWDATA_SCAN 100000
#define MAX_FRAMELINES 1000

char * write_to_datamem(char * stroke_data, char * stroke_end, u16 page, char * mem, char * mem_end);
char * datamem_scanstroke(char * start, char * end, const u32 max_scan, const u16 page, char ** stroke_start);

struct ScanDrawData 
{
   struct LinePackage * packages;
   struct SimpleLine * all_lines;

   //Status trackers
   struct LinePackage * current_package;
   struct LinePackage * last_package;
   struct SimpleLine * last_line;

   u32 packages_capacity;
   u32 lines_capacity;
};

void scandata_initialize(struct ScanDrawData * data, u32 max_line_buffer);
void scandata_free(struct ScanDrawData * data);
char * scandata_parse(struct ScanDrawData * scandata, char * drawdata, char * drawdata_end, u32 line_scancount, const u16 page);

#endif
