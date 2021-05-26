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

//Return the new page position given the existing position and the circlepad input
float calc_pagepos(circlePosition pos, float existing_pos)
{
   u16 cpadmag = abs(pos.dy);

   if(cpadmag > CPAD_DEADZONE)
   {
      return C2D_Clamp(existing_pos + (pos.dy < 0 ? 1 : -1) * 
            (CPAD_PAGECONST + pow(cpadmag * CPAD_PAGEMULT, CPAD_PAGECURVE)), 
            0, PAGEHEIGHT - SCREENHEIGHT);
   }
   else
   {
      return existing_pos;
   }
}

struct PageData {
   Tex3DS_SubTexture subtex;
   C3D_Tex texture;
   C3D_RenderTarget * target;
   C2D_Image image; //Simple structure
};

//Pass by value? whatever
void create_page(struct PageData * result, Tex3DS_SubTexture subtex, u32 initial_color)
{
   result->subtex = subtex;
   C3D_TexInitVRAM(&(result->texture), subtex.width, subtex.height, GPU_RGB8);
   result->target = C3D_RenderTargetCreateFromTex(&(result->texture), GPU_TEXFACE_2D, 0, -1);
   result->image.tex = &(result->texture);
   result->image.subtex = &(result->subtex); //This could be a problem
   C2D_TargetClear(result->target, initial_color); //hopefully transparent
}

void clear_page(struct PageData page)
{
   C3D_RenderTargetDelete(page.target);
   C3D_TexDelete(&page.texture);
}

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

   const u32 initial_color = C2D_Color32(255,255,255,0);

   struct PageData frontpg, backpg; 
   create_page(&frontpg, subtex, initial_color); //&subtex);
   //struct PageData backpg = create_page(&subtex);

   const u32 bg_color = C2D_Color32(255,255,255,255);

   const u32 color_a = C2D_Color32f(1,0,0,1.0f);
   const u32 color_b = C2D_Color32f(1,1,0,1.0f);
   const u32 color_x = C2D_Color32f(0,0,1,1.0f);
   const u32 color_y = C2D_Color32f(0,1,0,1.0f);

   const u32* color_selected = &color_a;

   //touchPosition start_touch;
   touchPosition last_touch;
   touchPosition current_touch;
   //touchPosition end_touch;
   bool touching = false;
   u32 current_frame = 0;
   //u32 start_frame = 0;
   float page_pos = 0;

   printf("Offscreen rendertarget test\n");
   printf("Touch the bottom screen to place a colored square centered on the touch point\n");
   printf("Press ABXY to change the color of the square\n");

   printf("\nPress START to quit.\n");


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
         //start_touch = current_touch;
         //start_frame = current_frame;
         //Just throw away last touch from last time, we don't care
         last_touch = current_touch;
      }
      if(kUp & KEY_TOUCH) {
         //end_touch = current_touch;
      }

      touching = (kHeld & KEY_TOUCH) > 0;

      circlePosition pos;
		hidCircleRead(&pos);
      //page_pos = calc_pagepos(pos, page_pos);

      // Render the scene
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
      C2D_TargetClear(screen, bg_color);

      if(touching)
      {
         C2D_SceneBegin(frontpg.target);

         //C2D_DrawRectSolid(current_touch.px - 2, current_touch.py - 2, 0.5f, 4, 4, *color_selected);
         C2D_DrawLine(last_touch.px, last_touch.py + page_pos, *color_selected,
               current_touch.px, current_touch.py + page_pos, *color_selected, 
               2, 0.5f);
      }

      C2D_SceneBegin(screen);
      C2D_DrawImageAt(frontpg.image, 0, -page_pos, 0.5f, NULL, 1.0f, 1.0f);
      C3D_FrameEnd(0);

      last_touch = current_touch;
      current_frame++;
   }

   clear_page(frontpg);
   //clear_page(backpg);

   C2D_Fini();
   C3D_Fini();
   gfxExit();
   return 0;
}
