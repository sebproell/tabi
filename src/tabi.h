/// This header file is not meant to be included anywhere except in your
/// build.tabi.c file.
#ifdef TABI_INCLUDED
#error "You already included the tabi header"
#else
#define TABI_INCLUDED
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABI_GENERATED_FILE_NAME "generated_tabi.ninja"
#define TABI_VERSION "0.1.0"

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

typedef struct
{
  TabiObject base;
  TabiBuildTargetType target_type;

  const char **source_files;
  size_t source_file_count;

  TabiCompiler *compiler;
  const char *output_file;
} TabiBuildTarget;

typedef struct
{
  TabiObject **items;
  size_t count;
  size_t capacity;
} TabiObjectList;

typedef struct
{
  /// Path where the build.tabi.c file resides
  const char *projet_root_path;

  /// Path where the build system is generated
  const char *project_build_path;

  bool need_bootstrap;

  /// Anything that tabi can build will be stored here
  TabiObjectList objects;
} TabiContext;

/// The context used within the build.tabi.c file
/// TODO: might need to make this an extern if build code may be used across
/// multiple translation units. For now this is prohibted by the include guard
TabiContext tabi_global_context;

static void
tabi_message (const char *format, ...)
{
  va_list args;
  va_start (args, format);
  vprintf (format, args);
  printf ("\n");
  va_end (args);
}

static const char *
tabi_internal_pathcat (const char *path1, const char *path2)
{
  size_t len1 = strlen (path1);
  size_t len2 = strlen (path2);
  char *result = (char *)calloc (len1 + len2 + 2, 1); // +1 for '/' +1 for '\0'
  strcpy (result, path1);
  result[len1] = '/';
  strcpy (result + len1 + 1, path2);
  return result;
}

static void
tabi_internal_object_list_init (TabiObjectList *list)
{
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

static void
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
        TabiCompiler *compiler = (TabiCompiler *)object;
        free ((void *)compiler->name);
        free ((void *)compiler->path);
        free ((void *)compiler->flags);
        break;
      }
    case TABI_OBJECT_TYPE_BUILD_TARGET:
      {
        TabiBuildTarget *target = (TabiBuildTarget *)object;

        free ((void *)target->output_file);

        for (size_t i = 0; i < target->source_file_count; ++i)
          {
            free ((void *)target->source_files[i]);
          }
        free (target->source_files);
        break;
      }
    }
  free (object);
}

static void
tabi_internal_object_list_deinit (TabiObjectList *list)
{
  // Iterate over all objects and free each of them based on their type
  for (size_t i = 0; i < list->count; ++i)
    {
      TabiObject *obj = list->items[i];
      tabi_internal_object_deinit (obj);
    }
  free (list->items);
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

static void
tabi_internal_object_list_add (TabiObjectList *list, TabiObject *object)
{
  if (list->count >= list->capacity)
    {
      size_t new_capacity = (list->capacity == 0) ? 4 : list->capacity * 2;
      list->items = (TabiObject **)realloc (
          list->items, new_capacity * sizeof (TabiObject *));
      list->capacity = new_capacity;
    }
  list->items[list->count++] = object;
}

static TabiCompiler *
tabi_compiler (const char *path, const char *flags)
{
  TabiCompiler *compiler = (TabiCompiler *)malloc (sizeof (TabiCompiler));
  compiler->base.type = TABI_OBJECT_TYPE_COMPILER;
  // TODO derive name from path
  compiler->name = strdup (path);
  compiler->path = strdup (path);
  compiler->flags = strdup (flags);

  tabi_internal_object_list_add (&tabi_global_context.objects,
                                 (TabiObject *)compiler);
  return compiler;
}

static TabiBuildTarget *
tabi_executable (TabiCompiler *compiler, const char *executable_name)
{
  TabiBuildTarget *target
      = (TabiBuildTarget *)malloc (sizeof (TabiBuildTarget));

  target->base.type = TABI_OBJECT_TYPE_BUILD_TARGET;
  target->target_type = TABI_BUILD_TARGET_TYPE_EXECUTABLE;
  target->compiler = compiler;
  target->output_file = strdup (executable_name);

  target->source_file_count = 0;
  target->source_files = NULL;

  tabi_internal_object_list_add (&tabi_global_context.objects,
                                 (TabiObject *)target);
  return target;
}

static TabiBuildTarget *
tabi_objects (TabiCompiler *compiler)
{
  TabiBuildTarget *target
      = (TabiBuildTarget *)malloc (sizeof (TabiBuildTarget));
  target->base.type = TABI_OBJECT_TYPE_BUILD_TARGET;
  target->target_type = TABI_BUILD_TARGET_TYPE_OBJECTS;
  target->compiler = compiler;
  target->output_file = NULL;

  target->source_file_count = 0;
  target->source_files = NULL;

  tabi_internal_object_list_add (&tabi_global_context.objects,
                                 (TabiObject *)target);
  return target;
}

static void
tabi_add_source (TabiBuildTarget *target, const char *source_file)
{
  target->source_files = (const char **)realloc (
      target->source_files,
      sizeof (const char *) * (target->source_file_count + 1));
  target->source_files[target->source_file_count] = tabi_internal_pathcat (
      tabi_global_context.projet_root_path, source_file);
  target->source_file_count += 1;
}

///
/// Internal functions
///

static void
tabi_internal_status (TabiContext *context)
{
  (void)context;
  tabi_message ("This is the tabi build system version " TABI_VERSION);
}

static void
tabi_internal_init (int argc, char **argv, TabiContext *context)
{
  if (argc < 3)
    {
      fprintf (stderr, "Usage: %s <project_root_path> <project_build_path>\n",
               argv[0]);
      exit (1);
    }

  // TODO better args parsing
  context->projet_root_path = argv[1];
  context->project_build_path = argv[2];

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

  tabi_internal_object_list_init (&context->objects);
}

static void
tabi_internal_deinit (TabiContext *context)
{
  tabi_internal_object_list_deinit (&context->objects);
}

static void
tabi_internal_bootstrap_if_necessary (TabiContext *context)
{
  if (!context->need_bootstrap)
    return;

  tabi_message ("Bootstrapping the build system");

  const char *build_file_path
      = tabi_internal_pathcat (context->project_build_path, "build.ninja");
  FILE *file = fopen (build_file_path, "w");
  free ((void *)build_file_path);

  fprintf (file, "# === variables ===\n");

  fprintf (file, "root_path = %s\n", context->projet_root_path);
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
      context->project_build_path, TABI_GENERATED_FILE_NAME);
  // Open the file to create it if it doesn't exist
  FILE *generated_file = fopen (generated_file_path, "w");
  fclose (generated_file);
  free ((void *)generated_file_path);
}

static void
tabi_internal_generate (TabiContext *context)
{
  FILE *f = fopen (TABI_GENERATED_FILE_NAME, "w");
  fprintf (f, "# Generated build instructions\n");

  for (size_t i = 0; i < context->objects.count; ++i)
    {
      TabiObject *obj = context->objects.items[i];
      switch (obj->type)
        {
        case TABI_OBJECT_TYPE_COMPILER:
          {
            TabiCompiler *compiler = (TabiCompiler *)obj;
            // A compiler can generate rules
            fprintf (f, "\n# Compiler: %s with flags: %s\n", compiler->name,
                     compiler->flags);
            fprintf (f, "rule %s\n", compiler->name);
            fprintf (f, "  command = %s %s -o $out $in\n", compiler->path,
                     compiler->flags);
            fprintf (f, "  description = Compiling $in\n");
            break;
          }
        case TABI_OBJECT_TYPE_BUILD_TARGET:
          {
            TabiBuildTarget *target = (TabiBuildTarget *)obj;
            if (target->target_type == TABI_BUILD_TARGET_TYPE_EXECUTABLE)
              {
                fprintf (f, "\n# Build target: %s\n", target->output_file);
                fprintf (f, "build %s: %s", target->output_file,
                         target->compiler->name);
                for (size_t j = 0; j < target->source_file_count; ++j)
                  {
                    fprintf (f, " %s", target->source_files[j]);
                  }
                fprintf (f, "\n");
              }
            break;
          }
        default:
          {
            // Ignore other object types for now
            break;
          }
        }
    }

  fclose (f);
}

/// Pass a void(*)() that specifies what to build
#define TABI_MAIN_ENTRY_POINT(user_entry_point)                               \
  int main (int argc, char **argv)                                            \
  {                                                                           \
    TabiContext ctxt;                                                         \
    tabi_internal_init (argc, argv, &tabi_global_context);                    \
    tabi_internal_status (&tabi_global_context);                              \
    tabi_internal_bootstrap_if_necessary (&tabi_global_context);              \
    user_entry_point ();                                                      \
    tabi_internal_generate (&tabi_global_context);                            \
    tabi_internal_deinit (&tabi_global_context);                              \
    return 0;                                                                 \
  }
