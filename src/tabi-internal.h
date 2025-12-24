#ifndef TABI_INTERNAL_H
#define TABI_INTERNAL_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABI_GENERATED_FILE_NAME "generated_tabi.ninja"
#define TABI_VERSION "0.1.0"

/// Check a condition and exit with an error message if it fails
#define TABI_CHECK(cond, msg)                                                 \
  do                                                                          \
    {                                                                         \
      if (!(cond))                                                            \
        {                                                                     \
          fprintf (stderr, "Error: %s\n", msg);                               \
          exit (1);                                                           \
        }                                                                     \
    }                                                                         \
  while (0)

///
/// Generic dynamic array support
///
/// assume that all dynamic arrays have the following layout:
///
/// typedef struct {
///   T *items;
///   size_t count;
///   size_t capacity;
/// } TList;
///
#define TABI_DECLARE_DYN_ARRAY(array_type, array_name)                        \
  typedef struct                                                              \
  {                                                                           \
    array_type *items;                                                        \
    size_t count;                                                             \
    size_t capacity;                                                          \
  } array_name;

#define TABI_DYN_ARRAY_INITIAL_CAPACITY 2
#define TABI_DYN_ARRAY_GROWTH_FACTOR 2

#define TABI_DYN_ARRAY_INIT(array)                                            \
  do                                                                          \
    {                                                                         \
      (array)->items = NULL;                                                  \
      (array)->count = 0;                                                     \
      (array)->capacity = 0;                                                  \
    }                                                                         \
  while (0)

#define TABI_DYN_ARRAY_DEINIT(array)                                          \
  do                                                                          \
    {                                                                         \
      free ((array)->items);                                                  \
      (array)->items = NULL;                                                  \
      (array)->count = 0;                                                     \
      (array)->capacity = 0;                                                  \
    }                                                                         \
  while (0)

#define TABI_DYN_ARRAY_APPEND(array, array_type, item)                        \
  do                                                                          \
    {                                                                         \
      if ((array)->count >= (array)->capacity)                                \
        {                                                                     \
          size_t new_capacity                                                 \
              = ((array)->capacity == 0)                                      \
                    ? TABI_DYN_ARRAY_INITIAL_CAPACITY                         \
                    : (array)->capacity * TABI_DYN_ARRAY_GROWTH_FACTOR;       \
          (array)->items = (array_type *)realloc (                            \
              (array)->items, sizeof (array_type) * new_capacity);            \
          (array)->capacity = new_capacity;                                   \
        }                                                                     \
      (array)->items[(array)->count] = (item);                                \
      (array)->count += 1;                                                    \
    }                                                                         \
  while (0)

#define TABI_DYN_ARRAY_FOR_EACH(array, iterator_type, it)                     \
  for (iterator_type it = (array)->items;                                     \
       it < (array)->items + (array)->count; ++it)

#endif
