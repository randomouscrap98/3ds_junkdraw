// #include "myutils.h"
#include "draw.h"

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

