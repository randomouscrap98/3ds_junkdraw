#ifndef __HEADER_LITTLETUI_MENU__
#define __HEADER_LITTLETUI_MENU__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef TUIMENU_MAXNAME
#define TUIMENU_MAXNAME 128
#endif
#ifndef TUIMENU_MAXENUMTOTAL
#define TUIMENU_MAXENUMTOTAL 256
#endif
#ifndef TUIMENU_MAXUISTRING
#define TUIMENU_MAXUISTRING 16
#endif
#ifndef TUIMENU_ITEMGROW
#define TUIMENU_ITEMGROW 10
#endif
#ifndef TUIMENU_VALUEWIDTH
#define TUIMENU_VALUEWIDTH 10
#endif
#ifndef TUIMENU_NAMEPADLEFT
#define TUIMENU_NAMEPADLEFT 0
#endif
#ifndef TUIMENU_NAMEPADRIGHT
#define TUIMENU_NAMEPADRIGHT 1
#endif
#ifndef TUIMENU_FLOATPRECISION
#define TUIMENU_FLOATPRECISION 2
#endif
#ifndef TUIMENU_UI_NAMETRUNCATE
#define TUIMENU_UI_NAMETRUNCATE ".."
#endif
#ifndef TUIMENU_UI_LISTEDGE
#define TUIMENU_UI_LISTEDGE "..."
#endif
#ifndef TUIMENU_UI_LEFT
#define TUIMENU_UI_LEFT "<"
#endif
#ifndef TUIMENU_UI_RIGHT
#define TUIMENU_UI_RIGHT ">"
#endif
#ifndef TUIMENU_UI_EMPTY
#define TUIMENU_UI_EMPTY ' ' // Difficult to make this not a single char
#endif
#ifndef TUIMENU_ENUM_SPLIT
#define TUIMENU_ENUM_SPLIT '\n'
#endif
#ifndef TUIMENU_UNIT_TYPE
#define TUIMENU_UNIT_TYPE int
#endif
#ifndef TUIMENU_NUMBER_TYPE
#define TUIMENU_NUMBER_TYPE int32_t
#endif
#ifndef TUIMENU_FLOAT_TYPE
#define TUIMENU_FLOAT_TYPE float
#endif
#ifndef TUIMENU_LOOP
#define TUIMENU_LOOP 1
#endif

// Whatever you pick for this, it must be signed....
typedef TUIMENU_UNIT_TYPE tui_menu_unit_t;
typedef TUIMENU_NUMBER_TYPE tui_menu_number_t;
typedef TUIMENU_FLOAT_TYPE tui_menu_float_t;

#define TUIMENU_TYPE_BASIC    0
#define TUIMENU_TYPE_NUMBER   1
#define TUIMENU_TYPE_FLOAT    2
#define TUIMENU_TYPE_ENUM     3
#define TUIMENU_TYPE_CALLBACK 4

#define TUIMENU_ACTION_NONE     (0)
#define TUIMENU_ACTION_MOVE     (1 << 0)  // Up or down (change menu option)
#define TUIMENU_ACTION_VALUE    (1 << 1)  // Left or right (change value)
#define TUIMENU_ACTION_ACCEPT   (1 << 2)  // Click on a menu item
#define TUIMENU_ACTION_CANCEL   (1 << 3)  // Exit a menu (or... other things?)
#define TUIMENU_ACTION_POSITION (1 << 4)  // Force an exact position in the menu

// There's a weird circular reference with these, so we typedef them
// early so we can hold pointers to them without knowing what they are yet
typedef struct tui_menu tui_menu;
typedef struct tui_menu_item tui_menu_item;

typedef struct {
  tui_menu_unit_t result;
  uint8_t running;
} tui_menu_result;

typedef struct {
  uint16_t action;
  tui_menu_unit_t offset;
} tui_menu_action;

struct tui_menu {
  tui_menu_item * items;
  char ui_listedge[TUIMENU_MAXUISTRING];
  char ui_left[TUIMENU_MAXUISTRING];
  char ui_right[TUIMENU_MAXUISTRING];
  char ui_start[TUIMENU_MAXUISTRING];
  char ui_nametruncate[TUIMENU_MAXUISTRING];
  char ui_empty;
  tui_menu_unit_t maxitems;
  tui_menu_unit_t numitems;
  tui_menu_unit_t current;
  tui_menu_unit_t top;
  tui_menu_unit_t height;      // we have a height but no width, as that depends on the size of the render output
  tui_menu_unit_t valuewidth;  // Yes, you must set this manually
  tui_menu_unit_t name_padleft;
  tui_menu_unit_t name_padright;
  uint8_t loop;
};

void tui_menu_init(tui_menu * tm, tui_menu_unit_t height);
void tui_menu_free(tui_menu * tm);
void tui_menu_reset(tui_menu * tm);
// COPIES the item into the menu. Note that once a menu is fully constructed,
// you CAN reuse it, since all values it points back to are pointers.
int tui_menu_push(tui_menu * tm, tui_menu_item * item);
// Whether the given RENDER line is the "current" (where the cursor is)
int tui_menu_iscurrent(tui_menu * tm, tui_menu_unit_t line);

// NOTE: out is expected to have enough capacity to store the render at width
// PLUS the null terminating character!! Buffer should be width + 1!!
void tui_menu_renderline(tui_menu * tm, char * out, tui_menu_unit_t width, tui_menu_unit_t line);
tui_menu_result tui_menu_run(tui_menu * tm, tui_menu_action action);


// Data which can fit snugly inside a mostly-empty menu item (all because enum is so large)
typedef union {
  void * ptr;
  tui_menu_number_t num;
  tui_menu_float_t flt;
  char str[TUIMENU_MAXENUMTOTAL];
  uint8_t raw[TUIMENU_MAXENUMTOTAL];
} tui_menu_item_data;

typedef struct {
  tui_menu_item_data data;
  uint8_t quit;
} tui_menu_basic;

typedef struct {
  tui_menu_number_t * value;
  tui_menu_number_t min;  // Inclusive
  tui_menu_number_t max;  // INCLUSIVE
  tui_menu_number_t step;
} tui_menu_number;

typedef struct {
  tui_menu_float_t * value;
  tui_menu_float_t min;   // Inclusive
  tui_menu_float_t max;   // INCLUSIVE
  tui_menu_float_t step;
  int precision;
} tui_menu_float;

typedef struct {
  char values[TUIMENU_MAXENUMTOTAL];
  tui_menu_number_t * value;    // enumerators are string display backed by number
  tui_menu_number_t numitems;
} tui_menu_enum;

typedef struct {
  tui_menu_result (*callback)(tui_menu_item_data * data, tui_menu_unit_t pos, tui_menu_action action);
  // We HAVE space in the union so just use it for simple data you don't want to allocate!
  tui_menu_item_data data;
} tui_menu_callback;


typedef union {
  tui_menu_number number;
  tui_menu_float floatingpoint;
  tui_menu_basic basic;
  tui_menu_enum enumerator;
  tui_menu_callback callback;
  tui_menu submenu;
} tui_menu_data;

struct tui_menu_item {
  char name[TUIMENU_MAXNAME];
  tui_menu_data data;
  uint8_t type;
  uint8_t loop; // General: whether items loop or not (if items)
};

// WARN: Max is INCLUSIVE, and this is ALSO not a standard "loop"! Large steps near the edges will
// only TAKE you to the edge, and large steps at the edge (if looping) will only take you to the first
// value on the other side, NOT a proper modulus etc! This is intended behavior and IMO more ergonomic 
// for menu usage, as you often generally want to jump to the top or bottom and people are not good at
// calculating modulus in their head to know "jump to +1 from top" is the same as jump down 47 or whatever
#define TUIMENU_LOOP_VALUE(value, offset, min, max, doloop) { \
  if(value == min && offset < 0) { \
    if(doloop) value = max; \
  } else if(value == max && offset > 0) { \
    if(doloop) value = min; \
  } else { \
    value += offset; \
    if(value < min) value = min; \
    if(value > max) value = max; \
  } \
}

// Assign to var the bones of a menu item with given type
#define TUIITEM_COMMON(var, _name, _type) \
  tui_menu_item var = (tui_menu_item) { \
    .type = (_type), \
    .loop = TUIMENU_LOOP, \
  }; \
  snprintf(var.name, TUIMENU_MAXNAME, "%s", (_name)); \

// Easily push a basic menu item (one you're expected to click on)
#define TUIMITEM_BASIC(tm, err, _name, _quit) { \
  TUIITEM_COMMON(_tmp, _name, TUIMENU_TYPE_BASIC); \
  _tmp.data.basic = (tui_menu_basic) { \
    .quit = (_quit), \
  }; \
  err = tui_menu_push((tm), &_tmp); \
}

// Easily push a number (integer) menu item
#define TUIMITEM_NUMBER(tm, err, _name, ptrval, _min, _max, _step) { \
  TUIITEM_COMMON(_tmp, _name, TUIMENU_TYPE_NUMBER); \
  _tmp.data.number = (tui_menu_number) { \
    .value = (ptrval), \
    .min = (_min), \
    .max = (_max), \
    .step = (_step), \
  }; \
  err = tui_menu_push((tm), &_tmp); \
}

// Easily push a double (floating point) menu item
#define TUIMITEM_FLOAT(tm, err, _name, ptrval, _min, _max, _step) { \
  TUIITEM_COMMON(_tmp, _name, TUIMENU_TYPE_FLOAT); \
  _tmp.data.floatingpoint = (tui_menu_float) { \
    .value = (ptrval), \
    .min = (_min), \
    .max = (_max), \
    .step = (_step), \
    .precision = TUIMENU_FLOATPRECISION, \
  }; \
  err = tui_menu_push((tm), &_tmp); \
}

// Easily push an enum menu item. NOTE: values should be separated by \n!!!
#define TUIMITEM_ENUM(tm, err, _name, ptrval, _values) { \
  TUIITEM_COMMON(_tmp, _name, TUIMENU_TYPE_ENUM); \
  _tmp.data.enumerator = (tui_menu_enum) { \
    .value = (ptrval), \
    .numitems = 1, /* Newlines go BETWEEN items. */ \
  }; \
  /* Now we need to copy the raw values and replace \n with 0 */ \
  snprintf(_tmp.data.enumerator.values, TUIMENU_MAXENUMTOTAL, "%s", _values); \
  size_t _vlen = strlen(_values); \
  for(size_t _i = 0; _i < _vlen; _i++) { \
    if(_tmp.data.enumerator.values[_i] == TUIMENU_ENUM_SPLIT) { \
      _tmp.data.enumerator.values[_i] = 0; \
      _tmp.data.enumerator.numitems++; \
    } \
  } \
  err = tui_menu_push((tm), &_tmp); \
}

// Easily push an callback menu item. Callback SHOULD NOT be null!
#define TUIMITEM_CALLBACK(tm, err, _name, _callback, _data) { \
  TUIITEM_COMMON(_tmp, _name, TUIMENU_TYPE_CALLBACK); \
  _tmp.data.callback = (tui_menu_callback) { \
    .callback = (_callback), \
    .data = _data, /* This is a rather large copy */ \
  }; \
  err = tui_menu_push((tm), &_tmp); \
}



#endif
