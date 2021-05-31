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
         printf("ERR: Expected %04x, got %04x, full: %08lx\n", col, half, full);
         return 1;
      }
   }

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

void run_tests()
{
   printf("Running tests; only errors will be displayed\n");
   if(test_transparenthalftofull()) return; 
   if(test_inttochars()) return; 
   if(test_signedtospecial()) return; 
   if(test_inttovarwidth()) return; 
   printf("\nAll tests passed\n");
}

