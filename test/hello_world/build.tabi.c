#include "../../src/tabi.h"

// Insert your build instructions here

static void
tabi_run ()
{
  TabiCompiler *gcc_compiler = tabi_compiler ("gcc", "-O2 -Wall");
  tabi_build_executable (gcc_compiler, "hello_world", "main.c");
}

TABI_MAIN_ENTRY_POINT (tabi_run);
