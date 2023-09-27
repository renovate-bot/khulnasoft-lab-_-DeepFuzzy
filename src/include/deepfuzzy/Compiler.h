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

#ifndef SRC_INCLUDE_DEEPFUZZY_COMPILER_H_
#define SRC_INCLUDE_DEEPFUZZY_COMPILER_H_

#include <stdio.h>
#include <stdlib.h>

/* Concatenation macros. */
#define DEEPFUZZY_CAT__(x, y) x ## y
#define DEEPFUZZY_CAT_(x, y) DEEPFUZZY_CAT__(x, y)
#define DEEPFUZZY_CAT(x, y) DEEPFUZZY_CAT_(x, y)

/* Stringify a macro parameter. */
#define DEEPFUZZY_TO_STR(a) _DEEPFUZZY_TO_STR(a)
#define _DEEPFUZZY_TO_STR(a) __DEEPFUZZY_TO_STR(a)
#define __DEEPFUZZY_TO_STR(a) #a

/* Mark a function as not returning. */
#if defined(_MSC_VER)
# define DEEPFUZZY_NORETURN __declspec(noreturn)
#else
# define DEEPFUZZY_NORETURN __attribute__((noreturn))
#endif

/* Mark a function for inlining. */
#if defined(_MSC_VER)
# define DEEPFUZZY_INLINE __forceinline
# define DEEPFUZZY_NOINLINE __declspec(noinline)
#else
# define DEEPFUZZY_INLINE inline __attribute__((always_inline))
# define DEEPFUZZY_NOINLINE __attribute__((noinline))
#endif

/* Introduce a trap instruction to halt execution. */
#if defined(_MSC_VER)
# include <intrin.h>
# define DeepFuzzy_Trap __debugbreak
#else
# define DeepFuzzy_Trap __builtin_trap
#endif

/* Wrap a block of code in `extern "C"` if we are compiling with a C++
 * compiler. */
#ifdef __cplusplus
# define DEEPFUZZY_BEGIN_EXTERN_C extern "C" {
# define DEEPFUZZY_END_EXTERN_C }
#else
# define DEEPFUZZY_BEGIN_EXTERN_C
# define DEEPFUZZY_END_EXTERN_C
#endif

/* Initializer/finalizer sample for MSVC and GCC/Clang.
 * 2010-2016 Joe Lowe. Released into the public domain.
 *
 * See: https://stackoverflow.com/a/2390626/247591 */
#ifdef __cplusplus
# define DEEPFUZZY_INITIALIZER(f) \
    static void f(void); \
    struct f ##_t_ { \
      f##_t_(void) { \
        f(); \
      } \
    }; \
    static f##_t_ f##_; \
    static void f(void)

#elif defined(_MSC_VER)
# pragma section(".CRT$XCU",read)
# define DEEPFUZZY_INITIALIZER2_(f, p) \
    static void f(void); \
    __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
    __pragma(comment(linker,"/include:" p #f "_")) \
    static void f(void)

# ifdef _WIN64
#   define DEEPFUZZY_INITIALIZER(f) DEEPFUZZY_INITIALIZER2_(f,"")
# else
#   define DEEPFUZZY_INITIALIZER(f) DEEPFUZZY_INITIALIZER2_(f,"_")
#  endif
#else
# define DEEPFUZZY_INITIALIZER(f) \
    static void f(void) __attribute__((constructor)); \
    static void f(void)
#endif

#define DEEPFUZZY_BARRIER() \
    asm volatile ("":::"memory")

#define DEEPFUZZY_USED(x) \
    asm volatile (""::"m"(x):"memory")

#endif  /* SRC_INCLUDE_DEEPFUZZY_COMPILER_H_ */
