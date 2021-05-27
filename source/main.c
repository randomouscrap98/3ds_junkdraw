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

#define MAX_STROKE_LINES 5000

#define TOOL_PENCIL 0
#define TOOL_ERASER 0

#define LINESTYLE_STROKE 0

#define SCROLL_WIDTH 3
#define SCROLL_BG C2D_Color32f(0.8,0.8,0.8,1)
#define SCROLL_BAR C2D_Color32f(0.5,0.5,0.5,1)

//#define DEBUG_COORD
#define DEBUG_RUNTESTS

// TODO: Figure out these weirdness things:
// - Can't draw on the first 8 pixels along the edge of a target, system crashes
// - Fill color works perfectly fine using line/rect calls, but ClearTarget
//   MUST have the proper 16 bit format....
// - ClearTarget with a transparent color seems to make the color stick using
//   DrawLine unless a DrawRect (or perhaps other) call is performed.


// -- SIMPLE UTILS --

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

u16 full_to_truehalf(u32 val)
{
   return full_to_half(val) >> 16;
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



// -- SCREEN UTILS? --

struct ScreenModifier
{
   float ofs_x;
   float ofs_y;
   float zoom;
};

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

void update_screenmodifier(struct ScreenModifier * mod, circlePosition pos)
{
   mod->ofs_x = calc_pagepos(pos.dx, mod->ofs_x, PAGEWIDTH - SCREENWIDTH);
   mod->ofs_y = calc_pagepos(-pos.dy, mod->ofs_y, PAGEHEIGHT - SCREENHEIGHT);
}



// -- DRAWING UTILS --

//A line doesn't contain all the data needed to draw itself. That would be a
//line package
struct SimpleLine {
   u16 x1, y1, x2, y2;
};

struct LinePackage {
   u8 style;
   u16 color;
   u8 layer;
   u8 width;
   struct SimpleLine * lines;
   u16 line_count;
};

//Assumes you're already on the appropriate page you want and all that
void draw_lines(const struct LinePackage * linepack, const struct ScreenModifier * mod)
{
   u16 width_ofs = linepack->width / 2;
   u32 color = half_to_full(linepack->color);
   struct SimpleLine * lines = linepack->lines;

   for(int i = 0; i < linepack->line_count; i++)
   {
      float old_x = C2D_Clamp(lines[i].x1 + mod->ofs_x, PAGE_EDGEBUF + width_ofs, PAGEWIDTH - 1);
      float old_y = C2D_Clamp(lines[i].y1 + mod->ofs_y, PAGE_EDGEBUF + width_ofs, PAGEHEIGHT - 1);
      float real_x = C2D_Clamp(lines[i].x2 + mod->ofs_x, PAGE_EDGEBUF + width_ofs, PAGEWIDTH - 1);
      float real_y = C2D_Clamp(lines[i].y2 + mod->ofs_y, PAGE_EDGEBUF + width_ofs, PAGEHEIGHT - 1);
      //For some reason, I can't just draw a line, because the system won't draw
      //it if the target is cleared with transparency. But drawing a rect first
      //fixes that, so... guess that's just part of the style of the thing.
      C2D_DrawRectSolid(real_x - width_ofs, real_y - width_ofs, 0.5f, 
            linepack->width, linepack->width, color);
      C2D_DrawLine(old_x, old_y, color, real_x, real_y, color, linepack->width, 0.5f);
      //TODO: doesn't take into account the type yet! Some types (like
      //rectangle) draw something other than lines!
   }
}

void draw_scrollbars(const struct ScreenModifier * mod)
{
   //Need to draw an n-thickness scrollbar on the right and bottom. Assumes
   //standard page size for screen modifier.

   //Bottom and right scrollbar bg
   C2D_DrawRectSolid(0, SCREENHEIGHT - SCROLL_WIDTH, 0.5f, 
         SCREENWIDTH, SCROLL_WIDTH, SCROLL_BG);
   C2D_DrawRectSolid(SCREENWIDTH - SCROLL_WIDTH, 0, 0.5f, 
         SCROLL_WIDTH, SCREENHEIGHT, SCROLL_BG);

   u16 sofs_x = (float)mod->ofs_x / PAGEWIDTH * SCREENWIDTH;
   u16 sofs_y = (float)mod->ofs_y / PAGEHEIGHT * SCREENHEIGHT;

   //bottom and right scrollbar bar
   C2D_DrawRectSolid(sofs_x, SCREENHEIGHT - SCROLL_WIDTH, 0.5f, 
         SCREENWIDTH * SCREENWIDTH / (float)PAGEWIDTH / mod->zoom, SCROLL_WIDTH, SCROLL_BAR);
   C2D_DrawRectSolid(SCREENWIDTH - SCROLL_WIDTH, sofs_y, 0.5f, 
         SCROLL_WIDTH, SCREENHEIGHT * SCREENHEIGHT / (float)PAGEHEIGHT / mod->zoom, SCROLL_BAR);
}



// -- LAYER UTILS --

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




// -- TESTS --

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
         printf("ERR: Expected %08lx, got %08lx, full: %08lx\n", col << 16, half, full);
         return 1;
      }
   }

   return 0;
}

void run_tests()
{
   printf("Running tests; only errors will be displayed\n");
   if(test_transparenthalftofull()) return; 
   printf("\nAll tests passed\n");
}




// -- MAIN, OFC --

int main(int argc, char** argv)
{
   gfxInitDefault();
   C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
   C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
   C2D_Prepare();

   consoleInit(GFX_TOP, NULL);
   C3D_RenderTarget* screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

   //weird byte order? 16 bits of color are at top
   const u32 layer_color = full_to_half(C2D_Color32(0,0,0,0)); //128,128,128,255));
   const u32 bg_color = C2D_Color32(255,255,255,255);
   //const u32 bg_color = C2D_Color32f(0.0,0.0,0.00,1.0);

   const u32 color_a = C2D_Color32f(1,0,0,1.0f);
   const u32 color_b = C2D_Color32f(1,1,0,1.0f);
   const u32 color_x = C2D_Color32f(0,0,1,1.0f);
   const u32 color_y = C2D_Color32f(0,1,0,1.0f);

   //const u32* color_selected = &color_a;

   const Tex3DS_SubTexture subtex = {
      PAGEWIDTH, PAGEHEIGHT,
      0.0f, 1.0f, 1.0f, 0.0f
   };

   struct PageData pages[PAGECOUNT];

   for(int i = 0; i < PAGECOUNT; i++)
      create_page(pages + i, subtex);

   struct ScreenModifier screen_mod = {0,0,1}; 

   touchPosition current_touch;
   touchPosition last_touch = current_touch; //Why? compiler warning shush
   //touchPosition start_touch;
   bool touching = false;
   bool page_initialized = false;
   u32 current_frame = 0;
   u32 start_frame = 0;
   u32 end_frame = 0;

   //u8 tool = 0;

   struct LinePackage pending;
   struct SimpleLine * pending_lines = malloc(MAX_STROKE_LINES * sizeof(struct SimpleLine));
   pending.lines = pending_lines; //Use the stack for my pending stroke
   pending.line_count = 0;
   pending.style = LINESTYLE_STROKE;
   pending.width = 2;
   pending.layer = PAGECOUNT - 1; //Always start on the top page
   pending.color = full_to_truehalf(color_a);

   printf("Press ABXY to change color\n");
   printf("Press SELECT to change layers\n");
   printf("C-pad to scroll up/down\n");
   printf("\nPress START to quit.\n");
   printf("STARTING COLOR: %08lx\n", layer_color);

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

      if(kDown & KEY_A) pending.color = full_to_truehalf(color_a);
      else if(kDown & KEY_B) pending.color = full_to_truehalf(color_b);
      else if(kDown & KEY_X) pending.color = full_to_truehalf(color_x);
      else if(kDown & KEY_Y) pending.color = full_to_truehalf(color_y);
      
      if(kDown & KEY_SELECT)
      {
         pending.layer = (pending.layer + 1) % PAGECOUNT;
         printf("Changing to layer %d\n", pending.layer);
      }

      hidTouchRead(&current_touch);

      if(kDown & KEY_TOUCH) {
         start_frame = current_frame;
         //start_touch = current_touch;
         //Just throw away last touch from last time, we don't care
         last_touch = current_touch;
      }
      if(kUp & KEY_TOUCH) {
         end_frame = current_frame;
      }

      touching = (page_initialized && (kHeld & KEY_TOUCH) > 0);

      circlePosition pos;
		hidCircleRead(&pos);
      update_screenmodifier(&screen_mod, pos);

      // Render the scene
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

      if(!page_initialized)
      {
         for(int i = 0; i < PAGECOUNT; i++)
            C2D_TargetClear(pages[i].target, layer_color); 
         page_initialized = true;
      }

      if(touching)
      {
         C2D_SceneBegin(pages[pending.layer].target);

         if(pending.line_count < MAX_STROKE_LINES)
         {
            //This is for a stroke, do different things if we have different tools!
            struct SimpleLine * line = pending.lines + pending.line_count;
            line->x1 = last_touch.px;
            line->y1 = last_touch.py;
            line->x2 = current_touch.px;
            line->y2 = current_touch.py;

            pending.lines = line; //Force the pending line to only show the end
            u16 oldcount = pending.line_count;
            pending.line_count = 1;

            //Draw ONLY the current line
            draw_lines(&pending, &screen_mod);
            //Reset pending lines to proper thing
            pending.lines = pending_lines;
            pending.line_count = oldcount + 1;
         }
      }

      //TODO: Here, you would NORMALLY save the line or whatever
      if(end_frame == current_frame)
      {
         pending.line_count = 0;
      }

      C2D_TargetClear(screen, bg_color);
      C2D_SceneBegin(screen);
      for(int i = 0; i < PAGECOUNT; i++)
      {
         C2D_DrawImageAt(pages[i].image, -screen_mod.ofs_x, -screen_mod.ofs_y, 0.5f, 
               NULL, screen_mod.zoom, screen_mod.zoom);
      }
      draw_scrollbars(&screen_mod);
      C3D_FrameEnd(0);

      last_touch = current_touch;
      current_frame++;
   }

   free(pending_lines);

   for(int i = 0; i < PAGECOUNT; i++)
      delete_page(pages[i]);

   C2D_Fini();
   C3D_Fini();
   gfxExit();
   return 0;
}
