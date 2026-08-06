#ifndef __UTILS_VECTOR_H__
#define __UTILS_VECTOR_H__

#include <stddef.h>
#include <stdlib.h>

// Macros to generate code for a custom vector type. This
// wastes code space but decreases complexity of trying to store
// arbitrary data in a vector

#define VECTOR_INIT_CAPACITY 4
#define VECTOR_GROW_FACTOR 1.5

// Create the struct and prototypes for a vector of the given type. This
// will create a new type called "vector_type", such as "vector_int" etc
#define VECTOR_DECLARE(type) \
  typedef struct { \
    type * array; \
    size_t length; \
    size_t capacity; \
  } vector_##type; \
  int  vector_##type##_init(vector_##type *); \
  void vector_##type##_free(vector_##type *); \
  void vector_##type##_clear(vector_##type *); \
  /* Preallocate the given amount of capacity, only if larger than current capacity */ \
  int  vector_##type##_reserve(vector_##type *, size_t); \
  /* Add one item to the end of the array and return a pointer to the index. More efficient than push (no copy)*/ \
  int  vector_##type##_increment(vector_##type *, size_t * index); \
  /* Remove one item from the end of the array. More efficient than pop (no copy)*/ \
  int  vector_##type##_decrement(vector_##type *); \
  int  vector_##type##_push(vector_##type *, type *); \
  int  vector_##type##_pop(vector_##type *, type * out); \
  /* Remove an item at the given index and swap the last item in the list with it. Treats vector like an unordered bag */ \
  int  vector_##type##_bag_remove(vector_##type *, size_t index); \
  int  vector_##type##_append(vector_##type * dest, vector_##type * src); \
  /* Append a slice of src to the end of the destination vector. End is exclusive, start inclusive */ \
  int  vector_##type##_append_range(vector_##type * dest, vector_##type * src, \
      size_t start, size_t end); \
  int  vector_##type##_set(vector_##type * v, size_t index, type * assign); \
  int  vector_##type##_get(vector_##type * v, size_t index, type * out); \
  int  vector_##type##_first(vector_##type * v, type *out); \
  int  vector_##type##_last(vector_##type * v, type *out); 

// TODO: missing
// - slow remove (move everything over)
// - slow insert (move everything over)
// - maybe a safe pointer to an element? useless though
// - sort? if possible? There's already a sort macro...


#define VECTOR_FUNC_INIT(type, init_cap) \
  int vector_##type##_init(vector_##type * v) { \
    v->capacity = init_cap; \
    v->length = 0; \
    return !(v->array = malloc(sizeof(type) * v->capacity)); \
  }

#define VECTOR_FUNC_FREE(type) \
  void vector_##type##_free(vector_##type * v) { \
    if(v->array) free(v->array); \
    v->array = NULL; \
  }

#define VECTOR_FUNC_CLEAR(type) \
  void vector_##type##_clear(vector_##type * v) { \
    v->length = 0; \
  }

#define VECTOR_FUNC_RESERVE(type) \
  int vector_##type##_reserve(vector_##type * v, size_t cap) { \
    if(cap <= v->capacity) { return 0; } \
    v->capacity = cap; \
    return !(v->array = realloc(v->array, sizeof(type) * v->capacity)); \
  }

#define VECTOR_FUNC_DECREMENT(type) \
  int vector_##type##_decrement(vector_##type * v) { \
    if(v->length == 0) { return 1; } \
    v->length--; \
    return 0; \
  }

#define VECTOR_FUNC_INCREMENT(type, grow_factor) \
  int vector_##type##_increment(vector_##type * v, size_t * out) { \
    if(v->length >= v->capacity) { \
      if(vector_##type##_reserve(v, (size_t)(v->capacity * (grow_factor)))) { \
        /* logerror("Couldn't reallocate vector during reserve (out of memory?)"); */ \
        return 1; \
      } \
    } \
    *out = v->length; \
    v->length++; \
    return 0; \
  }

#define VECTOR_FUNC_PUSH(type) \
  int vector_##type##_push(vector_##type * v, type * item) { \
    size_t dest; \
    int result = vector_##type##_increment(v, &dest); \
    if(result) { return result; } \
    v->array[dest] = *item; \
    return 0; \
  }

#define VECTOR_FUNC_POP(type) \
  int vector_##type##_pop(vector_##type * v, type * out) { \
    int result = vector_##type##_decrement(v); \
    if(result) { return result; } \
    *out = v->array[v->length]; \
    return 0; \
  }

#define VECTOR_FUNC_BAGREMOVE(type) \
  int vector_##type##_bag_remove(vector_##type * v, size_t index) { \
    if(index >= v->length) { return 1; } \
    if(index == v->length - 1) { return vector_##type##_decrement(v); } \
    v->length--; \
    v->array[index] = v->array[v->length]; \
    return 0; \
  }

#define VECTOR_FUNC_GET(type) \
  int vector_##type##_get(vector_##type * v, size_t index, type * out) { \
    if(index >= v->length) { return 1; } \
    *out = v->array[index]; \
    return 0; \
  }

#define VECTOR_FUNC_SET(type) \
  int vector_##type##_set(vector_##type * v, size_t index, type * assign) { \
    if(index >= v->length) { return 1; } \
    v->array[index] = *assign; \
    return 0; \
  }

#define VECTOR_FUNC_APPEND_RANGE(type) \
  int vector_##type##_append_range(vector_##type * dest, vector_##type * src, \
        size_t start, size_t end) { \
    if(end < start || end > src->length) { return 1; } \
    if(end == start) { return 0; } \
    /* Reserve all the space for the new one. No padding */ \
    size_t range = end - start; \
    size_t newlen = range + dest->length; \
    int result = vector_##type##_reserve(dest, newlen); \
    if(result) { return result; } \
    /* copy src into dest */ \
    for(size_t i = 0; i < range; i++) { \
      dest->array[dest->length + i] = src->array[i + start]; \
    } \
    dest->length = newlen; \
    return 0; \
  }

#define VECTOR_FUNC_APPEND(type) \
  int vector_##type##_append(vector_##type * dest, vector_##type * src) { \
    return vector_##type##_append_range(dest, src, 0, src->length); \
  }

#define VECTOR_FUNC_FIRST(type) \
  int vector_##type##_first(vector_##type * v, type *out) { \
    if(v->length == 0) { return 1; } \
    *out = v->array[0]; \
    return 0; \
  }

#define VECTOR_FUNC_LAST(type) \
  int vector_##type##_last(vector_##type * v, type *out) { \
    if(v->length == 0) { return 1; } \
    *out = v->array[v->length - 1]; \
    return 0; \
  }

#define VECTOR_DEFINE(type) \
  VECTOR_FUNC_INIT(type, VECTOR_INIT_CAPACITY) \
  VECTOR_FUNC_FREE(type) \
  VECTOR_FUNC_CLEAR(type) \
  VECTOR_FUNC_RESERVE(type) \
  VECTOR_FUNC_DECREMENT(type) \
  VECTOR_FUNC_INCREMENT(type, VECTOR_GROW_FACTOR) \
  VECTOR_FUNC_PUSH(type) \
  VECTOR_FUNC_POP(type) \
  VECTOR_FUNC_BAGREMOVE(type) \
  VECTOR_FUNC_GET(type) \
  VECTOR_FUNC_SET(type) \
  VECTOR_FUNC_APPEND(type) \
  VECTOR_FUNC_APPEND_RANGE(type) \
  VECTOR_FUNC_FIRST(type) \
  VECTOR_FUNC_LAST(type) 


#endif
