#include "utils.h"

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#define BREPEAT_DELAY 20
#define BREPEAT_INTERVAL 7
#define MAX_FILENAME 64

#define SCROLL_WIDTH 3
#define SCREEN_COLOR C2D_Color32(90, 90, 90, 255)
#define SCROLL_BG C2D_Color32f(0.8, 0.8, 0.8, 1)
#define SCROLL_BAR C2D_Color32f(0.5, 0.5, 0.5, 1)

bool isn3ds() {
  bool isn3ds = false;
  Result res = APT_CheckNew3DS(&isn3ds);

  if(R_SUCCEEDED(res)) {
    return isn3ds;
  } 
  return false;
}


int main() {
  gfxInitDefault();
  hidSetRepeatParameters(BREPEAT_DELAY, BREPEAT_INTERVAL);

  // Enable the higher clock speed on New 3DS
  osSetSpeedupEnable(true);

  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  //PrintConsole * console_ptr = 
  consoleInit(GFX_TOP, NULL);
  C3D_RenderTarget *screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  if(isn3ds()) {
    LOGDBG("New 3ds detected");
  } 


  char save_filename[MAX_FILENAME];

  LOGDBG("STARTING MAIN LOOP");

  while (aptMainLoop()) {
    hidScanInput();

    u32 kDown = hidKeysDown();
    u32 kUp = hidKeysUp();
    u32 kRepeat = hidKeysDownRepeat();
    u32 kHeld = hidKeysHeld();
    circlePosition pos;
    touchPosition current_touch;
    hidTouchRead(&current_touch);
    hidCircleRead(&pos);

    // =======================================
    // Render the scene
    // =======================================
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    // -- LAYER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE,
                   GPU_ZERO);

    C2D_Flush();

    // -- OTHER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA);

    C2D_TargetClear(screen, SCREEN_COLOR);
    C2D_SceneBegin(screen);

    // draw_layers(&layer_window, &sys);
    // draw_scrollbars(&sys.screen_state);
    // draw_colorpicker(&sys.colors, !sstate.palette_active);

    C3D_FrameEnd(0);

  }
ENDMAINLOOP:;

  C3D_RenderTargetDelete(screen);

  C2D_Fini();
  C3D_Fini();
  // exitRomfs();
  gfxExit();
  return 0;
}
