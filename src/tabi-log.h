/// tabi - A minimal build system library for C/C++ projects
///
/// Copyright (c) 2025 Sebastian Proell
///
/// SPDX-License-Identifier: MIT

/// Internal header for the tabi build system
#ifndef TABI_LOG_H
#define TABI_LOG_H

#include "tabi-types.h"

#include "stdarg.h"
#include "stdio.h"

typedef enum
{
  TABI_LOG_TYPE_INFO,
  TABI_LOG_TYPE_WARNING,
  TABI_LOG_TYPE_ERROR,
} TabiLogType;

void tabi_internal_log (TabiLogType type, const char *format, ...);

#define tabi_internal_message(format, ...)                                    \
  tabi_internal_log (TABI_LOG_TYPE_INFO, format, ##__VA_ARGS__)

#define tabi_internal_warning(format, ...)                                    \
  tabi_internal_log (TABI_LOG_TYPE_WARNING, format, ##__VA_ARGS__)

#define tabi_internal_error(format, ...)                                      \
  do                                                                          \
    {                                                                         \
      tabi_internal_log (TABI_LOG_TYPE_ERROR, format, ##__VA_ARGS__);         \
      abort ();                                                               \
    }                                                                         \
  while (0)

/// Definitions if desired
#ifdef TABI_IMPLEMENTATION

void
tabi_internal_log (TabiLogType type, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  switch (type)
    {
    case TABI_LOG_TYPE_INFO:
      vprintf (format, args);
      printf ("\n");
      break;
    case TABI_LOG_TYPE_WARNING:
      fprintf (stderr, "[Warn] ");
      vfprintf (stderr, format, args);
      fprintf (stderr, "\n");
      break;
    case TABI_LOG_TYPE_ERROR:
      fprintf (stderr, "[Error] ");
      vfprintf (stderr, format, args);
      fprintf (stderr, "\n");
      break;
    }
  va_end (args);
}

#endif

#endif
