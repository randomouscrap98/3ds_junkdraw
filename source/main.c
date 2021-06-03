#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// MUST define all these debug things before importing libraries, 
// as THEY use  them. 

//#define DEBUG_COORD
#define DEBUG_DATAPRINT
#define DEBUG_PRINT
#define DEBUG_RUNTESTS
#define DEBUG_PRINT_SPECIAL "\x1b[18;1H\x1b[33m"

#include "myutils.h"
#include "dcv.h"
#include "constants.h"


// TODO: Figure out these weirdness things:
// - Can't draw on the first 8 pixels along the edge of a target, system crashes
// - Fill color works perfectly fine using line/rect calls, but ClearTarget
//   MUST have the proper 16 bit format....
// - ClearTarget with a transparent color seems to make the color stick using
//   DrawLine unless a DrawRect (or perhaps other) call is performed.
//    THIS IS FIXED IN A LATER REVISION
// - Can't fill with transparency to clear.... 


// -- SCREEN UTILS? --

//Represents a transformation of the screen
struct ScreenModifier
{
   float ofs_x;
   float ofs_y;
   float zoom;
};

//Generic page difference using cpad values, return the new page position given 
//the existing position and the circlepad input
float calc_pagepos(s16 d, float existing_pos)
{
   u16 cpadmag = abs(d);

   if(cpadmag > CPAD_DEADZONE)
   {
      return existing_pos + (d < 0 ? -1 : 1) * 
            (CPAD_PAGECONST + pow(cpadmag * CPAD_PAGEMULT, CPAD_PAGECURVE));
   }
   else
   {
      return existing_pos;
   }
}

//Easy way to set the screen offset (translation) safely for the given screen
//modifier. Clamps the values appropriately
void set_screenmodifier_ofs(struct ScreenModifier * mod, u16 ofs_x, u16 ofs_y)
{
   mod->ofs_x = C2D_Clamp(ofs_x, 0, PAGEWIDTH * mod->zoom - SCREENWIDTH);
   mod->ofs_y = C2D_Clamp(ofs_y, 0, PAGEHEIGHT * mod->zoom - SCREENHEIGHT);
}

//Easy way to set the screen zoom while preserving the center of the screen.
//So, the image should not appear to shift too much while zooming. The offsets
//WILL be modified after this function is completed!
void set_screenmodifier_zoom(struct ScreenModifier * mod, float zoom)
{
   float zoom_ratio = zoom / mod->zoom;
   u16 center_x = SCREENWIDTH >> 1;
   u16 center_y = SCREENHEIGHT >> 1;
   u16 new_ofsx = zoom_ratio * (mod->ofs_x + center_x) - center_x;
   u16 new_ofsy = zoom_ratio * (mod->ofs_y + center_y) - center_y;
   mod->zoom = zoom;
   set_screenmodifier_ofs(mod, new_ofsx, new_ofsy);
}

//Update screen translation based on cpad input
void update_screenmodifier(struct ScreenModifier * mod, circlePosition pos)
{
   set_screenmodifier_ofs(mod, calc_pagepos(pos.dx, mod->ofs_x), calc_pagepos(-pos.dy, mod->ofs_y));
}



// -- Stroke/line utilities --

//A line doesn't contain all the data needed to draw itself. That would be a
//line package
struct SimpleLine {
   u16 x1, y1, x2, y2;
};

// A basic representation of a collection of lines. Doesn't understand "tools"
// or any of that, it JUST has the information required to draw the lines.
struct LinePackage {
   u8 style;
   u8 layer;
   u16 color;
   u8 width;
   struct SimpleLine * lines;
   u16 line_count;
};

//Add another stroke to a line collection (that represents a stroke). Works for
//the first stroke too.
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

   //Added a line
   pending->line_count++;

   return line;
}

//A true macro, as in just dump code into the function later. Used ONLY for 
//convert_lines, hence "CVL"
#define CVL_LINECHECK if(container_size - (ptr - container) < DCV_VARIMAXSCAN) { \
   LOGDBG("ERROR: ran out of space in line conversion: original size: %ld\n",container_size); \
   return NULL; }

//Convert lines into data, but if it doesn't fit, return a null pointer.
//Returned pointer points to end + 1 in container
char * convert_lines_to_data(struct LinePackage * lines, char * container, u32 container_size)
{
   char * ptr = container;

   if(lines->line_count < 1)
   {
      LOGDBG("WARN: NO LINES TO CONVERT!\n");
      return NULL;
   }

   //This is a check that will need to be performed a lot
   CVL_LINECHECK

   //NOTE: the data is JUST the info for the lines, it's not "wrapped" into a
   //package or anything. So for instance, the length of the data is not
   //included at the start, as it may be stored in the final product

   //1 byte style/layer, 1 byte width, 3 bytes color
   //3 bits of line style, 1 bit (for now) of layers, 2 unused
   ptr = int_to_chars((lines->style & 0x7) | (lines->layer << 3),1,ptr); 
   //6 bits of line width (minus 1)
   ptr = int_to_chars(lines->width - 1,1,ptr); 
   //16 bits of color (2 unused)
   ptr = int_to_chars(lines->color,3,ptr); 

   CVL_LINECHECK

   //Now for strokes, we store the first point, then move along the rest of the
   //points doing an offset storage
   if(lines->style == LINESTYLE_STROKE)
   {
      //Dump first point, save point data for later
      u16 x = lines->lines[0].x1;
      u16 y = lines->lines[0].y1;
      ptr = int_to_chars(x, 2, ptr);
      ptr = int_to_chars(y, 2, ptr);

      //Now compute distances between this point and previous, store those as
      //variable width values. This can save a significant amount for most
      //types of drawing.
      for(u16 i = 0; i < lines->line_count; i++)
      {
         CVL_LINECHECK

         if(x == lines->lines[i].x2 && y == lines->lines[i].y2)
            continue; //Don't need to store stationary lines

         ptr = int_to_varwidth(signed_to_special(lines->lines[i].x2 - x), ptr);
         ptr = int_to_varwidth(signed_to_special(lines->lines[i].y2 - y), ptr);

         x = lines->lines[i].x2;
         y = lines->lines[i].y2;
      }
   }
   else
   {
      //We DON'T support this! 
      LOGDBG("ERR: L2D UNSUPPORTED STROKE: %d\n", lines->style);
      return NULL;
   }

   return ptr;
}

//A true macro, as in just dump code into the function later. Used ONLY for 
//convert_data, hence "CVD"
#define CVD_LINECHECK(x,msg) if((data_end - endptr) < x) { \
   LOGDBG("ERROR: Not enough data to parse line! %s\n",msg); \
   return NULL; }

//Package needs to have a 'lines' array already assigned with enough space to
//hold the largest stroke. Data needs to start precisely where you want it.
//Parsed data is all stored in the given package. A partial parse may result 
//in a partially modified package. Converts a single chunk of line data.
//Returns the next position in the data to start reading a chunk
char * convert_data_to_lines(struct LinePackage * package, char * data, char * data_end)
{
   char * endptr = data;
   package->line_count = 0;

   //If data is so short that it can't even parse the preamble, quit
   CVD_LINECHECK(5,"PREAMBLE")

   u32 temp = chars_to_int(endptr, 1);
   package->style = temp & 0x7;
   package->layer = (temp >> 3) & 0x1; 
   package->width = chars_to_int(endptr + 1, 1) + 1;
   package->color = chars_to_int(endptr + 2, 3);
   endptr += 5;

   if(package->style == LINESTYLE_STROKE)
   {
      CVD_LINECHECK(4,"STROKE FIRST POINT")

      //First point is regular simple 4 byte data point.
      u16 x = chars_to_int(endptr, 2);
      u16 y = chars_to_int(endptr + 2, 2);
      endptr += 4;

      u8 scanned = 0;

      //This could be VERY VERY UNSAFE!!
      while(endptr < data_end)
      {
         struct SimpleLine * line = package->lines + package->line_count;

         //Store current end as first point
         line->x1 = x; line->y1 = y;
         //Read next endpoint
         x = x + special_to_signed(varwidth_to_int(endptr, &scanned)); 
         endptr += scanned;
         y = y + special_to_signed(varwidth_to_int(endptr, &scanned)); 
         endptr += scanned;
         //The end of us is the next endpoint
         line->x2 = x; line->y2 = y;

         //We added another line
         package->line_count++;

         if(package->line_count > MAX_STROKE_LINES)
         {
            LOGDBG("ERR: got a stroke that's too long!");
            return NULL;
         }
      }

      //The special case where there's no additional strokes
      if(package->line_count == 0)
      {
         package->lines[0].x1 = package->lines[0].x2 = x; 
         package->lines[0].y1 = package->lines[0].y2 = y;
      }
   }
   else
   {
      //We DON'T support this! 
      LOGDBG("ERR: D2L UNSUPPORTED STROKE: %d\n", package->style);
      return NULL;
   }

   return endptr;
}



// -- DRAWING UTILS --


//Some (hopefully temporary) globals to overcome some unforeseen limits
#define MY_C2DOBJLIMIT 16384
#define MY_C2DOBJLIMITSAFETY MY_C2DOBJLIMIT - 100
u32 _drw_cmd_cnt = 0;

void MY_FLUSH()
{
   C2D_Flush();
   _drw_cmd_cnt = 0;
}

void MY_FLUSHCHECK()
{
   if(_drw_cmd_cnt > MY_C2DOBJLIMITSAFETY)
   {
      LOGDBG("FLUSHING %ld DRAW CMDS PREMATURELY\n", _drw_cmd_cnt);
      MY_FLUSH();
   }
}

void MY_SOLIDRECT(float x, float y, float depth, float width, float height, u32 color)
{
   C2D_DrawRectSolid(x, y, depth, width, height, color);
   _drw_cmd_cnt++;
   MY_FLUSHCHECK();
}

float cr_lx = -1;
float cr_ly = -1;

//Draw a rectangle centered and pixel aligned around the given point.
void draw_centeredrect(float x, float y, u16 width, u32 color)
{
   float ofs = width / 2.0;
   x = round(x - ofs);
   y = round(y - ofs);
   if(x < PAGE_EDGEBUF || y < PAGE_EDGEBUF || (cr_lx == x && cr_ly == y)) return;
   MY_SOLIDRECT(x, y, 0.5f, width, width, color);
   cr_lx = x; cr_ly = y;
}

//Draw a line using a custom line drawing system (required like this because of
//javascript's general inability to draw non anti-aliased lines, and I want the
//strokes saved by this program to be 100% accurately reproducible on javascript)
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

//Draw the collection of lines given, starting at the given line and ending
//before the other given line (first inclusive, last exclusive)
//Assumes you're already on the appropriate page you want and all that
void draw_lines(const struct LinePackage * linepack, u16 pack_start, u16 pack_end)
{
   u32 color = rgba16_to_rgba32c(linepack->color);

   if(pack_end > linepack->line_count)
      pack_end = linepack->line_count;

   for(u16 i = pack_start; i < pack_end; i++)
      custom_drawline(&linepack->lines[i], linepack->width, color);
}

void draw_all_lines(const struct LinePackage * linepack)
{
   draw_lines(linepack, 0, linepack->line_count);
}

//Draw the scrollbars on the sides of the screen for the given screen
//modification (translation AND zoom affect the scrollbars)
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

//Draw (JUST draw) the entire color picker area, which may include other
//stateful controls
void draw_colorpicker(u16 * palette, u16 palette_size, u16 selected_index)
{
   u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
   C2D_DrawRectSolid(0, 0, 0.5f, 
         8 * shift + (PALETTE_OFSX << 1) + PALETTE_SWATCHMARGIN, 
         8 * shift + (PALETTE_OFSY << 1) + PALETTE_SWATCHMARGIN, 
         PALETTE_BG);

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
         PALETTE_SWATCHWIDTH, PALETTE_SWATCHWIDTH, rgba16_to_rgba32c(palette[i]));
   }
}



// -- CONTROL UTILS --

//Saved data for a tool. Each tool may (if it so desires) have different
//colors, widths, etc.
struct ToolData {
   s8 width;
   u16 color;
   u8 style;
   bool color_settable;
   //u8 color_redirect;
};

//Fill tools with default values for the start of the program.
void fill_defaulttools(struct ToolData * tool_data, u16 default_color)
{
   tool_data[TOOL_PENCIL].width = 2;
   tool_data[TOOL_PENCIL].color = default_color;
   tool_data[TOOL_PENCIL].style = LINESTYLE_STROKE;
   tool_data[TOOL_PENCIL].color_settable = true;
   //tool_data[TOOL_PENCIL].color_redirect = TOOL_PENCIL;
   tool_data[TOOL_ERASER].width = 4;
   tool_data[TOOL_ERASER].color = 0;
   tool_data[TOOL_ERASER].style = LINESTYLE_STROKE;
   tool_data[TOOL_ERASER].color_settable = false;
}

//Given a touch position (presumably on the color palette), update the selected
//palette index. 
void update_paletteindex(const touchPosition * pos, u8 * index)
{
   u16 shift = PALETTE_SWATCHWIDTH + 2 * PALETTE_SWATCHMARGIN;
   u16 xind = (pos->px - PALETTE_OFSX) / shift;
   u16 yind = (pos->py - PALETTE_OFSY) / shift;
   u16 new_index = (yind << 3) + xind;
   if(new_index >= 0 && new_index < PALETTE_COLORS)
      *index = new_index;
}

void update_package_with_tool(struct LinePackage * pending, const struct ToolData * tool_data)
{
   pending->color = tool_data->color;
   pending->style = tool_data->style;
   pending->width = tool_data->width;
}



// -- LAYER UTILS --

//All the data associated with a single layer. TODO: still called "page"
struct LayerData {
   Tex3DS_SubTexture subtex;     //Simple structures
   C3D_Tex texture;
   C2D_Image image;
   C3D_RenderTarget * target;    //Actual data?
};

//Create a LAYER from an off-screen texture.
void create_layer(struct LayerData * result, Tex3DS_SubTexture subtex)
{
   result->subtex = subtex;
   C3D_TexInitVRAM(&(result->texture), subtex.width, subtex.height, PAGEFORMAT);
   result->target = C3D_RenderTargetCreateFromTex(&(result->texture), GPU_TEXFACE_2D, 0, -1);
   result->image.tex = &(result->texture);
   result->image.subtex = &(result->subtex);
}

//Clean up a layer created by create_page
void delete_layer(struct LayerData page)
{
   C3D_RenderTargetDelete(page.target);
   C3D_TexDelete(&page.texture);
}



// -- DATA TRANSFER UTILS --

char * write_to_datamem(char * stroke_data, char * stroke_end, u16 page, char * mem, char * mem_end)
{
   u32 stroke_size = stroke_end - stroke_data;
   u32 test_size = DCV_VARIMAXSCAN + stroke_size + 1; //Assume page may be max width
   u32 mem_free = MAX_DRAW_DATA - (mem_end - mem);
   char * new_end = mem_end;

   //Oops, the size of the data is more than the leftover space in draw_data!
   if(test_size > mem_free)
   {
      LOGDBG("ERR: Couldn't store lines! Required: %ld, has: %ld\n",
            test_size, mem_free);
   }
   else
   {
      //FIRST, write the stroke identifier
      *new_end = DRAWDATA_ALIGNMENT;
      new_end++;

      //Next, store the page 
      new_end = int_to_varwidth(page, new_end);

      //Finally, memcopy the whole stroke chunk
      memcpy(new_end, stroke_data, sizeof(char) * stroke_size);
      new_end += stroke_size;

      //no need to free or anything, we're reusing the stroke buffer
#ifdef DEBUG_DATAPRINT
      *new_end = '\0'; //Will this be an issue??
      printf("\x1b[20;1HS: %-5ld MF: %-7ld\n%-300.300s", stroke_size, mem_free, mem_end);
#endif
   }

   return new_end;
}

//Scan through memory until either we reach the end, the max scan is reached,
//or we actually find the first occurence of a page that we want. Return 
//the place where we stopped scanning.
char * datamem_scanstroke(char * start, char * end, const u32 max_scan, 
              const u16 page, char ** stroke_start)
{
   char * tempptr;
   char * scanptr = start;
   *stroke_start = NULL;

   if(scanptr >= end)
   {
      LOGDBG("WARN: called scanstroke at or past end of data! Diff: %d\n", end - scanptr);
      return end;
   }

   //Perform a pre-check to realign ourselves if we're not aligned
   if(*scanptr != DRAWDATA_ALIGNMENT)
   {
      LOGDBG("SCAN ERROR: OUT OF ALIGNMENT!, linear scanning for next stroke\n");
      tempptr = memchr(scanptr, DRAWDATA_ALIGNMENT, end - scanptr);

      if(tempptr == NULL)
      {
         LOGDBG("SCAN ERROR: NO MORE ALIGNMENT CHARS! Skipping all the way to end\n");
         return end;
      }
      else
      {
         LOGDBG("SCAN SKIP: fast-forwarding %d characters to next alignment\n", tempptr - scanptr);
         scanptr = tempptr;
      }
   }

   u16 stroke_page;
   u8 varwidth;

   while(scanptr < end && (scanptr - start) < max_scan)
   {
      //Skip the alignment character (TODO: assuming it's 1 byte)
      scanptr++;

      //TODO: will crash if last character is the alignment char, or if there
      //just aren't enough characters to read up the page.
      stroke_page = varwidth_to_int(scanptr, &varwidth);
      tempptr = scanptr + varwidth; //tmpptr points at the stroke start

      //Move scanptr to the next stroke, always
      scanptr = memchr(scanptr, DRAWDATA_ALIGNMENT, (end - scanptr));

      //If no more strokes are found, we're at the end
      if(scanptr == NULL) scanptr = end;

      if(stroke_page == page)
      {
         *stroke_start = tempptr;
         break;
      }
   }

   return scanptr;
}



// -- BIG SCAN DRAW SYSTEM --

struct ScanDrawData 
{
   struct LinePackage * packages;
   struct SimpleLine * all_lines;

   //Status trackers
   struct LinePackage * current_package;
   struct LinePackage * last_package;
   struct SimpleLine * last_line;

   u32 packages_capacity;
   u32 lines_capacity;
};

void scandata_initialize(struct ScanDrawData * data, u32 max_line_buffer)
{
   //At MOST, we should have a maximum of the maximum allowed lines to read, as
   //each stroke NEEDS one line
   data->packages_capacity = max_line_buffer;
   data->lines_capacity = max_line_buffer + MAX_STROKE_LINES;
   data->packages = malloc(sizeof(struct LinePackage) * data->packages_capacity); 
   //But for ALL lines put together, the last line we scan COULD be as big as
   //the maximum stroke by accident, so always include additional space
   data->all_lines = malloc(sizeof(struct SimpleLine) * data->lines_capacity);
   data->current_package = NULL;
   data->last_package = data->packages;
   data->last_line = data->all_lines;
}

void scandata_free(struct ScanDrawData * data)
{
   free(data->packages);
   free(data->all_lines);
   data->last_package = NULL;
   data->last_line = NULL;
   data->packages = NULL;
   data->all_lines = NULL;
   data->packages_capacity = 0;
   data->lines_capacity = 0;
}

//Draw and track a certain number of lines from the given scandata.
u32 scandata_draw(struct ScanDrawData * scandata, u32 line_drawcount, 
      struct LayerData * layers, u8 layer_count)//, u8 last_layer)
{
   u32 current_drawcount = 0;

   //Only draw if there's something to start the whole thing
   if(scandata->current_package != NULL && 
         scandata->current_package != scandata->last_package)
   {
      u8 last_layer = (scandata->last_package - 1)->layer;

      struct LinePackage * stopped_on = NULL;
       
      //Loop over layers
      for(u8 i = 0; i < layer_count; i++)
      {
         //This is the end of the line, we stopped somewhere
         if(stopped_on != NULL) break;

         //Calculate actual layer (last_layer produces shifted window)
         u8 layer_i = (i + last_layer + 1) % layer_count;

         //Don't want to call this too often, so do as much as possible PER
         //layer instead of jumping around
         C2D_SceneBegin(layers[layer_i].target);

         //Scan over every package
         for(struct LinePackage * p = scandata->current_package; 
               p < scandata->last_package; p++)
         {
            //Just entirely skip data for layers we're not focusing on yet.
            if(p->layer != layer_i) continue;

            u16 packagedrawlines = p->line_count;
            u32 leftover_drawcount = line_drawcount - current_drawcount;

            //If this is going to be the last package we're drawing, track
            //where we stopped and don't draw ALL the lines.
            if(packagedrawlines > leftover_drawcount)
            {
               //This is where we stopped. 
               stopped_on = p;
               packagedrawlines = leftover_drawcount;
            }

            draw_lines(p, 0, packagedrawlines);
            line_drawcount += packagedrawlines;

            //If we didn't draw ALL the lines, move the line pointer forward.
            //We know that the line pointers in these packages points to a flat array
            if(packagedrawlines != p->line_count)
               p->lines += packagedrawlines;
         }
      }

      //At the end, only set package to NULL if we got all the way through.
      scandata->current_package = stopped_on;
   }

   return current_drawcount;
}

//Scan the data up to a certain amount, parse the data, and draw it. Kind of
//doing too much, but whatever, this is a small program. Return the location we
//scanned up to.
char * scandata_parse(struct ScanDrawData * scandata, char * drawdata, 
      char * drawdata_end, u32 line_scancount, const u16 page)
{
   char * parse_pointer = drawdata;
   char * stroke_pointer = NULL;
   u32 scan_remaining = MAX_DRAWDATA_SCAN;

   //There's VERY LITTLE safety in these functions! PLEASE BE CAREFUL
   //Assumptions: 
   //-the scandata that's given can hold the entirety of the desired scancount
   //-the scandata pointers are all at the start
   //-there's nothing in the scandata

   if(scandata->current_package != NULL)
      LOGDBG("WARN: Tried to scan_dataparse without an empty buffer!\n");

   //Reset all the data based on assumptions
   scandata->last_package = scandata->packages;
   scandata->last_line = scandata->all_lines;

   //Is there a possibility it will scan past the end? Can we do comparisons
   //like this on pointers? I'm pretty sure you can, they point to the same array
   while(parse_pointer < drawdata_end && 
         (scandata->last_line - scandata->all_lines) < line_scancount && 
         scan_remaining > 0)
   {
      //Remaining is always the maximum scan amount minus how far we've scanned
      //into the data.
      scan_remaining = MAX_DRAWDATA_SCAN - (parse_pointer - drawdata);

      //we should ALWAYS be able to trust parse_pointer
      parse_pointer = datamem_scanstroke(parse_pointer, drawdata_end, 
            scan_remaining, page, &stroke_pointer);

      //What do we do if it doesn't find a stroke pointer? We can't do anymore,
      //there was no stroke to be found.... right? Or does that mean no stroke to
      //be found, within the allotted maximum scanning range?
      if(stroke_pointer == NULL)
         continue; //We assume we just didn't find anything, try again

      //Pre-assign the next open line spot to this package
      scandata->last_package->lines = scandata->last_line;

      //Do we really care where the end of this ends up? no; don't use the return.
      //I don't know when this wouldn't be the case, but the parse_pointer
      //SHOULD point to the start of the next stroke, or in other words, 1 past
      //the last character in the stroke data, which is perfect for this.
      convert_data_to_lines(scandata->last_package, stroke_pointer, parse_pointer);

      //Move the "last" pointers forward
      scandata->last_line += scandata->last_package->line_count;
      scandata->last_package++;
   }

   //Only set a current_package if there's something there
   if(scandata->last_package > scandata->packages)
      scandata->current_package = scandata->packages;

   return parse_pointer;
}



// -- MENU/PRINT STUFF --

void print_controls()
{
   printf("     L - change color        R - general modifier\n");
   printf(" LF/RT - line width     UP/DWN - zoom\n");
   printf("SELECT - change layers   START - menu\n");
   printf("  ABXY - change tools    C-PAD - scroll canvas\n");
   printf("--------------------------------------------------");
}

void print_framing()
{
   printf("\x1b[29;1H--------------------------------------------------");
}

#define PSX1BLEN 30

void get_printmods(char * status_x1b, char * active_x1b, char * statusbg_x1b, char * activebg_x1b)
{
   //First is background, second is foreground (within string)
   if(status_x1b != NULL) sprintf(status_x1b, "\x1b[40m\x1b[%dm", STATUS_MAINCOLOR);
   if(active_x1b != NULL) sprintf(active_x1b, "\x1b[40m\x1b[%dm", STATUS_ACTIVECOLOR);
   if(statusbg_x1b != NULL) sprintf(statusbg_x1b, "\x1b[%dm\x1b[30m", 10 + STATUS_MAINCOLOR);
   if(activebg_x1b != NULL) sprintf(activebg_x1b, "\x1b[%dm\x1b[30m", 10 + STATUS_ACTIVECOLOR);
}

void print_status(u8 width, u8 layer, s8 zoom_power, u8 tool, u16 color, u16 page)
{
   char tool_chars[TOOL_COUNT + 1];
   strcpy(tool_chars, TOOL_CHARS);

   char status_x1b[PSX1BLEN];
   char active_x1b[PSX1BLEN];
   char statusbg_x1b[PSX1BLEN];
   char activebg_x1b[PSX1BLEN];
   get_printmods(status_x1b, active_x1b, statusbg_x1b, activebg_x1b);

   printf("\x1b[30;1H%s W:%s%02d%s L:", status_x1b, active_x1b, width, status_x1b);
   for(s8 i = PAGECOUNT - 1; i >= 0; i--)
      printf("%s ", i == layer ? activebg_x1b : statusbg_x1b);
   printf("%s Z:", status_x1b);
   for(s8 i = MIN_ZOOMPOWER; i <= MAX_ZOOMPOWER; i++)
      printf("%s%c", 
            i == zoom_power ? activebg_x1b : statusbg_x1b,
            i == 0 ? '|' : ' ');
   printf("%s T:", status_x1b);
   for(u8 i = 0; i < TOOL_COUNT; i++)
      printf("%s%c", i == tool ? activebg_x1b : active_x1b, tool_chars[i]);
   printf("%s P:%s%03d", status_x1b, active_x1b, page + 1);
   printf("%s C:%s%#06x", status_x1b, active_x1b, color);
}

void print_time(bool showcolon)
{
   char status_x1b[PSX1BLEN];
   get_printmods(status_x1b, NULL, NULL, NULL);

   time_t rawtime = time(NULL);
   struct tm * timeinfo = localtime(&rawtime);

   printf("\x1b[30;45H%s%02d%c%02d", 
         status_x1b, timeinfo->tm_hour, showcolon ? ':' : ' ', timeinfo->tm_min);
}



// -- TESTS --

#ifdef DEBUG_RUNTESTS
#include "tests.h" //Yes this is awful, don't care
#endif



// -- MAIN, OFC --

// Some macros used ONLY for main (think lambdas)
#define MAIN_UPDOWN(x) {   \
   if(kHeld & KEY_R) {     \
      current_page = UTILS_CLAMP(current_page + x * ((kHeld&KEY_L)?10:1), 0, MAX_PAGE);    \
      draw_pointer = draw_data;     \
      flush_layers = true;          \
   } else {                \
      zoom_power = UTILS_CLAMP(zoom_power + x, MIN_ZOOMPOWER, MAX_ZOOMPOWER);    \
   } }

int main(int argc, char** argv)
{
   gfxInitDefault();
   C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
   C2D_Init(MY_C2DOBJLIMIT);
   C2D_Prepare();

   consoleInit(GFX_TOP, NULL);
   C3D_RenderTarget* screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

   //weird byte order? 16 bits of color are at top
   const u32 screen_color = SCREEN_COLOR;
   const u32 bg_color = CANVAS_BG_COLOR;
   const u32 layer_color = rgba32c_to_rgba16c_32(CANVAS_LAYER_COLOR);

   u16 palette [PALETTE_COLORS];
   convert_palette(base_palette, palette, PALETTE_COLORS);
   u8 palette_index = PALETTE_STARTINDEX;

   const Tex3DS_SubTexture subtex = {
      PAGEWIDTH, PAGEHEIGHT,
      0.0f, 1.0f, 1.0f, 0.0f
   };

   struct LayerData layers[PAGECOUNT];

   for(int i = 0; i < PAGECOUNT; i++)
      create_layer(layers + i, subtex);

   struct ScreenModifier screen_mod = {0,0,1}; 

   bool touching = false;
   bool palette_active = false;
   bool flush_layers = true;

   circlePosition pos;
   touchPosition current_touch;
   u32 current_frame = 0;
   u32 end_frame = 0;
   s8 zoom_power = 0;
   s8 last_zoom_power = 0;
   u16 current_page = 0;

   u8 current_tool = 0;
   struct ToolData tool_data[TOOL_COUNT];
   fill_defaulttools(tool_data, palette[palette_index]);

   struct LinePackage pending;
   struct SimpleLine * pending_lines = malloc(MAX_STROKE_LINES * sizeof(struct SimpleLine));
   pending.lines = pending_lines;
   pending.line_count = 0;
   pending.layer = PAGECOUNT - 1; //Always start on the top page

   struct ScanDrawData scandata;
   scandata_initialize(&scandata, MAX_FRAMELINES);

   char * draw_data = malloc(MAX_DRAW_DATA * sizeof(char));
   char * stroke_data = malloc(MAX_STROKE_DATA * sizeof(char));
   char * draw_data_end = draw_data;
   char * draw_pointer = draw_data;
   char * saved_last = draw_data;

   print_controls();

#ifdef DEBUG_RUNTESTS
   run_tests();
#endif

   print_framing();

   while(aptMainLoop())
   {
      hidScanInput();
      u32 kDown = hidKeysDown();
      u32 kUp = hidKeysUp();
      u32 kHeld = hidKeysHeld();
      hidTouchRead(&current_touch);
		hidCircleRead(&pos);

      // Respond to user input
      if(kDown & KEY_L && !(kHeld & KEY_R)) palette_active = !palette_active;
      if(kDown & KEY_DUP) MAIN_UPDOWN(1)
      if(kDown & KEY_DDOWN) MAIN_UPDOWN(-1)
      if(kDown & KEY_DRIGHT) tool_data[current_tool].width += (kHeld & KEY_R ? 5 : 1);
      if(kDown & KEY_DLEFT) tool_data[current_tool].width -= (kHeld & KEY_R ? 5 : 1);
      if(kDown & KEY_SELECT) pending.layer = (pending.layer + 1) % PAGECOUNT;
      if(kDown & KEY_A) current_tool = TOOL_PENCIL;
      if(kDown & KEY_B) current_tool = TOOL_ERASER;
      if(kDown & KEY_START) 
      {
         u8 selected = easy_menu(MAINMENU_TITLE, MAINMENU_ITEMS, MAINMENU_TOP, KEY_B | KEY_START);
         if(selected == MAINMENU_EXIT)
         {
            if(saved_last == draw_data_end || easy_warn("WARN: UNSAVED DATA", "Really quit?", MAINMENU_TOP))
               break;
         }
      }
      if(kDown & KEY_TOUCH) update_package_with_tool(&pending, &tool_data[current_tool]);
      if(kUp & KEY_TOUCH) end_frame = current_frame;

      //Update zoom separately, since the update is always the same
      if(zoom_power != last_zoom_power) set_screenmodifier_zoom(&screen_mod, pow(2, zoom_power));
      tool_data[current_tool].width = C2D_Clamp(tool_data[current_tool].width, MIN_WIDTH, MAX_WIDTH);

      if(kDown & ~(KEY_TOUCH) || !current_frame)
      {
         print_status(tool_data[current_tool].width, pending.layer, zoom_power, 
               current_tool, tool_data[current_tool].color, current_page);
      }

      touching = (kHeld & KEY_TOUCH) > 0;

      update_screenmodifier(&screen_mod, pos);

      if(!(current_frame % 30))
         print_time(current_frame % 60);

      // Render the scene
      C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

      // -- LAYER DRAW SECTION --
      C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);

      //Apparently (not sure), all clearing should be done within our main loop?
      if(flush_layers)
      {
         for(int i = 0; i < PAGECOUNT; i++)
            C2D_TargetClear(layers[i].target, layer_color); 
         flush_layers = false;
      }
      //Ignore first frame touches
      else if(touching)
      {
         if(palette_active)
         {
            if(tool_data[current_tool].color_settable)
            {
               update_paletteindex(&current_touch, &palette_index);
               tool_data[current_tool].color = palette[palette_index];
            }
         }
         else
         {
            //Keep this outside the if statement below so it can be used for
            //background drawing too (draw commands from other people)
            C2D_SceneBegin(layers[pending.layer].target);

            if(pending.line_count < MAX_STROKE_LINES)
            {
               //This is for a stroke, do different things if we have different tools!
               add_stroke(&pending, &current_touch, &screen_mod);
               //Draw ONLY the current line
               draw_lines(&pending, pending.line_count - 1, pending.line_count);
            }

         }
      }

      //Just always try to draw whatever is leftover in the buffer
      if(draw_pointer < draw_data_end)
      {
         u32 maxdraw = MAX_FRAMELINES;
         maxdraw -= scandata_draw(&scandata, maxdraw, layers, PAGECOUNT);
         draw_pointer = scandata_parse(&scandata, draw_pointer, draw_data_end,
               maxdraw, current_page);
         scandata_draw(&scandata, maxdraw, layers, PAGECOUNT); 
      }

      MY_FLUSH();

      // -- OTHER DRAW SECTION --
      C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, 
            GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);

      //TODO: Eventually, change this to put the data in different places?
      if(end_frame == current_frame && pending.line_count > 0)
      {
         char * cvl_end = convert_lines_to_data(&pending, stroke_data, MAX_STROKE_DATA);

         if(cvl_end == NULL)
         {
            LOGDBG("ERR: Couldn't convert lines!\n");
         }
         else
         {
            char * previous_end = draw_data_end;
            draw_data_end = write_to_datamem(stroke_data, cvl_end, current_page, draw_data, draw_data_end);
            //An optimization: if draw_pointer was already at the end, don't
            //need to RE-draw what we've already drawn, move it forward with
            //the mem write. Note: there are instances where we WILL be drawing
            //twice, but it's difficult to determine what has or has not been
            //drawn when the pointer isn't at the end.
            if(previous_end == draw_pointer) draw_pointer = draw_data_end;
         }

         pending.line_count = 0;
      }

      C2D_TargetClear(screen, screen_color);
      C2D_SceneBegin(screen);

      //Draw the layers
      C2D_DrawRectSolid(-screen_mod.ofs_x, -screen_mod.ofs_y, 0.5f,
            PAGEWIDTH * screen_mod.zoom, PAGEHEIGHT * screen_mod.zoom, bg_color); //The bg color
      for(int i = 0; i < PAGECOUNT; i++)
      {
         C2D_DrawImageAt(layers[i].image, -screen_mod.ofs_x, -screen_mod.ofs_y, 0.5f, 
               NULL, screen_mod.zoom, screen_mod.zoom);
      }
      draw_scrollbars(&screen_mod);

      //The selected color thing
      if(palette_active)
      {
         draw_colorpicker(palette, PALETTE_COLORS, palette_index);
      }
      else
      {
         C2D_DrawRectSolid(0, 0, 0.5f, PALETTE_MINISIZE, PALETTE_MINISIZE, PALETTE_BG);
         C2D_DrawRectSolid(PALETTE_MINIPADDING, PALETTE_MINIPADDING, 0.5f, 
               PALETTE_MINISIZE - PALETTE_MINIPADDING * 2, 
               PALETTE_MINISIZE - PALETTE_MINIPADDING * 2, 
               rgba16_to_rgba32c(palette[palette_index])); 
      }


      C3D_FrameEnd(0);

      last_zoom_power = zoom_power;
      current_frame++;
   }

   free(pending_lines);
   free(draw_data);
   free(stroke_data);

   for(int i = 0; i < PAGECOUNT; i++)
      delete_layer(layers[i]);

   scandata_free(&scandata);

   C2D_Fini();
   C3D_Fini();
   gfxExit();
   return 0;
}
