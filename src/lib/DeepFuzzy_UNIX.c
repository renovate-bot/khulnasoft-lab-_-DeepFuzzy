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

#include "deepfuzzy/Platform.h"
#include "deepfuzzy/Option.h"
#include "deepfuzzy/Log.h"
#include "DeepFuzzy.h"

DEEPFUZZY_BEGIN_EXTERN_C

void DeepFuzzy_AllocCurrentTestRun(void) {
  int mem_prot = PROT_READ | PROT_WRITE;
  int mem_vis = MAP_ANONYMOUS | MAP_SHARED;
  void *shared_mem = mmap(NULL, sizeof(struct DeepFuzzy_TestRunInfo), mem_prot,
                          mem_vis, 0, 0);

  if (shared_mem == MAP_FAILED) {
    DeepFuzzy_Log(DeepFuzzy_LogError, "Unable to map shared memory");
    exit(1);
  }

  DeepFuzzy_CurrentTestRun = (struct DeepFuzzy_TestRunInfo *) shared_mem;
}

/* Return a string path to an input file or directory without parsing it to a type. This is
 * useful method in the case where a tested function only takes a path input in order
 * to generate some specialized structured type. Note: the returned path must be 
 * deallocated at the end by the caller. */
char* DeepFuzzy_InputPath(const char* testcase_path) {

  struct stat statbuf;
  char *abspath;

  /* Use specified path if no --input_test* flag specified. Override if --input_* args specified. */
  if (testcase_path) {
    if (!HAS_FLAG_input_test_file && !HAS_FLAG_input_test_files_dir) {
      abspath = realpath(testcase_path, NULL);
    }
  }

  /* Prioritize using CLI-specified input paths, for the sake of fuzzing */
  if (HAS_FLAG_input_test_file) {
    abspath = realpath(FLAGS_input_test_file, NULL);
  } else if (HAS_FLAG_input_test_files_dir) {
    abspath = realpath(FLAGS_input_test_files_dir, NULL);
  } else {
    DeepFuzzy_Abandon("No usable path specified for DeepFuzzy_InputPath.");
  }

  if (stat(abspath, &statbuf) != 0) {
    DeepFuzzy_Abandon("Specified input path does not exist.");
  }

  if (HAS_FLAG_input_test_files_dir) {
    if (!S_ISDIR(statbuf.st_mode)) {
      DeepFuzzy_Abandon("Specified input directory is not a directory.");
    }
  }

  DeepFuzzy_LogFormat(DeepFuzzy_LogInfo, "Using `%s` as input path.", abspath);
  return abspath;
}


void DeepFuzzy_RunSavedTakeOverCases(jmp_buf env,
                                     struct DeepFuzzy_TestInfo *test) {
  int num_failed_tests = 0;
  const char *test_case_dir = FLAGS_input_test_dir;

  DIR *dir_fd = opendir(test_case_dir);
  if (dir_fd == NULL) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogInfo,
                        "Skipping test `%s`, no saved test cases",
                        test->test_name);
    return;
  }

  struct dirent *dp;

  /* Read generated test cases and run a test for each file found. */
  while ((dp = readdir(dir_fd)) != NULL) {
    if (DeepFuzzy_IsTestCaseFile(dp->d_name)) {
      DeepFuzzy_InitCurrentTestRun(test);

      pid_t case_pid = fork();
      if (!case_pid) {
        DeepFuzzy_Begin(test);

        size_t path_len = 2 + sizeof(char) * (strlen(test_case_dir) +
                                              strlen(dp->d_name));
        char *path = (char *) malloc(path_len);
        if (path == NULL) {
          DeepFuzzy_Abandon("Error allocating memory");
        }
        snprintf(path, path_len, "%s/%s", test_case_dir, dp->d_name);
        DeepFuzzy_InitInputFromFile(path);
        free(path);

        longjmp(env, 1);
      }

      int wstatus;
      waitpid(case_pid, &wstatus, 0);

      /* If we exited normally, the status code tells us if the test passed. */
      if (WIFEXITED(wstatus)) {
        switch (DeepFuzzy_CurrentTestRun->result) {
        case DeepFuzzy_TestRunPass:
          DeepFuzzy_LogFormat(DeepFuzzy_LogTrace,
                              "Passed: TakeOver test with data from `%s`",
                              dp->d_name);
          break;
        case DeepFuzzy_TestRunFail:
          DeepFuzzy_LogFormat(DeepFuzzy_LogError,
                              "Failed: TakeOver test with data from `%s`",
                              dp->d_name);
          break;
        case DeepFuzzy_TestRunCrash:
          DeepFuzzy_LogFormat(DeepFuzzy_LogError,
                              "Crashed: TakeOver test with data from `%s`",
                              dp->d_name);
          break;
        case DeepFuzzy_TestRunAbandon:
          DeepFuzzy_LogFormat(DeepFuzzy_LogError,
                              "Abandoned: TakeOver test with data from `%s`",
                              dp->d_name);
          break;
        default:  /* Should never happen */
          DeepFuzzy_LogFormat(DeepFuzzy_LogError,
                              "Error: Invalid test run result %d from `%s`",
                              DeepFuzzy_CurrentTestRun->result, dp->d_name);
        }
      } else {
        /* If here, we exited abnormally but didn't catch it in the signal
         * handler, and thus the test failed due to a crash. */
        DeepFuzzy_LogFormat(DeepFuzzy_LogError,
                            "Crashed: TakeOver test with data from `%s`",
                            dp->d_name);
      }
    }
  }
  closedir(dir_fd);
}

int DeepFuzzy_TakeOver(void) {
  struct DeepFuzzy_TestInfo test = {
    .prev = NULL,
    .test_func = NULL,
    .test_name = "__takeover_test",
    .file_name = "__takeover_file",
    .line_number = 0,
  };

  DeepFuzzy_AllocCurrentTestRun();

  jmp_buf env;
  if (!setjmp(env)) {
    DeepFuzzy_RunSavedTakeOverCases(env, &test);
    exit(0);
  }

  return 0;
}


/* Fork and run `test`. */
extern enum DeepFuzzy_TestRunResult
DeepFuzzy_ForkAndRunTest(struct DeepFuzzy_TestInfo *test) {
  int wstatus = 0;
  pid_t test_pid;
  
  if (FLAGS_fork) {
    test_pid = fork();
    if (!test_pid) {
      DeepFuzzy_RunTest(test);
      /* No need to clean up in a fork; exit() is the ultimate garbage collector */
    }
  }
  if (FLAGS_fork) {
    waitpid(test_pid, &wstatus, 0);
  } else {
    wstatus = DeepFuzzy_RunTestNoFork(test);
    DeepFuzzy_CleanUp();
  }

  /* If we exited normally, the status code tells us if the test passed. */
  if (!FLAGS_fork) {
    return (enum DeepFuzzy_TestRunResult) wstatus;
  } else if (WIFEXITED(wstatus)) {
    uint8_t status = WEXITSTATUS(wstatus);
    return (enum DeepFuzzy_TestRunResult) status;
  }

  /* If here, we exited abnormally but didn't catch it in the signal
   * handler, and thus the test failed due to a crash. */
  return DeepFuzzy_TestRunCrash;
}

/* Checks if the given path corresponds to a regular file. */
bool DeepFuzzy_IsRegularFile(char *path){
  struct stat path_stat;

  stat(path, &path_stat);
  return S_ISREG(path_stat.st_mode);
}


/* Resets the global `DeepFuzzy_Input` buffer, then fills it with the
 * data found in the file `path`. */
void DeepFuzzy_InitInputFromFile(const char *path) {
  struct stat stat_buf;

  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    /* TODO(joe): Add error log with more info. */
    DeepFuzzy_Abandon("Unable to open file");
  }

  int fd = fileno(fp);
  if (fd < 0) {
    DeepFuzzy_Abandon("Tried to get file descriptor for invalid stream");
  }
  if (fstat(fd, &stat_buf) < 0) {
    DeepFuzzy_Abandon("Unable to access input file");
  };

  size_t to_read = stat_buf.st_size;

  if (stat_buf.st_size > sizeof(DeepFuzzy_Input)) {
    DeepFuzzy_LogFormat(DeepFuzzy_LogWarning, "File too large, truncating to max input size");
    to_read = DeepFuzzy_InputSize;
  }

  /* Reset the index. */
  DeepFuzzy_InputIndex = 0;
  DeepFuzzy_SwarmConfigsIndex = 0;

  size_t count = fread((void *) DeepFuzzy_Input, 1, to_read, fp);
  fclose(fp);

  if (count != to_read) {
    /* TODO(joe): Add error log with more info. */
    DeepFuzzy_Abandon("Error reading file");
  }

  DeepFuzzy_InputInitialized = count;

  DeepFuzzy_LogFormat(DeepFuzzy_LogTrace,
                      "Initialized test input buffer with %zu bytes of data from `%s`",
                      count, path);
}


DEEPFUZZY_END_EXTERN_C