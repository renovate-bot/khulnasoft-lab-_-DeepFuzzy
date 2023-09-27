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

#include <windows.h>

#include "deepfuzzy/Platform.h"
#include "deepfuzzy/Option.h"
#include "deepfuzzy/Log.h"
#include "DeepFuzzy.h"

DEEPFUZZY_BEGIN_EXTERN_C

void DeepFuzzy_AllocCurrentTestRun(void) {
  HANDLE shared_mem_handle = INVALID_HANDLE_VALUE;
  
  shared_mem_handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
                  0, sizeof(struct DeepFuzzy_TestRunInfo), "DeepFuzzy_CurrentTestRun");
  if (!shared_mem_handle){
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Unable to map shared memory (%d)", GetLastError());
    exit(1);
  }

  struct DeepFuzzy_TestRunInfo *shared_mem = (struct DeepFuzzy_TestRunInfo*) MapViewOfFile(shared_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(struct DeepFuzzy_TestRunInfo));
  if (!shared_mem){
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "Unable to map shared memory (%d)", GetLastError());
    exit(1);
  }

  DeepFuzzy_CurrentTestRun = (struct DeepFuzzy_TestRunInfo *) shared_mem;
}


/* Return a string path to an input file or directory without parsing it to a type. This is
 * useful method in the case where a tested function only takes a path input in order
 * to generate some specialized structured type. Note: the returned path must be 
 * deallocated at the end by the caller. */
char* DeepFuzzy_InputPath(const char* testcase_path) {

  char *abspath = (char*) malloc(MAX_CMD_LEN * sizeof(char));

  /* Use specified path if no --input_test* flag specified. Override if --input_* args specified. */
  if (testcase_path) {
    if (!HAS_FLAG_input_test_file && !HAS_FLAG_input_test_files_dir) {
      _fullpath(abspath, testcase_path, MAX_CMD_LEN);
    }
  }

  /* Prioritize using CLI-specified input paths, for the sake of fuzzing */
  if (HAS_FLAG_input_test_file) {
    _fullpath(abspath, FLAGS_input_test_file, MAX_CMD_LEN);
  } else if (HAS_FLAG_input_test_files_dir) {
    _fullpath(abspath, FLAGS_input_test_files_dir, MAX_CMD_LEN);
  } else {
    DeepFuzzy_Abandon("No usable path specified for DeepFuzzy_InputPath.");
  }

  DWORD file_attributes = GetFileAttributes(abspath);
  if (file_attributes == INVALID_FILE_ATTRIBUTES){
    DeepFuzzy_Abandon("Specified input path does not exist.");
  }

  if (HAS_FLAG_input_test_files_dir) {
    if (!(file_attributes & INVALID_FILE_ATTRIBUTES)){
      DeepFuzzy_Abandon("Specified input directory is not a directory.");
    }
  }

  DeepFuzzy_LogFormat(DeepFuzzy_LogInfo, "Using `%s` as input path.", abspath);
  return abspath;
}

void DeepFuzzy_RunSavedTakeOverCases(jmp_buf env,
                                     struct DeepFuzzy_TestInfo *test) {

  /* The method is not supported on Windows, and thus exit with an error. */
  DeepFuzzy_LogFormat(DeepFuzzy_LogError,
                      "Error: takeover works only on Unix based systems.");
}

int DeepFuzzy_TakeOver(void) {

  /* The method is not supported on Windows, and thus exit with an error. */
  DeepFuzzy_LogFormat(DeepFuzzy_LogError,
                      "Error: takeover works only on Unix based systems.");
  return -1;

}

/* Run a test case inside a new Windows process */
int DeepFuzzy_RunTestWin(struct DeepFuzzy_TestInfo *test){

  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  DWORD exit_code = DeepFuzzy_TestRunPass;

  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  ZeroMemory( &pi, sizeof(pi) );
  
  /* Get the fully qualified path of the current module */
  char command[MAX_CMD_LEN]; 
  if (!GetModuleFileName(NULL, command, MAX_CMD_LEN)){
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "GetModuleFileName failed (%d)", GetLastError());
    return DeepFuzzy_TestRunAbandon;
  }

  /* Append the parameters to specify which test to run and to run the test
      directly in the main process */
  snprintf(command, MAX_CMD_LEN, "%s --direct_run --input_which_test %s", command, test->test_name);

  if (HAS_FLAG_output_test_dir) {
    snprintf(command, MAX_CMD_LEN, "%s --output_test_dir %s", command, FLAGS_output_test_dir);
  }

  if (!FLAGS_fuzz || FLAGS_fuzz_save_passing) {
    snprintf(command, MAX_CMD_LEN, "%s --fuzz_save_passing", command);
  }

  /* Create the process */
  if(!CreateProcess(NULL, command, NULL, NULL, false, 0, NULL, NULL, &si, &pi)){
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "CreateProcess failed (%d)", GetLastError());
    return DeepFuzzy_TestRunAbandon;
  }

  /* Wait for the process to complete and get it's exit code */
  WaitForSingleObject(pi.hProcess, INFINITE);
  if (!GetExitCodeProcess(pi.hProcess, &exit_code)){
    DeepFuzzy_LogFormat(DeepFuzzy_LogError, "GetExitCodeProcess failed (%d)", GetLastError());
    return DeepFuzzy_TestRunAbandon;
  }

  /* If at this point the exit code is not DeepFuzzy_TestRunPass, it means that
   * DeepFuzzy_RunTest never reached the end of test->test_func(), and thus the 
   * function exited abnormally:
   *   test->test_func();
   *   exit(DeepFuzzy_TestRunPass); */
  if (exit_code != DeepFuzzy_TestRunPass){
    exit_code = DeepFuzzy_TestRunCrash;
  }

  return exit_code;
}

/* Run a single test. This function is intended to be executed within a new 
Windows process. */
void DeepFuzzy_RunSingle(){
  struct DeepFuzzy_TestInfo *test = DeepFuzzy_FirstTest(); 

  /* Seek for the TEST to run */
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
    return;
  }

  /* Run the test */
  DeepFuzzy_RunTest(test);

}

/* Fork and run `test`. */
extern enum DeepFuzzy_TestRunResult
DeepFuzzy_ForkAndRunTest(struct DeepFuzzy_TestInfo *test) {
  int wstatus;

  if (FLAGS_fork) {
    wstatus = DeepFuzzy_RunTestWin(test);
    return (enum DeepFuzzy_TestRunResult) wstatus;
  }
  wstatus = DeepFuzzy_RunTestNoFork(test);
  DeepFuzzy_CleanUp();
  return (enum DeepFuzzy_TestRunResult) wstatus;
}

/* Checks if the given path corresponds to a regular file. */
bool DeepFuzzy_IsRegularFile(char *path){
  DWORD file_attributes = GetFileAttributes(path);
  return file_attributes != INVALID_FILE_ATTRIBUTES && !(file_attributes & FILE_ATTRIBUTE_DIRECTORY);
}


/* Resets the global `DeepFuzzy_Input` buffer, then fills it with the
 * data found in the file `path`. */
void DeepFuzzy_InitInputFromFile(const char *path) {

  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    /* TODO(joe): Add error log with more info. */
    DeepFuzzy_Abandon("Unable to open file");
  }

  int fd = fileno(fp);
  if (fd < 0) {
    DeepFuzzy_Abandon("Tried to get file descriptor for invalid stream");
  }

  if (fseek(fp, 0L, SEEK_END) < 0){
    DeepFuzzy_Abandon("Unable to get test input size");
  }
  size_t to_read = ftell(fp);
  if(to_read < 0 || fseek(fp, 0L, SEEK_SET) < 0){
    DeepFuzzy_Abandon("Unable to get test input size");
  }

  if (to_read > sizeof(DeepFuzzy_Input)) {
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