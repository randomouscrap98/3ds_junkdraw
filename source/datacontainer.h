#ifndef __HEADER_3DSJUNKDRAW_DATACONTAINER__
#define __HEADER_3DSJUNKDRAW_DATACONTAINER__

#include <3ds.h>

// These types are defined here because it is up to the data
// container how big/etc these types ultimately are
typedef s16 page_t;
// typedef u16 color_t;
typedef s8 resolutionid_t;
typedef s8 onion_t;
typedef s8 layer_t;
typedef u8 style_t;
typedef u8 width_t;
typedef u16 coord_t;
typedef u16 lineidx_t;

// These constants are defined here because the data container dictates 
// how much space is given to the data
#define JDDC_MAXLAYERS 8
#define JDDC_MAXONION 4
#define JDDC_MAXSTROKELINES 5000

// General magic string (probably won't change)
#define JDDC_MAGICSTRING "JUNKDRAW"
#define JDDC_FVERSION "01"
#define JDDC_ALIGNMENT '.'
#define JDDC_FHEADER_BASE (JDDC_MAGICSTRING JDDC_FVERSION "______")
#define JDDC_FHEADER_LEN 16

// Data conversion stuff
#define JDDC_START '0'   // Starting character
#define JDDC_BITSPER 6
#define JDDC_VARIBITSPER 5
#define JDDC_MAXVAL(x) ((1 << (x * JDDC_BITSPER)) - 1)
#define JDDC_VARIMAXVAL(x) ((1 << (x * JDDC_VARIBITSPER)) - 1)
#define JDDC_VARISTEP (1 << JDDC_VARIBITSPER) 
#define JDDC_VARIMAXSCAN 7
#define JDDC_PAGEBYTES 2
#define JDDC_COORDBYTES 2
// Header bytes are the alignment character, the page bytes, then
// the stroke/layer/width/color bytes
#define JDDC_PREAMBLEBYTES 5
#define JDDC_STROKEHEADERBYTES (1 + JDDC_PAGEBYTES + JDDC_PREAMBLEBYTES)

#define JDDC_LINESTYLE_STROKE 0
#define JDDC_LINESTYLE_COLLECTION 1
#define JDDC_PAGE_DEL 4095
#define JDDC_PAGE_TMP 4094

typedef struct {
  resolutionid_t resolution_id;
  u16 bgcolor;
  layer_t layer_count;
  onion_t onion_count;
} DataHeader;

void dataheader_default(DataHeader * header);

// General storage container for encoded drawing data. Encoding and
// decoding is performed on a special "LinePackage" type, provided
// here with the container. Scanners across data can be created
typedef struct {
  char * container;
  char * start;   // Past any header, etc
  char * end;     // One past last written byte
  size_t capacity;
  u8 sequence;    // Used to invalidate scanners
} DataContainer;

int datacontainer_init(DataContainer * dc, size_t capacity);
void datacontainer_free(DataContainer * dc);

size_t datacontainer_length(DataContainer * dc);
page_t datacontainer_last_used_page(DataContainer * dc);
page_t datacontainer_last_total_page(DataContainer * dc);
// Return whether there's enough space to add the given amount
int datacontainer_enough(DataContainer * dc, size_t added_space);
size_t datacontainer_filled(DataContainer * dc);

void datacontainer_setheader(DataContainer * dc, DataHeader * dh);
void datacontainer_getheader(DataContainer * dc, DataHeader * dh);

typedef struct { 
  coord_t x1, y1, x2, y2; 
} LineSegment;

// Lines as the data container wants them
typedef struct {
   LineSegment * lines;
   u16 capacity;
   u16 length;
   page_t page;
   u16 color;
   style_t style;
   layer_t layer;
   width_t width;
} LineContainer;

// Initialize a line container specifically to hold a stroke and no more.
int linecontainer_init_stroke(LineContainer * lc);
void linecontainer_free(LineContainer * lc);

// Lines are always added at the end of the container. No need for scanning
int datacontainer_addline(DataContainer * dc, LineContainer * lc);

typedef struct {
  DataContainer * parent;
  char * current;
  ssize_t max_scan;
  page_t page;
  // layerid_t layer;
  u8 sequence;
} DataScanner;

typedef struct {
  char * data_start;
  char * stroke_start;
  char * data_end;
  page_t page;
} DataScannerResult;

// Note: scanners are throwaway and hold onto no data, so I just return
// by value to signify this a bit
DataScanner datacontainer_get_scanner(DataContainer * dc);

DataScannerResult datascanner_next(DataScanner * ds);
// Useful for loop: scan while strokes are found
int datascanner_next_loop(DataScanner * ds, DataScannerResult * dsr);
int datascanner_at_end(DataScanner * ds);
void datascanner_reset(DataScanner * ds);

void datascannerresult_overwritepage(DataScannerResult * dsr, page_t page);
int datascannerresult_parseline(DataScannerResult * dsr, LineContainer * lc);

#endif
