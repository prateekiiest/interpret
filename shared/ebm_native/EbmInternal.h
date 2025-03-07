// Copyright (c) 2018 Microsoft Corporation
// Licensed under the MIT license.
// Author: Paul Koch <code@koch.ninja>

#ifndef EBM_INTERNAL_H
#define EBM_INTERNAL_H

#include <inttypes.h>
#include <stddef.h> // size_t, ptrdiff_t
#include <limits> // numeric_limits
#include <type_traits> // is_integral
#include <stdlib.h> // free

#include "ebm_native.h"

// !!! VERY IMPORTANT !!!
// this file is incluable into separately compiled zones WITHOUT being put into an anonymous namespace
// as such, all functions and variables MUST be declared static (and preferably inline too) otherwise 
// we'll break the ODR (one definition rule).  Normally this is not a problem, but we compile with 
// different compiler flags for different translation units (*.cpp files) and that that would break 
// rules (5.1) or (5.3) from the C++ standard where it says:
// "There can be more than one definition of a... (and it lists class, enumeration, "
// "each definition of D shall consist of the same sequence of tokens"
// "in each definition of D, corresponding entities shall have the same language linkage"
// By using static we are avoiding the rule since we don't have external linkage
// see the file "IMPORTANT.md" for more details.

#define UNUSED(x) (void)(x)
// TODO: use OUT throughout our codebase
// this just visually tags parameters as being modified by their caller
#define OUT
#define INOUT

// here's how to detect the compiler type for a variety of compilers -> https://sourceforge.net/p/predef/wiki/Compilers/
// disabling warnings with _Pragma detailed info here https://stackoverflow.com/questions/3378560/how-to-disable-gcc-warnings-for-a-few-lines-of-code

#if defined(__clang__) // compiler type (clang++)

#define WARNING_PUSH _Pragma("clang diagnostic push")
#define WARNING_POP _Pragma("clang diagnostic pop")
#define WARNING_DISABLE_UNINITIALIZED_LOCAL_VARIABLE
#define WARNING_DISABLE_SIGNED_UNSIGNED_MISMATCH _Pragma("clang diagnostic ignored \"-Wsign-compare\"")
#define WARNING_DISABLE_POTENTIAL_DIVIDE_BY_ZERO
#define WARNING_DISABLE_NON_LITERAL_PRINTF_STRING _Pragma("clang diagnostic ignored \"-Wformat-nonliteral\"")
#define WARNING_DISABLE_USING_UNINITIALIZED_MEMORY

#if __has_feature(attribute_analyzer_noreturn)
#define ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#endif // __has_feature(attribute_analyzer_noreturn)

#elif defined(__GNUC__) // compiler type (g++)

#define WARNING_PUSH _Pragma("GCC diagnostic push")
#define WARNING_POP _Pragma("GCC diagnostic pop")
#define WARNING_DISABLE_UNINITIALIZED_LOCAL_VARIABLE _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#define WARNING_DISABLE_SIGNED_UNSIGNED_MISMATCH _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")
#define WARNING_DISABLE_POTENTIAL_DIVIDE_BY_ZERO
#define WARNING_DISABLE_NON_LITERAL_PRINTF_STRING
#define WARNING_DISABLE_USING_UNINITIALIZED_MEMORY

#define ANALYZER_NORETURN

#elif defined(__SUNPRO_CC) // compiler type (Oracle Developer Studio)

// The Oracle Developer Studio compiler doesn't seem to have a way to push/pop warning/error messages, but they do have the concept of the "default" which 
// acts as a pop for the specific warning that we turn on/off
// Since we can only default on previously changed warnings, we need to have matching warnings off/default sets, so use WARNING_DEFAULT_* 
// example: WARNING_DISABLE_SOMETHING   _Pragma("error_messages(off,something1,something2)")
// example: WARNING_DEFAULT_SOMETHING   _Pragma("error_messages(default,something1,something2)")

#define WARNING_PUSH
#define WARNING_POP
#define WARNING_DISABLE_UNINITIALIZED_LOCAL_VARIABLE
#define WARNING_DISABLE_SIGNED_UNSIGNED_MISMATCH
#define WARNING_DISABLE_POTENTIAL_DIVIDE_BY_ZERO
#define WARNING_DISABLE_NON_LITERAL_PRINTF_STRING
#define WARNING_DISABLE_USING_UNINITIALIZED_MEMORY

#define ANALYZER_NORETURN

#elif defined(_MSC_VER) // compiler type (Microsoft Visual Studio compiler)

#define WARNING_PUSH __pragma(warning(push))
#define WARNING_POP __pragma(warning(pop))
#define WARNING_DISABLE_UNINITIALIZED_LOCAL_VARIABLE __pragma(warning(disable: 4701))
#define WARNING_DISABLE_SIGNED_UNSIGNED_MISMATCH __pragma(warning(disable: 4018))
#define WARNING_DISABLE_POTENTIAL_DIVIDE_BY_ZERO __pragma(warning(disable: 4723))
#define WARNING_DISABLE_NON_LITERAL_PRINTF_STRING
#define WARNING_DISABLE_USING_UNINITIALIZED_MEMORY __pragma(warning(disable: 6001))

#define ANALYZER_NORETURN

// disable constexpr warning, since GetVectorLength is meant to be ambiguous and it's used everywhere
#pragma warning(disable : 26498)
// disable dereferencing NULL pointer, since the static analysis seems to think any access of a pointer is 
// dereferencing a NULL pointer potentially.
#pragma warning(disable : 6011)
// disable dereferencing NULL pointer (same pointer), since the static analysis seems to think any access 
// of a pointer is dereferencing a NULL pointer potentially.
#pragma warning(disable : 28182)

#else  // compiler type
#error compiler not recognized
#endif // compiler type

#if defined(__clang__) || defined(__GNUC__) || defined(__SUNPRO_CC) // compiler
#ifndef __has_builtin
#define __has_builtin(x) 0 // __has_builtin is supported in newer compilers.  On older compilers diable anything we would check with it
#endif // __has_builtin

#define LIKELY(b) __builtin_expect(static_cast<bool>(b), 1)
#define UNLIKELY(b) __builtin_expect(static_cast<bool>(b), 0)
#define PREDICTABLE(b) (b)

#if __has_builtin(__builtin_unpredictable)
#define UNPREDICTABLE(b) __builtin_unpredictable(b)
#else // __has_builtin(__builtin_unpredictable)
#define UNPREDICTABLE(b) (b)
#endif // __has_builtin(__builtin_unpredictable)

#define INLINE_ALWAYS inline __attribute__((always_inline))

// TODO : use EBM_RESTRICT_FUNCTION_RETURN EBM_RESTRICT_PARAM_VARIABLE and EBM_NOALIAS.  This helps performance by telling the compiler that pointers are 
//   not aliased
// EBM_RESTRICT_FUNCTION_RETURN tells the compiler that a pointer returned from a function in not aliased in any other part of the program 
// (the memory wasn't reached previously)
#define EBM_RESTRICT_FUNCTION_RETURN __declspec(restrict)
// EBM_RESTRICT_PARAM_VARIABLE tells the compiler that a pointer passed into a function doesn't refer to memory passed in via annohter pointer
#define EBM_RESTRICT_PARAM_VARIABLE __restrict
// EBM_NOALIAS tells the compiler that a function does not modify global state and only modified data DIRECTLY pointed to via it's parameters 
// (first level indirection)
#define EBM_NOALIAS __declspec(noalias)

#elif defined(_MSC_VER) // compiler type

#define LIKELY(b) (b)
#define UNLIKELY(b) (b)
#define PREDICTABLE(b) (b)
#define UNPREDICTABLE(b) (b)
#define INLINE_ALWAYS inline __forceinline

#else // compiler type
#error compiler not recognized
#endif // compiler type

// using inlining makes it much harder to debug inline functions (stepping and breakpoints don't work).  
// In debug builds we don't care as much about speed, but we do care about debugability, so we generally 
// want to turn off inlining in debug mode.  BUT, when I make everything non-inlined some trivial wrapper
// functions cause a big slowdown, so we'd rather have two classes of inlining.  The INLINE_ALWAYS
// version that inlines in debug mode, and the INLINE_RELEASE version that only inlines for release builds.  
// BUT, unfortunately, inline functions need to be in headers generally, but if you remove the inline, 
// then you get name collisions on the functions.  Using static is one possible solution, but it can create 
// duplicate copies of the function inside each module that the header is inlucded within if the linker 
// isn't smart.  Another option is to use a dummy template, which forces the compiler to allow 
// definition in a header but combines them afterwards. Lastly, using the non-forced inline works in most 
// cases since the compiler will not inline complicated functions by default.
#ifdef NDEBUG
#define INLINE_RELEASE_UNTEMPLATED INLINE_ALWAYS
#define INLINE_RELEASE_TEMPLATED INLINE_ALWAYS
#else //NDEBUG
#define INLINE_RELEASE_UNTEMPLATED template<bool bUnusedInline = false>
#define INLINE_RELEASE_TEMPLATED
#endif //NDEBUG

INLINE_ALWAYS static void StopClangAnalysis() noexcept ANALYZER_NORETURN {
}

INLINE_ALWAYS static char * strcpy_NO_WARNINGS(char * dest, const char * src) noexcept {
   StopClangAnalysis();
   return strcpy(dest, src);
}

#define FAST_EXP
#define FAST_LOG

// TODO: put a list of all the epilon constants that we use here throughout (use 1e-7 format).  Make it a percentage based on the FloatEbmType data type 
//   minimum eplison from 1 + minimal_change.  If we can make it a constant, then do that, or make it a percentage of a dynamically detected/changing value.  
//   Perhaps take the sqrt of the minimal change from 1?
// when comparing floating point numbers, check this info out: https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/


// TODO: search on all my epsilon values and see if they are being used consistently

// gain should be positive, so any number is essentially illegal, but let's make our number very very negative so that we can't confuse it with small 
// negative values close to zero that might occur due to numeric instability
constexpr static FloatEbmType k_illegalGain = std::numeric_limits<FloatEbmType>::lowest();
constexpr static FloatEbmType k_epsilonNegativeGainAllowed = -1e-7;
constexpr static FloatEbmType k_epsilonNegativeValidationMetricAllowed = -1e-7;
constexpr static FloatEbmType k_epsilonGradient = 1e-7;
#if defined(FAST_EXP) || defined(FAST_LOG)
// with the approximate exp function we can expect a bit of noise.  We might need to increase this further
constexpr static FloatEbmType k_epsilonGradientForBinaryToMulticlass = 1e-1;
#else // defined(FAST_EXP) || defined(FAST_LOG)
constexpr static FloatEbmType k_epsilonGradientForBinaryToMulticlass = 1e-7;
#endif // defined(FAST_EXP) || defined(FAST_LOG)
constexpr static FloatEbmType k_epsilonLogLoss = 1e-7;

// The C++ standard makes it undefined behavior to access memory past the end of an array with a declared length.
// So, without mitigation, the struct hack would be undefined behavior.  We can however formally turn an array 
// into a pointer, thus making our modified struct hack completely legal in C++.  So, for instance, the following
// is illegal in C++:
//
// struct MyStruct { int myInt[1]; };
// MyStruct * pMyStruct = malloc(sizeof(MyStruct) + sizeof(int));
// "pMyStruct->myInt[1] = 3;" 
// 
// Compilers have been getting agressive in using undefined behavior to optimize code, so even though the struct
// hack is still widely used, we don't want to risk invoking undefined behavior. By converting an array 
// into a pointer though with the ArrayToPointer function below, we can make this legal again by always writing: 
//
// "ArrayToPointer(pMyStruct->myInt)[1] = 3;"
//
// I've seen a lot of speculation on the internet that the struct hack is always illegal, but I believe this is
// incorrect using this modified access method.  To illustrate, everything in this example should be completely legal:
//
// struct MyStruct { int myInt[1]; };
// char * pMem = malloc(sizeof(MyStruct) + sizeof(int));
// size_t myOffset = offsetof(MyStruct, myInt);
// int * pInt = reinterpret_cast<int *>(pMem + myOffset);
// pInt[1] = 3;
//
// We endure all this hassle because in a number of places we co-locate memory for performance reasons.  We do allocate 
// sufficient memory for doing this, and we also statically check that our structures are standard layout structures, 
// which is required in order to use the offsetof macro, or in our case array to pointer conversion.
// 
template<typename T>
INLINE_ALWAYS static T * ArrayToPointer(T * const a) noexcept {
   return a;
}
template<typename T>
INLINE_ALWAYS static const T * ArrayToPointer(const T * const a) noexcept {
   return a;
}

// TODO : replace all std::min and std::max and similar comparions that get the min/max with this function
// unlike std::min, our version has explicit noexcept semantics
template<typename T>
INLINE_ALWAYS constexpr static T EbmMin(T v1, T v2) noexcept {
   return UNPREDICTABLE(v1 < v2) ? v1 : v2;
}
// unlike std::max, our version has explicit noexcept semantics
template<typename T>
INLINE_ALWAYS constexpr static T EbmMax(T v1, T v2) noexcept {
   return UNPREDICTABLE(v1 < v2) ? v2 : v1;
}

WARNING_PUSH
WARNING_DISABLE_SIGNED_UNSIGNED_MISMATCH
template<typename TTo, typename TFrom>
INLINE_ALWAYS constexpr static bool IsNumberConvertable(const TFrom number) noexcept {
   // the general rules of conversion are as follows:
   // calling std::numeric_limits<?>::max() returns an item of that type
   // casting and comparing will never give us undefined behavior.  It can give us implementation defined behavior or unspecified behavior, which is legal.
   // Undefined behavior results from overflowing negative integers, but we don't add or subtract.
   // C/C++ uses value preserving instead of sign preserving.  Generally, if you have two integer numbers that you're comparing then if one type can be 
   // converted into the other with no loss in range then that the smaller range integer is converted into the bigger range integer
   // if one type can't cover the entire range of the other, then items are converted to UNSIGNED values.  This is probably the most dangerous 
   // thing for us to deal with

   static_assert(std::is_integral<TTo>::value, "TTo must be integral");
   static_assert(std::is_integral<TFrom>::value, "TFrom must be integral");

   static_assert(std::numeric_limits<TTo>::is_specialized, "TTo must be specialized");
   static_assert(std::numeric_limits<TFrom>::is_specialized, "TFrom must be specialized");

   static_assert(std::numeric_limits<TTo>::is_signed || 0 == std::numeric_limits<TTo>::lowest(), "min of an unsigned TTo value must be zero");
   static_assert(std::numeric_limits<TFrom>::is_signed || 0 == std::numeric_limits<TFrom>::lowest(), "min of an unsigned TFrom value must be zero");
   static_assert(0 <= std::numeric_limits<TTo>::max(), "TTo max must be positive");
   static_assert(0 <= std::numeric_limits<TFrom>::max(), "TFrom max must be positive");
   static_assert(std::numeric_limits<TTo>::is_signed != std::numeric_limits<TFrom>::is_signed || 
      ((std::numeric_limits<TTo>::lowest() <= std::numeric_limits<TFrom>::lowest() && std::numeric_limits<TFrom>::max() <= std::numeric_limits<TTo>::max()) || 
      (std::numeric_limits<TFrom>::lowest() <= std::numeric_limits<TTo>::lowest() && std::numeric_limits<TTo>::max() <= std::numeric_limits<TFrom>::max())), 
      "types should entirely wrap their smaller types or be the same size"
   );

   return std::numeric_limits<TTo>::is_signed ? 
      (std::numeric_limits<TFrom>::is_signed ? (std::numeric_limits<TTo>::lowest() <= number && number <= std::numeric_limits<TTo>::max()) 
         : (number <= std::numeric_limits<TTo>::max())) : (std::numeric_limits<TFrom>::is_signed ? (0 <= number && number <= std::numeric_limits<TTo>::max()) :
         (number <= std::numeric_limits<TTo>::max()));

   // C++11 is pretty limited for constexpr functions and requires everything to be in 1 line (above).  In C++14 though the below more readable code should
   // be used.
   //if(std::numeric_limits<TTo>::is_signed) {
   //   if(std::numeric_limits<TFrom>::is_signed) {
   //      // To signed from signed
   //      // if both operands are the same size, then they should be the same type
   //      // if one operand is bigger, then both operands will be converted to that type and the result will not have unspecified behavior
   //      return std::numeric_limits<TTo>::lowest() <= number && number <= std::numeric_limits<TTo>::max();
   //   } else {
   //      // To signed from unsigned
   //      // if both operands are the same size, then max will be converted to the unsigned type, but that should be fine as max should fit
   //      // if one operand is bigger, then both operands will be converted to that type and the result will not have unspecified behavior
   //      return number <= std::numeric_limits<TTo>::max();
   //   }
   //} else {
   //   if(std::numeric_limits<TFrom>::is_signed) {
   //      // To unsigned from signed
   //      // the zero comparison is done signed.  If number is negative, then the results of the max comparison are unspecified, but we don't care because 
   //         it's not undefined and any value true or false will lead to the same answer since the zero comparison was false.
   //      // For the max comparison, if both operands are the same size, then number will be converted to the unsigned type, which will be fine since we 
   //         already checked that it wasn't zero
   //      // For the max comparison, if one operand is bigger, then both operands will be converted to that type and the result will not have 
   //         unspecified behavior
   //      return 0 <= number && number <= std::numeric_limits<TTo>::max();
   //   } else {
   //      // To unsigned from unsigned
   //      // both are unsigned, so both will be upconverted to the biggest data type and then compared.  There is no undefined or unspecified behavior here
   //      return number <= std::numeric_limits<TTo>::max();
   //   }
   //}
}
WARNING_POP

// there doesn't seem to be a reasonable upper bound for how high you can set the k_cCompilerOptimizedTargetClassesMax value.  The bottleneck seems to be 
// that setting it too high increases compile time and module size
// this is how much the runtime speeds up if you compile it with hard coded vector sizes
// 200 => 2.65%
// 32  => 3.28%
// 16  => 5.12%
// 8   => 5.34%
// 4   => 8.31%
// TODO: increase this up to something like 16.  I have decreased it to 8 in order to make compiling more efficient, and so that I regularily test the 
//   runtime looped version of our code

constexpr static ptrdiff_t k_cCompilerOptimizedTargetClassesMax = 8;
constexpr static ptrdiff_t k_cCompilerOptimizedTargetClassesStart = 3;

static_assert(
   2 <= k_cCompilerOptimizedTargetClassesMax, 
   "we special case binary classification to have only 1 output.  If we remove the compile time optimization for the binary class situation then we would "
   "output model files with two values instead of our special case 1");

typedef size_t StorageDataType;
typedef UIntEbmType ActiveDataType;

constexpr static ptrdiff_t k_regression = -1;
constexpr static ptrdiff_t k_dynamicClassification = 0;
constexpr static ptrdiff_t k_oneScore = 1;
INLINE_ALWAYS constexpr static bool IsRegression(const ptrdiff_t learningTypeOrCountTargetClasses) noexcept {
   return k_regression == learningTypeOrCountTargetClasses;
}
INLINE_ALWAYS constexpr static bool IsClassification(const ptrdiff_t learningTypeOrCountTargetClasses) noexcept {
   return 0 <= learningTypeOrCountTargetClasses;
}
INLINE_ALWAYS constexpr static bool IsBinaryClassification(const ptrdiff_t learningTypeOrCountTargetClasses) noexcept {
#ifdef EXPAND_BINARY_LOGITS
   return UNUSED(learningTypeOrCountTargetClasses), false;
#else // EXPAND_BINARY_LOGITS
   return 2 == learningTypeOrCountTargetClasses;
#endif // EXPAND_BINARY_LOGITS
}
INLINE_ALWAYS constexpr static bool IsMulticlass(const ptrdiff_t learningTypeOrCountTargetClasses) noexcept {
   return IsClassification(learningTypeOrCountTargetClasses) && !IsBinaryClassification(learningTypeOrCountTargetClasses);
}

INLINE_ALWAYS constexpr static size_t GetVectorLength(const ptrdiff_t learningTypeOrCountTargetClasses) noexcept {
   // this will work for anything except if learningTypeOrCountTargetClasses is set to DYNAMIC_CLASSIFICATION which means we should have passed in the 
   // dynamic value since DYNAMIC_CLASSIFICATION is a constant that doesn't tell us anything about the real value
#ifdef EXPAND_BINARY_LOGITS
   return learningTypeOrCountTargetClasses <= ptrdiff_t { 1 } ? size_t { 1 } : static_cast<size_t>(learningTypeOrCountTargetClasses);
#else // EXPAND_BINARY_LOGITS
   return learningTypeOrCountTargetClasses <= ptrdiff_t { 2 } ? size_t { 1 } : static_cast<size_t>(learningTypeOrCountTargetClasses);
#endif // EXPAND_BINARY_LOGITS
}

// THIS NEEDS TO BE A MACRO AND NOT AN INLINE FUNCTION -> an inline function will cause all the parameters to get resolved before calling the function
// We want any arguments to our macro to not get resolved if they are not needed at compile time so that we do less work if it's not needed
// This will effectively turn the variable into a compile time constant if it can be resolved at compile time
// The caller can put pTargetFeature->m_cBins inside the macro call and it will be optimize away if it isn't necessary
// having compile time counts of the target count of classes should allow for loop elimination in most cases and the restoration of SIMD instructions in
// places where you couldn't do so with variable loop iterations
#define GET_LEARNING_TYPE_OR_COUNT_TARGET_CLASSES(MACRO_compilerLearningTypeOrCountTargetClasses, MACRO_runtimeLearningTypeOrCountTargetClasses) \
   (k_dynamicClassification == (MACRO_compilerLearningTypeOrCountTargetClasses) ? (MACRO_runtimeLearningTypeOrCountTargetClasses) : \
   (MACRO_compilerLearningTypeOrCountTargetClasses))

// THIS NEEDS TO BE A MACRO AND NOT AN INLINE FUNCTION -> an inline function will cause all the parameters to get resolved before calling the function
// We want any arguments to our macro to not get resolved if they are not needed at compile time so that we do less work if it's not needed
// This will effectively turn the variable into a compile time constant if it can be resolved at compile time
// having compile time counts of the target count of classes should allow for loop elimination in most cases and the restoration of SIMD instructions in 
// places where you couldn't do so with variable loop iterations
// TODO: use this macro more
// TODO: do we really need the static_cast to size_t here?
#define GET_DIMENSIONS(MACRO_cCompilerDimensions, MACRO_cRuntimeDimensions) \
   (k_dynamicDimensions == (MACRO_cCompilerDimensions) ? static_cast<size_t>(MACRO_cRuntimeDimensions) : static_cast<size_t>(MACRO_cCompilerDimensions))

// THIS NEEDS TO BE A MACRO AND NOT AN INLINE FUNCTION -> an inline function will cause all the parameters to get resolved before calling the function
// We want any arguments to our macro to not get resolved if they are not needed at compile time so that we do less work if it's not needed
// This will effectively turn the variable into a compile time constant if it can be resolved at compile time
// having compile time counts of the target count of classes should allow for loop elimination in most cases and the restoration of SIMD instructions in 
// places where you couldn't do so with variable loop iterations
#define GET_ITEMS_PER_BIT_PACK(MACRO_compilerBitPack, MACRO_runtimeBitPack) \
   (k_cItemsPerBitPackDynamic2 == (MACRO_compilerBitPack) ? \
      static_cast<size_t>(MACRO_runtimeBitPack) : static_cast<size_t>(MACRO_compilerBitPack))

template<typename T>
constexpr static size_t CountBitsRequired(const T maxValue) noexcept {
   // this is a bit inefficient when called in the runtime, but we don't call it anywhere that's important performance wise.
   return T { 0 } == maxValue ? size_t { 0 } : size_t { 1 } + CountBitsRequired<T>(maxValue / T { 2 });
}
template<typename T>
INLINE_ALWAYS constexpr static size_t CountBitsRequiredPositiveMax() noexcept {
   return CountBitsRequired(std::numeric_limits<T>::max());
}

constexpr static size_t k_cBitsForSizeT = CountBitsRequiredPositiveMax<size_t>();

// It's impossible for us to have tensors with more than k_cDimensionsMax dimensions.  Even if we had the minimum 
// number of bins per feature (two), then we would have 2^N memory spaces at our binning step, and 
// that would exceed our memory size if it's greater than the number of bits allowed in a size_t, so on a 
// 64 bit machine, 64 dimensions is a hard maximum.  We can subtract one bit safely, since we know that 
// the rest of our program takes some memory, denying the full 64 bits of memory available.  This extra 
// bit is very helpful since we can then set the 64th bit without overflowing it inside loops and other places
//
// We strip out features with only 1 value since they provide no learning value and they break this nice property
// of having a maximum number of dimensions.
//
// TODO : we can restrict the dimensionatlity even more because HistogramBuckets aren't 1 byte, so we can see 
//        how many would fit into memory.
constexpr static size_t k_cDimensionsMax = k_cBitsForSizeT - 1;
static_assert(k_cDimensionsMax < k_cBitsForSizeT, "reserve the highest bit for bit manipulation space");

constexpr static size_t k_cCompilerOptimizedCountDimensionsMax = 2;

static_assert(1 <= k_cCompilerOptimizedCountDimensionsMax,
   "k_cCompilerOptimizedCountDimensionsMax can be 1 if we want to turn off dimension optimization, but 0 or less is disallowed.");
static_assert(k_cCompilerOptimizedCountDimensionsMax <= k_cDimensionsMax,
   "k_cCompilerOptimizedCountDimensionsMax cannot be larger than the maximum number of dimensions.");

constexpr static size_t k_dynamicDimensions = 0;

constexpr static size_t k_cBitsForStorageType = CountBitsRequiredPositiveMax<StorageDataType>();

INLINE_ALWAYS constexpr static size_t GetCountBits(const size_t cItemsBitPacked) noexcept {
   return k_cBitsForStorageType / cItemsBitPacked;
}

#ifndef TODO_remove_this
constexpr static size_t k_cItemsPerBitPackDynamic = 0;
constexpr static size_t k_cItemsPerBitPackMax = 0; // if there are more than 16 (4 bits), then we should just use a loop since the code will be pretty big
constexpr static size_t k_cItemsPerBitPackMin = 0; // our default binning leads us to 256 values, which is 8 units per 64-bit data pack
INLINE_ALWAYS constexpr static size_t GetNextCountItemsBitPacked(const size_t cItemsBitPackedPrev) noexcept {
   // for 64 bits, the progression is: 64,32,21,16, 12,10,9,8,7,6,5,4,3,2,1 [there are 15 of these]
   // for 32 bits, the progression is: 32,16,10,8,6,5,4,3,2,1 [which are all included in 64 bits]
   return k_cItemsPerBitPackMin == cItemsBitPackedPrev ?
      k_cItemsPerBitPackDynamic : k_cBitsForStorageType / ((k_cBitsForStorageType / cItemsBitPackedPrev) + 1);
}
#endif

constexpr static ptrdiff_t k_cItemsPerBitPackNone = ptrdiff_t { -1 }; // this is for when there is only 1 bin
// TODO : remove the 2 suffixes from these, and verify these are being used!!  AND at the same time verify that we like the sign of anything that uses these constants size_t vs ptrdiff_t
constexpr static ptrdiff_t k_cItemsPerBitPackDynamic2 = ptrdiff_t { 0 };
constexpr static ptrdiff_t k_cItemsPerBitPackMax2 = ptrdiff_t { k_cBitsForStorageType };
static_assert(k_cItemsPerBitPackMax2 <= ptrdiff_t { k_cBitsForStorageType }, "k_cItemsPerBitPackMax too big");
constexpr static ptrdiff_t k_cItemsPerBitPackMin2 = ptrdiff_t { 1 };
static_assert(1 <= k_cItemsPerBitPackMin2 || k_cItemsPerBitPackDynamic2 == k_cItemsPerBitPackMin2 && k_cItemsPerBitPackDynamic2 == k_cItemsPerBitPackMax2, "k_cItemsPerBitPackMin must be positive and can only be zero if both min and max are zero (which means we only use dynamic)");
static_assert(k_cItemsPerBitPackMin2 <= k_cItemsPerBitPackMax2, "bit pack max less than min");
static_assert(
   k_cItemsPerBitPackDynamic2 == k_cItemsPerBitPackMin2 ||
   k_cItemsPerBitPackMin2 == 
   ptrdiff_t { k_cBitsForStorageType } / (ptrdiff_t { k_cBitsForStorageType } / k_cItemsPerBitPackMin2),
   "k_cItemsPerBitPackMin needs to be on the progression series");
static_assert(
   k_cItemsPerBitPackDynamic2 == k_cItemsPerBitPackMax2 ||
   k_cItemsPerBitPackMax2 == 
   ptrdiff_t { k_cBitsForStorageType } / (ptrdiff_t { k_cBitsForStorageType } / k_cItemsPerBitPackMax2),
   "k_cItemsPerBitPackMax needs to be on the progression series");
// if we cover the entire range of possible bit packing, then we don't need the dynamic case!
constexpr static ptrdiff_t k_cItemsPerBitPackLast = (ptrdiff_t { k_cBitsForStorageType } == k_cItemsPerBitPackMax2 &&
   ptrdiff_t { 1 } == k_cItemsPerBitPackMin2) ? ptrdiff_t { 1 } : k_cItemsPerBitPackDynamic2;
INLINE_ALWAYS constexpr static ptrdiff_t GetNextBitPack(const ptrdiff_t cItemsBitPackedPrev) noexcept {
   // for 64 bits, the progression is: 64,32,21,16,12,10,9,8,7,6,5,4,3,2,1,0 (optionaly),-1 (never occurs in this function)
   // [there are 15 of these + the dynamic case + onebin case]
   // for 32 bits, the progression is: 32,16,10,8,6,5,4,3,2,1,0 (optionaly),-1 (never occurs in this function)
   // [which are all included in 64 bits + the dynamic case + onebin case]
   // we can have bit packs of -1, but this function should never see that value
   // this function should also never see the dynamic value 0 because we should terminate the chain at that point
   return k_cItemsPerBitPackMin2 == cItemsBitPackedPrev ? k_cItemsPerBitPackDynamic2 :
      ptrdiff_t { k_cBitsForStorageType } / ((ptrdiff_t { k_cBitsForStorageType } / cItemsBitPackedPrev) + 1);
}




WARNING_PUSH
WARNING_DISABLE_POTENTIAL_DIVIDE_BY_ZERO
INLINE_ALWAYS constexpr static bool IsMultiplyError(const size_t num1, const size_t num2) noexcept {
   // algebraically, we want to know if this is true: std::numeric_limits<size_t>::max() + 1 <= num1 * num2
   // which can be turned into: (std::numeric_limits<size_t>::max() + 1 - num1) / num1 + 1 <= num2
   // which can be turned into: (std::numeric_limits<size_t>::max() + 1 - num1) / num1 < num2
   // which can be turned into: (std::numeric_limits<size_t>::max() - num1 + 1) / num1 < num2
   // which works if num1 == 1, but does not work if num1 is zero, so check for zero first

   // it will never overflow if num1 is zero
   return 0 != num1 && ((std::numeric_limits<size_t>::max() - num1 + 1) / num1 < num2);
}
WARNING_POP

INLINE_ALWAYS constexpr static bool IsAddError(const size_t num1, const size_t num2) noexcept {
   // overflow for unsigned values is defined behavior in C++ and it causes a wrap arround
   return num1 + num2 < num1;
}

// we use the struct hack in a number of places in this code base for putting memory in the optimial location
// the struct hack isn't valid unless a class/struct is standard layout.  standard layout objects cannot
// be allocated with new and delete, so we need to use malloc and free for a number of our objects.  It was
// getting confusing to having some objects free with free and other objects use delete, so we just turned
// everything into malloc/free to keep to a single convention.
// 
// Also, using std::nothrow on new apparently doesn't always return nullptr on all compilers.  Sometimes it just 
// exits. This library sometimes allocates large amounts of memory and we'd like to gracefully handle the case where
// that large amount of memory is too large.  So, instead of using new[] and delete[] we use malloc and free.
//
// There's also a small subset of cases where we allocate a chunk of memory and use it for heterogenious types
// in which case we use pure malloc and then free instead of these helper functions.  In both cases we still
// use free though, so it's less likely to create bugs by accident.
template<typename T>
INLINE_ALWAYS static T * EbmMalloc() noexcept {
   static_assert(!std::is_same<T, void>::value, "don't try allocating a single void item with EbmMalloc");
   T * const a = static_cast<T *>(malloc(sizeof(T)));
   return a;
}
template<typename T>
INLINE_ALWAYS static T * EbmMalloc(const size_t cItems) noexcept {
   constexpr size_t cBytesPerItem = sizeof(typename std::conditional<std::is_same<T, void>::value, char, T>::type);
   static_assert(0 < cBytesPerItem, "can't have a zero sized item");
   bool bOneByte = 1 == cBytesPerItem;
   if(bOneByte) {
      const size_t cBytes = cItems;
      // TODO: !! BEWARE: we do use realloc in some parts of our program still!!
      T * const a = static_cast<T *>(malloc(cBytes));
      return a;
   } else {
      if(UNLIKELY(IsMultiplyError(cItems, cBytesPerItem))) {
         return nullptr;
      } else {
         const size_t cBytes = cItems * cBytesPerItem;
         // TODO: !! BEWARE: we do use realloc in some parts of our program still!!
         StopClangAnalysis(); // for some reason Clang-analysis thinks cBytes can be zero, despite the assert above.
         T * const a = static_cast<T *>(malloc(cBytes));
         return a;
      }
   }
}
template<typename T>
INLINE_ALWAYS static T * EbmMalloc(const size_t cItems, const size_t cBytesPerItem) noexcept {
   if(UNLIKELY(IsMultiplyError(cItems, cBytesPerItem))) {
      return nullptr;
   } else {
      const size_t cBytes = cItems * cBytesPerItem;
      // TODO: !! BEWARE: we do use realloc in some parts of our program still!!
      T * const a = static_cast<T *>(malloc(cBytes));
      return a;
   }
}

INLINE_RELEASE_UNTEMPLATED static const char * SkipWhitespace(const char * s) {
   char oneChar = *s;
   while(0x20 == oneChar || 0x9 <= oneChar && oneChar <= 0xd) {
      // skip whitespace
      ++s;
      oneChar = *s;
   }
   return s;
}

INLINE_ALWAYS static const char * ConvertStringToFloat(const char * const s, double * const pResultOut) {
   // we skip beginning whitespaces (strtod guarantees this)
   // unlike strtod, we also skip trailing whitespaces

   assert(nullptr != s); // can't use EBM_ASSERT in this header!
   assert(nullptr != pResultOut);

   // the C++ standard says this about strtod:
   //   If the subject sequence is empty or does not have the expected form, no
   //   conversion is performed; the value of nptr is stored in the object
   //   pointed to by endptr, provided that endptr is not a null pointer.
   //
   // But, I'm unwilling to trust that there is no variation in the C++ runtime libraries, so I'll do my best to 
   // trust but verify by setting sNext before calling strtod, even though that involves a const cast
   char * sNext = const_cast<char *>(s);
   const double ret = std::strtod(s, &sNext);
   if(s == sNext || nullptr == sNext) {
      // technically, sNext should never be nullptr, but we're again verifying our trust of the C++ library
      return nullptr;
   }
   *pResultOut = ret;
   return SkipWhitespace(sNext);
}

INLINE_RELEASE_UNTEMPLATED static const char * IsStringEqualsCaseInsensitive(const char * sMain, const char * sLabel) {
   // amazingly, C++ doesn't seem to provide a case insensive string comparer
   // this function skips inital whitespaces and trailing whitespaces
   // this function returns nullptr if there is no match, otherwise it returns a pointer to the 
   // first non-whitespace character

   char mainChar = *sMain;
   while(0x20 == mainChar || 0x9 <= mainChar && mainChar <= 0xd) {
      // skip whitespace
      ++sMain;
      mainChar = *sMain;
   }
   char labelChar = *sLabel;
   while(0 != labelChar) {
      if('A' <= mainChar && mainChar <= 'Z') {
         mainChar += 'a' - 'A';
      }
      if('A' <= labelChar && labelChar <= 'Z') {
         // in theory within our executable we could ensure that all labels are lower case, but we want
         // people to tweak the loss and metric registrations, so let's be defensive here and do a full
         // case insensitive compare
         labelChar += 'a' - 'A';
      }
      if(mainChar != labelChar) {
         return nullptr;
      }
      ++sMain;
      ++sLabel;
      mainChar = *sMain;
      labelChar = *sLabel;
   }
   while(0x20 == mainChar || 0x9 <= mainChar && mainChar <= 0xd) {
      // skip whitespace
      ++sMain;
      mainChar = *sMain;
   }
   return sMain;
}

// TODO: figure out if we really want/need to template the handling of different bit packing sizes.  It might
//       be the case that for specific bit sizes, like 8x8, we want to keep our memory stride as small as possible
//       but we might also find that we can apply SIMD at the outer loop level in the places where we use bit
//       packing, so we'd load eight 64-bit numbers at a time and then keep all the interior loops.  In this case
//       the only penalty would be one branch mispredict, but we'd be able to loop over 8 bit extractions at a time
//       We might also pay a penalty if our stride length for the outputs is too long, but we'll have to test that
constexpr static bool k_bUseSIMD = false;

//#define ZERO_FIRST_MULTICLASS_LOGIT

#endif // EBM_INTERNAL_H
