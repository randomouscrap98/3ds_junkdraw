#include "draw.h"
#include "color.h"

#include <stdlib.h>
#include <string.h>

// JUST needed for M_PI??
#include <citro3d.h>

// C lets you redefine stuff... right?
#ifndef LOGDBG
#define LOGDBG(f_, ...)
#endif

// ------- GENERAL UTILS ---------

// ------------------------
// -- DATA CONVERT UTILS --
// ------------------------

//Container needs at least 'chars' amount of space to store this int
char * int_to_chars(u32 num, const u8 chars, char * container)
{
   //WARN: clamping rather than ignoring! Hope this is ok
	num = DCV_CLAMP(num, 0, DCV_MAXVAL(chars)); 

   //Place each converted character, Little Endian
	for(int i = 0; i < chars; i++)
      container[i] = DCV_START + ((num >> (i * DCV_BITSPER)) & DCV_MAXVAL(1));

   //Return the next place you can start placing characters (so you can
   //continue reusing this function)
	return container + chars;
}

//Container should always be the start of where you want to read the int.
//The container should have at least count available. You can increment
//container by count afterwards (always)
u32 chars_to_int(const char * container, const u8 count)
{
   u32 result = 0;
   for(int i = 0; i < count; i++)
      result += ((container[i] - DCV_START) << (i * DCV_BITSPER));
   return result;
}

//A dumb form of 2's compliment that doesn't carry the leading 1's
s32 special_to_signed(u32 special)
{
   if(special & 1)
      return ((special >> 1) * -1) -1;
   else
      return special >> 1;
}

u32 signed_to_special(s32 value)
{
   if(value >= 0)
      return value << 1;
   else
      return ((value << 1) * -1) - 1;
}

//Container needs at LEAST 8 bytes free to store a variable width int
char * int_to_varwidth(u32 value, char * container)
{
   u8 c = 0;
   u8 i = 0;

   do 
   {
      c = value & DCV_VARIMAXVAL(1);
      value = value >> DCV_VARIBITSPER;
      if(value) c |= DCV_VARISTEP; //Continue on, set the uppermost bit
      container[i++] = DCV_START + c;
   } 
   while(value);

   if(i >= DCV_VARIMAXSCAN)
      LOGDBG("WARN: variable width create too long: %d\n",i);

   //Return the NEXT place you can place values (just like the other func)
   return container + i;
}

//Read a variable width value from the given container. Stops if it goes too
//far though, which may give bad values
u32 varwidth_to_int(const char * container, u8 * read_count)
{
   u8 c = 0;
   u8 i = 0;
   u32 result = 0;

   do 
   {
      c = container[i] - DCV_START;
      result += (c & DCV_VARIMAXVAL(1)) << (DCV_VARIBITSPER * i);
      i++;
   } 
   while(c & DCV_VARISTEP && (i < DCV_VARIMAXSCAN)); //Keep going while the high bit is set

   if(i >= DCV_VARIMAXSCAN)
      LOGDBG("WARN: variable width read too long: %d\n",i);

   *read_count = i;

   return result;
}

// ------------------------
// -- SIMPLE LINE UTILS --
// ------------------------

//Draw a line using a custom line drawing system (required like this because of
//javascript's general inability to draw non anti-aliased lines, and I want the
//strokes saved by this program to be 100% accurately reproducible on javascript)
void pixaligned_linefunc(const struct FullLine * line, rectangle_func rect_f)
   //const struct SimpleLine * line, u16 width, u32 color, rectangle_func rect_f)
{
   float xdiff = line->x2 - line->x1;
   float ydiff = line->y2 - line->y1;
   float dist = sqrt(xdiff * xdiff + ydiff * ydiff);
   float ang = atan(ydiff/(xdiff?xdiff:0.0001))+(xdiff<0?M_PI:0);
   float xang = cos(ang);
   float yang = sin(ang);

   float x, y; 
   float ox = -1;
   float oy = -1;

   //Where the "edge" of each rectangle to be drawn is
   float ofs = (line->width / 2.0) - 0.5;

   //Iterate over an acceptable amount of points on the line and draw
   //rectangles to create a line.
   for(float i = 0; i <= dist; i+=0.5)
   {
      x = floor(line->x1+xang*i - ofs);
      y = floor(line->y1+yang*i - ofs);

      //If we align to the same pixel as before, no need to draw again.
      if(ox == x && oy == y) continue;

      //Actually draw a centered rect... however you want to.
      (*rect_f)(x,y,line->width,line->color);

      ox = x; oy = y;
   }
}

//Draw the collection of lines given, starting at the given line and ending
//before the other given line (first inclusive, last exclusive)
//Assumes you're already on the appropriate page you want and all that
void pixaligned_linepackfunc(const struct LinePackage * linepack, u16 pack_start, u16 pack_end,
      rectangle_func rect_f)
{
   struct FullLine line;

   if(pack_end > linepack->line_count)
      pack_end = linepack->line_count;

   for(u16 i = pack_start; i < pack_end; i++) {
      convert_to_fullline(linepack, i, &line);
      pixaligned_linefunc(&line, rect_f);
   }
}

void init_linepackage(struct LinePackage * package) {
   if(package->lines != NULL) {
      LOGDBG("ERR: PACKAGE ALREADY INITIALIZED");
      return;
   }

   package->lines = malloc(sizeof(struct SimpleLine) * MAX_STROKE_LINES);
   package->max_lines = MAX_STROKE_LINES;

   if(!package->lines) {
      LOGDBG("ERR: Couldn't allocate stroke lines");
   }
}

void free_linepackage(struct LinePackage * package) {
   free(package->lines);
}

void convert_to_fullline(const struct LinePackage * package, u16 line_index, struct FullLine * result) {
   result->color = rgba16_to_rgba32c(package->color);
   result->layer = package->layer;
   result->style = package->style;
   result->width = package->width;
   result->x1 = package->lines[line_index].x1;
   result->x2 = package->lines[line_index].x2;
   result->y1 = package->lines[line_index].y1;
   result->y2 = package->lines[line_index].y2;
}

//A true macro, as in just dump code into the function later. Used ONLY for 
//convert_lines, hence "CVL"
#define CVL_LINECHECK if(container_size - (ptr - container) < DCV_VARIMAXSCAN) { \
   LOGDBG("ERROR: ran out of space in line conversion: original size: %ld\n",container_size); \
   return NULL; }

//Convert lines into data, but if it doesn't fit, return a null pointer.
//Returned pointer points to end + 1 in container
char * convert_linepack_to_data(struct LinePackage * lines, char * container, u32 container_size)
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
char * convert_data_to_linepack(struct LinePackage * package, char * data, char * data_end)
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

         if(package->line_count > package->max_lines)
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
         package->line_count++;
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

u32 last_used_page(char * data, u32 length) {
   if(length < 2) {
      return 0; // Just safety
   }
   // Simple: start at one before the end and search backwards for the '.'
   for(char * pos = data + length - 2; pos > data; pos--) {
      if(*pos == '.') {
         u8 length;
         return varwidth_to_int(pos + 1, &length);
      }
   }
   return 0;
}

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

// Scan through memory until either we reach the end, the max scan is reached,
// or we actually find the first occurence of a stroke on a page that we want. Return 
// the place where we stopped scanning (so you can call it repeatedly). Doesn't actually
// parse the stroke, just looks for the next one.
char * datamem_scanstroke(char * start, char * end, const u32 max_scan, const u16 page, char ** stroke_start)
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
