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

#ifndef SRC_INCLUDE_DEEPFUZZY_STREAM_H_
#define SRC_INCLUDE_DEEPFUZZY_STREAM_H_

#include <stdarg.h>
#include <stdint.h>

#include <deepfuzzy/Compiler.h>
#include <deepfuzzy/Log.h>

DEEPFUZZY_BEGIN_EXTERN_C

/* Clear the contents of the stream and don't log it. */
extern void DeepFuzzy_ClearStream(enum DeepFuzzy_LogLevel level);

/* Flush the contents of the stream to a log. */
extern void DeepFuzzy_LogStream(enum DeepFuzzy_LogLevel level);

/* Stream a C string into the stream's message. */
extern void DeepFuzzy_StreamCStr(enum DeepFuzzy_LogLevel level,
                                 const char *begin);

/* TODO(pag): Implement `DeepFuzzy_StreamWCStr` with `wchar_t`. */

/* Stream a some data in the inclusive range `[begin, end]` into the
 * stream's message. */
/*extern void DeepFuzzy_StreamData(
    enum DeepFuzzy_LogLevel level, const void *begin, const void *end); */

/* Stream some formatted input */
extern void DeepFuzzy_StreamFormat(
    enum DeepFuzzy_LogLevel level, const char *format, ...);

/* Stream some formatted input */
extern void DeepFuzzy_StreamVFormat(
    enum DeepFuzzy_LogLevel level, const char *format, va_list args);

#define DEEPFUZZY_DECLARE_STREAMER(Type, type) \
    extern void DeepFuzzy_Stream ## Type( \
        enum DeepFuzzy_LogLevel level, type val);

DEEPFUZZY_DECLARE_STREAMER(Double, double);
DEEPFUZZY_DECLARE_STREAMER(Pointer, const void *);

DEEPFUZZY_DECLARE_STREAMER(UInt64, uint64_t)
DEEPFUZZY_DECLARE_STREAMER(Int64, int64_t)

DEEPFUZZY_DECLARE_STREAMER(UInt32, uint32_t)
DEEPFUZZY_DECLARE_STREAMER(Int32, int32_t)

DEEPFUZZY_DECLARE_STREAMER(UInt16, uint16_t)
DEEPFUZZY_DECLARE_STREAMER(Int16, int16_t)

DEEPFUZZY_DECLARE_STREAMER(UInt8, uint8_t)
DEEPFUZZY_DECLARE_STREAMER(Int8, int8_t)

#undef DEEPFUZZY_DECLARE_STREAMER

DEEPFUZZY_INLINE static void DeepFuzzy_StreamFloat(
    enum DeepFuzzy_LogLevel level, float val) {
  DeepFuzzy_StreamDouble(level, (double) val);
}

/* Reset the formatting in a stream. */
extern void DeepFuzzy_StreamResetFormatting(enum DeepFuzzy_LogLevel level);

DEEPFUZZY_END_EXTERN_C

#endif  /* SRC_INCLUDE_DEEPFUZZY_STREAM_H_ */
