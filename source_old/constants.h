#ifndef __HEADER_CONSTANTS
#define __HEADER_CONSTANTS

// -- Some SUPER IMPORTANT constants

#define VERSION "0.2.3"
#define FOLDER_BASE "/3ds/junkdraw/"
#define SAVE_BASE FOLDER_BASE"saves/"
#define SCREENSHOTS_BASE FOLDER_BASE"screenshots/"

#define WLAN_PASSPHRASE VERSION"_wanted_to_call_it_junk_drawer"
#define WLAN_COMMID 0x5812FADB
#define CONTYPE_HOSTLOCAL 1
#define CONTYPE_CONNECTLOCAL 2

#define SCREENWIDTH 320
#define SCREENHEIGHT 240

#define LAYER_WIDTH 1000
#define LAYER_HEIGHT 1000
#define TEXTURE_WIDTH 1024
#define TEXTURE_HEIGHT 1024
#define LAYER_FORMAT GPU_RGBA5551
#define LAYER_EDGEBUF 8
#define LAYER_COUNT 2

#define CPAD_DEADZONE 40
#define CPAD_THEORETICALMAX 160
#define CPAD_PAGECONST 1
#define CPAD_PAGEMULT 0.02f
#define CPAD_PAGECURVE 3.2f

#define BREPEAT_DELAY 20
#define BREPEAT_INTERVAL 7

#define MAX_DRAW_DATA (u32)5000000
#define MAX_STROKE_LINES 5000
#define MAX_STROKE_DATA MAX_STROKE_LINES << 3
#define MIN_ZOOMPOWER -2
#define MAX_ZOOMPOWER 4
#define MIN_WIDTH 1
#define MAX_WIDTH 64
#define MAX_PAGE 0xFFFF
#define MAX_FRAMELINES 1000
#define MAX_DRAWDATA_SCAN 100000

#define MAX_FILENAME 64
#define MAX_FILENAMESHOW 5
#define MAX_FILEPATH 256
#define MAX_TEMPSTRING 2048
#define MAX_ALLFILENAMES 65535

#define DRAWDATA_ALIGNMENT '.'

#define TOOL_PENCIL 0
#define TOOL_ERASER 1
#define TOOL_COUNT 2
#define TOOL_CHARS "pe"

#define LINESTYLE_STROKE 0

#define SCREEN_COLOR C2D_Color32(90,90,90,255)
#define CANVAS_BG_COLOR C2D_Color32(255,255,255,255)
#define CANVAS_LAYER_COLOR C2D_Color32(0,0,0,0)

#define SCROLL_WIDTH 3
#define SCROLL_BG C2D_Color32f(0.8,0.8,0.8,1)
#define SCROLL_BAR C2D_Color32f(0.5,0.5,0.5,1)

#define PALETTE_COLORS 64
#define PALETTE_SWATCHWIDTH 18
#define PALETTE_SWATCHMARGIN 2
#define PALETTE_SELECTED_COLOR C2D_Color32f(1,0,0,1)
#define PALETTE_BG C2D_Color32f(0.3,0.3,0.3,1)
#define PALETTE_STARTINDEX 1
#define PALETTE_MINISIZE 10
#define PALETTE_MINIPADDING 1
#define PALETTE_OFSX 10
#define PALETTE_OFSY 10

#define MAINMENU_TOP 6
#define MAINMENU_TITLE "Main menu:"
#define MAINMENU_ITEMS "New\0Save\0Load\0Host Local\0Connect Local\0Exit App\0"
#define MAINMENU_NEW 0
#define MAINMENU_SAVE 1
#define MAINMENU_LOAD 2
#define MAINMENU_HOSTLOCAL 3
#define MAINMENU_CONNECTLOCAL 4
#define MAINMENU_EXIT 5

#define PRINTDATA_WIDTH 40
#define STATUS_MAINCOLOR 36
#define STATUS_ACTIVECOLOR 37
#define FILELOAD_MENUCOUNT 10
#define CONSTATUS_ANIMTIME 15 
#define CONSTATUS_ANIMFRAMES 4

//Weird stuff that should maybe be refactored
#define MY_C2DOBJLIMIT 8192
#define MY_C2DOBJLIMITSAFETY MY_C2DOBJLIMIT - 100
#define PSX1BLEN 30

typedef u16 page_num;
typedef u8 layer_num;
//typedef u16 stroke_num;


// The base palette definitions. Unless otherwise stated, all palettes are
// taken from lospec.com

/*u32 base_palette[] = { 
   //Endesga 64
   0xff0040, 0x131313, 0x1b1b1b, 0x272727, 0x3d3d3d, 0x5d5d5d, 0x858585, 0xb4b4b4,
   0xffffff, 0xc7cfdd, 0x92a1b9, 0x657392, 0x424c6e, 0x2a2f4e, 0x1a1932, 0x0e071b,
   0x1c121c, 0x391f21, 0x5d2c28, 0x8a4836, 0xbf6f4a, 0xe69c69, 0xf6ca9f, 0xf9e6cf,
   0xedab50, 0xe07438, 0xc64524, 0x8e251d, 0xff5000, 0xed7614, 0xffa214, 0xffc825,
   0xffeb57, 0xd3fc7e, 0x99e65f, 0x5ac54f, 0x33984b, 0x1e6f50, 0x134c4c, 0x0c2e44,
   0x00396d, 0x0069aa, 0x0098dc, 0x00cdf9, 0x0cf1ff, 0x94fdff, 0xfdd2ed, 0xf389f5,
   0xdb3ffd, 0x7a09fa, 0x3003d9, 0x0c0293, 0x03193f, 0x3b1443, 0x622461, 0x93388f,
   0xca52c9, 0xc85086, 0xf68187, 0xf5555d, 0xea323c, 0xc42430, 0x891e2b, 0x571c27,

   //Resurrect
   0x2e222f, 0x3e3546, 0x625565, 0x966c6c, 0xab947a, 0x694f62, 0x7f708a, 0x9babb2,
   0xc7dcd0, 0xffffff, 0x6e2727, 0xb33831, 0xea4f36, 0xf57d4a, 0xae2334, 0xe83b3b,
   0xfb6b1d, 0xf79617, 0xf9c22b, 0x7a3045, 0x9e4539, 0xcd683d, 0xe6904e, 0xfbb954,
   0x4c3e24, 0x676633, 0xa2a947, 0xd5e04b, 0xfbff86, 0x165a4c, 0x239063, 0x1ebc73,
   0x91db69, 0xcddf6c, 0x313638, 0x374e4a, 0x547e64, 0x92a984, 0xb2ba90, 0x0b5e65,
   0x0b8a8f, 0x0eaf9b, 0x30e1b9, 0x8ff8e2, 0x323353, 0x484a77, 0x4d65b4, 0x4d9be6,
   0x8fd3ff, 0x45293f, 0x6b3e75, 0x905ea9, 0xa884f3, 0xeaaded, 0x753c54, 0xa24b6f,
   0xcf657f, 0xed8099, 0x831c5d, 0xc32454, 0xf04f78, 0xf68181, 0xfca790, 0xfdcbb0,

   //AAP-64
   0x060608, 0x141013, 0x3b1725, 0x73172d, 0xb4202a, 0xdf3e23, 0xfa6a0a, 0xf9a31b,
   0xffd541, 0xfffc40, 0xd6f264, 0x9cdb43, 0x59c135, 0x14a02e, 0x1a7a3e, 0x24523b,
   0x122020, 0x143464, 0x285cc4, 0x249fde, 0x20d6c7, 0xa6fcdb, 0xffffff, 0xfef3c0,
   0xfad6b8, 0xf5a097, 0xe86a73, 0xbc4a9b, 0x793a80, 0x403353, 0x242234, 0x221c1a,
   0x322b28, 0x71413b, 0xbb7547, 0xdba463, 0xf4d29c, 0xdae0ea, 0xb3b9d1, 0x8b93af,
   0x6d758d, 0x4a5462, 0x333941, 0x422433, 0x5b3138, 0x8e5252, 0xba756a, 0xe9b5a3,
   0xe3e6ff, 0xb9bffb, 0x849be4, 0x588dbe, 0x477d85, 0x23674e, 0x328464, 0x5daf8d,
   0x92dcba, 0xcdf7e2, 0xe4d2aa, 0xc7b08b, 0xa08662, 0x796755, 0x5a4e44, 0x423934,

   //Famicube
   0x000000, 0xe03c28, 0xffffff, 0xd7d7d7, 0xa8a8a8, 0x7b7b7b, 0x343434, 0x151515,
   0x0d2030, 0x415d66, 0x71a6a1, 0xbdffca, 0x25e2cd, 0x0a98ac, 0x005280, 0x00604b,
   0x20b562, 0x58d332, 0x139d08, 0x004e00, 0x172808, 0x376d03, 0x6ab417, 0x8cd612,
   0xbeeb71, 0xeeffa9, 0xb6c121, 0x939717, 0xcc8f15, 0xffbb31, 0xffe737, 0xf68f37,
   0xad4e1a, 0x231712, 0x5c3c0d, 0xae6c37, 0xc59782, 0xe2d7b5, 0x4f1507, 0x823c3d,
   0xda655e, 0xe18289, 0xf5b784, 0xffe9c5, 0xff82ce, 0xcf3c71, 0x871646, 0xa328b3,
   0xcc69e4, 0xd59cfc, 0xfec9ed, 0xe2c9ff, 0xa675fe, 0x6a31ca, 0x5a1991, 0x211640,
   0x3d34a5, 0x6264dc, 0x9ba0ef, 0x98dcff, 0x5ba8ff, 0x0a89ff, 0x024aca, 0x00177d,

   //Sweet Canyon 64
   0x0f0e11, 0x2d2c33, 0x40404a, 0x51545c, 0x6b7179, 0x7c8389, 0xa8b2b6, 0xd5d5d5,
   0xeeebe0, 0xf1dbb1, 0xeec99f, 0xe1a17e, 0xcc9562, 0xab7b49, 0x9a643a, 0x86482f,
   0x783a29, 0x6a3328, 0x541d29, 0x42192c, 0x512240, 0x782349, 0x8b2e5d, 0xa93e89,
   0xd062c8, 0xec94ea, 0xf2bdfc, 0xeaebff, 0xa2fafa, 0x64e7e7, 0x54cfd8, 0x2fb6c3,
   0x2c89af, 0x25739d, 0x2a5684, 0x214574, 0x1f2966, 0x101445, 0x3c0d3b, 0x66164c,
   0x901f3d, 0xbb3030, 0xdc473c, 0xec6a45, 0xfb9b41, 0xf0c04c, 0xf4d66e, 0xfffb76,
   0xccf17a, 0x97d948, 0x6fba3b, 0x229443, 0x1d7e45, 0x116548, 0x0c4f3f, 0x0a3639,
   0x251746, 0x48246d, 0x69189c, 0x9f20c0, 0xe527d2, 0xff51cf, 0xff7ada, 0xff9edb,

   //Rewild 64
   0x172038, 0x253a5e, 0x3c5e8b, 0x4f8fba, 0x73bed3, 0xa4dddb, 0x193024, 0x245938,
   0x2b8435, 0x62ac4c, 0xa2dc6e, 0xc5e49b, 0x19332d, 0x25562e, 0x468232, 0x75a743,
   0xa8ca58, 0xd0da91, 0x5f6d43, 0x97933a, 0xa9b74c, 0xcfd467, 0xd5dc97, 0xd6dea6,
   0x382a28, 0x43322f, 0x564238, 0x715a42, 0x867150, 0xb1a282, 0x4d2b32, 0x7a4841,
   0xad7757, 0xc09473, 0xd7b594, 0xe7d5b3, 0x341c27, 0x602c2c, 0x884b2b, 0xbe772b,
   0xde9e41, 0xe8c170, 0x241527, 0x411d31, 0x752438, 0xa53030, 0xcf573c, 0xda863e,
   0x1e1d39, 0x402751, 0x7a367b, 0xa23e8c, 0xc65197, 0xdf84a5, 0x090a14, 0x10141f,
   0x151d28, 0x202e37, 0x394a50, 0x577277, 0x819796, 0xa8b5b2, 0xc7cfcc, 0xebede9

};*/

#endif