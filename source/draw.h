#ifndef __HEADER_DRAW
#define __HEADER_DRAW

#include <3ds.h>

#define DRAWDATA_ALIGNMENT '.'
#define LAYER_WIDTH 1000
#define LAYER_HEIGHT 1000

// Some guarantees made by the drawing system. I guess you don't HAVE to follow them, 
// but if you're consuming this, you probably should...
#define MAX_DRAW_DATA (u32)5000000
#define MAX_STROKE_LINES 5000

// ---------- GENERAL UTILS ----------------

#define CENTER_RECT_ALIGNPIXEL(x,y,width) \
   float __c_r_ap_ofs = (width / 2.0) - 0.5; \
   x = floor(x - __c_r_ap_ofs); y = floor(y - __c_r_ap_ofs);

typedef void (* rectangle_func)(float, float, u16, u32); //X,Y,width,32-bit color
//typedef struct FullLine * (* get_)

// ------- Draw data conversion -------------

#define DCV_CLAMP(x, mn, mx) (x <= mn ? mn : x >= mx ? mx : x)
#define DCV_LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#define DCV_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define DCV_MAX(a, b) ((a) >= (b) ? (a) : (b))

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
struct FullLine {
   u32 color; // Must be pre-converted.
   u16 x1, y1, x2, y2;
   u8 style;
   u8 layer;
   u8 width;
};

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

void init_linepackage(struct LinePackage * package);
void free_linepackage(struct LinePackage * package);
void convert_to_fullline(const struct LinePackage * package, u16 line_index, struct FullLine * result);

void pixaligned_linefunc (const struct FullLine * line, rectangle_func rect_f);
void pixaligned_linepackfunc(const struct LinePackage * linepack, u16 pack_start, u16 pack_end, rectangle_func rect_f);

char * convert_linepack_to_data(struct LinePackage * lines, char * container, u32 container_size);
char * convert_data_to_linepack(struct LinePackage * package, char * data, char * data_end);

char * write_to_datamem(char * stroke_data, char * stroke_end, u16 page, char * mem, char * mem_end);
char * datamem_scanstroke(char * start, char * end, const u32 max_scan, const u16 page, char ** stroke_start);

// Find last used page within data given. Should be pretty fast...
u32 last_used_page(char * data, u32 length);

// -------------- Scan draw system ------------

#endif
