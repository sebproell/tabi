#include "../../src/tabi.h"

// Insert your build instructions here

static void
tabi_run ()
{
  TabiCompiler *gcc_compiler = tabi_compiler ("gcc", "-O2 -Wall");

  TabiBuildTarget *exe = tabi_executable (gcc_compiler, "main");
  tabi_add_source (exe, "main.c");
  tabi_add_source (exe, "helper.c");
}

TABI_MAIN_ENTRY_POINT (tabi_run);
