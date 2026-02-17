/// tabi - A minimal build system library for C/C++ projects
///
/// Copyright (c) 2025 Sebastian Proell
///
/// SPDX-License-Identifier: MIT

/// Internal header for the tabi build system
#ifndef TABI_INTERNAL_H
#define TABI_INTERNAL_H

#include "tabi-log.h"
#include "tabi-mem.h"
#include "tabi-types.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABI_GENERATED_FILE_NAME "generated_tabi.ninja"
#define TABI_VERSION "0.1.0"

///
/// Generic dynamic array support
///
/// assume that all dynamic arrays have the following layout:
///
/// typedef struct {
///   T *items;
///   u64 count;
///   u64 capacity;
/// } TList;
///
#define TABI_DECLARE_DYN_ARRAY(array_type, array_name)                        \
  typedef struct                                                              \
  {                                                                           \
    array_type *items;                                                        \
    u64 count;                                                                \
    u64 capacity;                                                             \
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
          u64 new_capacity                                                    \
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

TABI_DECLARE_DYN_ARRAY (const char *, TabiStringArray);

typedef enum
{
  TABI_OBJECT_TYPE_UNKNOWN,
  TABI_OBJECT_TYPE_COMPILER,
  TABI_OBJECT_TYPE_BUILD_TARGET,
} TabiObjectType;

typedef struct
{
  TabiObjectType type;
} TabiObject;

TABI_DECLARE_DYN_ARRAY (TabiObject *, TabiObjectArray);

typedef struct
{
  TabiObject base;
  const char *name;
  const char *path;
  const char *flags;
} TabiCompiler;

typedef enum
{
  TABI_BUILD_TARGET_TYPE_EXECUTABLE,
  TABI_BUILD_TARGET_TYPE_OBJECTS,
} TabiBuildTargetType;

typedef struct TabiBuildTarget TabiBuildTarget;

TABI_DECLARE_DYN_ARRAY (TabiBuildTarget *, TabiBuildTargetArray);

typedef struct TabiBuildTarget
{
  TabiObject base;
  TabiBuildTargetType target_type;

  TabiStringArray source_files;

  TabiCompiler *compiler;
  const char *target_name;

  TabiBuildTargetArray dependencies;
} TabiBuildTarget;

typedef struct
{
  /// Path where the build.tabi.c file resides
  const char *project_root_path;

  /// Path where the build system is generated
  const char *project_build_path;

  /// Path where tabi may place build artifacts and temporary files
  const char *tabi_build_path;

  b8 need_bootstrap;

  /// Everything that we create with tabi
  TabiObjectArray objects;

  /// Arena allocator
  TabiInternalMem mem;
} TabiContext;

/// The context used within the build.tabi.c file
extern TabiContext tabi_global_context;

void tabi_internal_object_deinit (TabiObject *object);

///
/// Internal functions
///

void tabi_internal_status (TabiContext *context);

void tabi_internal_init (i32 argc, char **argv, TabiContext *context);

void tabi_internal_deinit (TabiContext *context);

void tabi_internal_bootstrap_if_necessary (TabiContext *context);

/// Generate all rules and build targets
void tabi_internal_generate (TabiContext *context);

void tabi_internal_summary (TabiContext *context);

static int
tabi_internal_main_entry_point (int argc, char **argv,
                                void (*user_entry_point) ())
{
  tabi_internal_init (argc, argv, &tabi_global_context);
  tabi_internal_status (&tabi_global_context);
  tabi_internal_bootstrap_if_necessary (&tabi_global_context);
  user_entry_point ();
  tabi_internal_generate (&tabi_global_context);
  tabi_internal_summary (&tabi_global_context);
  tabi_internal_deinit (&tabi_global_context);
  return 0;
}

#define TABI_INTERNAL_MAIN_ENTRY_POINT(options)                               \
  int main (int argc, char **argv)                                            \
  {                                                                           \
    return tabi_internal_main_entry_point (argc, argv, options);              \
  }

/// Definitions if desired
#ifdef TABI_IMPLEMENTATION

/// The context used within the build.tabi.c file
TabiContext tabi_global_context;

void
tabi_internal_object_deinit (TabiObject *object)
{
  switch (object->type)
    {
    case TABI_OBJECT_TYPE_UNKNOWN:
      {
        break;
      }
    case TABI_OBJECT_TYPE_COMPILER:
      {
        break;
      }
    case TABI_OBJECT_TYPE_BUILD_TARGET:
      {
        TabiBuildTarget *target = (TabiBuildTarget *)object;

        TABI_DYN_ARRAY_DEINIT (&target->source_files);
        TABI_DYN_ARRAY_DEINIT (&target->dependencies);
        break;
      }
    }
}

///
/// Internal functions
///

void
tabi_internal_status (TabiContext *context)
{
  (void)context;
  tabi_internal_message (
      "This is the tabi build system version " TABI_VERSION);
}

void
tabi_internal_init (i32 argc, char **argv, TabiContext *context)
{
  if (argc < 3)
    {
      fprintf (stderr, "Usage: %s <project_root_path> <project_build_path>\n",
               argv[0]);
      exit (1);
    }

  tabi_internal_mem_init (&context->mem, TABI_MiB (1));

  // TODO better args parsing
  context->project_root_path = tabi_internal_strdup (&context->mem, argv[1]);
  context->project_build_path = tabi_internal_strdup (&context->mem, argv[2]);
  context->tabi_build_path = tabi_internal_pathcat (
      &context->mem, context->project_build_path, "tabi_build");

  // Check if build.ninja exists in the build path
  char build_file_path[1024] = { 0 };
  strcat (build_file_path, context->project_build_path);
  strcat (build_file_path, "/build.ninja");
  FILE *file = fopen (build_file_path, "r");
  if (file == NULL)
    {
      context->need_bootstrap = true;
    }
  else
    {
      context->need_bootstrap = false;
      fclose (file);
    }
}

void
tabi_internal_deinit (TabiContext *context)
{
  TABI_DYN_ARRAY_FOR_EACH (&context->objects, TabiObject **, obj)
  {
    tabi_internal_object_deinit (*obj);
  }
  TABI_DYN_ARRAY_DEINIT (&context->objects);

  tabi_internal_mem_deinit (&context->mem);
}

void
tabi_internal_bootstrap_if_necessary (TabiContext *context)
{
  if (!context->need_bootstrap)
    return;

  tabi_internal_message ("Bootstrapping the build system");

  const char *build_file_path = tabi_internal_pathcat (
      &context->mem, context->project_build_path, "build.ninja");
  FILE *file = fopen (build_file_path, "w");

  fprintf (file, "# === variables ===\n");

  fprintf (file, "root_path = %s\n", context->project_root_path);
  fprintf (file, "build_path = %s\n", context->project_build_path);
  fprintf (file, "\n");

  fprintf (file, "# === rules ===\n"
                 "rule cc_tabi\n"
                 "  command = cc -O2 -o $out $in\n"
                 "  description = Compiling $in\n"
                 // TODO depfile for tabi.h?
                 "\n"
                 "rule run_tabi\n"
                 "  command = ./$in $root_path $build_path\n"
                 "  description = Running user-defined $in\n"
                 "  generator = 1 \n"
                 "\n"
                 "# === build targets ===\n"
                 "\n"
                 "# Compile the generator\n"
                 "build build_tabi: cc_tabi ${root_path}/build.tabi.c\n"
                 "\n"
                 "# Run the user's build instructions finalizing build.ninja\n"
                 "build generated_tabi.ninja: run_tabi build_tabi\n"
                 "\n"
                 "include generated_tabi.ninja\n");

  const char *generated_file_path = tabi_internal_pathcat (
      &context->mem, context->project_build_path, TABI_GENERATED_FILE_NAME);
  // Open the file to create it if it doesn't exist
  FILE *generated_file = fopen (generated_file_path, "w");
  fclose (generated_file);
}

/// Generate all rules and build targets
void
tabi_internal_generate (TabiContext *context)
{
  FILE *f = fopen (TABI_GENERATED_FILE_NAME, "w");
  fprintf (f, "# Generated build instructions\n");

  for (u64 i = 0; i < context->objects.count; ++i)
    {
      TabiObject *obj = context->objects.items[i];
      switch (obj->type)
        {
        case TABI_OBJECT_TYPE_UNKNOWN:
          {
            tabi_internal_error (
                "Unknown object type encountered during generation");
          }
        case TABI_OBJECT_TYPE_COMPILER:
          {
            TabiCompiler *compiler = (TabiCompiler *)obj;
            fprintf (f, "rule %s_compile\n", compiler->name);
            fprintf (f, "  command = %s %s -c -o $out $in\n", compiler->path,
                     compiler->flags);
            fprintf (f, "  description = Compiling $in\n");

            fprintf (f, "rule %s_link\n", compiler->name);
            fprintf (f, "  command = %s %s -o $out $in\n", compiler->path,
                     compiler->flags);
            fprintf (f, "  description = Linking $in\n");
            break;
          }

        //
        // TODO: need to include the correct transitive dependencies here
        //
        case TABI_OBJECT_TYPE_BUILD_TARGET:
          {
            TabiBuildTarget *target = (TabiBuildTarget *)obj;
            switch (target->target_type)
              {
              case TABI_BUILD_TARGET_TYPE_EXECUTABLE:
                {
                  TABI_DYN_ARRAY_FOR_EACH (&target->source_files,
                                           const char **, source_file)
                  {
                    fprintf (f, "build %s/%s/%s.o: %s_compile %s/%s\n",
                             context->tabi_build_path, target->target_name,
                             *source_file, target->compiler->name,
                             context->project_root_path, *source_file);
                  }

                  // TODO: Place executables in the build path root (or maybe
                  // bin?)
                  fprintf (f, "build %s/%s: %s_link",
                           context->project_build_path, target->target_name,
                           target->compiler->name);

                  // Link with the direct source files of this target
                  TABI_DYN_ARRAY_FOR_EACH (&target->source_files,
                                           const char **, source_file)
                  {
                    fprintf (f, " %s/%s/%s.o", context->tabi_build_path,
                             target->target_name, *source_file);
                  }
                  // Link with object files from dependencies
                  TABI_DYN_ARRAY_FOR_EACH (&target->dependencies,
                                           TabiBuildTarget **, dep)
                  {
                    // TODO: needs to recursively include transitive
                    // dependencies
                    TABI_DYN_ARRAY_FOR_EACH (&(*dep)->source_files,
                                             const char **, dep_source_file)
                    {
                      fprintf (f, " %s/%s/%s.o", context->tabi_build_path,
                               (*dep)->target_name, *dep_source_file);
                    }
                  }
                  fprintf (f, "\n");
                }
                break;
              case TABI_BUILD_TARGET_TYPE_OBJECTS:
                {
                  TABI_DYN_ARRAY_FOR_EACH (&target->source_files,
                                           const char **, source_file)
                  {
                    fprintf (f, "build %s/%s/%s.o: %s_compile %s/%s\n",
                             context->tabi_build_path, target->target_name,
                             *source_file, target->compiler->name,
                             context->project_root_path, *source_file);
                  }
                  fprintf (f, "\n");

                  break;
                }
              }
            break;
          }
        }
    }

  fclose (f);
}

void
tabi_internal_summary (TabiContext *context)
{
  (void)context;
  tabi_internal_message ("Build instructions generated successfully");

  f32 mem_percent
      = (f32)context->mem.count / (f32)context->mem.capacity * 100.0f;
  f32 avg_alloc_size
      = context->mem.metadata.n_allocations > 0
            ? (f32)context->mem.count / context->mem.metadata.n_allocations
            : 0.0f;

  tabi_internal_message ("Memory stats: %llu / %llu bytes used (%.2f%%), %llu "
                         "allocations, average %.2f bytes per allocation",
                         context->mem.count, context->mem.capacity,
                         mem_percent, context->mem.metadata.n_allocations,
                         avg_alloc_size);
}

#endif

#endif
