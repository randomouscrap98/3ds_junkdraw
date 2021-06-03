// Using this file like a macro rather than properly compiling since I don't
// care at all

int test_transparenthalftofull()
{
   //It's such a small space, literally just run the gamut of 16 bit colors
   for(u32 i = 0; i <= 0xFFFF; i++)
   {
      u16 col = i;
      u32 full = rgba16c_to_rgba32c(col);
      u16 half = rgba32c_to_rgba16c(full);
      if((col & 0xFFF) == 0xFFF) printf("."); //tracker
      if(col != half) //(col << 16) != half)
      {
         printf("ERR: h2f Expected %04x, got %04x, full: %08lx\n", col, half, full);
         return 1;
      }
   }

   return 0;
}

int test_transparent16to32()
{
   //It's such a small space, literally just run the gamut of 16 bit colors
   for(u32 i = 0; i <= 0xFFFF; i++)
   {
      u16 col = i;
      u32 full = rgba16_to_rgba32c(col);
      u16 half = rgba32c_to_rgba16(full);
      if((col & 0xFFF) == 0xFFF) printf("."); //tracker
      if(col != half) //(col << 16) != half)
      {
         printf("ERR: 16-32 Expected %04x, got %04x, full: %08lx\n", col, half, full);
         return 1;
      }
   }

   return 0;
}

#define TST_16CF(c32,c16) if(rgba32c_to_rgba16(c32) != c16) { \
   printf("ERR: 16cf Expected %04x, got %04x, full: %08lx\n", c16, rgba32c_to_rgba16(c32), c32); \
   return 1; } else { printf("."); }

int test_16bitcolorformat()
{
   TST_16CF(C2D_Color32(255,0,0,255), 0xFC00); //Red
   TST_16CF(C2D_Color32(0,255,0,255), 0x83E0); //Blue
   TST_16CF(C2D_Color32(0,0,255,255), 0x801F); //Green

   return 0;
}

int test_inttochars_single(u32 num, u8 chars)
{
   char ptr[10];
   char * rptr = NULL;

   rptr = int_to_chars(num, chars, ptr);

   if(rptr != (ptr + chars))
   {
      printf("Resulting pointer not correct for %ld, %d\n", num, chars);
      return 1;
   }

   u32 result = chars_to_int(ptr, chars);

   if(result != num)
   {
      printf("IntToChars not transparent for: %ld, %d (got %ld)\n", num, chars, result);
      return 1;
   }

   printf(".");
   return 0;
}

int test_inttochars()
{
   printf("%d", sizeof(char));

   //Arbitrary values
   if(test_inttochars_single(47, 1)) return 1;
   if(test_inttochars_single(105, 2)) return 1;
   if(test_inttochars_single(6000, 3)) return 1;
   if(test_inttochars_single(303030, 4)) return 1;
   if(test_inttochars_single(17891234, 5)) return 1;

   //Particular values
   if(test_inttochars_single(0x3F, 1)) return 1; //All bits set 6 bits
   if(test_inttochars_single(0x40, 2)) return 1; //One above needing 1 byte
   if(test_inttochars_single(0xFFF, 2)) return 1; //All bits set 12 bits
   if(test_inttochars_single(0x1000, 3)) return 1; //One above needing 2 bytes
   if(test_inttochars_single(0x3FFFF, 3)) return 1; //All bits set 18 bits
   if(test_inttochars_single(0x40000, 4)) return 1; //One above needing 3 bytes

   return 0;
}

int test_signedtospecial()
{
   s32 vals[10] = { -1, 55, -128, 600, -1029, 4096, 0, 1, 32, -32 };

   for(int i = 0; i < 10; i++)
   {
      u32 result = signed_to_special(vals[i]);
      s32 back = special_to_signed(result);

      if(back != vals[i])
      {
         printf("Special/signed not transparent for %ld\n", vals[i]);
         return 1;
      }

      printf(".");
   }

   return 0;
}

int test_inttovarwidth_single(u32 num, u8 chars)
{
   char ptr[10];
   char * rptr = NULL;
   u8 read_chars = 0;

   rptr = int_to_varwidth(num, ptr);

   if(rptr != (ptr + chars))
   {
      printf("IntToVarWidth Resulting pointer not correct for %ld, %d\n", num, chars);
      return 1;
   }

   u32 result = varwidth_to_int(ptr, &read_chars);

   if(result != num)
   {
      printf("IntToVarWidth not transparent for: %ld, %d (got %ld)\n", num, chars, result);
      return 1;
   }

   if(read_chars != chars)
   {
      printf("IntToVarWidth incorrect character width for: %ld, %d (got %d)\n", num, chars, read_chars);
      return 1;
   }

   printf(".");
   return 0;
}

int test_inttovarwidth()
{
   //Arbitrary values
   if(test_inttovarwidth_single(30, 1)) return 1;
   if(test_inttovarwidth_single(500, 2)) return 1;
   if(test_inttovarwidth_single(16789, 3)) return 1;
   if(test_inttovarwidth_single(277777, 4)) return 1;
   if(test_inttovarwidth_single(17505666, 5)) return 1;

   //Particular values
   if(test_inttovarwidth_single(0x1F, 1)) return 1; //All bits set 5 bits
   if(test_inttovarwidth_single(0x20, 2)) return 1; //One above needing 1 byte
   if(test_inttovarwidth_single(0x3FF, 2)) return 1; //All bits set 10 bits
   if(test_inttovarwidth_single(0x400, 3)) return 1; //One above needing 2 bytes
   if(test_inttovarwidth_single(0x7FFF, 3)) return 1; //All bits set 15 bits
   if(test_inttovarwidth_single(0x8000, 4)) return 1; //One above needing 3 bytes

   return 0;
}

bool test_linearr_eq(struct SimpleLine * l1, struct SimpleLine * l2, u16 cnt)
{
   for(u16 i = 0; i < cnt; i++)
      if(l1[i].x1 != l2[i].x1 || l1[i].y1 != l2[i].y1 ||
         l1[i].x2 != l2[i].x2 || l1[i].y2 != l2[i].y2)
         return false;

   return true;
}

#define TST_CVLDAT 200
#define TST_CVLEQ(x,y) if(!(x.line_count == y.line_count) && \
      x.layer == y.layer && x.color == y.color && x.width == y.width && \
      x.style == y.style && test_linearr_eq(x.lines, y.lines, x.line_count)) { \
   printf("ConvertLines the lines did not match: (%d,%d,%d,%d) / (%d,%d,%d,%d)\n", \
         x.lines[0].x1, x.lines[0].y1, x.lines[0].x2, x.lines[0].y2, \
         y.lines[0].x1, y.lines[0].y1, y.lines[0].x2, y.lines[0].y2); \
   return 1; }

#define TST_CVLTST \
   endptr = convert_lines_to_data(&package1, data, TST_CVLDAT); \
   if(endptr == NULL) return 1; \
   endptr2 = convert_data_to_lines(&package2, data, endptr); \
   if(endptr2 == NULL) return 1; \
   TST_CVLEQ(package1, package2) \
   printf(".");

int test_convertlines()
{
   struct SimpleLine lines1[50];
   struct SimpleLine lines2[50];
   struct LinePackage package1;
   struct LinePackage package2;
   package1.lines = lines1;
   package2.lines = lines2;
   char data[TST_CVLDAT];
   char * endptr;
   char * endptr2;

   //First, the absolute most basic thing. One line.
   package1.line_count = 1;
   package1.layer = 1;
   package1.color = rgba32c_to_rgba16c(C2D_Color32(218, 128, 45, 255));
   package1.width = 35;
   package1.style = LINESTYLE_STROKE;
   lines1[0].x1 = 9;
   lines1[0].y1 = 14;
   lines1[0].x2 = 40;
   lines1[0].y2 = 100;

   TST_CVLTST

   //Now a simple test: stroke that doesn't move
   lines1[0].x1 = lines1[0].x2 = 508;
   lines1[0].y1 = lines2[0].y2 = 747;

   TST_CVLTST

   package1.line_count = 20;
   package1.layer = 0;
   package1.color = 0;
   package1.width = 1;

   u16 x = 40; u16 y = 40;
   for(int i = 0; i < 20; i++)
   {
      lines1[i].x1 = x; lines1[i].y1 = y;
      x = rand() & 0x3FF;
      y = rand() & 0x3FF;
      lines1[i].x2 = x; lines1[i].y2 = y;
   }

   TST_CVLTST

   //TODO: Add more tests


   return 0;
}

void run_tests()
{
   printf("Running tests; only errors will be displayed\n");
   if(test_transparent16to32()) return; 
   if(test_transparenthalftofull()) return; 
   if(test_16bitcolorformat()) return; 
   if(test_inttochars()) return; 
   if(test_signedtospecial()) return; 
   if(test_inttovarwidth()) return; 
   if(test_convertlines()) return; 
   printf("\nAll tests passed\n");
}

