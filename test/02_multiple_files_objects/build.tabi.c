#include "../../src/tabi.h"

// Insert your build instructions here

static void
tabi_run ()
{
  TabiCompiler *gcc_compiler = tabi_compiler ("gcc", "-O2 -Wall");

  TabiBuildTarget *obj_files = tabi_objects (gcc_compiler, "lib");
  tabi_add_source (obj_files, "helper.c");

  {
    TabiBuildTarget *exe = tabi_executable (gcc_compiler, "main");
    tabi_add_source (exe, "main.c");
    tabi_depends_on (exe, obj_files);
  }

  // Build another executable to demonstrate reusing object files
  {
    TabiBuildTarget *exe = tabi_executable (gcc_compiler, "main2");
    tabi_add_source (exe, "main.c");
    tabi_depends_on (exe, obj_files);
  }
}

TABI_MAIN_ENTRY_POINT (tabi_run);
