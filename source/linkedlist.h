#ifndef __UTILS_LINKEDLIST_H__
#define __UTILS_LINKEDLIST_H__

#include <stddef.h>
#include <stdlib.h>

// Macros to generate code for a custom linked list type. This
// wastes code space but decreases complexity of trying to store
// arbitrary data in a linked list.

// Include this in the middle of your struct to turn it into
// a linked list entry. it's a doubly linked list.
#define LL_FIELDS(name) \
  struct name * next; \
  struct name * prev;

// This is something you can do on your own without this macro
#define LL_BASICSTRUCT(name, type) \
  typedef struct name { \
    type data; \
    LL_FIELDS(name) \
  } name;

// Put this in your header to create the prototype for your
// new custom linked list type
#define LL_PROTOTYPE(name) \
  void name##_init(name ** list); \
  void name##_free(name ** list, void (*pre_free)(name *)); \
  int  name##_create_after(name ** list, name * afterthis, name ** out); \
  int  name##_create_before(name ** list, name * beforethis, name ** out); \
  void name##_remove(name ** list, name * item); \
  size_t name##_count(name * list);

#define LL_FUNC_INIT(name) \
  void name##_init(name ** list) { *list = NULL; }

#define LL_FUNC_FREE(name) \
  void name##_free(name ** list, void (*pre_free)(name *)) { \
    while(*list != NULL) { \
      if(pre_free) pre_free(*list); \
      name * cur = *list; \
      *list = (*list)->next; \
      free(cur); \
    } \
  }

// Whether inserting before or after target, you always malloc
// and always replace the head if the target is null
#define _LL_FUNC_CREATE_COMMON(name, list, out, target) \
    *out = (name*)malloc(sizeof(name)); \
    if(*out == NULL) return 1; \
    (*out)->next = (*out)->prev = NULL; \
    if(target == NULL) { \
      if(*list) (*list)->prev = *out; \
      (*out)->next = *list; \
      *list = *out; \
    }

// Insert into given list after given element. If NULL passed, inserts
// at the very beginning of the list, replacing the head.
#define LL_FUNC_CREATE_AFTER(name) \
  int name##_create_after(name ** list, name * afterthis, name ** out) { \
    _LL_FUNC_CREATE_COMMON(name, list, out, afterthis) \
    else { \
      (*out)->next = afterthis->next; \
      (*out)->prev = afterthis; \
      if(afterthis->next != NULL) afterthis->next->prev = *out; \
      afterthis->next = *out; \
    } \
    return 0; \
  }

// Insert into given list before given element. If NULL passed, inserts
// at the very beginning of the list, replacing the head
#define LL_FUNC_CREATE_BEFORE(name) \
  int name##_create_before(name ** list, name * beforethis, name ** out) { \
    _LL_FUNC_CREATE_COMMON(name, list, out, beforethis) \
    else { \
      (*out)->next = beforethis; \
      (*out)->prev = beforethis->prev; \
      if(beforethis->prev != NULL) beforethis->prev->next = *out; \
      else *list = *out; \
      beforethis->prev = *out; \
    } \
    return 0; \
  }

#define LL_FUNC_REMOVE(name) \
  void name##_remove(name ** list, name * item) { \
    if(item->next != NULL) item->next->prev = item->prev; \
    if(item->prev != NULL) item->prev->next = item->next; \
    if(*list == item) *list = item->next; \
    free(item); \
  }

// Count starting from ANY element to the end.
#define LL_FUNC_COUNT(name) \
  size_t name##_count(name * list) { \
    size_t count = 0; \
    while(list) { \
      count++; \
      list = list->next; \
    } \
    return count; \
  }

// Call this in your c file to insert the generated code for the 
// implementation for your custom linked list type
#define LL_DEFINITION(name) \
  LL_FUNC_INIT(name) \
  LL_FUNC_FREE(name) \
  LL_FUNC_CREATE_AFTER(name) \
  LL_FUNC_CREATE_BEFORE(name) \
  LL_FUNC_REMOVE(name) \
  LL_FUNC_COUNT(name)

#endif
