/// This header file is not meant to be included anywhere except in your
/// build.tabi.c file. The bootstrap process does this automatically for you.
#ifdef TABI_INCLUDED
#error "You already included the tabi header"
#else
#define TABI_INCLUDED
#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define TABI_GENERATED_FILE_NAME "generated_tabi.ninja"

typedef struct
{
  /// Path where the build.tabi.c file resides
  const char *projet_root_path;

  /// Path where the build system is generated
  const char *project_build_path;

  bool need_bootstrap;

} TabiContext;


static void
tabi_init (int argc, char **argv, TabiContext *context)
{
  if (argc < 3)
    {
      fprintf (stderr, "Usage: %s <project_root_path> <project_build_path>\n",
               argv[0]);
      return;
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
}

static void
tabi_clean (TabiContext *context)
{
}

static void
tabi_bootstrap_if_necessary (TabiContext *context)
{
  if (!context->need_bootstrap)
    return;

  char build_file_path[1024] = { 0 };
  strcat (build_file_path, context->project_build_path);
  strcat (build_file_path, "/build.ninja");

  FILE *file = fopen (build_file_path, "w");

  fprintf (file, "# === variables ===\n");

  fprintf (file, "root_path = %s\n", context->projet_root_path);
  fprintf (file, "build_path = %s\n", context->project_build_path);
  fprintf (file, "\n");

  fprintf (file, "# === rules ===\n"
                 "rule cc\n"
                 "  command = cc -O2 -o $out $in\n"
                 "  description = Compiling $in\n"
                 "\n"
                 "rule run_generator\n"
                 "  command = ./$in $root_path $build_path\n"
                 "  description = Running user-defined $in\n"
                 "  generator = 1 \n"
                 "\n"
                 "# === build targets ===\n"
                 "\n"
                 "# Compile the generator\n"
                 "build build_tabi: cc ${root_path}/build.tabi.c\n"
                 "\n"
                 "# Run the user's build instructions finalizing build.ninja\n"
                 "build generated_tabi.ninja: run_generator build_tabi\n"
                 "\n"
                 "include generated_tabi.ninja\n");

  char generated_file_path[1024] = { 0 };
  strcat (generated_file_path, context->project_build_path);
  strcat (generated_file_path, TABI_GENERATED_FILE_NAME);

  FILE *generated_file = fopen (generated_file_path, "w");
  fclose (generated_file);
}

static void
tabi_generate (TabiContext *context)
{
  FILE *f = fopen (TABI_GENERATED_FILE_NAME, "w");
  fprintf (f, "# Generated build instructions\n");

  fclose (f);
}

TabiContext tabi_global_context;

/// Pass a void(*)() that specifies what to build
#define TABI_MAIN_ENTRY_POINT(user_entry_point)                               \
  int main (int argc, char **argv)                                            \
  {                                                                           \
    TabiContext ctxt;                                                         \
    tabi_init (argc, argv, &tabi_global_context);                             \
    tabi_bootstrap_if_necessary (&tabi_global_context);                       \
    user_entry_point ();                                                      \
    tabi_generate (&tabi_global_context);                                     \
    tabi_clean (&tabi_global_context);                                        \
    return 0;                                                                 \
  }
