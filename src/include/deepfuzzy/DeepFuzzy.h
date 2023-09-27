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

#ifndef SRC_INCLUDE_DEEPFUZZY_DEEPFUZZY_H_
#define SRC_INCLUDE_DEEPFUZZY_DEEPFUZZY_H_

#include <assert.h>
#include <dirent.h>
#include <inttypes.h>
#include <libgen.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <deepfuzzy/Platform.h>
#include <deepfuzzy/Log.h>
#include <deepfuzzy/Compiler.h>
#include <deepfuzzy/Option.h>
#include <deepfuzzy/Stream.h>

#ifdef assert
# undef assert
#endif

#define assert DeepFuzzy_Assert
#define assume DeepFuzzy_Assume
#define check DeepFuzzy_Check

#ifdef DEEPFUZZY_TAKEOVER_RAND
#define rand DeepFuzzy_RandInt
#define srand DeepFuzzy_Warn_srand
#endif

#ifndef DEEPFUZZY_SIZE
#define DEEPFUZZY_SIZE 32768
#endif

#ifndef DEEPFUZZY_MAX_SWARM_CONFIGS
#define DEEPFUZZY_MAX_SWARM_CONFIGS 1024
#endif

#ifndef DEEPFUZZY_SWARM_MAX_PROB_RATIO
#define DEEPFUZZY_SWARM_MAX_PROB_RATIO 16
#endif

#define MAYBE(...) \
    if (DeepFuzzy_Bool()) { \
      __VA_ARGS__ ; \
    }

DEEPFUZZY_BEGIN_EXTERN_C

DECLARE_string(input_test_dir);
DECLARE_string(input_test_file);
DECLARE_string(input_test_files_dir);
DECLARE_string(input_which_test);
DECLARE_string(output_test_dir);
DECLARE_string(test_filter);

DECLARE_bool(input_stdin);
DECLARE_bool(take_over);
DECLARE_bool(abort_on_fail);
DECLARE_bool(exit_on_fail);
DECLARE_bool(verbose_reads);
DECLARE_bool(fuzz);
DECLARE_bool(fuzz_save_passing);
DECLARE_bool(fork);
DECLARE_bool(list_tests);
DECLARE_bool(boring_only);
DECLARE_bool(run_disabled);

DECLARE_int(min_log_level);
DECLARE_int(seed);
DECLARE_int(timeout);

enum {
  DeepFuzzy_InputSize = DEEPFUZZY_SIZE
};


/* Byte buffer that will contain symbolic data that is used to supply requests
 * for symbolic values (e.g. `int`s). */
extern volatile uint8_t DeepFuzzy_Input[DeepFuzzy_InputSize];

#define DEEPFUZZY_READBYTE ((DeepFuzzy_UsingSymExec ? 1 : (DeepFuzzy_InputIndex < DeepFuzzy_InputInitialized ? 1 : (DeepFuzzy_InternalFuzzing ? (DeepFuzzy_Input[DeepFuzzy_InputIndex] = (char)rand()) : (DeepFuzzy_Input[DeepFuzzy_InputIndex] = 0)))), DeepFuzzy_Input[DeepFuzzy_InputIndex++])

/* Index into the `DeepFuzzy_Input` array that tracks how many input bytes have
 * been consumed. */
extern uint32_t DeepFuzzy_InputIndex;
extern uint32_t DeepFuzzy_InputInitialized;
extern uint32_t DeepFuzzy_InternalFuzzing;

enum DeepFuzzy_SwarmType {
  DeepFuzzy_SwarmTypePure = 0,
  DeepFuzzy_SwarmTypeMixed = 1,
  DeepFuzzy_SwarmTypeProb = 2
};

/* Contains info about a swarm configuration */
struct DeepFuzzy_SwarmConfig {
  char* file;
  unsigned line;
  unsigned orig_fcount;
  /* We identify a configuration by these first three elements of the struct */

  /* These fields allow us to map choices to the restricted configuration */
  unsigned fcount;
  unsigned* fmap;
};

/* Index into the set of swarm configurations. */
extern uint32_t DeepFuzzy_SwarmConfigsIndex;

/* Function to return a swarm configuration. */
extern struct DeepFuzzy_SwarmConfig* DeepFuzzy_GetSwarmConfig(unsigned fcount, const char* file, unsigned line, enum DeepFuzzy_SwarmType stype);


#define DEEPFUZZY_FOR_EACH_INTEGER(X) \
    X(Size, size_t, size_t) \
    X(Long, long, unsigned long) \
    X(Int64, int64_t, uint64_t) \
    X(UInt64, uint64_t, uint64_t) \
    X(Int, int, unsigned) \
    X(UInt, unsigned, unsigned) \
    X(Short, short, unsigned short) \
    X(UShort, unsigned short, unsigned short) \
    X(Char, char, unsigned char) \
    X(UChar, unsigned char, unsigned char)

/* Return a symbolic value of a given type. */
extern int DeepFuzzy_Bool(void);

#define DEEPFUZZY_DECLARE(Tname, tname, utname) \
    extern tname DeepFuzzy_ ## Tname (void);

DEEPFUZZY_DECLARE(Float, float, void)
DEEPFUZZY_DECLARE(Double, double, void)
DEEPFUZZY_FOR_EACH_INTEGER(DEEPFUZZY_DECLARE)
#undef DEEPFUZZY_DECLARE

/* Returns the minimum satisfiable value for a given symbolic value, given
 * the constraints present on that value. */
extern uint32_t DeepFuzzy_MinUInt(uint32_t);
extern int32_t DeepFuzzy_MinInt(int32_t);

extern uint32_t DeepFuzzy_MaxUInt(uint32_t);
extern int32_t DeepFuzzy_MaxInt(int32_t);

DEEPFUZZY_INLINE static uint16_t DeepFuzzy_MinUShort(uint16_t v) {
  return DeepFuzzy_MinUInt(v);
}

DEEPFUZZY_INLINE static uint8_t DeepFuzzy_MinUChar(uint8_t v) {
  return (uint8_t) DeepFuzzy_MinUInt(v);
}

DEEPFUZZY_INLINE static int16_t DeepFuzzy_MinShort(int16_t v) {
  return (int16_t) DeepFuzzy_MinInt(v);
}

DEEPFUZZY_INLINE static int8_t DeepFuzzy_MinChar(int8_t v) {
  return (int8_t) DeepFuzzy_MinInt(v);
}

DEEPFUZZY_INLINE static uint16_t DeepFuzzy_MaxUShort(uint16_t v) {
  return (uint16_t) DeepFuzzy_MaxUInt(v);
}

DEEPFUZZY_INLINE static uint8_t DeepFuzzy_MaxUChar(uint8_t v) {
  return (uint8_t) DeepFuzzy_MaxUInt(v);
}

DEEPFUZZY_INLINE static int16_t DeepFuzzy_MaxShort(int16_t v) {
  return (int16_t) DeepFuzzy_MaxInt(v);
}

DEEPFUZZY_INLINE static int8_t DeepFuzzy_MaxChar(int8_t v) {
  return (int8_t) DeepFuzzy_MaxInt(v);
}


/* Result of a single forked test run.
 * Will be passed to the parent process as an exit code. */
enum DeepFuzzy_TestRunResult {
  DeepFuzzy_TestRunPass = 0,
  DeepFuzzy_TestRunFail = 1,
  DeepFuzzy_TestRunCrash = 2,
  DeepFuzzy_TestRunAbandon = 3,
};

/* Contains information about a test case */
struct DeepFuzzy_TestInfo {
  struct DeepFuzzy_TestInfo *prev;
  void (*test_func)(void);
  const char *test_name;
  const char *file_name;
  unsigned line_number;
};

struct DeepFuzzy_TestRunInfo {
  struct DeepFuzzy_TestInfo *test;
  enum DeepFuzzy_TestRunResult result;
  const char *reason;
};

/* Information about the current test run, if any. */
extern struct DeepFuzzy_TestRunInfo *DeepFuzzy_CurrentTestRun;

/* Function to clean up generated strings, and any other DeepFuzzy-managed data. */
extern void DeepFuzzy_CleanUp();

/* Returns `1` if `expr` is true, and `0` otherwise. This is kind of an indirect
 * way to take a symbolic value, introduce a fork, and on each size, replace its
 * value with a concrete value. */
extern int DeepFuzzy_IsTrue(int expr);

/* Always returns `1`. */
extern int DeepFuzzy_One(void);

/* Always returns `0`. */
extern int DeepFuzzy_Zero(void);

/* Always returns `0`. */
extern int DeepFuzzy_ZeroSink(int);

/* Symbolize the data in the exclusive range `[begin, end)`. */
extern void DeepFuzzy_SymbolizeData(void *begin, void *end);

/* Symbolize the data in the exclusive range `[begin, end)` with no nulls. */
extern void DeepFuzzy_SymbolizeDataNoNull(void *begin, void *end);

/* Concretize some data in exclusive the range `[begin, end)`. Returns a
 * concrete pointer to the beginning of the concretized data. */
extern void *DeepFuzzy_ConcretizeData(void *begin, void *end);

/* Assign a symbolic C string of _strlen_ `len` -- with only chars in allowed,
 * if `allowed` is non-null; needs space for null + len bytes */
extern void DeepFuzzy_AssignCStr_C(char* str, size_t len, const char* allowed);

/* Assign a symbolic C string of _strlen_ `len` -- with only chars in allowed,
 * if `allowed` is non-null; needs space for null + len bytes */
extern void DeepFuzzy_SwarmAssignCStr_C(const char* file, unsigned line, int mix,
					char* str, size_t len, const char* allowed);

/* Return a symbolic C string of strlen `len`. */
extern char *DeepFuzzy_CStr_C(size_t len, const char* allowed);

/* Return a symbolic C string of strlen `len`. */
extern char *DeepFuzzy_SwarmCStr_C(const char* file, unsigned line, int mix,
				   size_t len, const char* allowed);

/* Symbolize a C string */
void DeepFuzzy_SymbolizeCStr_C(char *begin, const char* allowed);

/* Symbolize a C string */
void DeepFuzzy_SwarmSymbolizeCStr_C(const char* file, unsigned line, int mix,
				    char *begin, const char* allowed);

/* Concretize a C string. Returns a pointer to the beginning of the
 * concretized C string. */
extern const char *DeepFuzzy_ConcretizeCStr(const char *begin);

/* Allocate and return a pointer to `num_bytes` symbolic bytes. */
extern void *DeepFuzzy_Malloc(size_t num_bytes);

/* Allocate and return a pointer to `num_bytes` symbolic bytes.
   Ptr will be freed by DeepFuzzy at end of test. */
extern void *DeepFuzzy_GCMalloc(size_t num_bytes);

/* Initialize the current test run */
extern void DeepFuzzy_InitCurrentTestRun(struct DeepFuzzy_TestInfo *test);

/* Fork and run `test`. Platform specific function. */
extern enum DeepFuzzy_TestRunResult DeepFuzzy_ForkAndRunTest(struct DeepFuzzy_TestInfo *test);

/* Portable and architecture-independent memory scrub without dead store elimination. */
extern void *DeepFuzzy_MemScrub(void *pointer, size_t data_size);

/* Checks if the given path corresponds to a regular file. */
extern bool DeepFuzzy_IsRegularFile(char *path);

/* Returns the path to a testcase without parsing to any aforementioned types. 
 * Platform specific function. */
extern char *DeepFuzzy_InputPath(const char* testcase_path);

#define DEEPFUZZY_MAKE_SYMBOLIC_ARRAY(Tname, tname, utname) \
    DEEPFUZZY_INLINE static \
    tname *DeepFuzzy_Symbolic ## Tname ## Array(size_t num_elms) { \
      tname *arr = (tname *) malloc(sizeof(tname) * num_elms); \
      DeepFuzzy_SymbolizeData(arr, &(arr[num_elms])); \
      return arr; \
    }

DEEPFUZZY_FOR_EACH_INTEGER(DEEPFUZZY_MAKE_SYMBOLIC_ARRAY)
#undef DEEPFUZZY_MAKE_SYMBOLIC_ARRAY

/* Creates an assumption about a symbolic value. Returns `1` if the assumption
 * can hold and was asserted. */
extern void _DeepFuzzy_Assume(int expr, const char *expr_str, const char *file,
                              unsigned line);

#define DeepFuzzy_Assume(x) _DeepFuzzy_Assume(!!(x), #x, __FILE__, __LINE__)

/* Abandon this test. We've hit some kind of internal problem. */
DEEPFUZZY_NORETURN
extern void DeepFuzzy_Abandon(const char *reason);

/* Mark this test as having crashed. */
extern void DeepFuzzy_Crash(void);

DEEPFUZZY_NORETURN
extern void DeepFuzzy_Fail(void);

/* Mark this test as failing, but don't hard exit. */
extern void DeepFuzzy_SoftFail(void);

DEEPFUZZY_NORETURN
extern void DeepFuzzy_Pass(void);

/* Asserts that `expr` must hold. If it does not, then the test fails and
 * immediately stops. */
DEEPFUZZY_INLINE static void DeepFuzzy_Assert(int expr) {
  if (!expr) {
    DeepFuzzy_Fail();
  }
}

/* Used to make DeepFuzzy really crash for fuzzers, on any platform. */
DEEPFUZZY_INLINE static void DeepFuzzy_HardCrash() {
  raise(SIGABRT);
}

/* Asserts that `expr` must hold. If it does not, then the test fails, but
 * nonetheless continues on. */
DEEPFUZZY_INLINE static void DeepFuzzy_Check(int expr) {
  if (!expr) {
    DeepFuzzy_SoftFail();
  }
}

/* Return a symbolic value in a the range `[low_inc, high_inc]`. */
#ifdef DEEPFUZZY_RANGE_BOUNDARY_BIAS
  #define DEEPFUZZY_MAKE_SYMBOLIC_RANGE(Tname, tname, utname) \
    DEEPFUZZY_INLINE static tname DeepFuzzy_ ## Tname ## InRange( \
        tname low, tname high) { \
      if (low == high) { \
        return low; \
      } else if (low > high) { \
        const tname copy = high; \
        high = low; \
        low = copy; \
      } \
      tname x = DeepFuzzy_ ## Tname(); \
      if (DeepFuzzy_UsingSymExec) { \
        (void) DeepFuzzy_Assume(low <= x && x <= high); \
        return x;					\
      } \
      if (FLAGS_verbose_reads) { \
        printf("Range read low %" PRId64 " high %" PRId64 "\n", \
               (int64_t)low, (int64_t)high); \
      } \
      if (x < low) \
        return low; \
      if (x > high) \
        return high; \
      return x; \
    }
#else
  #define DEEPFUZZY_MAKE_SYMBOLIC_RANGE(Tname, tname, utname) \
    DEEPFUZZY_INLINE static tname DeepFuzzy_ ## Tname ## InRange( \
        tname low, tname high) { \
      if (low == high) { \
        return low; \
      } else if (low > high) { \
        const tname copy = high; \
        high = low; \
        low = copy; \
      } \
      tname x = DeepFuzzy_ ## Tname(); \
      if (DeepFuzzy_UsingSymExec) { \
        (void) DeepFuzzy_Assume(low <= x && x <= high); \
        return x;					\
      } \
      if (FLAGS_verbose_reads) { \
        printf("Range read low %" PRId64 " high %" PRId64 "\n", \
               (int64_t)low, (int64_t)high); \
      } \
      if ((x < low) || (x > high)) { \
        const utname ux = (utname) x; \
        utname usize; \
	if (__builtin_sub_overflow(high, low, &usize)) {	\
	  return low; /* Always legal */ 			\
	} \
	if (__builtin_add_overflow(usize, 1, &usize)) { \
	  return high; /* Always legal */ \
        } \
        const utname ux_clamped = ux % usize; \
        const tname x_clamped = (tname) ux_clamped; \
	tname ret; \
	if (__builtin_add_overflow(low, x_clamped, &ret)) {	\
	  return high; /* Always legal */ \
	} \
        if (FLAGS_verbose_reads) { \
          printf("Converting out-of-range value to %" PRId64 "\n", \
                 (int64_t)ret); \
        } \
        return ret; \
      } \
      return x; \
    }
#endif


DEEPFUZZY_FOR_EACH_INTEGER(DEEPFUZZY_MAKE_SYMBOLIC_RANGE)
#undef DEEPFUZZY_MAKE_SYMBOLIC_RANGE

extern float DeepFuzzy_FloatInRange(float low, float high);
extern double DeepFuzzy_DoubleInRange(double low, double high);

/* Predicates to check whether or not a particular value is symbolic */
extern int DeepFuzzy_IsSymbolicUInt(uint32_t x);

/* The following predicates are implemented in terms of `DeepFuzzy_IsSymbolicUInt`.
 * This simplifies the portability of hooking this predicate interface across
 * architectures, because basically all hooking mechanisms know how to get at
 * the first integer argument. Passing in floating point values, or 64-bit
 * integers on 32-bit architectures, can be more subtle. */

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicInt(int x) {
  return DeepFuzzy_IsSymbolicUInt((uint32_t) x);
}

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicUShort(uint16_t x) {
  return DeepFuzzy_IsSymbolicUInt((uint32_t) x);
}

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicShort(int16_t x) {
  return DeepFuzzy_IsSymbolicUInt((uint32_t) (uint16_t) x);
}

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicUChar(unsigned char x) {
  return DeepFuzzy_IsSymbolicUInt((uint32_t) x);
}

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicChar(char x) {
  return DeepFuzzy_IsSymbolicUInt((uint32_t) (unsigned char) x);
}

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicUInt64(uint64_t x) {
  return DeepFuzzy_IsSymbolicUInt((uint32_t) x) ||
         DeepFuzzy_IsSymbolicUInt((uint32_t) (x >> 32U));
}

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicInt64(int64_t x) {
  return DeepFuzzy_IsSymbolicUInt64((uint64_t) x);
}

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicBool(int x) {
  return DeepFuzzy_IsSymbolicInt(x);
}

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicFloat(float x) {
  return DeepFuzzy_IsSymbolicUInt(*((uint32_t *) &x));
}

DEEPFUZZY_INLINE static int DeepFuzzy_IsSymbolicDouble(double x) {
  return DeepFuzzy_IsSymbolicUInt64(*((uint64_t *) &x));
}

/* Basically an ASSUME that also assigns to v; P should be side-effect
   free, and type of v should be integral. */
#ifndef DEEPFUZZY_MAX_SEARCH_ITERS
#define DEEPFUZZY_MAX_SEARCH_ITERS 4294967296 // 2^32 is enough expense
#endif

#define ASSIGN_SATISFYING(v, expr, P) \
  do { \
    v = (expr); \
    if (DeepFuzzy_UsingSymExec) { \
      (void) DeepFuzzy_Assume(P); \
    } else { \
      unsigned long long DeepFuzzy_assume_iters = 0; \
      unsigned long long DeepFuzzy_safe_incr_v = (unsigned long long) v; \
      unsigned long long DeepFuzzy_safe_decr_v = (unsigned long long) v; \
      while(!(P)) { \
	if (DeepFuzzy_assume_iters > DEEPFUZZY_MAX_SEARCH_ITERS) { \
	  (void) DeepFuzzy_Assume(0); \
	} \
	DeepFuzzy_assume_iters++; \
	DeepFuzzy_safe_incr_v++; \
        v = DeepFuzzy_safe_incr_v; \
	if (!(P)) { \
	  DeepFuzzy_safe_decr_v--;   \
          v = DeepFuzzy_safe_decr_v; \
	} \
      } \
    } \
  } while (0);

/* Basically an ASSUME that also assigns to v in range low to high;
   P should be side-effect free, and type of v should be integral. */

#define ASSIGN_SATISFYING_IN_RANGE(v, expr, low, high, P) \
  do { \
    v = (expr); \
    (void) DeepFuzzy_Assume(low <= v && v <= high); \
    if (DeepFuzzy_UsingSymExec) { \
      (void) DeepFuzzy_Assume(P);\
    } else { \
      unsigned long long DeepFuzzy_assume_iters = 0; \
      long long DeepFuzzy_safe_incr_v = (long long) v; \
      long long DeepFuzzy_safe_decr_v = (long long) v; \
      while(!(P)) { \
	if (DeepFuzzy_assume_iters > DEEPFUZZY_MAX_SEARCH_ITERS) { \
	  (void) DeepFuzzy_Assume(0); \
	} \
	DeepFuzzy_assume_iters++; \
	if (DeepFuzzy_safe_incr_v < high) {	\
	  DeepFuzzy_safe_incr_v++; \
          v = DeepFuzzy_safe_incr_v; \
	} else if (DeepFuzzy_safe_decr_v == low) { \
	  (void) DeepFuzzy_Assume(0); \
	} \
	if (!(P) && (DeepFuzzy_safe_decr_v > low)) {	\
	  DeepFuzzy_safe_decr_v--; \
          v = DeepFuzzy_safe_decr_v; \
	} \
      } \
    } \
  } while (0);

/* Used to define the entrypoint of a test case. */
#define DeepFuzzy_EntryPoint(test_name) \
    _DeepFuzzy_EntryPoint(test_name, __FILE__, __LINE__)


/* Pointer to the last registered `TestInfo` structure. */
extern struct DeepFuzzy_TestInfo *DeepFuzzy_LastTestInfo;

/* Pointer to first structure of ordered `TestInfo` list (reverse of LastTestInfo). */
extern struct DeepFuzzy_TestInfo *DeepFuzzy_FirstTestInfo;

extern int DeepFuzzy_TakeOver(void);

/* Defines the entrypoint of a test case. This creates a data structure that
 * contains the information about the test, and then creates an initializer
 * function that runs before `main` that registers the test entrypoint with
 * DeepFuzzy. */
#define _DeepFuzzy_EntryPoint(test_name, file, line) \
    static void DeepFuzzy_Test_ ## test_name (void); \
    static void DeepFuzzy_Run_ ## test_name (void) { \
      DeepFuzzy_Test_ ## test_name(); \
      DeepFuzzy_Pass(); \
    } \
    static struct DeepFuzzy_TestInfo DeepFuzzy_Info_ ## test_name = { \
      NULL, \
      DeepFuzzy_Run_ ## test_name, \
      DEEPFUZZY_TO_STR(test_name), \
      file, \
      line, \
    }; \
    DEEPFUZZY_INITIALIZER(DeepFuzzy_Register_ ## test_name) { \
      DeepFuzzy_Info_ ## test_name.prev = DeepFuzzy_LastTestInfo; \
      DeepFuzzy_LastTestInfo = &(DeepFuzzy_Info_ ## test_name); \
    } \
    void DeepFuzzy_Test_ ## test_name(void)

/* Set up DeepFuzzy. */
extern void DeepFuzzy_Setup(void);

/* Tear down DeepFuzzy. */
extern void DeepFuzzy_Teardown(void);

/* Notify that we're about to begin a test. */
extern void DeepFuzzy_Begin(struct DeepFuzzy_TestInfo *info);

/* Return the first test case to run. */
extern struct DeepFuzzy_TestInfo *DeepFuzzy_FirstTest(void);

/* Returns `true` if a failure was caught for the current test case. */
extern bool DeepFuzzy_CatchFail(void);

/* Returns `true` if the current test case was abandoned. */
extern bool DeepFuzzy_CatchAbandoned(void);

/* Save a passing test to the output test directory. */
extern void DeepFuzzy_SavePassingTest(void);

/* Save a failing test to the output test directory. */
extern void DeepFuzzy_SaveFailingTest(void);

/* Save a crashing test to the output test directory. */
extern void DeepFuzzy_SaveCrashingTest(void);

/* Jump buffer for returning to `DeepFuzzy_Run`. */
extern jmp_buf DeepFuzzy_ReturnToRun;

/* Checks a filename to see if might be a saved test case.
 *
 * Valid saved test cases have the suffix `.pass` or `.fail`. */
static bool DeepFuzzy_IsTestCaseFile(const char *name) {
  const char *suffix = strchr(name, '.');
  if (suffix == NULL) {
    return false;
  }

  const char *extensions[] = {
    ".pass",
    ".fail",
    ".crash",
  };
  const size_t ext_count = sizeof(extensions) / sizeof(char *);

  for (size_t i = 0; i < ext_count; i++) {
    if (!strcmp(suffix, extensions[i])) {
      return true;
    }
  }

  return false;
}

extern void DeepFuzzy_Warn_srand(unsigned int seed);

/* Resets the global `DeepFuzzy_Input` buffer, then fills it with the
 * data found in the file `path`. */
extern void DeepFuzzy_InitInputFromFile(const char *path);

/* Resets the global `DeepFuzzy_Input` buffer, then fills it with the
 * data found in the file `path`. */
static void DeepFuzzy_InitInputFromStdin() {

  /* Reset the index. */
  DeepFuzzy_InputIndex = 0;
  DeepFuzzy_SwarmConfigsIndex = 0;

  size_t count = read(STDIN_FILENO, (void *) DeepFuzzy_Input, DeepFuzzy_InputSize);

  DeepFuzzy_InputInitialized = count;

  DeepFuzzy_LogFormat(DeepFuzzy_LogTrace,
                      "Initialized test input buffer with %zu bytes of data from stdin",
                      count);
}

/* Run a test case, assuming we have forked from the test harness to do so.
 *
 * An exit code of 0 indicates that the test passed. Any other exit
 * code, or termination by a signal, indicates a test failure. */
static void DeepFuzzy_RunTest(struct DeepFuzzy_TestInfo *test) {
  /* Run the test. */
  if (!setjmp(DeepFuzzy_ReturnToRun)) {
    /* Convert uncaught C++ exceptions into a test failure. */
#if defined(__cplusplus) && defined(__cpp_exceptions)
    try {
#endif  /* __cplusplus */

      test->test_func();  /* Run the test function. */
      exit(DeepFuzzy_TestRunPass);

#if defined(__cplusplus) && defined(__cpp_exceptions)
    } catch(...) {
      DeepFuzzy_Fail();
    }
#endif  /* __cplusplus */

    /* We caught a failure when running the test. */
  } else if (DeepFuzzy_CatchFail()) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Failed: %s", test->test_name);
    if (HAS_FLAG_output_test_dir) {
      DeepFuzzy_SaveFailingTest();
    }
    exit(DeepFuzzy_TestRunFail);

    /* The test was abandoned. We may have gotten soft failures before
     * abandoning, so we prefer to catch those first. */
  } else if (DeepFuzzy_CatchAbandoned()) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogTrace, "Abandoned: %s", test->test_name);
    exit(DeepFuzzy_TestRunAbandon);

    /* The test passed. */
  } else {
    DeepFuzzy_LogFormat(DeepFuzzy_LogTrace, "Passed: %s", test->test_name);
    if (HAS_FLAG_output_test_dir) {
      if (!FLAGS_fuzz || FLAGS_fuzz_save_passing) {
	DeepFuzzy_SavePassingTest();
      }
    }
    exit(DeepFuzzy_TestRunPass);
  }
}

/* Run a test case, but in libFuzzer, so not inside a fork. */
static int DeepFuzzy_RunTestNoFork(struct DeepFuzzy_TestInfo *test) {
  /* Run the test. */
  if (!setjmp(DeepFuzzy_ReturnToRun)) {
    /* Convert uncaught C++ exceptions into a test failure. */
#if defined(__cplusplus) && defined(__cpp_exceptions)
    try {
#endif  /* __cplusplus */

      test->test_func();  /* Run the test function. */
      return(DeepFuzzy_TestRunPass);

#if defined(__cplusplus) && defined(__cpp_exceptions)
    } catch(...) {
      DeepFuzzy_Fail();
    }
#endif  /* __cplusplus */

    /* We caught a failure when running the test. */
  } else if (DeepFuzzy_CatchFail()) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Failed: %s", test->test_name);
    if (HAS_FLAG_output_test_dir) {
      DeepFuzzy_SaveFailingTest();
    }
    if (HAS_FLAG_abort_on_fail) {
      DeepFuzzy_HardCrash();
    }
    return(DeepFuzzy_TestRunFail);

    /* The test was abandoned. We may have gotten soft failures before
     * abandoning, so we prefer to catch those first. */
  } else if (DeepFuzzy_CatchAbandoned()) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogTrace, "Abandoned: %s", test->test_name);
    return(DeepFuzzy_TestRunAbandon);

    /* The test passed. */
  } else {
    DeepFuzzy_LogFormat(DeepFuzzy_LogTrace, "Passed: %s", test->test_name);
    if (HAS_FLAG_output_test_dir) {
      if (!FLAGS_fuzz || FLAGS_fuzz_save_passing) {
        DeepFuzzy_SavePassingTest();
      }
    }
    return(DeepFuzzy_TestRunPass);
  }
}

extern enum DeepFuzzy_TestRunResult DeepFuzzy_FuzzOneTestCase(struct DeepFuzzy_TestInfo *test);

/* Run a single saved test case with input initialized from the file
 * `name` in directory `dir`. */
static enum DeepFuzzy_TestRunResult
DeepFuzzy_RunSavedTestCase(struct DeepFuzzy_TestInfo *test, const char *dir,
                           const char *name) {
  if (!setjmp(DeepFuzzy_ReturnToRun)) {
    size_t path_len = 2 + sizeof(char) * (strlen(dir) + strlen(name));
    char *path = (char *) malloc(path_len);
    if (path == NULL) {
      DeepFuzzy_Abandon("Error allocating memory");
    }
    if (strncmp(dir, "", strlen(dir)) != 0) {
      snprintf(path, path_len, "%s/%s", dir, name);
    } else {
      snprintf(path, path_len, "%s", name);
    }

    if (!(strncmp(name, "** STDIN **", strlen(name)) == 0)) {
      DeepFuzzy_InitInputFromFile(path);
    } else {
      DeepFuzzy_InitInputFromStdin();
    }

    DeepFuzzy_Begin(test);

    enum DeepFuzzy_TestRunResult result = DeepFuzzy_ForkAndRunTest(test);

    if (result == DeepFuzzy_TestRunFail) {
      DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Test case %s failed", path);
      free(path);
    }
    else if (result == DeepFuzzy_TestRunCrash) {
      DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Crashed: %s", test->test_name);
      DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Test case %s crashed", path);
      free(path);
      if (HAS_FLAG_output_test_dir) {
        DeepFuzzy_SaveCrashingTest();
      }

      DeepFuzzy_Crash();
    } else {
      free(path);
    }

    return result;
  } else {
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Something went wrong running the test case %s", name);
    return DeepFuzzy_TestRunCrash;
  }
}

/* Run a single test many times, initialized against each saved test case in
 * `FLAGS_input_test_dir`. */
static int DeepFuzzy_RunSavedCasesForTest(struct DeepFuzzy_TestInfo *test) {
  int num_failed_tests = 0;
  const char *test_file_name = basename((char *) test->file_name);

  size_t test_case_dir_len = 3 + strlen(FLAGS_input_test_dir)
    + strlen(test_file_name) + strlen(test->test_name);
  char *test_case_dir = (char *) malloc(test_case_dir_len);
  if (test_case_dir == NULL) {
    DeepFuzzy_Abandon("Error allocating memory");
  }
  snprintf(test_case_dir, test_case_dir_len, "%s/%s/%s",
           FLAGS_input_test_dir, test_file_name, test->test_name);

  struct dirent *dp;
  DIR *dir_fd;

  dir_fd = opendir(test_case_dir);
  if (dir_fd == NULL) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogInfo,
                        "Skipping test `%s`, no saved test cases",
                        test->test_name);
    free(test_case_dir);
    return 0;
  }

  unsigned int i = 0;

  /* Read generated test cases and run a test for each file found. */
  while ((dp = readdir(dir_fd)) != NULL) {
    if (DeepFuzzy_IsTestCaseFile(dp->d_name)) {
      i++;
      enum DeepFuzzy_TestRunResult result =
        DeepFuzzy_RunSavedTestCase(test, test_case_dir, dp->d_name);

      if (result != DeepFuzzy_TestRunPass) {
        num_failed_tests++;
      }
    }
  }
  closedir(dir_fd);
  free(test_case_dir);

  DeepFuzzy_LogFormat(DeepFuzzy_LogInfo, "Ran %u tests for %s; %d tests failed",
		      i, test->test_name, num_failed_tests);

  return num_failed_tests;
}

/* Returns a sorted list of all available tests to run, and exits after */
static int DeepFuzzy_RunListTests(void) {
  char buff[4096];
  ssize_t write_len = 0;

  int total_test_count = 0;
  int boring_count = 0;
  int disabled_count = 0;

  struct DeepFuzzy_TestInfo *current_test = DeepFuzzy_FirstTestInfo;

  sprintf(buff, "Available Tests:\n\n");
  write_len = write(STDERR_FILENO, buff, strlen(buff));

  /* Print each test and increment counter from linked list */
  for (; current_test != NULL; current_test = current_test->prev) {

	const char * curr_test = current_test->test_name;

	/* Classify tests */
	if (strstr(curr_test, "Boring") || strstr(curr_test, "BORING")) {
	  boring_count++;
	} else if (strstr(curr_test, "Disabled") || strstr(curr_test, "DISABLED")) {
	  disabled_count++;
	}

    /* TODO(alan): also output file name, luckily its sorted :) */
    sprintf(buff, " *  %s (line %d)\n", curr_test, current_test->line_number);
	write_len = write(STDERR_FILENO, buff, strlen(buff));
    total_test_count++;
  }

  sprintf(buff, "\nBoring Tests: %d\nDisabled Tests: %d\n", boring_count, disabled_count);
  write_len = write(STDERR_FILENO, buff, strlen(buff));

  sprintf(buff, "\nTotal Number of Tests: %d\n", total_test_count);
  write_len = write(STDERR_FILENO, buff, strlen(buff));
  return 0;
}

/* Run test from `FLAGS_input_test_file`, under `FLAGS_input_which_test`
 * or first test, if not defined. */
static int DeepFuzzy_RunSingleSavedTestCase(void) {
  int num_failed_tests = 0;
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

  enum DeepFuzzy_TestRunResult result =
    DeepFuzzy_RunSavedTestCase(test, "", FLAGS_input_test_file);

  if ((result == DeepFuzzy_TestRunFail) || (result == DeepFuzzy_TestRunCrash)) {
    if (FLAGS_abort_on_fail) {
      DeepFuzzy_HardCrash();
    }
    if (FLAGS_exit_on_fail) {
      exit(255); // Terminate the testing
    }
    num_failed_tests++;
  }

  DeepFuzzy_Teardown();

  return num_failed_tests;
}

/* Run test from stdin, under `FLAGS_input_which_test`
 * or first test, if not defined. */
static int DeepFuzzy_RunTestFromStdin(void) {
  int num_failed_tests = 0;
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

  enum DeepFuzzy_TestRunResult result =
    DeepFuzzy_RunSavedTestCase(test, "", "** STDIN **");

  if ((result == DeepFuzzy_TestRunFail) || (result == DeepFuzzy_TestRunCrash)) {
    if (FLAGS_abort_on_fail) {
      DeepFuzzy_HardCrash();
    }
    if (FLAGS_exit_on_fail) {
      exit(255); // Terminate the testing
    }
    num_failed_tests++;
  }

  DeepFuzzy_Teardown();

  return num_failed_tests;
}

extern int DeepFuzzy_Fuzz(void);

/* Run tests from `FLAGS_input_test_files_dir`, under `FLAGS_input_which_test`
 * or first test, if not defined. */
static int DeepFuzzy_RunSingleSavedTestDir(void) {
  int num_failed_tests = 0;
  struct DeepFuzzy_TestInfo *test = NULL;

  if (!HAS_FLAG_min_log_level) {
    FLAGS_min_log_level = 2;
  }

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

  struct dirent *dp;
  DIR *dir_fd;

  #if defined(__unix)
    struct stat path_stat;
  #endif
  
  dir_fd = opendir(FLAGS_input_test_files_dir);
  if (dir_fd == NULL) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogInfo,
                        "No tests to run");
    return 0;
  }

  unsigned int i = 0;

  /* Read generated test cases and run a test for each file found. */
  while ((dp = readdir(dir_fd)) != NULL) {
    size_t path_len = 2 + sizeof(char) * (strlen(FLAGS_input_test_files_dir) + strlen(dp->d_name));
    char *path = (char *) malloc(path_len);
    snprintf(path, path_len, "%s/%s", FLAGS_input_test_files_dir, dp->d_name);

    if (!DeepFuzzy_IsRegularFile(path)){
      continue;
    }

    i++;
    enum DeepFuzzy_TestRunResult result =
      DeepFuzzy_RunSavedTestCase(test, FLAGS_input_test_files_dir, dp->d_name);

    if ((result == DeepFuzzy_TestRunFail) || (result == DeepFuzzy_TestRunCrash)) {
      if (FLAGS_abort_on_fail) {
        DeepFuzzy_HardCrash();
      }
      if (FLAGS_exit_on_fail) {
        exit(255); // Terminate the testing
      }
      num_failed_tests++;
    }
  }
  closedir(dir_fd);

  DeepFuzzy_LogFormat(DeepFuzzy_LogInfo, "Ran %u tests; %d tests failed",
		      i, num_failed_tests);

  return num_failed_tests;
}

/* Run test `FLAGS_input_which_test` with saved input from `FLAGS_input_test_file`.
 *
 * For each test unit and case, see if there are input files in the
 * expected directories. If so, use them to initialize
 * `DeepFuzzy_Input`, then run the test. If not, skip the test. */
static int DeepFuzzy_RunSavedTestCases(void) {
  int num_failed_tests = 0;
  struct DeepFuzzy_TestInfo *test = NULL;

  if (!HAS_FLAG_min_log_level) {
    FLAGS_min_log_level = 2;
  }

  for (test = DeepFuzzy_FirstTest(); test != NULL; test = test->prev) {
    num_failed_tests += DeepFuzzy_RunSavedCasesForTest(test);
  }

  DeepFuzzy_Teardown();

  return num_failed_tests;
}

/* Start DeepFuzzy and run the tests. Returns the number of failed tests. */
static int DeepFuzzy_Run(void) {
  if (!DeepFuzzy_OptionsAreInitialized) {
    DeepFuzzy_Abandon("Please call DeepFuzzy_InitOptions(argc, argv) in main");
  }

  if (HAS_FLAG_list_tests) {
    return DeepFuzzy_RunListTests();
  }

  ENABLE_DIRECT_RUN_FLAG;

  if (HAS_FLAG_input_test_file) {
    return DeepFuzzy_RunSingleSavedTestCase();
  }

  if (HAS_FLAG_input_stdin) {
    return DeepFuzzy_RunTestFromStdin();
  }

  if (HAS_FLAG_input_test_dir) {
    return DeepFuzzy_RunSavedTestCases();
  }

  if (HAS_FLAG_input_test_files_dir) {
    return DeepFuzzy_RunSingleSavedTestDir();
  }

  if (FLAGS_fuzz) {
    return DeepFuzzy_Fuzz();
  }

  int num_failed_tests = 0;
  struct DeepFuzzy_TestInfo *test = NULL;


  for (test = DeepFuzzy_FirstTest(); test != NULL; test = test->prev) {

	const char * curr_test = test->test_name;

	/* Run only the Boring* tests */
	if (HAS_FLAG_boring_only) {
	  if (strstr(curr_test, "Boring") || strstr(curr_test, "BORING")) {
        DeepFuzzy_Begin(test);
		if (DeepFuzzy_ForkAndRunTest(test) != 0) {
		  num_failed_tests++;
		}
	  } else {
		continue;
	  }
	}

	/* Check if pattern match exists in test, skip if not */
	if (HAS_FLAG_test_filter) {
    if (REG_MATCH(FLAGS_test_filter, curr_test)){
      continue;
    }	  
	}

	/* Check if --run_disabled is set, and if not, skip Disabled* tests */
	if (!HAS_FLAG_run_disabled) {
	  if (strstr(curr_test, "Disabled") || strstr(test->test_name, "DISABLED")) {
		continue;
	  }
	}

	DeepFuzzy_Begin(test);
    if (DeepFuzzy_ForkAndRunTest(test) != 0) {
      num_failed_tests++;
    }
  }

  DeepFuzzy_Teardown();

  return num_failed_tests;
}

DEEPFUZZY_END_EXTERN_C

#endif  /* SRC_INCLUDE_DEEPFUZZY_DEEPFUZZY_H_ */
