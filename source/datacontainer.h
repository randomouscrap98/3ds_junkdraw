#ifndef __HEADER_3DSJUNKDRAW_DATACONTAINER__
#define __HEADER_3DSJUNKDRAW_DATACONTAINER__

#include <3ds.h>

// These types are defined here because it is up to the data
// container how big/etc these types ultimately are
typedef s16 page_t;
typedef u16 color_t;
typedef s8 resolutionid_t;
typedef s8 onionid_t;
typedef s8 layerid_t;

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

typedef struct {
  resolutionid_t resolution_id;
  color_t bgcolor;
  layerid_t layer_count;
  onionid_t onion_count;
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

void datacontainer_setheader(DataContainer * dc, DataHeader * dh);
void datacontainer_getheader(DataContainer * dc, DataHeader * dh);

typedef struct {
  DataContainer * parent;
  char * current;
  ssize_t max_scan;
  page_t page;
  // layerid_t layer;
  u8 sequence;
} DataScanner;

typedef struct {
  char * stroke_start;
  char * next_alignment;
} DataScannerResult;

DataScannerResult datascanner_next(DataScanner * ds);

// Note: scanners are throwaway and hold onto no data, so I just return
// by value to signify this a bit
DataScanner datacontainer_get_scanner(DataContainer * dc);

// Data conersion stuff
#define JDDC_START '0'   // Starting character
#define JDDC_BITSPER 6
#define JDDC_VARIBITSPER 5
#define JDDC_MAXVAL(x) ((1 << (x * JDDC_BITSPER)) - 1)
#define JDDC_VARIMAXVAL(x) ((1 << (x * JDDC_VARIBITSPER)) - 1)
#define JDDC_VARISTEP (1 << JDDC_VARIBITSPER) 
#define JDDC_VARIMAXSCAN 7
#define JDDC_PAGEBYTES 2

// Some optimized reads
#define JDDC_CHARS_TO_INT_1(container) ((container)[0] - JDDC_START)
#define JDDC_CHARS_TO_INT_2(container) (((container)[0] - JDDC_START) + \
  (((container)[1] - JDDC_START) << JDDC_BITSPER))

#endif
