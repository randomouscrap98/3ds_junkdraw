#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCREENWIDTH 320
#define SCREENHEIGHT 240
#define PAGEWIDTH 512
#define PAGEHEIGHT 512
#define CPAD_DEADZONE 40
#define CPAD_THEORETICALMAX 160
#define CPAD_PAGECONST 1
#define CPAD_PAGEMULT 0.02f
#define CPAD_PAGECURVE 3.2f

int main(int argc, char** argv)
{
   gfxInitDefault();
   C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
   C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
   C2D_Prepare();

   consoleInit(GFX_TOP, NULL);
   C3D_RenderTarget* screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

   const Tex3DS_SubTexture subtex = {
      PAGEWIDTH, PAGEHEIGHT,
      0.0f, 1.0f, 1.0f, 0.0f
   };
   C3D_Tex tex;
   C3D_TexInitVRAM(&tex, subtex.width, subtex.height, GPU_RGB8);
   C3D_RenderTarget* target = C3D_RenderTargetCreateFromTex(&tex, GPU_TEXFACE_2D, 0, -1);
   C2D_Image img = {&tex, &subtex};

   const u32 clear_color = C2D_Color32(255,255,255,255);

   const u32 color_a = C2D_Color32f(1,0,0,1.0f);
   const u32 color_b = C2D_Color32f(1,1,0,1.0f);
   const u32 color_x = C2D_Color32f(0,0,1,1.0f);
   const u32 color_y = C2D_Color32f(0,1,0,1.0f);

   const u32* color_selected = &color_a;

   touchPosition start_touch;
   touchPosition last_touch;
   touchPosition current_touch;
   touchPosition end_touch;
   bool touching = false;
   u32 current_frame = 0;
   u32 start_frame = 0;
   u16 cpadmag = 0;
   float page_pos = 0;
   int sign = 0;

   printf("Offscreen rendertarget test\n");
   printf("Touch the bottom screen to place a colored square centered on the touch point\n");
   printf("Press ABXY to change the color of the square\n");

   printf("\nPress START to quit.\n");

   C2D_TargetClear(target, clear_color);

   while(aptMainLoop())
   {
      hidScanInput();
      u32 kDown = hidKeysDown();
      u32 kUp = hidKeysUp();
      u32 kHeld = hidKeysHeld();

      // Respond to user input
      if(kDown & KEY_START) break;

      if(kDown & KEY_A) color_selected = &color_a;
      else if(kDown & KEY_B) color_selected = &color_b;
      else if(kDown & KEY_X) color_selected = &color_x;
      else if(kDown & KEY_Y) color_selected = &color_y;

      hidTouchRead(&current_touch);

      if(kDown & KEY_TOUCH) {
         start_touch = current_touch;
         start_frame = current_frame;
         //Just throw away last touch from last time, we don't care
         last_touch = current_touch;
      }
      if(kUp & KEY_TOUCH) {
         end_touch = current_touch;
      }

      touching = (kHeld & KEY_TOUCH) > 0;

      circlePosition pos;

		//Read the CirclePad position
		hidCircleRead(&pos);
      cpadmag = abs(pos.dy);

      if(cpadmag > CPAD_DEADZONE)
      {
         page_pos = C2D_Clamp(page_pos + (pos.dy < 0 ? 1 : -1) * 
               (CPAD_PAGECONST + pow(cpadmag * CPAD_PAGEMULT, CPAD_PAGECURVE)), 
               0, PAGEHEIGHT - SCREENHEIGHT);
      }

      // Render the scene
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      C2D_TargetClear(screen, clear_color);

      if(touching)
      {
         C2D_SceneBegin(target);

         //C2D_DrawRectSolid(current_touch.px - 2, current_touch.py - 2, 0.5f, 4, 4, *color_selected);
         C2D_DrawLine(last_touch.px, last_touch.py + page_pos, *color_selected,
               current_touch.px, current_touch.py + page_pos, *color_selected, 
               2, 0.5f);
      }

      C2D_SceneBegin(screen);
      C2D_DrawImageAt(img, 0, -page_pos, 0.5f, NULL, 1.0f, 1.0f);
      C3D_FrameEnd(0);

      last_touch = current_touch;
      current_frame++;
   }

   C3D_RenderTargetDelete(target);
   C3D_TexDelete(&tex);

   C2D_Fini();
   C3D_Fini();
   gfxExit();
   return 0;
}
