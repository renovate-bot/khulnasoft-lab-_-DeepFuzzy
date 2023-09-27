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

#ifndef SRC_INCLUDE_DEEPFUZZY_LOG_H_
#define SRC_INCLUDE_DEEPFUZZY_LOG_H_

#include <stdarg.h>

#include <deepfuzzy/Compiler.h>

DEEPFUZZY_BEGIN_EXTERN_C

extern int DeepFuzzy_UsingLibFuzzer;
extern int DeepFuzzy_UsingSymExec;

struct DeepFuzzy_Stream;

struct DeepFuzzy_VarArgs {
  va_list args;
};

enum DeepFuzzy_LogLevel {
  DeepFuzzy_LogDebug = 0,
  DeepFuzzy_LogTrace = 1,  
  DeepFuzzy_LogInfo = 2,  
  DeepFuzzy_LogWarning = 3,
  DeepFuzzy_LogWarn = DeepFuzzy_LogWarning,
  DeepFuzzy_LogError = 4,
  DeepFuzzy_LogExternal = 5,
  DeepFuzzy_LogFatal = 6,
  DeepFuzzy_LogCritical = DeepFuzzy_LogFatal,
};

/* Log a C string. */
extern void DeepFuzzy_Log(enum DeepFuzzy_LogLevel level, const char *str);

/* Log some formatted output. */
extern void DeepFuzzy_LogFormat(enum DeepFuzzy_LogLevel level,
                                const char *format, ...);

/* Log some formatted output. */
extern void DeepFuzzy_LogVFormat(enum DeepFuzzy_LogLevel level,
                                 const char *format, va_list args);

DEEPFUZZY_END_EXTERN_C

#endif  /* SRC_INCLUDE_DEEPFUZZY_LOG_H_ */
