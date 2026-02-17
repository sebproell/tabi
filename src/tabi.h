/// tabi - A minimal build system library for C/C++ projects
///
/// Copyright (c) 2025 Sebastian Proell
///
/// SPDX-License-Identifier: MIT

#ifndef TABI_H
#define TABI_H

/// The public header for the tabi build system in stb style.
/// This means that including this header provides the interface,
/// and defining TABI_IMPLEMENTATION before including it provides the
/// implementation. Additionally, defining TABI_STRIP_PREFIX will
/// remove the 'tabi_' prefix from all public symbols.
///
/// Usually, you want to include this header into your build.tabi.c file
/// and write your build instructions there. There is no direct support for
/// splitting build instructions across multiple translation units yet although
/// you might of course use includes within your build.tabi.c file.

#include "tabi-internal.h"

/// Emit an informational message.
#define tabi_message(format, ...) tabi_internal_message (format, ##__VA_ARGS__)

/// Emit a warning message. This will not abort the build configuration
/// process.
#define tabi_warning(format, ...) tabi_internal_warning (format, ##__VA_ARGS__)

/// Emit an error message. This will also abort the build configuration
/// process.
#define tabi_error(format, ...) tabi_internal_error (format, ##__VA_ARGS__)

///
/// Create a compiler object that can be used to compile source files
///
TabiCompiler *tabi_compiler (const char *path, const char *flags);

///
/// Create a build target that compiles source files into an executable
///
TabiBuildTarget *tabi_executable (TabiCompiler *compiler,
                                  const char *executable_name);

///
/// Create a build target that compiles source files into object files
///
TabiBuildTarget *tabi_objects (TabiCompiler *compiler,
                               const char *target_name);

///
/// Add a source file to a build target
///
void tabi_add_source (TabiBuildTarget *target, const char *source_file);

///
/// Specify that a build target depends on another build target
///
void tabi_depends_on (TabiBuildTarget *target, TabiBuildTarget *dependency);

///
/// Define the main entry point for the tabi build instructions. This will
/// generate the necessary main function and call your user-defined entry
/// point. Pass a void(*)() that specifies what to build.
#define TABI_MAIN_ENTRY_POINT(user_entry_point)                               \
  TABI_INTERNAL_MAIN_ENTRY_POINT (user_entry_point)

/// Definitions if desired
#ifdef TABI_IMPLEMENTATION

TabiCompiler *
tabi_compiler (const char *path, const char *flags)
{
  TabiCompiler *compiler = tabi_internal_mem_calloc (&tabi_global_context.mem,
                                                    sizeof (TabiCompiler));
  compiler->base.type = TABI_OBJECT_TYPE_COMPILER;
  // TODO derive name from path
  compiler->name = tabi_internal_strdup (&tabi_global_context.mem, path);
  compiler->path = tabi_internal_strdup (&tabi_global_context.mem, path);
  compiler->flags = tabi_internal_strdup (&tabi_global_context.mem, flags);

  TABI_DYN_ARRAY_APPEND (&tabi_global_context.objects, TabiObject *,
                         (TabiObject *)compiler);

  return compiler;
}

TabiBuildTarget *
tabi_executable (TabiCompiler *compiler, const char *executable_name)
{
  TabiBuildTarget *target = tabi_internal_mem_calloc (&tabi_global_context.mem,
                                                     sizeof (TabiBuildTarget));

  target->base.type = TABI_OBJECT_TYPE_BUILD_TARGET;
  target->target_type = TABI_BUILD_TARGET_TYPE_EXECUTABLE;
  target->compiler = compiler;
  target->target_name
      = tabi_internal_strdup (&tabi_global_context.mem, executable_name);

  TABI_DYN_ARRAY_INIT (&target->source_files);

  TABI_DYN_ARRAY_APPEND (&tabi_global_context.objects, TabiObject *,
                         (TabiObject *)target);

  return target;
}

TabiBuildTarget *
tabi_objects (TabiCompiler *compiler, const char *target_name)
{
  TabiBuildTarget *target = tabi_internal_mem_calloc (&tabi_global_context.mem,
                                                     sizeof (TabiBuildTarget));
  target->base.type = TABI_OBJECT_TYPE_BUILD_TARGET;
  target->target_type = TABI_BUILD_TARGET_TYPE_OBJECTS;
  target->compiler = compiler;
  target->target_name
      = tabi_internal_strdup (&tabi_global_context.mem, target_name);

  TABI_DYN_ARRAY_INIT (&target->source_files);

  TABI_DYN_ARRAY_APPEND (&tabi_global_context.objects, TabiObject *,
                         (TabiObject *)target);
  return target;
}

void
tabi_add_source (TabiBuildTarget *target, const char *source_file)
{
  TABI_DYN_ARRAY_APPEND (
      &target->source_files, const char *,
      tabi_internal_strdup (&tabi_global_context.mem, source_file));
}

void
tabi_depends_on (TabiBuildTarget *target, TabiBuildTarget *dependency)
{
  if (dependency->target_type != TABI_BUILD_TARGET_TYPE_OBJECTS)
    tabi_error ("tabi_depends_on: Dependency must be of type OBJECTS");

  if (target->target_type != TABI_BUILD_TARGET_TYPE_EXECUTABLE)
    tabi_error ("tabi_depends_on: Target must be of type EXECUTABLE");

  // TODO: safety: prevent multiple inlcusion and cycles
  TABI_DYN_ARRAY_APPEND (&target->dependencies, TabiBuildTarget *, dependency);
}

#endif // TABI_IMPLEMENTATION

#ifdef TABI_STRIP_PREFIX

#define compiler tabi_compiler
#define executable tabi_executable
#define objects tabi_objects
#define add_source tabi_add_source
#define depends_on tabi_depends_on

#endif

#endif
