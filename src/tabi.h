/// This header file is not meant to be included anywhere except in your
/// build.tabi.c file.
#ifdef TABI_INCLUDED
#error "You already included the tabi header"
#else
#define TABI_INCLUDED
#endif

#include "tabi-internal.h"

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
  const char *projet_root_path;

  /// Path where the build system is generated
  const char *project_build_path;

  /// Path where tabi may place build artifacts and temporary files
  const char *tabi_build_path;

  bool need_bootstrap;

  /// Anything that tabi can build will be stored here
  TabiObjectArray objects;
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

/// Concatenate two paths with a '/' in between. The returned string must be
/// freed by the caller.
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

        free ((void *)target->target_name);
        TABI_DYN_ARRAY_FOR_EACH (&target->source_files, const char **,
                                 source_file)
        {
          // We strdup'ed these paths when adding them
          free ((void *)*source_file);
        }
        TABI_DYN_ARRAY_DEINIT (&target->source_files);
        break;
      }
    }
  free (object);
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

  TABI_DYN_ARRAY_APPEND (&tabi_global_context.objects, TabiObject *,
                         (TabiObject *)compiler);

  return compiler;
}

///
/// Create a build target that compiles source files into an executable
///
static TabiBuildTarget *
tabi_executable (TabiCompiler *compiler, const char *executable_name)
{
  TabiBuildTarget *target
      = (TabiBuildTarget *)malloc (sizeof (TabiBuildTarget));

  target->base.type = TABI_OBJECT_TYPE_BUILD_TARGET;
  target->target_type = TABI_BUILD_TARGET_TYPE_EXECUTABLE;
  target->compiler = compiler;
  target->target_name = strdup (executable_name);

  TABI_DYN_ARRAY_INIT (&target->source_files);

  TABI_DYN_ARRAY_APPEND (&tabi_global_context.objects, TabiObject *,
                         (TabiObject *)target);

  return target;
}

///
/// Create a build target that compiles source files into object files
///
static TabiBuildTarget *
tabi_objects (TabiCompiler *compiler, const char *target_name)
{
  TabiBuildTarget *target
      = (TabiBuildTarget *)malloc (sizeof (TabiBuildTarget));
  target->base.type = TABI_OBJECT_TYPE_BUILD_TARGET;
  target->target_type = TABI_BUILD_TARGET_TYPE_OBJECTS;
  target->compiler = compiler;
  target->target_name = strdup (target_name);

  TABI_DYN_ARRAY_INIT (&target->source_files);

  TABI_DYN_ARRAY_APPEND (&tabi_global_context.objects, TabiObject *,
                         (TabiObject *)target);
  return target;
}

static void
tabi_add_source (TabiBuildTarget *target, const char *source_file)
{
  TABI_DYN_ARRAY_APPEND (&target->source_files, const char *,
                         strdup (source_file));
}

///
/// Specify that a build target depends on another build target
///
static void
tabi_depends_on (TabiBuildTarget *target, TabiBuildTarget *dependency)
{
  TABI_CHECK (dependency->target_type == TABI_BUILD_TARGET_TYPE_OBJECTS,
              "tabi_depends_on: Dependency must be of type OBJECTS");

  TABI_CHECK (target->target_type == TABI_BUILD_TARGET_TYPE_EXECUTABLE,
              "tabi_depends_on: Target must be of type EXECUTABLE");

  // TODO: safety: prevent multiple inlcusion and cycles
  TABI_DYN_ARRAY_APPEND (&target->dependencies, TabiBuildTarget *, dependency);
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
  context->projet_root_path = strdup (argv[1]);
  context->project_build_path = strdup (argv[2]);
  context->tabi_build_path
      = tabi_internal_pathcat (context->project_build_path, "tabi_build");

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

  TABI_DYN_ARRAY_INIT (&context->objects);
}

static void
tabi_internal_deinit (TabiContext *context)
{
  free ((void *)context->projet_root_path);
  free ((void *)context->project_build_path);
  free ((void *)context->tabi_build_path);

  TABI_DYN_ARRAY_FOR_EACH (&context->objects, TabiObject **, obj)
  {
    tabi_internal_object_deinit (*obj);
  }

  TABI_DYN_ARRAY_DEINIT (&context->objects);
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

/// Generate all rules and build targets
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
        case TABI_OBJECT_TYPE_UNKNOWN:
          {
            TABI_CHECK (false,
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
                             context->projet_root_path, *source_file);
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
                             context->projet_root_path, *source_file);
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
