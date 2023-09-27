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

#ifndef SRC_INCLUDE_DEEPFUZZY_OPTION_H_
#define SRC_INCLUDE_DEEPFUZZY_OPTION_H_

#include "deepfuzzy/Compiler.h"

#include <stdint.h>

#define DEEPFUZZY_FLAG_NAME(name) FLAGS_ ## name
#define DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name) HAS_FLAG_ ## name

#define DEEPFUZZY_REGISTER_OPTION(name, group, parser, docstring) \
  static struct DeepFuzzy_Option DeepFuzzy_Option_ ## name = { \
      NULL, \
      DEEPFUZZY_TO_STR(name), \
      DEEPFUZZY_TO_STR(no_ ## name), \
      group, \
      &parser, \
      (void *) &DEEPFUZZY_FLAG_NAME(name), \
      &DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name), \
      docstring, \
  }; \
  DEEPFUZZY_INITIALIZER(DeepFuzzy_AddOption_ ## name) { \
    DeepFuzzy_AddOption(&(DeepFuzzy_Option_ ## name)); \
  }

#define DEFINE_string(name, group, default_value, docstring) \
  DECLARE_string(name); \
  DEEPFUZZY_REGISTER_OPTION(name, group, DeepFuzzy_ParseStringOption, docstring) \
  int DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name) = 0; \
  const char *DEEPFUZZY_FLAG_NAME(name) = default_value

#define DECLARE_string(name) \
  extern int DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name); \
  extern const char *DEEPFUZZY_FLAG_NAME(name)

#define DEFINE_bool(name, group, default_value, docstring) \
  DECLARE_bool(name); \
  DEEPFUZZY_REGISTER_OPTION(name, group, DeepFuzzy_ParseBoolOption, docstring) \
  int DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name) = 0; \
  int DEEPFUZZY_FLAG_NAME(name) = default_value

#define DECLARE_bool(name) \
  extern int DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name); \
  extern int DEEPFUZZY_FLAG_NAME(name)

#define DEFINE_int(name, group, default_value, docstring) \
  DECLARE_int(name); \
  DEEPFUZZY_REGISTER_OPTION(name, group, DeepFuzzy_ParseIntOption, docstring) \
  int DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name) = 0; \
  int DEEPFUZZY_FLAG_NAME(name) = default_value

#define DECLARE_int(name) \
  extern int DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name); \
  extern int DEEPFUZZY_FLAG_NAME(name)

#define DEFINE_uint(name, group, default_value, docstring) \
  DECLARE_uint(name); \
  DEEPFUZZY_REGISTER_OPTION(name, group, DeepFuzzy_ParseUIntOption, docstring) \
  int DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name) = 0; \
  unsigned DEEPFUZZY_FLAG_NAME(name) = default_value

#define DECLARE_uint(name) \
  extern int DEEPFUZZY_HAS_DEEPFUZZY_FLAG_NAME(name); \
  extern unsigned DEEPFUZZY_FLAG_NAME(name)
DEEPFUZZY_BEGIN_EXTERN_C

/* Enum for defining command-line groups that options can reside under */
typedef enum {
  InputOutputGroup,
  AnalysisGroup,
  ExecutionGroup,
  TestSelectionGroup,
  MiscGroup,
} DeepFuzzy_OptGroup;

/* Backing structure for describing command-line options to DeepFuzzy. */
struct DeepFuzzy_Option {
  struct DeepFuzzy_Option *next;
  const char * const name;
  const char * const alt_name;  /* Only used for booleans. */
  DeepFuzzy_OptGroup group;
  void (* const parse)(struct DeepFuzzy_Option *);
  void * const value;
  int * const has_value;
  const char * const docstring;
};

extern int DeepFuzzy_OptionsAreInitialized;

/* Initialize the options from the command-line arguments. */
void DeepFuzzy_InitOptions(int argc, ... /* const char **argv */);

/* Works for `--help` option: print out each options along with
 * their documentation. */
void DeepFuzzy_PrintAllOptions(const char *prog_name);

/* Initialize an option. */
void DeepFuzzy_AddOption(struct DeepFuzzy_Option *option);

/* Parse an option that is a string. */
void DeepFuzzy_ParseStringOption(struct DeepFuzzy_Option *option);

/* Parse an option that will be interpreted as a boolean value. */
void DeepFuzzy_ParseBoolOption(struct DeepFuzzy_Option *option);

/* Parse an option that will be interpreted as an integer. */
void DeepFuzzy_ParseIntOption(struct DeepFuzzy_Option *option);

/* Parse an option that will be interpreted as an unsigned integer. */
void DeepFuzzy_ParseUIntOption(struct DeepFuzzy_Option *option);

DEEPFUZZY_END_EXTERN_C

#endif  /* SRC_INCLUDE_DEEPFUZZY_OPTION_H_ */
