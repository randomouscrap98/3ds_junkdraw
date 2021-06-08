
// -- Some SUPER IMPORTANT constants

#define VERSION "0.2.1"
#define FOLDER_BASE "/3ds/junkdraw/"
#define SAVE_BASE FOLDER_BASE"saves/"
#define SCREENSHOTS_BASE FOLDER_BASE"screenshots/"

#define SCREENWIDTH 320
#define SCREENHEIGHT 240

#define LAYER_WIDTH 1024
#define LAYER_HEIGHT 1024
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

//Weird stuff that should maybe be refactored
#define MY_C2DOBJLIMIT 8192
#define MY_C2DOBJLIMITSAFETY MY_C2DOBJLIMIT - 100
#define PSX1BLEN 30

typedef u16 page_num;
typedef u8 layer_num;
//typedef u16 stroke_num;


// The base palette definition

u32 base_palette[] = { 
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
   0xcf657f, 0xed8099, 0x831c5d, 0xc32454, 0xf04f78, 0xf68181, 0xfca790, 0xfdcbb0

};
