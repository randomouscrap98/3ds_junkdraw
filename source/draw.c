// #include "myutils.h"
#include "draw.h"

// C lets you redefine stuff... right?
#define LOGDBG(f_, ...)

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

/* //Add another stroke to a line collection (that represents a stroke). Works for
//the first stroke too.
struct SimpleLine * add_point_to_stroke(struct LinePackage * pending, 
      const touchPosition * pos, const struct ScreenState * mod)
{
   //This is for a stroke, do different things if we have different tools!
   struct SimpleLine * line = pending->lines + pending->line_count;

   line->x2 = pos->px / mod->zoom + mod->offset_x / mod->zoom;
   line->y2 = pos->py / mod->zoom + mod->offset_y / mod->zoom;

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
}*/

//Draw the collection of lines given, starting at the given line and ending
//before the other given line (first inclusive, last exclusive)
//Assumes you're already on the appropriate page you want and all that
void pixaligned_linepackfunc(const struct LinePackage * linepack, u16 pack_start, u16 pack_end,
      rectangle_func rect_f)
{
   u32 color = rgba16_to_rgba32c(linepack->color);

   if(pack_end > linepack->line_count)
      pack_end = linepack->line_count;

   for(u16 i = pack_start; i < pack_end; i++)
      pixaligned_linefunc(&linepack->lines[i], linepack->width, color, rect_f);
}

void pixaligned_linepackfunc_all(const struct LinePackage * linepack, rectangle_func rect_f)
{
   pixaligned_linepackfunc(linepack, 0, linepack->line_count, rect_f);
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


// ------------------------
// -- SCANDRAW UTILS --
// ------------------------

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

   //Even though each package TECHNICALLY points to the mega line array, we
   //still don't want any individual package to go over our max
   for(int i = 0; i < data->packages_capacity; i++)
      data->packages[i].max_lines = MAX_STROKE_LINES;
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

/*//Draw and track a certain number of lines from the given scandata.
u32 scandata_draw(struct ScanDrawData * scandata, u32 line_drawcount, struct LayerData * layers, u8 layer_count)//, u8 last_layer)
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

            pixaligned_linepackfunc(p, 0, packagedrawlines, MY_SOLIDRECT);
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
}*/

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
      convert_data_to_linepack(scandata->last_package, stroke_pointer, parse_pointer);

      //Move the "last" pointers forward
      scandata->last_line += scandata->last_package->line_count;
      scandata->last_package++;
   }

   //Only set a current_package if there's something there
   if(scandata->last_package > scandata->packages)
      scandata->current_package = scandata->packages;

   return parse_pointer;
}