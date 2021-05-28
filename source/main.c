#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "constants.h"

//#define DEBUG_COORD
//#define DEBUG_RUNTESTS

// TODO: Figure out these weirdness things:
// - Can't draw on the first 8 pixels along the edge of a target, system crashes
// - Fill color works perfectly fine using line/rect calls, but ClearTarget
//   MUST have the proper 16 bit format....
// - ClearTarget with a transparent color seems to make the color stick using
//   DrawLine unless a DrawRect (or perhaps other) call is performed.


// -- SIMPLE UTILS --

//Take an rgb WITHOUT alpha and convert to 3ds full color (with alpha)
u32 rgb_to_full(u32 rgb)
{
   return 0xFF000000 | ((rgb >> 16) & 0x0000FF) | (rgb & 0x00FF00) | ((rgb << 16) & 0xFF0000);
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

void convert_palette(u32 * original, u16 * destination, u16 size)
{
   for(int i = 0; i < size; i++)
      destination[i] = full_to_truehalf(rgb_to_full(original[i]));
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
   mod->ofs_x = calc_pagepos(pos.dx, mod->ofs_x, PAGEWIDTH * mod->zoom - SCREENWIDTH);
   mod->ofs_y = calc_pagepos(-pos.dy, mod->ofs_y, PAGEHEIGHT * mod->zoom - SCREENHEIGHT);
}



// -- Stroke/line utilities --

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

struct SimpleLine * add_stroke(struct LinePackage * pending, 
      const touchPosition * pos, const struct ScreenModifier * mod)
{
   //This is for a stroke, do different things if we have different tools!
   struct SimpleLine * line = pending->lines + pending->line_count;

   line->x2 = pos->px / mod->zoom + mod->ofs_x / mod->zoom;
   line->y2 = pos->py / mod->zoom + mod->ofs_y / mod->zoom;

   if(pending->line_count == 0)
   {
      line->x1 = line->x2;
      line->y1 = line->y2;
   }
   else
   {
      line->x1 = pending->lines[pending->line_count - 1].x2;
      line->y1 = pending->lines[pending->line_count - 1].y2;
   }

   return line;
}



// -- DRAWING UTILS --

void draw_centeredrect(float x, float y, u16 width, u32 color)
{
   float ofs = width / 2.0;
   x = round(x - ofs);
   y = round(y - ofs);
   if(x < PAGE_EDGEBUF || y < PAGE_EDGEBUF) return;
   C2D_DrawRectSolid(x, y, 0.5f, width, width, color);
}

//Draw a line using a custom line drawing system (required like this because of
//javascript's general inability to draw non anti-aliased lines)
void custom_drawline(const struct SimpleLine * line, u16 width, u32 color)
{
   float xdiff = line->x2 - line->x1;
   float ydiff = line->y2 - line->y1;
   float dist = sqrt(xdiff * xdiff + ydiff * ydiff);
   float ang = atan(ydiff/(xdiff?xdiff:0.0001))+(xdiff<0?M_PI:0);
   float xang = cos(ang);
   float yang = sin(ang);

   for(float i = 0; i <= dist; i+=0.5)
      draw_centeredrect(line->x1+xang*i, line->y1+yang*i, width, color);
}

//Assumes you're already on the appropriate page you want and all that
void draw_lines(const struct LinePackage * linepack, const struct ScreenModifier * mod)
{
   //u16 width_ofs = linepack->width / 2;
   u32 color = half_to_full(linepack->color);
   struct SimpleLine * lines = linepack->lines;

   for(int i = 0; i < linepack->line_count; i++)
      custom_drawline(&lines[i], linepack->width, color);
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

   u16 sofs_x = (float)mod->ofs_x / PAGEWIDTH / mod->zoom * SCREENWIDTH;
   u16 sofs_y = (float)mod->ofs_y / PAGEHEIGHT / mod->zoom * SCREENHEIGHT;

   //bottom and right scrollbar bar
   C2D_DrawRectSolid(sofs_x, SCREENHEIGHT - SCROLL_WIDTH, 0.5f, 
         SCREENWIDTH * SCREENWIDTH / (float)PAGEWIDTH / mod->zoom, SCROLL_WIDTH, SCROLL_BAR);
   C2D_DrawRectSolid(SCREENWIDTH - SCROLL_WIDTH, sofs_y, 0.5f, 
         SCROLL_WIDTH, SCREENHEIGHT * SCREENHEIGHT / (float)PAGEHEIGHT / mod->zoom, SCROLL_BAR);
}

void draw_colorpicker(u16 * palette, u16 palette_size, u16 selected_index)
{
   C2D_DrawRectSolid(0, 0, 0.5f, SCREENWIDTH, SCREENHEIGHT, PALETTE_BG);

   u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
   for(u16 i = 0; i < palette_size; i++)
   {
      //TODO: an implicit 8 wide thing
      u16 x = i & 7;
      u16 y = i >> 3;

      if(i == selected_index)
      {
         C2D_DrawRectSolid(
            PALETTE_OFSX + x * shift, 
            PALETTE_OFSY + y * shift, 0.5f, 
            shift, shift, PALETTE_SELECTED_COLOR);
      }

      C2D_DrawRectSolid(
         PALETTE_OFSX + x * shift + PALETTE_SWATCHMARGIN, 
         PALETTE_OFSY + y * shift + PALETTE_SWATCHMARGIN, 0.5f, 
         PALETTE_SWATCHWIDTH, PALETTE_SWATCHWIDTH, half_to_full(palette[i]));
   }
}



// -- CONTROL UTILS --

struct ToolData {
   u8 width;
   u16 color;
   u8 style;
};

void fill_defaulttools(struct ToolData * tool_data, u16 default_color)
{
   tool_data[TOOL_PENCIL].width = 2;
   tool_data[TOOL_PENCIL].color = default_color;
   tool_data[TOOL_PENCIL].style = LINESTYLE_STROKE;
   tool_data[TOOL_ERASER].width = 4;
   tool_data[TOOL_ERASER].color = 0;
   tool_data[TOOL_ERASER].style = LINESTYLE_STROKE;
}

void update_paletteindex(const touchPosition * pos, u8 * index)
{
   u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
   u16 xind = (pos->px - PALETTE_OFSX) / shift;
   u16 yind = (pos->py - PALETTE_OFSY) / shift;
   u16 new_index = (yind << 3) + xind;
   if(new_index >= 0 && new_index < PALETTE_COLORS)
      *index = new_index;
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
   const u32 screen_color = SCREEN_COLOR;
   const u32 bg_color = CANVAS_BG_COLOR;
   const u32 layer_color = full_to_half(CANVAS_LAYER_COLOR);

   u16 palette [PALETTE_COLORS];
   convert_palette(base_palette, palette, PALETTE_COLORS);
   u8 palette_index = PALETTE_STARTINDEX;

   const Tex3DS_SubTexture subtex = {
      PAGEWIDTH, PAGEHEIGHT,
      0.0f, 1.0f, 1.0f, 0.0f
   };

   struct PageData pages[PAGECOUNT];

   for(int i = 0; i < PAGECOUNT; i++)
      create_page(pages + i, subtex);

   struct ScreenModifier screen_mod = {0,0,1}; 

   bool touching = false;
   bool page_initialized = false;
   bool palette_active = false;

   circlePosition pos;
   touchPosition current_touch;
   u32 current_frame = 0;
   u32 start_frame = 0;
   u32 end_frame = 0;
   s8 zoom_power = 0;
   s8 last_zoom_power = 0;

   u8 current_tool = 0;
   struct ToolData tool_data[TOOL_COUNT];
   fill_defaulttools(tool_data, palette[palette_index]);

   struct LinePackage pending;
   struct SimpleLine * pending_lines = malloc(MAX_STROKE_LINES * sizeof(struct SimpleLine));
   pending.lines = pending_lines; //Use the stack for my pending stroke
   pending.line_count = 0;
   pending.layer = PAGECOUNT - 1; //Always start on the top page

   printf("     L - change color\n");
   printf("SELECT - change layers\n");
   printf(" C-PAD - scroll canvas\n");
   printf(" START - quit.\n\n");

#ifdef DEBUG_RUNTESTS
   run_tests();
#endif

   while(aptMainLoop())
   {
      hidScanInput();
      u32 kDown = hidKeysDown();
      u32 kUp = hidKeysUp();
      u32 kHeld = hidKeysHeld();
      hidTouchRead(&current_touch);
		hidCircleRead(&pos);

      // Respond to user input
      if(kDown & KEY_START) break;
      if(kDown & KEY_L) palette_active = !palette_active;
      if(kDown & KEY_DUP && zoom_power < MAX_ZOOM) zoom_power++;
      if(kDown & KEY_DDOWN && zoom_power > MIN_ZOOM) zoom_power--;
      if(kDown & KEY_DRIGHT && tool_data[current_tool].width < MAX_WIDTH) tool_data[current_tool].width++;
      if(kDown & KEY_DLEFT && tool_data[current_tool].width > MIN_WIDTH) tool_data[current_tool].width--;
      if(kDown & KEY_SELECT) pending.layer = (pending.layer + 1) % PAGECOUNT;
      if(kDown & KEY_A) current_tool = TOOL_PENCIL;
      if(kDown & KEY_B) current_tool = TOOL_ERASER;
      if(kDown & KEY_TOUCH)
      {
         start_frame = current_frame;
         pending.color = tool_data[current_tool].color;
         pending.style = tool_data[current_tool].style;
         pending.width = tool_data[current_tool].width;
      }
      if(kUp & KEY_TOUCH) end_frame = current_frame;

      if(zoom_power != last_zoom_power) screen_mod.zoom = pow(2, zoom_power);

      if(kDown & ~(KEY_TOUCH) || !current_frame)
      {
         printf("\x1b[30;1HW:%02d L:%d Z:%03.2f T:%d C:%#06x",
               tool_data[current_tool].width, 
               pending.layer,
               screen_mod.zoom,
               current_tool,
               tool_data[current_tool].color
         );
      }

      touching = (kHeld & KEY_TOUCH) > 0;

      update_screenmodifier(&screen_mod, pos);

      // Render the scene
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

      if(!page_initialized)
      {
         for(int i = 0; i < PAGECOUNT; i++)
            C2D_TargetClear(pages[i].target, layer_color); 
         page_initialized = true;
      }

      if(touching && page_initialized)
      {
         if(palette_active)
         {
            update_paletteindex(&current_touch, &palette_index);
            tool_data[current_tool].color = palette[palette_index];
         }
         else
         {
            C2D_SceneBegin(pages[pending.layer].target);

            if(pending.line_count < MAX_STROKE_LINES)
            {
               //This is for a stroke, do different things if we have different tools!
               struct SimpleLine * line = add_stroke(&pending, &current_touch, &screen_mod);

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
      }

      //TODO: Here, you would NORMALLY save the line or whatever
      if(end_frame == current_frame)
      {
         pending.line_count = 0;
      }

      C2D_TargetClear(screen, screen_color);
      C2D_SceneBegin(screen);

      if(palette_active)
      {
         draw_colorpicker(palette, PALETTE_COLORS, palette_index);
      }
      else
      {
         C2D_DrawRectSolid(-screen_mod.ofs_x, -screen_mod.ofs_y, 0.5f,
               PAGEWIDTH * screen_mod.zoom, PAGEHEIGHT * screen_mod.zoom, bg_color); //The bg color
         for(int i = 0; i < PAGECOUNT; i++)
         {
            C2D_DrawImageAt(pages[i].image, -screen_mod.ofs_x, -screen_mod.ofs_y, 0.5f, 
                  NULL, screen_mod.zoom, screen_mod.zoom);
         }
         //The selected color thing
         C2D_DrawRectSolid(0, 0, 0.5f, PALETTE_MINISIZE, PALETTE_MINISIZE, PALETTE_BG);
         C2D_DrawRectSolid(PALETTE_MINIPADDING, PALETTE_MINIPADDING, 0.5f, 
               PALETTE_MINISIZE - PALETTE_MINIPADDING * 2, 
               PALETTE_MINISIZE - PALETTE_MINIPADDING * 2, 
               half_to_full(palette[palette_index])); 
         draw_scrollbars(&screen_mod);
      }

      C3D_FrameEnd(0);

      //last_touch = current_touch;
      last_zoom_power = zoom_power;
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
