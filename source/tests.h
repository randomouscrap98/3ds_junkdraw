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
   endptr = convert_linepack_to_data(&package1, data, TST_CVLDAT); \
   if(endptr == NULL) return 1; \
   endptr2 = convert_data_to_linepack(&package2, data, endptr); \
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
   package1.max_lines = 50;
   package2.max_lines = 50;
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

int test_insert1evt(struct GameEvent ** list, void * handler, u8 priority, u32 eid, u8 epos)
{
   struct GameEvent * result = insert_gameevent(list, handler, priority);
   //printf("l%p;", list);

   if(result == NULL)
   {
      printf("insert_gameevent returned null for expected eid %ld!\n", eid);
      return 1;
   }

   if(result->id != eid)
   {
      printf("Expected insert_gamevent id: %ld, actual: %ld\n", eid, result->id);
      return 1;
   }

   int pos = 0;
   struct GameEvent * this;

   for(this = *list; this != NULL; this = this->next_event)
   {
      if(this->id == eid)
      {
         if(pos == epos)
         {
            printf(".");
            return 0;
         }
         else
         {
            printf("Expected insert_gamevent (eid %ld) pos: %d, actual: %d\n", eid, epos, pos);
            return 1;
         }
      }
      pos++;
   }

   printf("Inserted element missing from insert_gameevent! eid: %ld", eid);
   return 1;
}

int test_remove1evt(struct GameEvent ** list, u32 eid, u32 epos, u32 size)
{
   s32 pos = remove_gameevent(list, eid);

   if(pos != epos)
   {
      printf("remove_gameevent eid %ld returned pos %ld, expected %ld!\n", eid, pos, epos);
      return 1;
   }

   struct GameEvent * this;

   for(this = *list; this != NULL; this = this->next_event)
   {
      if(this->id == eid)
      {
         printf("remove_gameevent eid %ld, removed element still exists!\n", eid);
         return 1;
      }
   }

   u32 real_size = gameevent_queue_count(list);
   if(real_size != size)
   {
      printf("remove_gameevent eid %ld, list size is %ld, expected %ld!\n", eid, real_size, size);
      return 1;
   }

   if(size == 0 && (*list) != NULL)
   {
      printf("remove_gameevent eid %ld, list not null after removing last element!\n", eid);
      return 1;
   }

   printf(".");
   return 0;
}

int test_insertevent()
{
   reset_gameevent_globalid();

   //Start with a null pointer, it's an empty list
   struct GameEvent * list = NULL;

   //Empty insert
   if(test_insert1evt(&list, NULL, DEFAULT_EVENT_PRIORITY, 1, 0)) return 1;

   //Insert with same priority, should go second and third
   if(test_insert1evt(&list, NULL, DEFAULT_EVENT_PRIORITY, 2, 1)) return 1;
   if(test_insert1evt(&list, NULL, DEFAULT_EVENT_PRIORITY, 3, 2)) return 1;

   //Insert with high priority, should go first
   if(test_insert1evt(&list, NULL, HIGH_EVENT_PRIORITY, 4, 0)) return 1;

   //insert with very low priority, should go last
   if(test_insert1evt(&list, NULL, DEFAULT_EVENT_PRIORITY - 1, 5, 4)) return 1;

   //Insert with medium priority again, should go second to last
   if(test_insert1evt(&list, NULL, DEFAULT_EVENT_PRIORITY, 6, 4)) return 1;

   //Now you have to be able to remove every event!
   u32 freed_count = free_gameevent_queue(&list);
   if(freed_count != 6)
   {
      printf("free_gameevent_queue in test_insert expected %d elements, freed %ld\n", 6, freed_count);
      return 1;
   }
   else
   {
      printf(".");
      return 0;
   }
}

int test_removeevent()
{
   reset_gameevent_globalid();

   //Start with a null pointer, it's an empty list
   struct GameEvent * list = NULL;

   //First, can we remove from an empty list? We should get -1
   if(test_remove1evt(&list, 1, -1, 0)) return 1;
   if(test_insert1evt(&list, NULL, DEFAULT_EVENT_PRIORITY, 1, 0)) return 1;
   if(test_remove1evt(&list, 2, -1, 1)) return 1;

   //Now, can we remove the first element?
   if(test_remove1evt(&list, 1, 0, 0)) return 1;

   //Well now we need to insert more elements
   if(test_insert1evt(&list, NULL, DEFAULT_EVENT_PRIORITY, 2, 0)) return 1;
   if(test_insert1evt(&list, NULL, DEFAULT_EVENT_PRIORITY, 3, 1)) return 1;
   if(test_insert1evt(&list, NULL, DEFAULT_EVENT_PRIORITY, 4, 2)) return 1;

   //If we remove the MIDDLE and LAST element, does the list still work?
   if(test_remove1evt(&list, 3, 1, 2)) return 1;
   if(test_remove1evt(&list, 4, 1, 1)) return 1;

   //And then the first element again for good measure (and to make the list empty)
   if(test_remove1evt(&list, 2, 0, 0)) return 1;

   return 0;
}

//assumes a temp variable t exists
#define TST_BTFLP(x, p1, p2, e) \
   t = swap_bits(x, p1, p2); if(t != e) { \
      printf("Flip %#010lx (%d,%d), expected %#010lx, got %#010lx", \
            (long unsigned int)x, p1, p2, (long unsigned int)e, (long unsigned int)t); return 1; } \
   printf(".");

#define TST_BTFLPM(x, m1, m2, e) \
   t = swap_bits_mask(x, m1, m2); if(t != e) { \
      printf("Flip-m %#010lx (%#010lx,%#010lx), expected %#010lx, got %#010lx", \
            (long unsigned int)x, (long unsigned int)m1, (long unsigned int)m2, (long unsigned int)e, \
            (long unsigned int)t); return 1; } \
   printf(".");

int test_bitflip()
{
   u32 t;

   TST_BTFLP(0x0000, 1, 2, 0x0000);
   TST_BTFLP(0x0110, 4, 8, 0x0110);
   TST_BTFLP(0x00F7, 3, 5, 0x00DF);

   TST_BTFLPM(0x0000, 0x0002, 0x0004, 0x0000);
   TST_BTFLPM(0x0110, 0x0010, 0x0100, 0x0110);
   TST_BTFLPM(0x00F7, 0x0008, 0x0020, 0x00DF);

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
   if(test_insertevent()) return;
   if(test_removeevent()) return;
   if(test_bitflip()) return;
   printf("\nAll tests passed, hooray!\n");
}

