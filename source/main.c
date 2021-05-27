#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCREENWIDTH 320
#define SCREENHEIGHT 240

#define PAGEWIDTH 1024
#define PAGEHEIGHT 1024
#define PAGEFORMAT GPU_RGBA5551
#define PAGE_EDGEBUF 8
#define PAGECOUNT 2

#define CPAD_DEADZONE 40
#define CPAD_THEORETICALMAX 160
#define CPAD_PAGECONST 1
#define CPAD_PAGEMULT 0.02f
#define CPAD_PAGECURVE 3.2f

//#define DEBUG_COORD
#define DEBUG_RUNTESTS

// TODO: Figure out these weirdness things:
// - Can't draw on the first 8 pixels along the edge of a target, system crashes
// - Fill color works perfectly fine using line/rect calls, but ClearTarget
//   MUST have the proper 16 bit format....
// - ClearTarget with a transparent color seems to make the color stick using
//   DrawLine unless a DrawRect (or perhaps other) call is performed.

//Generic page difference using cpad values, return the new page position given 
//the existing position and the circlepad input
float calc_pagepos(s16 d, float existing_pos, u32 max_pos)
{
   u16 cpadmag = abs(d);

   if(cpadmag > CPAD_DEADZONE)
   {
      return C2D_Clamp(existing_pos + (d < 0 ? -1 : 1) * 
            (CPAD_PAGECONST + pow(cpadmag * CPAD_PAGEMULT, CPAD_PAGECURVE)), 
            0, max_pos);
   }
   else
   {
      return existing_pos;
   }
}

float calc_pagepos_x(circlePosition pos, float existing_pos) {
   return calc_pagepos(pos.dx, existing_pos, PAGEWIDTH - SCREENWIDTH);
}
float calc_pagepos_y(circlePosition pos, float existing_pos) {
   return calc_pagepos(-pos.dy, existing_pos, PAGEHEIGHT - SCREENHEIGHT);
}



struct SimpleLine {
   u8 layer;
   u16 color;
   u8 width;
   u16 x1;
   u16 y1;
   u16 x2;
   u16 y2;
   bool rectangle;
};




struct PageData {
   Tex3DS_SubTexture subtex;     //Simple structures
   C3D_Tex texture;
   C2D_Image image;
   C3D_RenderTarget * target;    //Actual data?
};

void create_page(struct PageData * result, Tex3DS_SubTexture subtex)
{
   result->subtex = subtex;
   C3D_TexInitVRAM(&(result->texture), subtex.width, subtex.height, PAGEFORMAT);
   result->target = C3D_RenderTargetCreateFromTex(&(result->texture), GPU_TEXFACE_2D, 0, -1);
   result->image.tex = &(result->texture);
   result->image.subtex = &(result->subtex);
}

void delete_page(struct PageData page)
{
   C3D_RenderTargetDelete(page.target);
   C3D_TexDelete(&page.texture);
}



u32 full_to_half(u32 val)
{
   //Format: 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   //Half  : 0b GGBBBBBA RRRRRGGG 00000000 00000000
   u8 green = (val & 0x0000FF00) >> 11; //crush last three bits

   return 
      (
         (val & 0xFF000000 ? 0x0100 : 0) |   //Alpha is lowest bit on upper byte
         (val & 0x000000F8) |                //Red is slightly nice because it's already in the right position
         ((green & 0x1c) >> 2) | ((green & 0x03) << 14) | //Green is split between bytes
         ((val & 0x00F80000) >> 10)          //Blue is just shifted and crushed
      ) << 16; //first 2 bytes are trash
}

u32 half_to_full(u16 val)
{
   //Half : 0b                   GGBBBBBA RRRRRGGG
   //Full : 0b AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
   return
      (
         (val & 0x0100 ? 0xFF000000 : 0)  |  //Alpha always 255 or 0
         ((val & 0x3E00) << 10) |            //Blue just shift a lot
         ((val & 0x0007) << 13) | ((val & 0xc000) >> 3) |  //Green is split
         (val & 0x00f8)                      //Red is easy, already in the right place
      );
}



void run_tests()
{
   printf("Running tests; only errors will be displayed\n");
   if(test_transparenthalftofull()) return; 
   printf("\nAll tests passed\n");
}

int test_transparenthalftofull()
{
   //It's such a small space, literally just run the gamut of 16 bit colors
   for(u32 col = 0; col <= 0xFFFF; col++)
   {
      u32 full = half_to_full(col);
      u32 half = full_to_half(full);
      if((col & 0xFFF) == 0xFFF) printf(".");
      if((col << 16) != half)
      {
         printf("ERR: Expected %08x, got %08x, full: %08x\n", col << 16, half, full);
         return 1;
      }
   }

   return 0;
}


int main(int argc, char** argv)
{
   gfxInitDefault();
   C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
   C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
   C2D_Prepare();

   consoleInit(GFX_TOP, NULL);
   C3D_RenderTarget* screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

   //weird byte order? 16 bits of color are at top
   const u32 initial_color = full_to_half(C2D_Color32(128,128,128,255));
   //const u32 bg_color = C2D_Color32(255,255,255,255);
   const u32 bg_color = C2D_Color32f(0.0,0.0,0.00,1.0);

   const u32 color_a = C2D_Color32f(1,0,0,1.0f);
   const u32 color_b = C2D_Color32f(1,1,0,1.0f);
   const u32 color_x = C2D_Color32f(0,0,1,1.0f);
   const u32 color_y = C2D_Color32f(0,1,0,1.0f);

   const u32* color_selected = &color_a;

   const Tex3DS_SubTexture subtex = {
      PAGEWIDTH, PAGEHEIGHT,
      0.0f, 1.0f, 1.0f, 0.0f
   };

   struct PageData pages[PAGECOUNT]; // frontpg, backpg; 

   for(int i = 0; i < PAGECOUNT; i++)
      create_page(pages + i, subtex);

   touchPosition current_touch;
   touchPosition last_touch = current_touch; //Why? compiler warning shush
   touchPosition start_touch;
   bool touching = false;
   bool page_initialized = false;
   u32 current_frame = 0;
   u32 start_frame = 0;
   float page_pos_y = 0;
   float page_pos_x = 0;
   u16 width = 2;
   u16 current_page = PAGECOUNT - 1; //Always go on the top page

   printf("Press ABXY to change color\n");
   printf("Press SELECT to change layers\n");
   printf("C-pad to scroll up/down\n");
   printf("\nPress START to quit.\n");
   printf("STARTING COLOR: %08x\n", initial_color);

#ifdef DEBUG_RUNTESTS
   run_tests();
#endif

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
      
      if(kDown & KEY_SELECT)
      {
         current_page = (current_page + 1) % PAGECOUNT;
         printf("Changing to layer %d\n", current_page);
      }

      hidTouchRead(&current_touch);

      if(kDown & KEY_TOUCH) {
         start_frame = current_frame;
         start_touch = current_touch;
         //Just throw away last touch from last time, we don't care
         last_touch = current_touch;
      }
      if(kUp & KEY_TOUCH) {
         //Nothing yet
      }

      touching = (kHeld & KEY_TOUCH) > 0;

      circlePosition pos;
		hidCircleRead(&pos);
      page_pos_x = calc_pagepos_x(pos, page_pos_x);
      page_pos_y = calc_pagepos_y(pos, page_pos_y);

      // Render the scene
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

      if(!page_initialized)
      {
         for(int i = 0; i < PAGECOUNT; i++)
            C2D_TargetClear(pages[i].target, initial_color); 
         page_initialized = true;
      }

      if(touching)
      {
         C2D_SceneBegin(pages[current_page].target);

         u16 width_ofs = width / 2;
         float old_x = C2D_Clamp(last_touch.px + page_pos_x, PAGE_EDGEBUF + width_ofs, PAGEWIDTH - 1);
         float old_y = C2D_Clamp(last_touch.py + page_pos_y, PAGE_EDGEBUF + width_ofs, PAGEHEIGHT - 1);
         float real_x = C2D_Clamp(current_touch.px + page_pos_x, PAGE_EDGEBUF + width_ofs, PAGEWIDTH - 1);
         float real_y = C2D_Clamp(current_touch.py + page_pos_y, PAGE_EDGEBUF + width_ofs, PAGEHEIGHT - 1);

#ifdef DEBUGCOORD
         printf("%d,%d: %.1f,%.1f  %.1f,%.1f\n", 
               current_touch.px, current_touch.py, 
               page_pos_x, page_pos_y, real_x, real_y);
#endif

         //Calling JUST C2D_DrawLine (like I want) produces no output. Calling
         //C2DDrawRectSolid beforehand makes it all work.
         //if(start_frame == current_frame)
         //{
         //C2D_DrawRectSolid(0,0,0.5f,1,1,*color_selected);
               //real_x - 1, real_y - 1, 0.5f, 2, 2, *color_selected);
         C2D_DrawRectSolid(real_x - width_ofs, real_y - width_ofs, 0.5f, width, width, *color_selected);
         //}

         C2D_DrawLine(old_x, old_y, *color_selected, real_x, real_y, *color_selected, 
               width, 0.5f);
      }

      C2D_TargetClear(screen, bg_color);
      C2D_SceneBegin(screen);
      for(int i = 0; i < PAGECOUNT; i++)
         C2D_DrawImageAt(pages[i].image, -page_pos_x, -page_pos_y, 0.5f, NULL, 1.0f, 1.0f);
      C3D_FrameEnd(0);

      last_touch = current_touch;
      current_frame++;
   }

END:
   for(int i = 0; i < PAGECOUNT; i++)
      delete_page(pages[i]);

   C2D_Fini();
   C3D_Fini();
   gfxExit();
   return 0;
}
