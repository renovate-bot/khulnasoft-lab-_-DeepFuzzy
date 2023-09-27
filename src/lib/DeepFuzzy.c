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

#include "deepfuzzy/DeepFuzzy.h"
#include "deepfuzzy/Option.h"
#include "deepfuzzy/Log.h"
#include "DeepFuzzy.h"

#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEEPFUZZY_TAKEOVER_RAND
#undef rand
#undef srand
#endif

DEEPFUZZY_BEGIN_EXTERN_C

/* Basic input and output options, specifies files for read/write before and after test analysis */
DEFINE_string(input_test_dir, InputOutputGroup, "", "Directory of saved tests to run.");
DEFINE_string(input_test_file, InputOutputGroup, "", "Saved test to run.");
DEFINE_string(input_test_files_dir, InputOutputGroup, "", "Directory of saved test files to run (flat structure).");
DEFINE_string(output_test_dir, InputOutputGroup, "", "Directory where tests will be saved.");
DEFINE_bool(input_stdin, InputOutputGroup, false, "Run a test from stdin.");

/* Test execution-related options, configures how an execution run is carried out */
DEFINE_bool(take_over, ExecutionGroup, false, "Replay test cases in take-over mode.");
DEFINE_bool(abort_on_fail, ExecutionGroup, false, "Abort on file replay failure (useful in file fuzzing).");
DEFINE_bool(exit_on_fail, ExecutionGroup, false, "Exit with status 255 on test failure.");
DEFINE_bool(verbose_reads, ExecutionGroup, false, "Report on bytes being read during execution of test.");
DEFINE_int(min_log_level, ExecutionGroup, 0, "Minimum level of logging to output (default 0, 0=debug, 1=trace, 2=info, ...).");
DEFINE_int(timeout, ExecutionGroup, 3600, "Timeout for brute force fuzzing.");
DEFINE_uint(num_workers, ExecutionGroup, 1, "Number of workers to spawn for testing and test generation.");
#if defined(_WIN32) || defined(_MSC_VER)
DEFINE_bool(direct_run, ExecutionGroup, false, "Run test function directly.");
#endif

/* Fuzzing and symex related options, baked in to perform analysis-related tasks without auxiliary tools */
DEFINE_bool(fuzz, AnalysisGroup, false, "Perform brute force unguided fuzzing.");
DEFINE_bool(fuzz_save_passing, AnalysisGroup, false, "Save passing tests during fuzzing.");
DEFINE_bool(fork, AnalysisGroup, true, "Fork when running a test.");
DEFINE_int(seed, AnalysisGroup, 0, "Seed for brute force fuzzing (uses time if not set).");

/* Test selection options to configure what test or tests should be executed during a run */
DEFINE_string(input_which_test, TestSelectionGroup, "", "Test to use with --input_test_file or --input_test_files_dir.");
DEFINE_string(test_filter, TestSelectionGroup, "", "Run all tests matched with wildcard pattern.");
DEFINE_bool(list_tests, TestSelectionGroup, false, "List all available tests instead of running tests.");
DEFINE_bool(boring_only, TestSelectionGroup, false, "Run Boring concrete tests only.");
DEFINE_bool(run_disabled, TestSelectionGroup, false, "Run Disabled tests alongside other tests.");

/* Set to 1 by Manticore/Angr/etc. when we're running symbolically. */
int DeepFuzzy_UsingSymExec = 0;

/* Set to 1 when we're using libFuzzer. */
int DeepFuzzy_UsingLibFuzzer = 0;

/* To make libFuzzer louder on mac OS. */
int DeepFuzzy_LibFuzzerLoud = 0;

/* Array of DeepFuzzy generated allocations.  Impossible for there to
 * be more than there are input bytes.  Index stores where we are. */
char* DeepFuzzy_GeneratedAllocs[DeepFuzzy_InputSize];
uint32_t DeepFuzzy_GeneratedAllocsIndex = 0;

/* Pointer to the last registers DeepFuzzy_TestInfo data structure */
struct DeepFuzzy_TestInfo *DeepFuzzy_LastTestInfo = NULL;

/* Pointer to structure for ordered DeepFuzzy_TestInfo */
struct DeepFuzzy_TestInfo *DeepFuzzy_FirstTestInfo = NULL;

/* Pointer to the test being run in this process by Dr. Fuzz. */
static struct DeepFuzzy_TestInfo *DeepFuzzy_DrFuzzTest = NULL;

/* Initialize global input buffer and index / initialized index. */
volatile uint8_t DeepFuzzy_Input[DeepFuzzy_InputSize] = {};
uint32_t DeepFuzzy_InputIndex = 0;
uint32_t DeepFuzzy_InputInitialized = 0;

/* Used if we need to generate on-the-fly data while we fuzz */
uint32_t DeepFuzzy_InternalFuzzing = 0;

/* Swarm related state. */
uint32_t DeepFuzzy_SwarmConfigsIndex = 0;
struct DeepFuzzy_SwarmConfig *DeepFuzzy_SwarmConfigs[DEEPFUZZY_MAX_SWARM_CONFIGS];

/* Jump buffer for returning to `DeepFuzzy_Run`. */
jmp_buf DeepFuzzy_ReturnToRun = {};

/* Information about the current test run, if any. */
extern struct DeepFuzzy_TestRunInfo *DeepFuzzy_CurrentTestRun = NULL;

static void DeepFuzzy_SetTestPassed(void) {
  DeepFuzzy_CurrentTestRun->result = DeepFuzzy_TestRunPass;
}

static void DeepFuzzy_SetTestFailed(void) {
  DeepFuzzy_CurrentTestRun->result = DeepFuzzy_TestRunFail;
}

static void DeepFuzzy_SetTestAbandoned(const char *reason) {
  DeepFuzzy_CurrentTestRun->result = DeepFuzzy_TestRunAbandon;
  DeepFuzzy_CurrentTestRun->reason = reason;
}

void DeepFuzzy_InitCurrentTestRun(struct DeepFuzzy_TestInfo *test) {
  DeepFuzzy_CurrentTestRun->test = test;
  DeepFuzzy_CurrentTestRun->result = DeepFuzzy_TestRunPass;
  DeepFuzzy_CurrentTestRun->reason = NULL;
}

/* Abandon this test. We've hit some kind of internal problem. */
DEEPFUZZY_NORETURN
void DeepFuzzy_Abandon(const char *reason) {
  DeepFuzzy_Log(DeepFuzzy_LogError, reason);

  DeepFuzzy_CurrentTestRun->result = DeepFuzzy_TestRunAbandon;
  DeepFuzzy_CurrentTestRun->reason = reason;

  longjmp(DeepFuzzy_ReturnToRun, 1);
}

/* Abandon this test due to failed assumption. Less important to log. */
DEEPFUZZY_NORETURN
void DeepFuzzy_Abandon_Due_to_Assumption(const char *reason) {
  DeepFuzzy_CurrentTestRun->result = DeepFuzzy_TestRunAbandon;
  DeepFuzzy_CurrentTestRun->reason = reason;

  longjmp(DeepFuzzy_ReturnToRun, 1);
}

/* Mark this test as having crashed. */
void DeepFuzzy_Crash(void) {
  DeepFuzzy_SetTestFailed();
}

/* Mark this test as failing. */
DEEPFUZZY_NORETURN
void DeepFuzzy_Fail(void) {
  DeepFuzzy_SetTestFailed();

  if (FLAGS_take_over) {
    // We want to communicate the failure to a parent process, so exit.
    exit(DeepFuzzy_TestRunFail);
  } else {
    longjmp(DeepFuzzy_ReturnToRun, 1);
  }
}

/* Mark this test as passing. */
DEEPFUZZY_NORETURN
void DeepFuzzy_Pass(void) {
  longjmp(DeepFuzzy_ReturnToRun, 0);
}

void DeepFuzzy_SoftFail(void) {
  DeepFuzzy_SetTestFailed();
}

/* Symbolize the data in the exclusive range `[begin, end)`. */
void DeepFuzzy_SymbolizeData(void *begin, void *end) {
  uintptr_t begin_addr = (uintptr_t) begin;
  uintptr_t end_addr = (uintptr_t) end;

  if (begin_addr > end_addr) {
    DeepFuzzy_Abandon("Invalid data bounds for DeepFuzzy_SymbolizeData");
  } else if (begin_addr == end_addr) {
    return;
  } else {
    uint8_t *bytes = (uint8_t *) begin;
    for (uintptr_t i = 0, max_i = (end_addr - begin_addr); i < max_i; ++i) {
      if (DeepFuzzy_InputIndex >= DeepFuzzy_InputSize) {
        DeepFuzzy_Abandon("Exceeded set input limit. Set or expand DEEPFUZZY_SIZE to write more bytes.");
      }
      if (FLAGS_verbose_reads) {
        printf("Reading byte at %u\n", DeepFuzzy_InputIndex);
      }
      bytes[i] = DEEPFUZZY_READBYTE;
    }
  }
}

/* Symbolize the data in the exclusive range `[begin, end)` without null
 * characters included.  Primarily useful for C strings. */
void DeepFuzzy_SymbolizeDataNoNull(void *begin, void *end) {
  uintptr_t begin_addr = (uintptr_t) begin;
  uintptr_t end_addr = (uintptr_t) end;

  if (begin_addr > end_addr) {
    DeepFuzzy_Abandon("Invalid data bounds for DeepFuzzy_SymbolizeData");
  } else if (begin_addr == end_addr) {
    return;
  } else {
    uint8_t *bytes = (uint8_t *) begin;
    for (uintptr_t i = 0, max_i = (end_addr - begin_addr); i < max_i; ++i) {
      if (DeepFuzzy_InputIndex >= DeepFuzzy_InputSize) {
        DeepFuzzy_Abandon("Exceeded set input limit. Set or expand DEEPFUZZY_SIZE to write more bytes.");
      }
      if (FLAGS_verbose_reads) {
        printf("Reading byte at %u\n", DeepFuzzy_InputIndex);
      }
      bytes[i] = DEEPFUZZY_READBYTE;
      if (bytes[i] == 0) {
        bytes[i] = 1;
      }
    }
  }
}

/* Concretize some data in exclusive the range `[begin, end)`. */
void *DeepFuzzy_ConcretizeData(void *begin, void *end) {
  return begin;
}

/* Assign a symbolic C string of strlen length `len`.  str should include
 * storage for both `len` characters AND the null terminator.  Allowed
 * is a set of chars that are allowed (ignored if null). */
void DeepFuzzy_AssignCStr_C(char* str, size_t len, const char* allowed) {
  if (SIZE_MAX <= len) {
    DeepFuzzy_Abandon("Can't create a SIZE_MAX-length string.");
  }
  if (NULL == str) {
    DeepFuzzy_Abandon("Attempted to populate null pointer.");
  }
  if (len) {
    if (allowed == 0) {
      DeepFuzzy_SymbolizeDataNoNull(str, &(str[len]));
    } else {
      uint32_t allowed_size = strlen(allowed);
      for (int i = 0; i < len; i++) {
        str[i] = allowed[DeepFuzzy_UIntInRange(0, allowed_size-1)];
      }
    }
  }
  str[len] = '\0';
}

void DeepFuzzy_SwarmAssignCStr_C(const char* file, unsigned line, int stype,
				 char* str, size_t len, const char* allowed) {
  if (SIZE_MAX <= len) {
    DeepFuzzy_Abandon("Can't create a SIZE_MAX-length string.");
  }
  if (NULL == str) {
    DeepFuzzy_Abandon("Attempted to populate null pointer.");
  }
  char swarm_allowed[256];  
  if (allowed == 0) {
    /* In swarm mode, if there is no allowed string, create one over all chars. */
    for (int i = 0; i < 255; i++) {
      swarm_allowed[i] = i+1;
    }
    swarm_allowed[255] = 0;
    allowed = (const char*)&swarm_allowed;
  }
  if (len) {
    uint32_t allowed_size = strlen(allowed);
    struct DeepFuzzy_SwarmConfig* sc = DeepFuzzy_GetSwarmConfig(allowed_size, file, line, stype);
    for (int i = 0; i < len; i++) {
      str[i] = allowed[sc->fmap[DeepFuzzy_UIntInRange(0U, sc->fcount-1)]];
    }
  }
  str[len] = '\0';
}

/* Return a symbolic C string of strlen `len`. */
char *DeepFuzzy_CStr_C(size_t len, const char* allowed) {
  if (SIZE_MAX <= len) {
    DeepFuzzy_Abandon("Can't create a SIZE_MAX-length string");
  }
  char *str = (char *) malloc(sizeof(char) * (len + 1));
  if (NULL == str) {
    DeepFuzzy_Abandon("Can't allocate memory");
  }
  DeepFuzzy_GeneratedAllocs[DeepFuzzy_GeneratedAllocsIndex++] = str;
  if (len) {
    if (allowed == 0) {
      DeepFuzzy_SymbolizeDataNoNull(str, &(str[len]));
    } else {
      uint32_t allowed_size = strlen(allowed);
      for (int i = 0; i < len; i++) {
        str[i] = allowed[DeepFuzzy_UIntInRange(0, allowed_size-1)];
      }
    }
  }
  str[len] = '\0';
  return str;
}

char *DeepFuzzy_SwarmCStr_C(const char* file, unsigned line, int stype,
			    size_t len, const char* allowed) {
  if (SIZE_MAX <= len) {
    DeepFuzzy_Abandon("Can't create a SIZE_MAX-length string");
  }
  char *str = (char *) malloc(sizeof(char) * (len + 1));
  if (NULL == str) {
    DeepFuzzy_Abandon("Can't allocate memory");
  }
  char swarm_allowed[256];  
  if (allowed == 0) {
    /* In swarm mode, if there is no allowed string, create one over all chars. */
    for (int i = 0; i < 255; i++) {
      swarm_allowed[i] = i+1;
    }
    swarm_allowed[255] = 0;
    allowed = (const char*)&swarm_allowed;
  }
  DeepFuzzy_GeneratedAllocs[DeepFuzzy_GeneratedAllocsIndex++] = str;
  if (len) {
    uint32_t allowed_size = strlen(allowed);
    struct DeepFuzzy_SwarmConfig* sc = DeepFuzzy_GetSwarmConfig(allowed_size, file, line, stype);
    for (int i = 0; i < len; i++) {
      str[i] = allowed[sc->fmap[DeepFuzzy_UIntInRange(0U, sc->fcount-1)]];
    }
  }
  str[len] = '\0';
  return str;
}

/* Symbolize a C string; keeps the null terminator where it was. */
void DeepFuzzy_SymbolizeCStr_C(char *begin, const char* allowed) {
  if (begin && begin[0]) {
    if (allowed == 0) {
      DeepFuzzy_SymbolizeDataNoNull(begin, begin + strlen(begin));
    } else {
      uint32_t allowed_size = strlen(allowed);
      uint8_t *bytes = (uint8_t *) begin;
      uintptr_t begin_addr = (uintptr_t) begin;
      uintptr_t end_addr = (uintptr_t) (begin + strlen(begin));
      for (uintptr_t i = 0, max_i = (end_addr - begin_addr); i < max_i; ++i) {
        bytes[i] = allowed[DeepFuzzy_UIntInRange(0, allowed_size-1)];
      }
    }
  }
}

void DeepFuzzy_SwarmSymbolizeCStr_C(const char* file, unsigned line, int stype,
				    char *begin, const char* allowed) {
  if (begin && begin[0]) {
    char swarm_allowed[256];    
    if (allowed == 0) {
      /* In swarm mode, if there is no allowed string, create one over all chars. */
      for (int i = 0; i < 255; i++) {
	swarm_allowed[i] = i+1;
      }
      swarm_allowed[255] = 0;
      allowed = (const char*)&swarm_allowed;      
    }
    uint32_t allowed_size = strlen(allowed);
    struct DeepFuzzy_SwarmConfig* sc = DeepFuzzy_GetSwarmConfig(allowed_size, file, line, stype);
    uint8_t *bytes = (uint8_t *) begin;
    uintptr_t begin_addr = (uintptr_t) begin;
    uintptr_t end_addr = (uintptr_t) (begin + strlen(begin));
    for (uintptr_t i = 0, max_i = (end_addr - begin_addr); i < max_i; ++i) {
      bytes[i] = allowed[sc->fmap[DeepFuzzy_UIntInRange(0U, sc->fcount-1)]];
    }
  }
}

/* Concretize a C string */
const char *DeepFuzzy_ConcretizeCStr(const char *begin) {
  return begin;
}

/* Allocate and return a pointer to `num_bytes` symbolic bytes. */
void *DeepFuzzy_Malloc(size_t num_bytes) {
  void *data = malloc(num_bytes);
  uintptr_t data_end = ((uintptr_t) data) + num_bytes;
  DeepFuzzy_SymbolizeData(data, (void *) data_end);
  return data;
}

/* Allocate and return a pointer to `num_bytes` symbolic bytes. */
void *DeepFuzzy_GCMalloc(size_t num_bytes) {
  void *data = malloc(num_bytes);
  uintptr_t data_end = ((uintptr_t) data) + num_bytes;
  DeepFuzzy_SymbolizeData(data, (void *) data_end);
  DeepFuzzy_GeneratedAllocs[DeepFuzzy_GeneratedAllocsIndex++] = data;
  return data;
}

/* Portable and architecture-independent memory scrub without dead store elimination. */
void *DeepFuzzy_MemScrub(void *pointer, size_t data_size) {
  volatile unsigned char *p = pointer;
  while (data_size--) {
    *p++ = 0;
  }
  return pointer;
}

/* Generate a new swarm configuration. */
struct DeepFuzzy_SwarmConfig *DeepFuzzy_NewSwarmConfig(unsigned fcount, const char* file, unsigned line,
						       enum DeepFuzzy_SwarmType stype) {
  struct DeepFuzzy_SwarmConfig *new_config = malloc(sizeof(struct DeepFuzzy_SwarmConfig));
  size_t buf_len = strlen(file) + 1;
  new_config->file = malloc(buf_len);
  strncpy(new_config->file, file, buf_len);
  new_config->line = line;
  new_config->orig_fcount = fcount;
  new_config->fcount = 0;
  if (stype == DeepFuzzy_SwarmTypeProb) {
    new_config->fmap = malloc(sizeof(unsigned) * fcount * DEEPFUZZY_SWARM_MAX_PROB_RATIO);
    for (int i = 0; i < fcount; i++) {
      unsigned int prob = DeepFuzzy_UIntInRange(0U, DEEPFUZZY_SWARM_MAX_PROB_RATIO);
      for (int j = 0; j < prob; j++) {
	new_config->fmap[new_config->fcount++] = i;
      }
    }
    if (new_config->fcount == 0) {
      new_config->fmap[new_config->fcount++] = DeepFuzzy_UIntInRange(0, fcount-1);
    }
  } else {
    new_config->fmap = malloc(sizeof(unsigned) * fcount);
    /* In mix mode, "half" the time just use everything */
    int full_config = (stype == DeepFuzzy_SwarmTypeMixed) && DeepFuzzy_Bool();
    if ((stype == DeepFuzzy_SwarmTypeMixed) && DeepFuzzy_UsingSymExec) {
      /* We don't want to make additional pointless paths to explore for symex */
      (void) DeepFuzzy_Assume(full_config);
    }
    for (int i = 0; i < fcount; i++) {
      if (full_config) {
	new_config->fmap[new_config->fcount++] = i;
      } else {
	int in_swarm = DeepFuzzy_Bool();
	if (DeepFuzzy_UsingSymExec) {
	  /* If not in mix mode, just allow everything in each configuration for symex */
	  (void) DeepFuzzy_Assume(in_swarm);
	}
	if (in_swarm) {
	  new_config->fmap[new_config->fcount++] = i;
	}
      }
    }
  }
  /* We always need to allow at least one option! */
  if (new_config->fcount == 0) {
    new_config->fmap[new_config->fcount++] = DeepFuzzy_UIntInRange(0, fcount-1);
  }
  return new_config;
}

/* Either fetch existing configuration, or generate a new one. */
struct DeepFuzzy_SwarmConfig *DeepFuzzy_GetSwarmConfig(unsigned fcount, const char* file, unsigned line,
						       enum DeepFuzzy_SwarmType stype) {
  /* In general, there should be few enough OneOfs in a harness that linear search is fine. */
  for (int i = 0; i < DeepFuzzy_SwarmConfigsIndex; i++) {
    struct DeepFuzzy_SwarmConfig* sc = DeepFuzzy_SwarmConfigs[i];
    if ((sc->line == line) && (sc->orig_fcount == fcount) && (strncmp(sc->file, file, strlen(file)) == 0)) {
      return sc;
    }
  }
  if (DeepFuzzy_SwarmConfigsIndex == DEEPFUZZY_MAX_SWARM_CONFIGS) {
    DeepFuzzy_Abandon("Exceeded swarm config limit. Set or expand DEEPFUZZY_MAX_SWARM_CONFIGS. This is highly unusual.");
  }
  DeepFuzzy_SwarmConfigs[DeepFuzzy_SwarmConfigsIndex] = DeepFuzzy_NewSwarmConfig(fcount, file, line, stype);
  return DeepFuzzy_SwarmConfigs[DeepFuzzy_SwarmConfigsIndex++];
}

DEEPFUZZY_NOINLINE int DeepFuzzy_One(void) {
  return 1;
}

DEEPFUZZY_NOINLINE int DeepFuzzy_Zero(void) {
  return 0;
}

/* Always returns `0`. */
int DeepFuzzy_ZeroSink(int sink) {
  (void) sink;
  return 0;
}

/* Returns `1` if `expr` is true, and `0` otherwise. This is kind of an indirect
 * way to take a symbolic value, introduce a fork, and on each size, replace its
* value with a concrete value. */
int DeepFuzzy_IsTrue(int expr) {
  if (expr == DeepFuzzy_Zero()) {
    return DeepFuzzy_Zero();
  } else {
    return DeepFuzzy_One();
  }
}

/* Return a symbolic value of a given type. */
int DeepFuzzy_Bool(void) {
  if (DeepFuzzy_InputIndex >= DeepFuzzy_InputSize) {
    DeepFuzzy_Abandon("Exceeded set input limit. Set or expand DEEPFUZZY_SIZE to write more bytes.");
  }
  if (FLAGS_verbose_reads) {
    printf("Reading byte as boolean at %u\n", DeepFuzzy_InputIndex);
  }
  return DEEPFUZZY_READBYTE & 1;
}



#define MAKE_SYMBOL_FUNC(Type, type) \
    type DeepFuzzy_ ## Type(void) { \
      if ((DeepFuzzy_InputIndex + sizeof(type)) > DeepFuzzy_InputSize) { \
        DeepFuzzy_Abandon("Exceeded set input limit. Set or expand DEEPFUZZY_SIZE to write more bytes."); \
      } \
      type val = 0; \
      if (FLAGS_verbose_reads) { \
        printf("STARTING MULTI-BYTE READ\n"); \
      } \
      _Pragma("unroll") \
      for (size_t i = 0; i < sizeof(type); ++i) { \
        if (FLAGS_verbose_reads) { \
          printf("Reading byte at %u\n", DeepFuzzy_InputIndex); \
        } \
        val = (val << 8) | ((type) DEEPFUZZY_READBYTE); \
      } \
      if (FLAGS_verbose_reads) { \
        printf("FINISHED MULTI-BYTE READ\n"); \
      } \
      return val; \
    }


MAKE_SYMBOL_FUNC(Size, size_t)
MAKE_SYMBOL_FUNC(Long, long)

float DeepFuzzy_Float(void) {
  float float_v;
  DeepFuzzy_SymbolizeData(&float_v, &float_v + 1);
  return float_v;
}

double DeepFuzzy_Double(void) {
  double double_v;
  DeepFuzzy_SymbolizeData(&double_v, &double_v + 1);
  return double_v;
}

MAKE_SYMBOL_FUNC(UInt64, uint64_t)
int64_t DeepFuzzy_Int64(void) {
  return (int64_t) DeepFuzzy_UInt64();
}

MAKE_SYMBOL_FUNC(UInt, unsigned)
int DeepFuzzy_Int(void) {
  return (int) DeepFuzzy_UInt();
}

MAKE_SYMBOL_FUNC(UShort, unsigned short)
short DeepFuzzy_Short(void) {
  return (short) DeepFuzzy_UShort();
}

MAKE_SYMBOL_FUNC(UChar, unsigned char)
char DeepFuzzy_Char(void) {
  return (char) DeepFuzzy_UChar();
}

#undef MAKE_SYMBOL_FUNC

float DeepFuzzy_FloatInRange(float low, float high) {
  if (low > high) {
    return DeepFuzzy_FloatInRange(high, low);
  }
  if (low < 0.0) { // Handle negatives differently
    if (high > 0.0) {
      if (DeepFuzzy_Bool()) {
        return -(DeepFuzzy_FloatInRange(0.0, -low));
      } else {
        return DeepFuzzy_FloatInRange(0.0, high);
      }
    } else {
      return -(DeepFuzzy_FloatInRange(-high, -low));
    }
  }
  int32_t int_v = DeepFuzzy_IntInRange(*(int32_t *)&low, *(int32_t *)&high);
  float float_v = *(float*)&int_v;
  assume (float_v >= low);
  assume (float_v <= high);
  return float_v;
}

double DeepFuzzy_DoubleInRange(double low, double high) {
  if (low > high) {
    return DeepFuzzy_DoubleInRange(high, low);
  }
  if (low < 0.0) { // Handle negatives differently
    if (high > 0.0) {
      if (DeepFuzzy_Bool()) {
        return -(DeepFuzzy_DoubleInRange(0.0, -low));
      } else {
        return DeepFuzzy_DoubleInRange(0.0, high);
      }
    } else {
      return -(DeepFuzzy_DoubleInRange(-high, -low));
    }
  }
  int64_t int_v = DeepFuzzy_Int64InRange(*(int64_t *)&low, *(int64_t *)&high);
  double double_v = *(double*)&int_v;
  assume (double_v >= low);
  assume (double_v <= high);
  return double_v;
}

int32_t DeepFuzzy_RandInt() {
  return DeepFuzzy_IntInRange(0, RAND_MAX);
}

/* Returns the minimum satisfiable value for a given symbolic value, given
 * the constraints present on that value. */
uint32_t DeepFuzzy_MinUInt(uint32_t v) {
  return v;
}

int32_t DeepFuzzy_MinInt(int32_t v) {
  return (int32_t) (DeepFuzzy_MinUInt(((uint32_t) v) + 0x80000000U) -
                    0x80000000U);
}

/* Returns the maximum satisfiable value for a given symbolic value, given
 * the constraints present on that value. */
uint32_t DeepFuzzy_MaxUInt(uint32_t v) {
  return v;
}

int32_t DeepFuzzy_MaxInt(int32_t v) {
  return (int32_t) (DeepFuzzy_MaxUInt(((uint32_t) v) + 0x80000000U) -
                    0x80000000U);
}

/* Function to clean up generated strings, and any other DeepFuzzy-managed data. */
extern void DeepFuzzy_CleanUp() {
  for (int i = 0; i < DeepFuzzy_GeneratedAllocsIndex; i++) {
    free(DeepFuzzy_GeneratedAllocs[i]);
  }
  DeepFuzzy_GeneratedAllocsIndex = 0;
  
  for (int i = 0; i < DeepFuzzy_SwarmConfigsIndex; i++) {
    free(DeepFuzzy_SwarmConfigs[i]->file);
    free(DeepFuzzy_SwarmConfigs[i]->fmap);
    free(DeepFuzzy_SwarmConfigs[i]);
  }
  DeepFuzzy_SwarmConfigsIndex = 0;
}

void _DeepFuzzy_Assume(int expr, const char *expr_str, const char *file,
                       unsigned line) {
  if (!expr) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogTrace,
                        "%s(%u): Assumption %s failed",
                        file, line, expr_str);
    DeepFuzzy_Abandon_Due_to_Assumption("Assumption failed");
  }
}

int DeepFuzzy_IsSymbolicUInt(uint32_t x) {
  (void) x;
  return 0;
}

/* Defined in Stream.c */
extern void _DeepFuzzy_StreamInt(enum DeepFuzzy_LogLevel level,
                                 const char *format,
                                 const char *unpack, uint64_t *val);

extern void _DeepFuzzy_StreamFloat(enum DeepFuzzy_LogLevel level,
                                   const char *format,
                                   const char *unpack, double *val);

extern void _DeepFuzzy_StreamString(enum DeepFuzzy_LogLevel level,
                                    const char *format,
                                    const char *str);

/* A DeepFuzzy-specific symbol that is needed for hooking. */
struct DeepFuzzy_IndexEntry {
  const char * const name;
  void * const address;
};

/* An index of symbols that the symbolic executors will hook or
 * need access to. */
const struct DeepFuzzy_IndexEntry DeepFuzzy_API[] = {

  /* Control-flow during the test. */
  {"Pass",            (void *) DeepFuzzy_Pass},
  {"Crash",           (void *) DeepFuzzy_Crash},
  {"Fail",            (void *) DeepFuzzy_Fail},
  {"SoftFail",        (void *) DeepFuzzy_SoftFail},
  {"Abandon",         (void *) DeepFuzzy_Abandon},

  /* Locating the tests. */
  {"LastTestInfo",    (void *) &DeepFuzzy_LastTestInfo},

  /* Source of symbolic bytes. */
  {"InputBegin",      (void *) &(DeepFuzzy_Input[0])},
  {"InputEnd",        (void *) &(DeepFuzzy_Input[DeepFuzzy_InputSize])},
  {"InputIndex",      (void *) &DeepFuzzy_InputIndex},

  /* Solver APIs. */
  {"Assume",          (void *) _DeepFuzzy_Assume},
  {"IsSymbolicUInt",  (void *) DeepFuzzy_IsSymbolicUInt},
  {"ConcretizeData",  (void *) DeepFuzzy_ConcretizeData},
  {"ConcretizeCStr",  (void *) DeepFuzzy_ConcretizeCStr},
  {"MinUInt",         (void *) DeepFuzzy_MinUInt},
  {"MaxUInt",         (void *) DeepFuzzy_MaxUInt},

  /* Logging API. */
  {"Log",             (void *) DeepFuzzy_Log},

  /* Streaming API for deferred logging. */
  {"ClearStream",     (void *) DeepFuzzy_ClearStream},
  {"LogStream",       (void *) DeepFuzzy_LogStream},
  {"StreamInt",       (void *) _DeepFuzzy_StreamInt},
  {"StreamFloat",     (void *) _DeepFuzzy_StreamFloat},
  {"StreamString",    (void *) _DeepFuzzy_StreamString},

  {"UsingLibFuzzer", (void *) &DeepFuzzy_UsingLibFuzzer},
  {"UsingSymExec", (void *) &DeepFuzzy_UsingSymExec},

  {NULL, NULL},
};

/* Set up DeepFuzzy. */
DEEPFUZZY_NOINLINE
void DeepFuzzy_Setup(void) {
  static int was_setup = 0;
  if (!was_setup) {
    DeepFuzzy_AllocCurrentTestRun();
    was_setup = 1;
  }

  /* Sort the test cases by line number. */
  struct DeepFuzzy_TestInfo *current = DeepFuzzy_LastTestInfo;
  if (current == NULL) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogError,
       "No tests to run have been defined!  Did you include the harness to compile?");
    exit(1);
  }
  struct DeepFuzzy_TestInfo *min_node = current->prev;
  current->prev = NULL;

  while (min_node != NULL) {
    struct DeepFuzzy_TestInfo *temp = min_node;

    min_node = min_node->prev;
    temp->prev = current;
    current = temp;
  }
  DeepFuzzy_FirstTestInfo = current;
}

/* Tear down DeepFuzzy. */
void DeepFuzzy_Teardown(void) {

}

/* Notify that we're about to begin a test. */
void DeepFuzzy_Begin(struct DeepFuzzy_TestInfo *test) {
  DeepFuzzy_InitCurrentTestRun(test);
  DeepFuzzy_LogFormat(DeepFuzzy_LogTrace, "Running: %s from %s(%u)",
                      test->test_name, test->file_name, test->line_number);
}

void DeepFuzzy_Warn_srand(unsigned int seed) {
  DeepFuzzy_LogFormat(DeepFuzzy_LogWarning,
              "srand under DeepFuzzy has no effect: rand is re-defined as DeepFuzzy_Int");
}

/* Right now "fake" a hexdigest by just using random bytes.  Not ideal. */
void makeFilename(char *name, size_t size) {
  const char *entities = "0123456789abcdef";
  for (int i = 0; i < size; i++) {
    name[i] = entities[rand()%16];
  }
}

void writeInputData(char* name, int important) {
  size_t path_len = 2 + sizeof(char) * (strlen(FLAGS_output_test_dir) + strlen(name));
  char *path = (char *) malloc(path_len);
  snprintf(path, path_len, "%s/%s", FLAGS_output_test_dir, name);
  FILE *fp = fopen(path, "wb");
  if (fp == NULL) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Failed to create file `%s`", path);
    free(path);
    return;
  }
  size_t written = fwrite((void *)DeepFuzzy_Input, 1, DeepFuzzy_InputIndex, fp);
  if (written != DeepFuzzy_InputIndex) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Failed to write to file `%s`", path);
  } else {
    if (important) {
      DeepFuzzy_LogFormat(DeepFuzzy_LogInfo, "Saved test case in file `%s`", path);
    } else {
      DeepFuzzy_LogFormat(DeepFuzzy_LogTrace, "Saved test case in file `%s`", path);
    }
  }
  free(path);
  fclose(fp);
}

/* Save a passing test to the output test directory. */
void DeepFuzzy_SavePassingTest(void) {
  char name[48];
  makeFilename(name, 40);
  name[40] = 0;
  strncat(name, ".pass", 48);
  writeInputData(name, 0);
}

/* Save a failing test to the output test directory. */
void DeepFuzzy_SaveFailingTest(void) {
  char name[48];
  makeFilename(name, 40);
  name[40] = 0;
  strncat(name, ".fail", 48);
  writeInputData(name, 1);
}

/* Save a crashing test to the output test directory. */
void DeepFuzzy_SaveCrashingTest(void) {
  char name[48];
  makeFilename(name, 40);
  name[40] = 0;
  strncat(name, ".crash", 48);
  writeInputData(name, 1);
}

/* Return the first test case to run. */
struct DeepFuzzy_TestInfo *DeepFuzzy_FirstTest(void) {
  return DeepFuzzy_FirstTestInfo;
}

/* Returns `true` if a failure was caught for the current test case. */
bool DeepFuzzy_CatchFail(void) {
  return DeepFuzzy_CurrentTestRun->result == DeepFuzzy_TestRunFail;
}

/* Returns `true` if the current test case was abandoned. */
bool DeepFuzzy_CatchAbandoned(void) {
  return DeepFuzzy_CurrentTestRun->result == DeepFuzzy_TestRunAbandon;
}

/* Fuzz test `FLAGS_input_which_test` or first test, if not defined.
   Has to be defined here since we redefine rand in the header. */
int DeepFuzzy_Fuzz(void){
  DeepFuzzy_LogFormat(DeepFuzzy_LogInfo, "Starting fuzzing");

  if (!HAS_FLAG_min_log_level) {
    FLAGS_min_log_level = 2;
  }

  if (HAS_FLAG_seed) {
    srand(FLAGS_seed);
  } else {
    unsigned int seed = time(NULL);
    DeepFuzzy_LogFormat(DeepFuzzy_LogWarning, "No seed provided; using %u", seed);
    srand(seed);
  }

  if (HAS_FLAG_fork) {
    if (FLAGS_fork) {
      DeepFuzzy_LogFormat(DeepFuzzy_LogFatal,
			  "Forking should not be combined with brute force fuzzing.");
    }
  } else {
    FLAGS_fork = 0;
  }

  long start = (long)time(NULL);
  long current = (long)time(NULL);
  unsigned diff = 0;
  unsigned int i = 0;

  int num_failed_tests = 0;
  int num_passed_tests = 0;
  int num_abandoned_tests = 0;

  struct DeepFuzzy_TestInfo *test = NULL;


  for (test = DeepFuzzy_FirstTest(); test != NULL; test = test->prev) {
    if (HAS_FLAG_input_which_test) {
      if (strcmp(FLAGS_input_which_test, test->test_name) == 0) {
        break;
      }
    } else {
      DeepFuzzy_LogFormat(DeepFuzzy_LogWarning,
			  "No test specified, defaulting to first test defined (%s)",
			  test->test_name);
      break;
    }
  }

  if (test == NULL) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogInfo,
                        "Could not find matching test for %s",
                        FLAGS_input_which_test);
    return 0;
  }

  unsigned int last_status = 0;

  while (diff < FLAGS_timeout) {
    i++;
    if ((diff != last_status) && ((diff % 30) == 0) ) {
      time_t t = time(NULL);
      struct tm tm = *localtime(&t);
      DeepFuzzy_LogFormat(DeepFuzzy_LogInfo, "%d-%02d-%02d %02d:%02d:%02d: %u tests/second: %d failed/%d passed/%d abandoned",
			  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, i/diff,
			  num_failed_tests, num_passed_tests, num_abandoned_tests);
      last_status = diff;
    }
    enum DeepFuzzy_TestRunResult result = DeepFuzzy_FuzzOneTestCase(test);
    if ((result == DeepFuzzy_TestRunFail) || (result == DeepFuzzy_TestRunCrash)) {
      num_failed_tests++;
    } else if (result == DeepFuzzy_TestRunPass) {
      num_passed_tests++;
    } else if (result == DeepFuzzy_TestRunAbandon) {
      num_abandoned_tests++;
    }

    current = (long)time(NULL);
    diff = current-start;
  }

  DeepFuzzy_LogFormat(DeepFuzzy_LogInfo, "Done fuzzing! Ran %u tests (%u tests/second) with %d failed/%d passed/%d abandoned tests",
		      i, i/diff, num_failed_tests, num_passed_tests, num_abandoned_tests);
  return num_failed_tests;
}


/* Run a test case with input initialized by fuzzing.
   Has to be defined here since we redefine rand in the header. */
enum DeepFuzzy_TestRunResult DeepFuzzy_FuzzOneTestCase(struct DeepFuzzy_TestInfo *test) {
  DeepFuzzy_InputIndex = 0;
  DeepFuzzy_InputInitialized = 0;
  DeepFuzzy_SwarmConfigsIndex = 0;
  DeepFuzzy_InternalFuzzing = 1;

  DeepFuzzy_Begin(test);

  enum DeepFuzzy_TestRunResult result = DeepFuzzy_ForkAndRunTest(test);

  if (result == DeepFuzzy_TestRunCrash) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Crashed: %s", test->test_name);

    if (HAS_FLAG_output_test_dir) {
      DeepFuzzy_SaveCrashingTest();
    }

    DeepFuzzy_Crash();
  }

  if (FLAGS_abort_on_fail && ((result == DeepFuzzy_TestRunCrash) ||
                  (result == DeepFuzzy_TestRunFail))) {
    DeepFuzzy_HardCrash();
  }

  if (FLAGS_exit_on_fail && ((result == DeepFuzzy_TestRunCrash) ||
                  (result == DeepFuzzy_TestRunFail))) {
    exit(255); // Terminate the testing
  }

  return result;
}

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  if (Size > sizeof(DeepFuzzy_Input)) {
    return 0; // Just ignore any too-big inputs
  }

  DeepFuzzy_UsingLibFuzzer = 1;

  FLAGS_min_log_level = 3;

  const char* log_control = getenv("DEEPFUZZY_LOG");
  if (log_control != NULL) {
    FLAGS_min_log_level = atoi(log_control);
  }

  const char* loud = getenv("LIBFUZZER_LOUD");
  if (loud != NULL) {
    DeepFuzzy_LibFuzzerLoud = 1;
  }

  struct DeepFuzzy_TestInfo *test = NULL;

  DeepFuzzy_InitOptions(0, "");
  DeepFuzzy_Setup();

  /* we also want to manually allocate CurrentTestRun */
  void *mem = malloc(sizeof(struct DeepFuzzy_TestRunInfo));
  DeepFuzzy_CurrentTestRun = (struct DeepFuzzy_TestRunInfo *) mem;

  test = DeepFuzzy_FirstTest();
  const char* which_test = getenv("LIBFUZZER_WHICH_TEST");
  if (which_test != NULL) {
    for (test = DeepFuzzy_FirstTest(); test != NULL; test = test->prev) {
      if (strncmp(which_test, test->test_name, strnlen(which_test, 1024)) == 0) {
        break;
      }
    }
  }
  
  if (test == NULL) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogFatal,
                        "Could not find matching test for %s (from LIBFUZZER_WHICH_TEST)",
                        which_test);
    exit(255);
  }

  DeepFuzzy_InputIndex = 0;
  DeepFuzzy_SwarmConfigsIndex = 0;

  memcpy((void *) DeepFuzzy_Input, (void *) Data, Size);
  DeepFuzzy_InputInitialized = Size;

  DeepFuzzy_Begin(test);

  enum DeepFuzzy_TestRunResult result = DeepFuzzy_RunTestNoFork(test);
  DeepFuzzy_CleanUp();

  const char* abort_check = getenv("LIBFUZZER_ABORT_ON_FAIL");
  if (abort_check != NULL) {
    if ((result == DeepFuzzy_TestRunFail) || (result == DeepFuzzy_TestRunCrash)) {
      assert(0); // Terminate the testing more permanently
    }
  }

  const char* exit_check = getenv("LIBFUZZER_EXIT_ON_FAIL");
  if (exit_check != NULL) {
    if ((result == DeepFuzzy_TestRunFail) || (result == DeepFuzzy_TestRunCrash)) {
      exit(255); // Terminate the testing
    }
  }

  DeepFuzzy_Teardown();
  DeepFuzzy_CurrentTestRun = NULL;
  free(mem);

  return 0;  // Non-zero return values are reserved for future use.
}

extern int FuzzerEntrypoint(const uint8_t *data, size_t size) {
  return LLVMFuzzerTestOneInput(data, size);
}

/* Overwrite libc's abort. */
void abort(void) {
  DeepFuzzy_Fail();
}

void __assert_fail(const char * assertion, const char * file,
                   unsigned int line, const char * function) {
  DeepFuzzy_LogFormat(DeepFuzzy_LogFatal,
                      "%s(%u): Assertion %s failed in function %s",
                      file, line, assertion, function);
  if (FLAGS_abort_on_fail) {
    DeepFuzzy_HardCrash();
  }
  __builtin_unreachable();
}

void __stack_chk_fail(void) {
  DeepFuzzy_Log(DeepFuzzy_LogFatal, "Stack smash detected");
  __builtin_unreachable();
}

#ifndef LIBFUZZER
#ifndef HEADLESS
#if defined(__unix)
__attribute__((weak))
#endif
int main(int argc, char *argv[]) {
  int ret = 0;
  DeepFuzzy_Setup();
  DeepFuzzy_InitOptions(argc, argv);
  ret = DeepFuzzy_Run();
  DeepFuzzy_Teardown();
  return ret;
}
#endif
#endif

DEEPFUZZY_END_EXTERN_C
