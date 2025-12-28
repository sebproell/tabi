#define TABI_IMPLEMENTATION

// Define this to remove the 'tabi_' prefix from all function names
#define TABI_STRIP_PREFIX

#include "../../src/tabi.h"

// Insert your build instructions here

static void
tabi_run ()
{
  TabiCompiler *gcc_compiler = compiler ("gcc", "-O2 -Wall");

  // Create a build target for object files. This can be reused in multiple
  // executables.
  TabiBuildTarget *obj_files = objects (gcc_compiler, "lib");
  add_source (obj_files, "helper.c");

  {
    TabiBuildTarget *exe = executable (gcc_compiler, "main");
    add_source (exe, "main.c");
    depends_on (exe, obj_files);
  }

  // Build another executable to demonstrate reusing object files
  {
    TabiBuildTarget *exe = executable (gcc_compiler, "main2");
    add_source (exe, "main.c");
    depends_on (exe, obj_files);
  }
}

TABI_MAIN_ENTRY_POINT (tabi_run);
