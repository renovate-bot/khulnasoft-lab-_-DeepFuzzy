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

#ifndef SRC_INCLUDE_DEEPFUZZY_DEEPFUZZY_HPP_
#define SRC_INCLUDE_DEEPFUZZY_DEEPFUZZY_HPP_

#include <deepfuzzy/DeepFuzzy.h>
#include <deepfuzzy/Stream.hpp>

#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace deepfuzzy {

DEEPFUZZY_INLINE static void *Malloc(size_t num_bytes) {
  return DeepFuzzy_Malloc(num_bytes);
}

DEEPFUZZY_INLINE static void SymbolizeData(void *begin, void *end) {
  DeepFuzzy_SymbolizeData(begin, end);
}

DEEPFUZZY_INLINE static bool Bool(void) {
  return static_cast<bool>(DeepFuzzy_Bool());
}

DEEPFUZZY_INLINE static size_t Size(void) {
  return DeepFuzzy_Size();
}

DEEPFUZZY_INLINE static uint64_t UInt64(void) {
  return DeepFuzzy_UInt64();
}

DEEPFUZZY_INLINE static int64_t Int64(void) {
  return DeepFuzzy_Int64();
}

DEEPFUZZY_INLINE static uint32_t UInt(void) {
  return DeepFuzzy_UInt();
}

DEEPFUZZY_INLINE static int32_t Int(void) {
  return DeepFuzzy_Int();
}

DEEPFUZZY_INLINE static uint16_t UShort(void) {
  return DeepFuzzy_UShort();
}

DEEPFUZZY_INLINE static int16_t Short(void) {
  return DeepFuzzy_Short();
}

DEEPFUZZY_INLINE static unsigned char UChar(void) {
  return DeepFuzzy_UChar();
}

DEEPFUZZY_INLINE static char Char(void) {
  return DeepFuzzy_Char();
}

DEEPFUZZY_INLINE static bool IsSymbolic(uint64_t x) {
  return DeepFuzzy_IsSymbolicUInt64(x);
}

DEEPFUZZY_INLINE static int IsSymbolic(int64_t x) {
  return DeepFuzzy_IsSymbolicInt64(x);
}

DEEPFUZZY_INLINE static bool IsSymbolic(uint32_t x) {
  return DeepFuzzy_IsSymbolicUInt(x);
}

DEEPFUZZY_INLINE static bool IsSymbolic(int32_t x) {
  return DeepFuzzy_IsSymbolicInt(x);
}

DEEPFUZZY_INLINE static int IsSymbolic(uint16_t x) {
  return DeepFuzzy_IsSymbolicUShort(x);
}

DEEPFUZZY_INLINE static bool IsSymbolic(int16_t x) {
  return DeepFuzzy_IsSymbolicShort(x);
}

DEEPFUZZY_INLINE static bool IsSymbolic(unsigned char x) {
  return DeepFuzzy_IsSymbolicUChar(x);
}

DEEPFUZZY_INLINE static bool IsSymbolic(char x) {
  return DeepFuzzy_IsSymbolicChar(x);
}

DEEPFUZZY_INLINE static bool IsSymbolic(float x) {
  return DeepFuzzy_IsSymbolicFloat(x);
}

DEEPFUZZY_INLINE static bool IsSymbolic(double x) {
  return DeepFuzzy_IsSymbolicDouble(x);
}

// A test fixture.
class Test {
 public:
  Test(void) = default;
  ~Test(void) = default;
  inline void SetUp(void) {}
  inline void TearDown(void) {}

 private:
  Test(const Test &) = delete;
  Test(Test &&) = delete;
  Test &operator=(const Test &) = delete;
  Test &operator=(Test &&) = delete;
};

template <typename T>
class Symbolic {
 public:
  template <typename... Args>
  DEEPFUZZY_INLINE Symbolic(Args&& ...args)
      : value(std::forward<Args...>(args)...) {}

  DEEPFUZZY_INLINE Symbolic(void) {
    T *val_ptr = &value;
    DeepFuzzy_SymbolizeData(val_ptr, &(val_ptr[1]));
  }

  DEEPFUZZY_INLINE operator T (void) const {
    return value;
  }

  T value;
};

template <typename T>
class Symbolic<T &> {};

template <typename T>
class SymbolicLinearContainer {
 public:
  DEEPFUZZY_INLINE explicit SymbolicLinearContainer(size_t len)
      : value(len) {
    if (!value.empty()) {
      DeepFuzzy_SymbolizeData(&(value.front()), &(value.back()));
    }
  }

  DEEPFUZZY_INLINE SymbolicLinearContainer(void) {
    value.reserve(32);
    value.resize(DeepFuzzy_SizeInRange(0, 32));  // Avoids symbolic `malloc`.
  }

  DEEPFUZZY_INLINE operator T (void) const {
    return value;
  }

  T value;
};

template <>
class Symbolic<std::string> : public SymbolicLinearContainer<std::string> {
  using SymbolicLinearContainer::SymbolicLinearContainer;
};

template <>
class Symbolic<std::wstring> : public SymbolicLinearContainer<std::wstring> {
  using SymbolicLinearContainer::SymbolicLinearContainer;
};

template <typename T>
class Symbolic<std::vector<T>> :
    public SymbolicLinearContainer<std::vector<T>> {};

#define MAKE_SYMBOL_SPECIALIZATION(Tname, tname) \
    template <> \
    class Symbolic<tname> { \
     public: \
      using SelfType = Symbolic<tname>; \
      \
      DEEPFUZZY_INLINE Symbolic(void) \
          : value(DeepFuzzy_ ## Tname()) {} \
      \
      DEEPFUZZY_INLINE Symbolic(tname that) \
          : value(that) {} \
      \
      DEEPFUZZY_INLINE Symbolic(const SelfType &that) \
          : value(that.value) {} \
      \
      DEEPFUZZY_INLINE Symbolic(SelfType &&that) \
          : value(std::move(that.value)) {} \
      \
      DEEPFUZZY_INLINE operator tname (void) const { \
        return value; \
      } \
      SelfType &operator=(const SelfType &that) = default; \
      SelfType &operator=(SelfType &&that) = default; \
      SelfType &operator=(tname that) { \
        value = that; \
        return *this; \
      } \
      SelfType &operator+=(tname that) { \
        value += that; \
        return *this; \
      } \
      SelfType &operator-=(tname that) { \
        value -= that; \
        return *this; \
      } \
      SelfType &operator*=(tname that) { \
        value *= that; \
        return *this; \
      } \
      SelfType &operator/=(tname that) { \
        value /= that; \
        return *this; \
      } \
      SelfType &operator>>=(tname that) { \
        value >>= that; \
        return *this; \
      } \
      SelfType &operator<<=(tname that) { \
        value <<= that; \
        return *this; \
      } \
      tname &operator++(void) { \
        return ++value; \
      } \
      tname operator++(int) { \
        auto prev_value = value; \
        value++; \
        return prev_value; \
      } \
      tname value; \
    };

MAKE_SYMBOL_SPECIALIZATION(UInt64, uint64_t)
MAKE_SYMBOL_SPECIALIZATION(Int64, int64_t)
MAKE_SYMBOL_SPECIALIZATION(UInt, uint32_t)
MAKE_SYMBOL_SPECIALIZATION(Int, int32_t)
MAKE_SYMBOL_SPECIALIZATION(UShort, uint16_t)
MAKE_SYMBOL_SPECIALIZATION(Short, int16_t)
MAKE_SYMBOL_SPECIALIZATION(UChar, uint8_t)
MAKE_SYMBOL_SPECIALIZATION(Char, int8_t)


using symbolic_char = Symbolic<char>;
using symbolic_short = Symbolic<short>;
using symbolic_int = Symbolic<int>;
using symbolic_unsigned = Symbolic<unsigned>;
using symbolic_long = Symbolic<long>;

using symbolic_int8_t = Symbolic<int8_t>;
using symbolic_uint8_t = Symbolic<uint8_t>;
using symbolic_int16_t = Symbolic<int16_t>;
using symbolic_uint16_t = Symbolic<uint16_t>;
using symbolic_int32_t = Symbolic<int32_t>;
using symbolic_uint32_t = Symbolic<uint32_t>;
using symbolic_int64_t = Symbolic<int64_t>;
using symbolic_uint64_t = Symbolic<uint64_t>;

#undef MAKE_SYMBOL_SPECIALIZATION

#define MAKE_MINIMIZER(Type, type) \
    DEEPFUZZY_INLINE static type Minimize(type val) { \
      return DeepFuzzy_Min ## Type(val); \
    } \
    DEEPFUZZY_INLINE static type Maximize(type val) { \
      return DeepFuzzy_Max ## Type(val); \
    }

MAKE_MINIMIZER(UInt, uint32_t)
MAKE_MINIMIZER(Int, int32_t)
MAKE_MINIMIZER(UShort, uint16_t)
MAKE_MINIMIZER(Short, int16_t)
MAKE_MINIMIZER(UChar, uint8_t)
MAKE_MINIMIZER(Char, int8_t)

#undef MAKE_MINIMIZER

template <typename T>
static T Pump(T val, unsigned max=10) {
  if (!IsSymbolic(val)) {
    return val;
  }
  if (!max) {
    DeepFuzzy_Abandon("Must have a positive maximum number of values to Pump");
  }
  for (auto i = 0U; i < max - 1; ++i) {
    T min_val = Minimize(val);
    if (val == min_val) {
      DEEPFUZZY_USED(min_val);  // Force the concrete `min_val` to be returned,
                                // as opposed to compiler possibly choosing to
                                // return `val`.
      return min_val;
    }
  }
  return Minimize(val);
}

template <typename... Args>
inline static void ForAll(void (*func)(Args...)) {
  func(Symbolic<Args>()...);
}

template <typename... Args, typename Closure>
inline static void ForAll(Closure func) {
  func(Symbolic<Args>()...);
}

#define PureSwarmOneOf(...) _SwarmOneOf(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define MixedSwarmOneOf(...) _SwarmOneOf(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define ProbSwarmOneOf(...) _SwarmOneOf(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)

#ifndef DEEPFUZZY_PURE_SWARM
#ifndef DEEPFUZZY_MIXED_SWARM
#ifndef DEEPFUZZY_PROB_SWARM
#define OneOf(...) NoSwarmOneOf(__VA_ARGS__)
#endif
#endif
#endif

#ifdef DEEPFUZZY_PURE_SWARM
#define OneOf(...) _SwarmOneOf(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#endif

#ifdef DEEPFUZZY_MIXED_SWARM
#define OneOf(...) _SwarmOneOf(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#endif

#ifdef DEEPFUZZY_PROB_SWARM
#define OneOf(...) _SwarmOneOf(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)
#endif

template <typename... FuncTys>
inline static void NoSwarmOneOf(FuncTys&&... funcs) {
  if (FLAGS_verbose_reads) {
    printf("STARTING OneOf CALL\n");
  }
  std::function<void(void)> func_arr[sizeof...(FuncTys)] = {funcs...};
  unsigned index = DeepFuzzy_UIntInRange(
      0U, static_cast<unsigned>(sizeof...(funcs))-1);
  func_arr[Pump(index, sizeof...(funcs))]();
  if (FLAGS_verbose_reads) {
    printf("FINISHED OneOf CALL\n");
  }
}

template <typename... FuncTys>
inline static void _SwarmOneOf(const char* file, unsigned line, enum DeepFuzzy_SwarmType stype,
			       FuncTys&&... funcs) {
  unsigned fcount = static_cast<unsigned>(sizeof...(funcs));
  std::function<void(void)> func_arr[sizeof...(FuncTys)] = {funcs...};
  struct DeepFuzzy_SwarmConfig* sc = DeepFuzzy_GetSwarmConfig(fcount, file, line, stype);
  if (FLAGS_verbose_reads) {
    printf("STARTING OneOf CALL\n");
  }
  unsigned index = DeepFuzzy_UIntInRange(0U, sc->fcount-1);
  func_arr[sc->fmap[Pump(index, sc->fcount)]]();
  if (FLAGS_verbose_reads) {
    printf("FINISHED OneOf CALL\n");
  }
}

size_t PickIndex(double *probs, size_t length) {
  double total = 0.0;
  size_t missing = 0;
  for (size_t i = 0; i < length; ++i) {
    if (probs[i] >= 0.0) {
      total += probs[i];
    } else{
      ++missing;
    }
  }
  if (total > 1.0) {
    DeepFuzzy_Abandon("Probabilities sum to more than 1.0");
  }
  if (missing > 0) {
    double remainder = (1.0 - total) / missing;
    for (size_t i = 0; i < length; ++i) {
      if (probs[i] < 0.0) {
	probs[i] = remainder;
      }
    }
  } else if (total < 0.999) {
    DeepFuzzy_Abandon("Total of probabilities is significantly less than 1.0");
  }

  // We cannot use DeepFuzzy_Float/Double here because the distribution is very bad
  double P = DeepFuzzy_UIntInRange(0, 10000000)/10000000.0;
  unsigned index = 0;
  double sum = 0.0;
  while ((index < length) && (P > (sum + probs[index]))) {
    sum += probs[index];
    index++;
  }
  return index;
}

typedef std::function<void(void)> func_t;

void ActuallySelectSomething(double *probs, func_t *funcs, size_t length) {
  if (FLAGS_verbose_reads) {
    printf("STARTING OneOf CALL\n");
  }

  funcs[PickIndex(probs, length)]();
  if (FLAGS_verbose_reads) {
    printf("FINISHED OneOf CALL\n");
  }
}

// These two helper functions participate in the splitting.
// Base case does nothing
void SplitArgs(double* probs, func_t *funcs) {
}

// recursive case
template<typename TyFunc, typename... Rest>
void SplitArgs(double* probs, func_t *funcs, double firstProb, TyFunc &&firstFunc, Rest &&... rest) {
  *probs = firstProb;
  *funcs = firstFunc;
  SplitArgs(probs + 1, funcs + 1, std::forward<Rest>(rest)...);
}

// The entry point for OneOfP over lambdas
template<typename... Args>
void OneOfP(Args &&... args) {
  constexpr auto argsLen = sizeof...(Args);
  static_assert((argsLen % 2) == 0, "OneOfP expects probability/lambda pairs");
  constexpr auto length = argsLen / 2;

  double probs[length];
  func_t funcs[length];
  SplitArgs(probs, funcs, std::forward<Args>(args)...);

  ActuallySelectSomething(probs, funcs, length);
}

inline static char NoSwarmOneOf(const char *str) {
  if (!str || !str[0]) {
    DeepFuzzy_Abandon("NULL or empty string passed to OneOf");
  }
  return str[DeepFuzzy_IntInRange(0, strlen(str) - 1)];
}

inline static char _SwarmOneOf(const char* file, unsigned line, enum DeepFuzzy_SwarmType stype, const char *str) {
  if (!str || !str[0]) {
    DeepFuzzy_Abandon("NULL or empty string passed to OneOf");
  }
  unsigned fcount = strlen(str);
  struct DeepFuzzy_SwarmConfig* sc = DeepFuzzy_GetSwarmConfig(fcount, file, line, stype);
  unsigned index = sc->fmap[DeepFuzzy_UIntInRange(0U, sc->fcount-1)];
  return str[index];
}

template <typename T>
inline static const T &NoSwarmOneOf(std::vector<T> &arr) {
  if (arr.empty()) {
    DeepFuzzy_Abandon("Empty vector passed to OneOf");
  }
  return arr[DeepFuzzy_IntInRange(0, arr.size() - 1)];
}

size_t PickListIndex(std::initializer_list<double> probs, size_t length) {
  // The list is interpreted as follows:  a negative probability means "use even distribution"
  // over all probabilities not specified", and the same strategy is used to fill out the list
  // to match the count of items to be chosen among.
  if (probs.size() > length) {
    DeepFuzzy_Abandon("Probability list size greater than number of choices");
  }
  double P[length];
  size_t iP = 0;
  for (std::initializer_list<double>::iterator it = probs.begin(); it != probs.end(); ++it) {
    P[iP++] = *it;
  }
  while (iP < length) {
    P[iP++] = -1.0;
  }

  return PickIndex(P, length);
}

template <typename T>
inline static const T &OneOfP(std::initializer_list<double> probs, std::vector<T> &arr) {
  if (arr.empty()) {
    DeepFuzzy_Abandon("Empty vector passed to OneOf");
  }
  size_t index = PickListIndex(probs, arr.size());
  return arr[index];
}

template <typename T>
inline static const T &_SwarmOneOf(const char* file, unsigned line, enum DeepFuzzy_SwarmType stype, const std::vector<T> &arr) {
  if (arr.empty()) {
    DeepFuzzy_Abandon("Empty vector passed to OneOf");
  }
  unsigned fcount = arr.size();
  struct DeepFuzzy_SwarmConfig* sc = DeepFuzzy_GetSwarmConfig(fcount, file, line, stype);
  unsigned index = sc->fmap[DeepFuzzy_UIntInRange(0U, sc->fcount-1)];
  return arr[index];
}

template <typename T, int len>
inline static const T &NoSwarmOneOf(T (&arr)[len]) {
  if (!len) {
    DeepFuzzy_Abandon("Empty array passed to OneOf");
  }
  return arr[DeepFuzzy_IntInRange(0, len - 1)];
}

template <typename T, int len>
inline static const T &OneOfP(std::initializer_list<double> probs, T (&arr)[len]) {
  if (!len) {
    DeepFuzzy_Abandon("Empty array passed to OneOf");
  }
  size_t index = PickListIndex(probs, len);
  return arr[index];
}

template <typename T, int len>
inline static const T &_SwarmOneOf(const char* file, unsigned line, enum DeepFuzzy_SwarmType stype, T (&arr)[len]) {
  if (!len) {
    DeepFuzzy_Abandon("Empty array passed to OneOf");
  }
  struct DeepFuzzy_SwarmConfig*	sc = DeepFuzzy_GetSwarmConfig(len, file, line, stype);
  unsigned index = sc->fmap[DeepFuzzy_UIntInRange(0U, sc->fcount-1)];
  return arr[index];
}


template <typename T, int k=sizeof(T) * 8>
struct ExpandedCompareIntegral {
  template <typename C>
  static DEEPFUZZY_INLINE bool Compare(T a, T b, C cmp) {
    if (cmp((a & 0xFF), (b & 0xFF))) {
      return ExpandedCompareIntegral<T, k - 8>::Compare(a >> 8, b >> 8, cmp);
    }
    return DeepFuzzy_ZeroSink(k);  // Also false.
  }
};

template <typename T>
struct ExpandedCompareIntegral<T, 0> {
  template <typename C>
  static DEEPFUZZY_INLINE bool Compare(T a, T b, C cmp) {
    if (cmp((a & 0xFF), (b & 0xFF))) {
      return DeepFuzzy_ZeroSink(0);
    } else {
      return DeepFuzzy_ZeroSink(100);
    }
  }
};

template <typename T>
struct DeclType {
  using Type = T;
};

template <typename T>
struct DeclType<T &> : public DeclType<T> {};

template <typename T>
struct DeclType<Symbolic<T>> : public DeclType<T> {};

template <typename T>
struct DeclType<Symbolic<T> &> : public DeclType<T> {};

template <typename T>
struct IsIntegral : public std::is_integral<T> {};

template <typename T>
struct IsIntegral<T &> : public IsIntegral<T> {};

template <typename T>
struct IsIntegral<Symbolic<T>> : public IsIntegral<T> {};

template <typename T>
struct IsSigned : public std::is_signed<T> {};

template <typename T>
struct IsSigned<T &> : public IsSigned<T> {};

template <typename T>
struct IsSigned<Symbolic<T>> : public IsSigned<T> {};

template <typename T>
struct IsUnsigned : public std::is_unsigned<T> {};

template <typename T>
struct IsUnsigned<T &> : public IsUnsigned<T> {};

template <typename T>
struct IsUnsigned<Symbolic<T>> : public std::is_unsigned<T> {};

template <typename A, typename B>
struct BestType {

  // type alias for bools, since std::make_unsigned<bool> returns unexpected behavior
  using _A = typename std::conditional<std::is_same<A, bool>::value, unsigned int, A>::type;
  using _B = typename std::conditional<std::is_same<B, bool>::value, unsigned int, B>::type;

  using UA = typename std::conditional<
      IsUnsigned<B>::value,
      typename std::make_unsigned<_A>::type, A>::type;

  using UB = typename std::conditional<
      IsUnsigned<A>::value,
      typename std::make_unsigned<_B>::type, B>::type;

  using Type = typename std::conditional<(sizeof(UA) > sizeof(UB)),
                                         UA, UB>::type;
};

template <typename A, typename B>
struct Comparer {
  static constexpr bool kIsIntegral = IsIntegral<A>() && IsIntegral<B>();
  static constexpr bool IsBool = std::is_same<A, bool>::value && std::is_same<B, bool>::value;

  struct tag_int {};
  struct tag_not_int {};

  using tag = typename std::conditional<kIsIntegral,tag_int,tag_not_int>::type;

  template <typename C>
  static DEEPFUZZY_INLINE bool Do(const A &a, const B &b, C cmp, tag_not_int) {
    return cmp(a, b);
  }

  template <typename C>
  static DEEPFUZZY_INLINE bool Do(A a, B b, C cmp, tag_int) {
    using T = typename ::deepfuzzy::BestType<A, B>::Type;
    if (cmp(a, b)) {
      return true;
    }
    DEEPFUZZY_USED(a);  // These make the compiler forget everything it knew
    DEEPFUZZY_USED(b);  // about `a` and `b`.
    return ::deepfuzzy::ExpandedCompareIntegral<T>::Compare(a, b, cmp);
  }

  template <typename C>
  static DEEPFUZZY_INLINE bool Do(const A &a, const B &b, C cmp) {

    // IsIntegral returns true for booleans, so we override to basic overloaded method
    // if we have boolean template parameters passed to prevent error in ASSERT_EQ
    if (IsBool) {
      return Do(a, b, cmp, tag_not_int());
    }
    return Do(a, b, cmp, tag());
  }

};

#define DeepFuzzy_PureSwarmAssignCStr(...) _DeepFuzzy_SwarmAssignCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define DeepFuzzy_MixedSwarmAssignCStr(...) _DeepFuzzy_SwarmAssignCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define DeepFuzzy_ProbSwarmAssignCStr(...) _DeepFuzzy_SwarmAssignCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)

#define DeepFuzzy_PureSwarmAssignCStrUpToLen(...) _DeepFuzzy_SwarmAssignCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define DeepFuzzy_MixedSwarmAssignCStrUpToLen(...) _DeepFuzzy_SwarmAssignCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define DeepFuzzy_ProbSwarmAssignCStrUpToLen(...) _DeepFuzzy_SwarmAssignCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)

#define DeepFuzzy_PureSwarmCStr(...) _DeepFuzzy_SwarmCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define DeepFuzzy_MixedSwarmCStr(...) _DeepFuzzy_SwarmCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define DeepFuzzy_ProbSwarmCStr(...) _DeepFuzzy_SwarmCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)

#define DeepFuzzy_PureSwarmCStrUpToLen(...) _DeepFuzzy_SwarmCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define DeepFuzzy_MixedSwarmCStrUpToLen(...) _DeepFuzzy_SwarmCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define DeepFuzzy_ProbSwarmCStrUpToLen(...) _DeepFuzzy_SwarmCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)

#define DeepFuzzy_PureSwarmSymbolizeCStr(...) _DeepFuzzy_SwarmSymbolizeCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define DeepFuzzy_MixedSwarmSymbolizeCStr(...) _DeepFuzzy_SwarmSymbolizeCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define DeepFuzzy_ProbSwarmSymbolizeCStr(...) _DeepFuzzy_SwarmSymbolizeCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)

#ifndef DEEPFUZZY_PURE_SWARM
#ifndef DEEPFUZZY_MIXED_SWARM
#ifndef DEEPFUZZY_PROB_SWARM
#define DeepFuzzy_AssignCStr(...) DeepFuzzy_NoSwarmAssignCStr(__VA_ARGS__)
#define DeepFuzzy_AssignCStrUpToLen(...) DeepFuzzy_NoSwarmAssignCStrUpToLen(__VA_ARGS__)
#define DeepFuzzy_CStr(...) DeepFuzzy_NoSwarmCStr(__VA_ARGS__)
#define DeepFuzzy_CStrUpToLen(...) DeepFuzzy_NoSwarmCStrUpToLen(__VA_ARGS__)
#define DeepFuzzy_SymbolizeCStr(...) DeepFuzzy_NoSwarmSymbolizeCStr(__VA_ARGS__)
#endif
#endif
#endif


#ifdef DEEPFUZZY_PURE_SWARM
#define DeepFuzzy_AssignCStr(...) _DeepFuzzy_SwarmAssignCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define DeepFuzzy_AssignCStrUpToLen(...) _DeepFuzzy_SwarmAssignCStrUpToLen(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define DeepFuzzy_CStr(...) _DeepFuzzy_SwarmCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define DeepFuzzy_CStrUpToLen(...) _DeepFuzzy_SwarmCStrUpToLen(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#define DeepFuzzy_SymbolizeCStr(...) _DeepFuzzy_SwarmSymbolizeCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypePure, __VA_ARGS__)
#endif

#ifdef DEEPFUZZY_MIXED_SWARM
#define DeepFuzzy_AssignCStr(...) _DeepFuzzy_SwarmAssignCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define DeepFuzzy_AssignCStrUpToLen(...) _DeepFuzzy_SwarmAssignCStrUpToLen(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define DeepFuzzy_CStr(...) _DeepFuzzy_SwarmCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define DeepFuzzy_CStrUpToLen(...) _DeepFuzzy_SwarmCStrUpToLen(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#define DeepFuzzy_SymbolizeCStr(...) _DeepFuzzy_SwarmSymbolizeCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeMixed, __VA_ARGS__)
#endif

#ifdef DEEPFUZZY_PROB_SWARM
#define DeepFuzzy_AssignCStr(...) _DeepFuzzy_SwarmAssignCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)
#define DeepFuzzy_AssignCStrUpToLen(...) _DeepFuzzy_SwarmAssignCStrUpToLen(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)
#define DeepFuzzy_CStr(...) _DeepFuzzy_SwarmCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)
#define DeepFuzzy_CStrUpToLen(...) _DeepFuzzy_SwarmCStrUpToLen(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)
#define DeepFuzzy_SymbolizeCStr(...) _DeepFuzzy_SwarmSymbolizeCStr(__FILE__, __LINE__, DeepFuzzy_SwarmTypeProb, __VA_ARGS__)
#endif

/* Like DeepFuzzy_AssignCStr_C, but fills in a null `allowed` value. */
inline static void DeepFuzzy_NoSwarmAssignCStr(char* str, size_t len,
					       const char* allowed = 0) {
  DeepFuzzy_AssignCStr_C(str, len, allowed);
}

inline static void _DeepFuzzy_SwarmAssignCStr(const char* file, unsigned line, enum DeepFuzzy_SwarmType stype,
					      char* str, size_t len,
					      const char* allowed = 0) {
  DeepFuzzy_SwarmAssignCStr_C(file, line, stype, str, len, allowed);
}

/* Like DeepFuzzy_AssignCStr, but Pumps through possible string sizes. */
inline static void DeepFuzzy_NoSwarmAssignCStrUpToLen(char* str, size_t max_len,
						      const char* allowed = 0) {
  uint32_t len = DeepFuzzy_UIntInRange(0, max_len);
  DeepFuzzy_AssignCStr_C(str, Pump(len, max_len+1), allowed);
}

inline static void _DeepFuzzy_SwarmAssignCStrUpToLen(const char* file, unsigned line, enum DeepFuzzy_SwarmType stype,
						     char* str, size_t max_len,
						     const char* allowed = 0) {
  uint32_t len = DeepFuzzy_UIntInRange(0, max_len);
  DeepFuzzy_SwarmAssignCStr_C(file, line, stype, str, Pump(len, max_len+1), allowed);
}

/* Like DeepFuzzy_CStr_C, but fills in a null `allowed` value. */
inline static char* DeepFuzzy_NoSwarmCStr(size_t len, const char* allowed = 0) {
  return DeepFuzzy_CStr_C(len, allowed);
}

inline static char* _DeepFuzzy_SwarmCStr(const char* file, unsigned line, enum DeepFuzzy_SwarmType stype,
					size_t len, const char* allowed = 0) {
  return DeepFuzzy_SwarmCStr_C(file, line, stype, len, allowed);
}

/* Like DeepFuzzy_CStr, but Pumps through possible string sizes. */
inline static char* DeepFuzzy_NoSwarmCStrUpToLen(size_t max_len, const char* allowed = 0) {
  uint32_t len = DeepFuzzy_UIntInRange(0, max_len);
  return DeepFuzzy_CStr_C(Pump(len, max_len+1), allowed);
}

inline static char* _DeepFuzzy_SwarmCStrUpToLen(const char* file, unsigned line, enum DeepFuzzy_SwarmType stype,
					       size_t max_len, const char* allowed = 0) {
  uint32_t len = DeepFuzzy_UIntInRange(0, max_len);
  return DeepFuzzy_SwarmCStr_C(file, line, stype, Pump(len, max_len+1), allowed);
}

/* Like DeepFuzzy_Symbolize_CStr, but fills in null `allowed` value. */
inline static void DeepFuzzy_NoSwarmSymbolizeCStr(char *begin, const char* allowed = 0) {
  DeepFuzzy_SymbolizeCStr_C(begin, allowed);
}

inline static void _DeepFuzzy_SwarmSymbolizeCStr(const char* file, unsigned line, enum DeepFuzzy_SwarmType stype,
						char *begin, const char* allowed = 0) {
  DeepFuzzy_SwarmSymbolizeCStr_C(file, line, stype, begin, allowed);
}
 
}  // namespace deepfuzzy

#define ONE_OF ::deepfuzzy::OneOf

#define TEST(category, name) \
    DeepFuzzy_EntryPoint(category ## _ ## name)

#define _TEST_F(fixture_name, test_name, file, line) \
    class fixture_name ## _ ## test_name : public fixture_name { \
     public: \
      void DoRunTest(void); \
      static void RunTest(void) { \
        do { \
          fixture_name ## _ ## test_name self; \
          self.SetUp(); \
          self.DoRunTest(); \
          self.TearDown(); \
        } while (false); \
        DeepFuzzy_Pass(); \
      } \
      static struct DeepFuzzy_TestInfo kTestInfo; \
    }; \
    struct DeepFuzzy_TestInfo fixture_name ## _ ## test_name::kTestInfo = { \
      nullptr, \
      fixture_name ## _ ## test_name::RunTest, \
      DEEPFUZZY_TO_STR(fixture_name ## _ ## test_name), \
      file, \
      line, \
    }; \
    DEEPFUZZY_INITIALIZER(DeepFuzzy_Register_ ## test_name) { \
      fixture_name ## _ ## test_name::kTestInfo.prev = DeepFuzzy_LastTestInfo; \
      DeepFuzzy_LastTestInfo = &(fixture_name ## _ ## test_name::kTestInfo); \
    } \
    void fixture_name ## _ ## test_name :: DoRunTest(void)


#define _EXPAND_COMPARE(a, b, op) \
  ([] (decltype(a) __a0, decltype(b) __b0) -> bool { \
    using __A = typename ::deepfuzzy::DeclType<decltype(__a0)>::Type; \
    using __B = typename ::deepfuzzy::DeclType<decltype(__b0)>::Type; \
    auto __cmp = [] (__A __a4, __B __b4) { return __a4 op __b4; }; \
    return ::deepfuzzy::Comparer<__A, __B>::Do(__a0, __b0, __cmp); \
  })((a), (b))

#define TEST_F(fixture_name, test_name) \
    _TEST_F(fixture_name, test_name, __FILE__, __LINE__)

#define LOG_DEBUG(cond) \
    ::deepfuzzy::Stream(DeepFuzzy_LogDebug, (cond), __FILE__, __LINE__)

#define LOG_TRACE(cond) \
    ::deepfuzzy::Stream(DeepFuzzy_LogTrace, (cond), __FILE__, __LINE__)

#define LOG_INFO(cond) \
    ::deepfuzzy::Stream(DeepFuzzy_LogInfo, (cond), __FILE__, __LINE__)

#define LOG_WARNING(cond) \
    ::deepfuzzy::Stream(DeepFuzzy_LogWarning, (cond), __FILE__, __LINE__)

#define LOG_WARN(cond) \
    ::deepfuzzy::Stream(DeepFuzzy_LogWarning, (cond), __FILE__, __LINE__)

#define LOG_ERROR(cond) \
    ::deepfuzzy::Stream(DeepFuzzy_LogError, (cond), __FILE__, __LINE__)

#define LOG_FATAL(cond) \
    ::deepfuzzy::Stream(DeepFuzzy_LogFatal, (cond), __FILE__, __LINE__)

#define LOG_CRITICAL(cond) \
    ::deepfuzzy::Stream(DeepFuzzy_LogFatal, (cond), __FILE__, __LINE__)

#define LOG(LEVEL) LOG_ ## LEVEL(true)

#define LOG_IF(LEVEL, cond) LOG_ ## LEVEL(cond)

#define DEEPFUZZY_LOG_EQNE(a, b, op, level) \
    ::deepfuzzy::Stream( \
        level, !(_EXPAND_COMPARE(a, b, op)), __FILE__, __LINE__)

#define DEEPFUZZY_LOG_BINOP(a, b, op, level) \
    ::deepfuzzy::Stream( \
        level, !(a op b), __FILE__, __LINE__)

#define ASSERT_EQ(a, b) DEEPFUZZY_LOG_EQNE(a, b, ==, DeepFuzzy_LogFatal)
#define ASSERT_NE(a, b) DEEPFUZZY_LOG_EQNE(a, b, !=, DeepFuzzy_LogFatal)
#define ASSERT_LT(a, b) DEEPFUZZY_LOG_BINOP(a, b, <, DeepFuzzy_LogFatal)
#define ASSERT_LE(a, b) DEEPFUZZY_LOG_BINOP(a, b, <=, DeepFuzzy_LogFatal)
#define ASSERT_GT(a, b) DEEPFUZZY_LOG_BINOP(a, b, >, DeepFuzzy_LogFatal)
#define ASSERT_GE(a, b) DEEPFUZZY_LOG_BINOP(a, b, >=, DeepFuzzy_LogFatal)

#define CHECK_EQ(a, b) DEEPFUZZY_LOG_EQNE(a, b, ==, DeepFuzzy_LogError)
#define CHECK_NE(a, b) DEEPFUZZY_LOG_EQNE(a, b, !=, DeepFuzzy_LogError)
#define CHECK_LT(a, b) DEEPFUZZY_LOG_BINOP(a, b, <, DeepFuzzy_LogError)
#define CHECK_LE(a, b) DEEPFUZZY_LOG_BINOP(a, b, <=, DeepFuzzy_LogError)
#define CHECK_GT(a, b) DEEPFUZZY_LOG_BINOP(a, b, >, DeepFuzzy_LogError)
#define CHECK_GE(a, b) DEEPFUZZY_LOG_BINOP(a, b, >=, DeepFuzzy_LogError)

#define ASSERT(expr) \
    ::deepfuzzy::Stream( \
        DeepFuzzy_LogFatal, !(expr), __FILE__, __LINE__)

#define ASSERT_TRUE ASSERT
#define ASSERT_FALSE(expr) ASSERT(!(expr))

#define CHECK(expr) \
    ::deepfuzzy::Stream( \
        DeepFuzzy_LogError, !(expr), __FILE__, __LINE__)

#define CHECK_TRUE CHECK
#define CHECK_FALSE(expr) CHECK(!(expr))

#define ASSUME(expr) \
    DeepFuzzy_Assume(expr), ::deepfuzzy::Stream( \
        DeepFuzzy_LogTrace, true, __FILE__, __LINE__)

#define DEEPFUZZY_ASSUME_BINOP(a, b, op) \
    DeepFuzzy_Assume((a op b)), ::deepfuzzy::Stream( \
        DeepFuzzy_LogTrace, true, __FILE__, __LINE__)

#define ASSUME_EQ(a, b) DEEPFUZZY_ASSUME_BINOP(a, b, ==)
#define ASSUME_NE(a, b) DEEPFUZZY_ASSUME_BINOP(a, b, !=)
#define ASSUME_LT(a, b) DEEPFUZZY_ASSUME_BINOP(a, b, <)
#define ASSUME_LE(a, b) DEEPFUZZY_ASSUME_BINOP(a, b, <=)
#define ASSUME_GT(a, b) DEEPFUZZY_ASSUME_BINOP(a, b, >)
#define ASSUME_GE(a, b) DEEPFUZZY_ASSUME_BINOP(a, b, >=)

#endif  // SRC_INCLUDE_DEEPFUZZY_DEEPFUZZY_HPP_