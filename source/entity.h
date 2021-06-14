#ifndef __HEADER_MYENTITY
#define __HEADER_MYENTITY

struct DrawState
{
   u32 frame;
};

struct MyEntity
{
   //bool active;
   bool invalidated;
   void (* clear_func)(const struct DrawState *, struct MyEntity *);
   void (* draw_func)(const struct DrawState *, struct MyEntity *);
   void (* run_func)(const struct DrawState *, struct MyEntity *);
   void (* signal_func)(struct MyEntity *);
   void * data;
};

void entity_init(struct MyEntity *);
void entity_clean(const struct DrawState *, struct MyEntity * pe);

struct MyPrintAnimation
{
   u8 x;
   u8 y;
   char ** frames; //Always add extra frame for clearing
   u16 frame_count; //This can be inferred, but it's faster to just tell us
   u16 frame_time;
   u16 current_frame;
   char * preamble; //What to print before printing the animation
};

//Animation stuff. You can also use animations as static printing
void entityinit_printanimation(struct MyPrintAnimation *, struct MyEntity *);
void printanimation_run(const struct DrawState *, struct MyEntity *);
void printanimation_draw(const struct DrawState *, struct MyEntity *);
void printanimation_clear(const struct DrawState *, struct MyEntity *);
void printanimation_drawframe(u16 frame, struct MyEntity *);

//struct MyPrintEntity
//{
//   u8 x; 
//   u8 y; 
//   u8 width;
//   u8 height;
//   bool invalidated;
//   void (* clear_func)(const struct DrawState *, struct * MyPrintEntity);
//   void (* draw_func)(const struct DrawState *, struct * MyPrintEntity);
//   void * data;
//   const char * clear_preamble;
//   const char * preamble;
////typedef void (* rectangle_func)(float, float, u16, u32);
//};

//struct MyPrintAnimation
//{
//   char ** frames; //Always add extra frame for clearing
//   u16 frame_time;
//};
//
//void printentity_basicclear(const struct * DrawState ds, struct * MyPrintEntity);
//
//void printentity_baseinit(struct * MyPrintEntity pe);
//void printentity_clean(const struct DrawState *, struct * MyPrintEntity pe);
//
//void printentity_initanimation(struct * MyPrintEntity pe, struct * MyPrintAnimation);
//void printanimation_draw(const struct DrawState *, struct * MyPrintEntity);

#endif
