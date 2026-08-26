#ifndef __HEADER_JD_CONTROLS__
#define __HEADER_JD_CONTROLS__

#include <3ds.h>
#include "littlemenu.h"

// Some suggested values
#define BREPEAT_DELAY 20
#define BREPEAT_INTERVAL 7

#define CTRL_UNDO     (1 << 1)
#define CTRL_REDO     (1 << 2)
#define CTRL_MENU     (1 << 3)
#define CTRL_PALETTE  (1 << 4)
#define CTRL_ZOOMIN   (1 << 5)
#define CTRL_ZOOMOUT  (1 << 6)
#define CTRL_PAGEUP   (1 << 7)
#define CTRL_PAGEDOWN (1 << 8)
#define CTRL_WIDTHUP  (1 << 9)
#define CTRL_WIDTHDOWN (1 << 10)
// #define CTRL_SETTOOL  (1 << 11)     // Just an indicator: we actually update the tool in config
#define CTRL_PENCIL   (1 << 11)
#define CTRL_ERASER   (1 << 12)
#define CTRL_SLOWPEN  (1 << 13)
#define CTRL_NEXTPALETTE (1 << 14)
#define CTRL_LAYER    (1 << 15)
#define CTRL_PREVCOLOR (1 << 16)
#define CTRL_NEXTREF  (1 << 17)
#define CTRL_PREVREF  (1 << 18)

// These are the tools the CONTROLS understand and will swap between, since
// tools are mapped to certain controls. Not necessarily all the controls or their
// final ids/order/etc.
#define CTRL_TOOL_PENCIL 0
#define CTRL_TOOL_ERASER 1
#define CTRL_TOOL_SLOW 2

typedef struct {
  u32 kdown;
  u32 kup;
  u32 krepeat;
  u32 kheld;
  circlePosition cpos;
  touchPosition tpos;
} control_inputs;

typedef struct {
  u8 scheme;
  //u8 numtools;
  u8 tool;
} control_config;

typedef struct {
  tui_menu_action menuaction;
  u32 action;
} control_action;

void control_setup_defaults();
control_inputs control_get_inputs();
control_action control_get_action(const control_config * config, control_inputs * input);

#endif
