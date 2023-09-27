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

#ifndef SRC_INCLUDE_DEEPFUZZY_DEEPFUZZY_PRIVATE_H_
#define SRC_INCLUDE_DEEPFUZZY_DEEPFUZZY_PRIVATE_H_

#include <deepfuzzy/DeepFuzzy.h>

DEEPFUZZY_BEGIN_EXTERN_C


/* Function that allocates the shared memory for the current test run. Platform 
 * specific function. */
extern void DeepFuzzy_AllocCurrentTestRun(void);

/* Run saved take over cases. Platform specific function. */
extern void DeepFuzzy_RunSavedTakeOverCases(jmp_buf env, struct DeepFuzzy_TestInfo *test);

/* Run take over. Platform specific function. */
extern int DeepFuzzy_TakeOver(void);


DEEPFUZZY_END_EXTERN_C

#endif /* SRC_INCLUDE_DEEPFUZZY_DEEPFUZZY_PRIVATE_H_ */