#ifndef __HEADER_VERSION__
#define __HEADER_VERSION__

#define VERSION "0.5.2_p1"

// General magic string (probably won't change)
#define MAGICSTRING "JUNKDRAW"

// Get a raw string literal for a header with given version
#define BASEFHEADER(v) (MAGICSTRING v "______")

// Version specific stuff (for file format)
#define ALIGNMENT_00 '.'
#define FHEADER_LEN_01 16
#define FVERSION_01 "01"

// Current version stuff
#define CUR_FVERSION FVERSION_01
#define CUR_FHEADER_LEN FHEADER_LEN_01
#define CUR_PUTFHEADER(d) memcpy(d, BASEFHEADER(CUR_FVERSION), CUR_FHEADER_LEN)

#endif
