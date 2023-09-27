/*
 * Copyright (c) 2019 KhulnaSoft DevOps, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Helps to avoid conflicting declaration types of `__printf_chk`. */
#define printf printf_foo
#define vprintf vprintf_foo
#define fprintf fprintf_foo
#define vfprintf vfprintf_foo
#define __printf_chk __printf_chk_foo
#define __vprintf_chk __vprintf_chk_foo
#define __fprintf_chk __fprintf_chk_foo
#define __vfprintf_chk __vfprintf_chk_foo

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "deepfuzzy/DeepFuzzy.h"
#include "deepfuzzy/Log.h"

#undef printf
#undef vprintf
#undef fprintf
#undef vfprintf
#undef __printf_chk
#undef __vprintf_chk
#undef __fprintf_chk
#undef __vfprintf_chk

DEEPFUZZY_BEGIN_EXTERN_C

/* Returns a printable string version of the log level. */
static const char *DeepFuzzy_LogLevelStr(enum DeepFuzzy_LogLevel level) {
  switch (level) {
    case DeepFuzzy_LogDebug:
      return "DEBUG";
    case DeepFuzzy_LogTrace:
      return "TRACE";
    case DeepFuzzy_LogInfo:
      return "INFO";
    case DeepFuzzy_LogWarning:
      return "WARNING";
    case DeepFuzzy_LogError:
      return "ERROR";
    case DeepFuzzy_LogExternal:
      return "EXTERNAL";
    case DeepFuzzy_LogFatal:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}

enum {
  DeepFuzzy_LogBufSize = 4096
};

extern int DeepFuzzy_UsingLibFuzzer;
extern int DeepFuzzy_LibFuzzerLoud;

char DeepFuzzy_LogBuf[DeepFuzzy_LogBufSize + 1] = {};

/* Log a C string. */
DEEPFUZZY_NOINLINE
void DeepFuzzy_Log(enum DeepFuzzy_LogLevel level, const char *str) {
  if ((level < FLAGS_min_log_level) &&
      !(DeepFuzzy_UsingLibFuzzer && level == 0)) {
    return;
  }
  //Removed because I don't see why we need to zero this before writing
  //to it.
  //DeepFuzzy_MemScrub(DeepFuzzy_LogBuf, DeepFuzzy_LogBufSize);
  snprintf(DeepFuzzy_LogBuf, DeepFuzzy_LogBufSize, "%s: %s\n",
           DeepFuzzy_LogLevelStr(level), str);
  fputs(DeepFuzzy_LogBuf, stderr);

  if (DeepFuzzy_LogError == level) {
    DeepFuzzy_SoftFail();
  } else if (DeepFuzzy_LogFatal == level) {
    /* `DeepFuzzy_Fail()` calls `longjmp()`, so we need to make sure
     * we clean up the log buffer first. */
    DeepFuzzy_ClearStream(level);
    DeepFuzzy_Fail();
  }
}

/* Log some formatted output. */
DEEPFUZZY_NOINLINE
void DeepFuzzy_LogVFormat(enum DeepFuzzy_LogLevel level,
                          const char *format, va_list args) {
  struct DeepFuzzy_VarArgs va;
  va_copy(va.args, args);
  if (DeepFuzzy_UsingLibFuzzer && !DeepFuzzy_LibFuzzerLoud &&
      (level != DeepFuzzy_LogDebug)) {
    return;
  }
  DeepFuzzy_LogStream(level);
  DeepFuzzy_StreamVFormat(level, format, va.args);
  DeepFuzzy_LogStream(level);
}

/* Log some formatted output. */
DEEPFUZZY_NOINLINE
void DeepFuzzy_LogVFormatLLVM(enum DeepFuzzy_LogLevel level,
			      const char *format, va_list args) {
  struct DeepFuzzy_VarArgs va;
  va_copy(va.args, args);
  DeepFuzzy_LogStream(level);
  DeepFuzzy_StreamVFormat(level, format, va.args);
  DeepFuzzy_LogStream(level);
}

/* Log some formatted output. */
DEEPFUZZY_NOINLINE
void DeepFuzzy_LogFormat(enum DeepFuzzy_LogLevel level,
                         const char *format, ...) {
  va_list args;
  va_start(args, format);
  DeepFuzzy_LogVFormat(level, format, args);
  va_end(args);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"

/* Override libc! */
DEEPFUZZY_NOINLINE
int puts(const char *str) {
  DeepFuzzy_Log(DeepFuzzy_LogTrace, str);
  return 0;
}

DEEPFUZZY_NOINLINE
int printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  DeepFuzzy_LogVFormat(DeepFuzzy_LogTrace, format, args);
  va_end(args);
  return 0;
}

DEEPFUZZY_NOINLINE
int __printf_chk(int flag, const char *format, ...) {
  va_list args;
  va_start(args, format);
  DeepFuzzy_LogVFormat(DeepFuzzy_LogTrace, format, args);
  va_end(args);
  return 0;
}

DEEPFUZZY_NOINLINE
int vprintf(const char *format, va_list args) {
  DeepFuzzy_LogVFormat(DeepFuzzy_LogTrace, format, args);
  return 0;
}

DEEPFUZZY_NOINLINE
int __vprintf_chk(int flag, const char *format, va_list args) {
  DeepFuzzy_LogVFormat(DeepFuzzy_LogTrace, format, args);
  return 0;
}

DEEPFUZZY_NOINLINE
int vfprintf(FILE *file, const char *format, va_list args) {
  if (stderr == file) {
    DeepFuzzy_LogVFormat(DeepFuzzy_LogDebug, format, args);
  } else if (stdout == file) {
    DeepFuzzy_LogVFormat(DeepFuzzy_LogTrace, format, args);
  } else {
    DeepFuzzy_LogVFormat(DeepFuzzy_LogExternal, format, args);
  }
  /*
    Old code.  Now let's just log everything with odd dest as "external."

    if (!DeepFuzzy_UsingLibFuzzer) {
      if (strstr(format, "INFO:") != NULL) {
	// Assume such a string to an nonstd target is libFuzzer
	DeepFuzzy_LogVFormat(DeepFuzzy_LogExternal, format, args);
      } else {
	DeepFuzzy_LogStream(DeepFuzzy_LogWarning);
	DeepFuzzy_Log(DeepFuzzy_LogWarning,
		      "vfprintf with non-stdout/stderr stream follows:");
	DeepFuzzy_LogVFormat(DeepFuzzy_LogInfo, format, args);
      }
    } else {
      DeepFuzzy_LogVFormat(DeepFuzzy_LogExternal, format, args);
    }
  */
  return 0;
}

DEEPFUZZY_NOINLINE
int fprintf(FILE *file, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(file, format, args);
  va_end(args);
  return 0;
}

DEEPFUZZY_NOINLINE
int __fprintf_chk(int flag, FILE *file, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(file, format, args);
  va_end(args);
  return 0;
}

DEEPFUZZY_NOINLINE
int __vfprintf_chk(int flag, FILE *file, const char *format, va_list args) {
  vfprintf(file, format, args);
  return 0;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop

DEEPFUZZY_END_EXTERN_C
