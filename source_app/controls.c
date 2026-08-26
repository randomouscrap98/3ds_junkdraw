#include "controls.h"
#include "3ds/services/hid.h"
#include "littlemenu.h"

void control_setup_defaults() {
  hidSetRepeatParameters(BREPEAT_DELAY, BREPEAT_INTERVAL);
}

control_inputs control_get_inputs() {
  hidScanInput();
  control_inputs result = {
    .kdown = hidKeysDown(),
    .kup = hidKeysUp(),
    .krepeat = hidKeysDownRepeat(),
    .kheld = hidKeysHeld(),
  };
  hidTouchRead(&result.tpos);
  hidCircleRead(&result.cpos);
  return result;
}

// Return a constant representing the action the user took 
control_action control_get_action(const control_config * config, control_inputs * input) {
  control_action result = {
    .action = 0,
    .menuaction.action = TUIMENU_ACTION_NONE,
    .menuaction.offset = 0,
  };
  u32 kDown = input->kdown;
  u32 kUp = input->kup;
  u32 kHeld = input->kheld;
  u32 kRepeat = input->krepeat;
  // Move menu checks outside, even if they're technically the same
  if(kRepeat & KEY_DUP) {
    result.menuaction.action = TUIMENU_ACTION_MOVE;
    result.menuaction.offset = -1;
  }
  if(kRepeat & KEY_DDOWN) {
    result.menuaction.action = TUIMENU_ACTION_MOVE;
    result.menuaction.offset = 1;
  }
  if(kRepeat & KEY_DRIGHT) {
    result.menuaction.action = TUIMENU_ACTION_VALUE;
    result.menuaction.offset = 1;
  }
  if(kRepeat & KEY_DLEFT) {
    result.menuaction.action = TUIMENU_ACTION_VALUE;
    result.menuaction.offset = -1;
  }
  if (kDown & KEY_START) {
    result.menuaction.action = TUIMENU_ACTION_FULLSTOP;
  }
  if (kDown & KEY_B) {
    result.menuaction.action = TUIMENU_ACTION_CANCEL;
  }
  if (kDown & KEY_A) {
    result.menuaction.action = TUIMENU_ACTION_ACCEPT;
  }
  if(result.menuaction.action & (TUIMENU_ACTION_MOVE | TUIMENU_ACTION_VALUE)) {
    if(kHeld & (KEY_R | KEY_ZR)) {
      result.menuaction.offset *= 10;
    }
  }
  // These change with control schemes
  if(config->scheme == 0) {
    if (kDown & KEY_A) {
      if (kHeld & (KEY_R | KEY_ZR)) {
        result.action |= CTRL_REDO;
      } else {
        result.action |= CTRL_PENCIL;
      }
    }
    if (kDown & KEY_B) {
      if (kHeld & (KEY_R | KEY_ZR)) {
        result.action |= CTRL_UNDO;
      } else {
        result.action |= CTRL_ERASER;
      }
    }
    if (kDown & KEY_Y) {
      result.action |= CTRL_SLOWPEN;
    }
  } else if(config->scheme == 1) {
    if(kDown & KEY_X) { 
      result.action |= CTRL_UNDO;
    }
    if(kDown & KEY_Y) { 
      result.action |= CTRL_REDO;
    }
    if(kDown & KEY_B) { 
      result.action |= CTRL_PREVCOLOR;
    }
    if(kDown & KEY_A) {
      switch(config->tool) {
      case CTRL_TOOL_PENCIL:
        result.action |= CTRL_ERASER;
        break;
      case CTRL_TOOL_ERASER:
        result.action |= CTRL_SLOWPEN;
        break;
      case CTRL_TOOL_SLOW:
        result.action |= CTRL_PENCIL;
        break;
      }
    }
  }
  // These don't change with control schemes (yet)
  if (kDown & (KEY_L | KEY_ZL) && !(kHeld & (KEY_R | KEY_ZR))) {
    result.action |= CTRL_PALETTE;
  }
  if (kRepeat & KEY_DUP) {
    if (kHeld & (KEY_R | KEY_ZR)) {
      if (kHeld & KEY_START) {
        result.action |= CTRL_NEXTREF;
      } else {
        result.action |= CTRL_PAGEUP;
      }
    } else {
      result.action |= CTRL_ZOOMIN;
    }
  }
  if (kRepeat & KEY_DDOWN) {
    if (kHeld & (KEY_R | KEY_ZR)) {
      if (kHeld & KEY_START) {
        result.action |= CTRL_PREVREF;
      } else {
        result.action |= CTRL_PAGEDOWN;
      }
    } else {
      result.action |= CTRL_ZOOMOUT;
    }
  }
  if (kRepeat & KEY_DRIGHT) {
    result.action |= CTRL_WIDTHUP;
  }
  if (kRepeat & KEY_DLEFT) {
    result.action |= CTRL_WIDTHDOWN;
  }
  if (kHeld & (KEY_L | KEY_ZL) && kDown & (KEY_R | KEY_ZR)) {
    result.action |= CTRL_NEXTPALETTE;
  }
  if (kDown & KEY_SELECT) {
    result.action |= CTRL_LAYER;
  }
  if (kDown & KEY_START) {
    if(!(kHeld & (KEY_R | KEY_ZR))) {
      result.action |= CTRL_MENU;
    }
  }
  return result;
}
