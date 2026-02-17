/// tabi - A minimal build system library for C/C++ projects
///
/// Copyright (c) 2026 Sebastian Proell
///
/// SPDX-License-Identifier: MIT

#ifndef TABI_MEM_H
#define TABI_MEM_H

#include "tabi-log.h"
#include "tabi-types.h"

#include "stddef.h"
#include "stdlib.h"
#include "string.h"

#define TABI_KiB(x) ((x) * 1024)
#define TABI_MiB(x) (TABI_KiB (x) * 1024)
#define TABI_GiB(x) (TABI_MiB (x) * 1024)

#define TABI_INTERNAL_MEM_MAGIC 42

// Start every allocation with a header that stores the size of the allocation
// and a magic number for sanity checking.
typedef struct
{
  u64 size : 56;
  u64 magic : 8;
} TabiInternalMemHeader;

typedef struct
{
  u64 count;
  u64 capacity;
  void *data;
  struct
  {
    u32 n_allocations;
  } metadata;
} TabiInternalMem;

void tabi_internal_mem_init (TabiInternalMem *mem, u64 capacity);
void tabi_internal_mem_deinit (TabiInternalMem *mem);

// Alloc uninitialized
void *tabi_internal_mem_alloc (TabiInternalMem *mem, u64 size);

// Alloc zero-initialized
void *tabi_internal_mem_calloc (TabiInternalMem *mem, u64 size);

/// Duplicate a string
const char *tabi_internal_strdup (TabiInternalMem *mem, const char *str);

/// Concatenate two paths with a '/' in between.
const char *tabi_internal_pathcat (TabiInternalMem *mem, const char *path1,
                                   const char *path2);

/// Definitions if desired
#ifdef TABI_IMPLEMENTATION

void
tabi_internal_mem_init (TabiInternalMem *mem, u64 capacity)
{
  memset (mem, 0, sizeof (TabiInternalMem));

  mem->data = malloc (capacity);
  if (!mem->data)
    tabi_internal_error ("Failed to allocate requested capacity of %llu bytes",
                         capacity);
  mem->capacity = capacity;
}

void
tabi_internal_mem_deinit (TabiInternalMem *mem)
{
  free (mem->data);
  memset (mem, 0, sizeof (TabiInternalMem));
}

void *
tabi_internal_mem_alloc (TabiInternalMem *mem, u64 size)
{
  if (size == 0)
    tabi_internal_error ("Requested 0 bytes.");

  if (!mem->data)
    tabi_internal_error ("Internal error: memory arena not allocated.");

  const u64 pos_aligned
      = (mem->count + sizeof (void *) - 1) & ~(sizeof (void *) - 1);
  const u64 new_count = pos_aligned + size + sizeof (TabiInternalMemHeader);

  if (new_count > mem->capacity)
    tabi_internal_error ("Failed to allocate requested size of %llu bytes",
                         size);

  void *new_block = (u8 *)mem->data + pos_aligned;
  TabiInternalMemHeader *header = (TabiInternalMemHeader *)new_block;
  header->size = size;
  header->magic = TABI_INTERNAL_MEM_MAGIC;

  mem->count = new_count;
  mem->metadata.n_allocations++;

  return new_block + sizeof (TabiInternalMemHeader);
}

void *
tabi_internal_mem_calloc (TabiInternalMem *mem, u64 size)
{
  void *ptr = tabi_internal_mem_alloc (mem, size);
  memset (ptr, 0, size);
  return ptr;
}

const char *
tabi_internal_strdup (TabiInternalMem *mem, const char *str)
{
  u64 len = strlen (str);
  char *copy = tabi_internal_mem_alloc (mem, len + 1);
  memcpy (copy, str, len + 1);

  return copy;
}

const char *
tabi_internal_pathcat (TabiInternalMem *mem, const char *path1,
                       const char *path2)
{
  u64 len1 = strlen (path1);
  u64 len2 = strlen (path2);

  // +1 for '/' +1 for '\0'
  char *result = (char *)tabi_internal_mem_alloc (mem, len1 + len2 + 2);

  strcpy (result, path1);
  result[len1] = '/';
  strcpy (result + len1 + 1, path2);

  return result;
}

#endif

#endif
