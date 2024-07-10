#include <stdlib.h>

#include "myutils.h"
#include "entity.h"


// -- ENTITY --

void entity_init(struct MyEntity * e)
{
   e->invalidated = false;
   e->data = NULL;
   e->draw_func = NULL;
   e->clear_func = NULL;
   e->run_func = NULL;
   e->signal_func = NULL;
}

void entity_clean(const struct DrawState * ds, struct MyEntity * e)
{
   if(e->clear_func != NULL)
      e->clear_func(ds, e);
   if(e->data != NULL)
      free(e->data);
}

void entityinit_printanimation(struct MyPrintAnimation * pad, struct MyEntity * e)
{
   entity_init(e);
   e->data = (void *)pad;
   e->run_func = printanimation_run;
   e->draw_func = printanimation_draw;
   e->clear_func = printanimation_clear;
}


// -- PRINTANIMATION --

void printanimation_run(const struct DrawState * ds, struct MyEntity * e)
{
   struct MyPrintAnimation * pad = (struct MyPrintAnimation *)e->data;

   if((ds->frame % pad->frame_time) == 0)
   {
      e->invalidated = true;
      pad->current_frame = (ds->frame / pad->frame_time) % pad->frame_count;
   }
}

void printanimation_draw(const struct DrawState *, struct MyEntity * e) 
{
   printanimation_drawframe(((struct MyPrintAnimation *)e->data)->current_frame, e);
}

void printanimation_clear(const struct DrawState * ds, struct MyEntity * e) 
{
   printanimation_drawframe(((struct MyPrintAnimation *)e->data)->frame_count, e);
}

void printanimation_drawframe(u16 frame, struct MyEntity * e)
{
   struct MyPrintAnimation * pad = (struct MyPrintAnimation *)e->data;

   //Go find the frame in the frame list (if we're not outside the range) and
   //print it.
   if(frame <= pad->frame_count)
   {
      printf("%s\x1b[%d;%dH%s", pad->preamble != NULL ? pad->preamble : "",
            pad->y, pad->x, pad->frames[frame]);
   }
}


// -- JUNK --
//void printentity_basicclear(const struct * DrawState ds, struct * MyPrintEntity pe)
//{
//   //Unfortunately, has to be a for loop
//   for(u8 y = 0; y < pe->height; y++)
//   {
//      printf("%s\x1b[%d;%dH%*s", pe->clear_preamble != NULL ? pe->clear_preamble : "",
//            pe->y + y, pe->x, pe->width, "");
//   }
//}
//
//void printentity_baseinit(struct * MyPrintEntity pe)
//{
//   pe->x = 1; //use the printf positions, 1 indexed
//   pe->y = 1;
//   pe->width = 50;
//   pe->height = 0;
//   pe->invalidated = false;
//   pe->data = NULL;
//   pe->draw_func = NULL;
//   pe->clear_func = printentity_basicclear;
//   pe->preamble = "\x1b[0m";
//   pe->clear_preamble = pe->preamble;
//}
//
//void printentity_clean(const struct * DrawState ds, struct * MyPrintEntity pe)
//{
//   if(pe->clear_func != NULL)
//      pe->clear_func(ds, pe);
//   if(pe->data != NULL)
//      free(pe->data);
//}
//
//void printentity_initanimation(struct * MyPrintEntity pe, struct * MyPrintAnimation)
//{
//
//}
//
//void printanimation_draw(const struct DrawState *, struct * MyPrintEntity)
//{
//
//}
