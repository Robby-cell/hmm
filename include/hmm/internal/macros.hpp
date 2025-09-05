#ifndef HMM_INTERNAL_MACROS_HPP
#define HMM_INTERNAL_MACROS_HPP

#include <cassert>

#ifdef _MSVC_LANG
#define HMM_STD_CPP _MSVC_LANG
#else
#define HMM_STD_CPP __cplusplus
#endif

#define HMM_HAS_CXX_11 (HMM_STD_CPP >= 201103L)
#define HMM_HAS_CXX_14 (HMM_STD_CPP >= 201402L)
#define HMM_HAS_CXX_17 (HMM_STD_CPP >= 201703L)
#define HMM_HAS_CXX_20 (HMM_STD_CPP >= 202002L)
#define HMM_HAS_CXX_23 (HMM_STD_CPP >= 202302L)

#if HMM_HAS_CXX_23
#define HMM_STATIC_CALL static
#define HMM_STATIC_CALL_CONST
#else
#define HMM_STATIC_CALL
#define HMM_STATIC_CALL_CONST const
#endif

#if HMM_HAS_CXX_14
#define HMM_CONSTEXPR_14 constexpr
#else
#define HMM_CONSTEXPR_14
#endif

#if HMM_HAS_CXX_17
#define HMM_CONSTEXPR_17 constexpr
#else
#define HMM_CONSTEXPR_17
#endif

#if HMM_HAS_CXX_20
#define HMM_CONSTEXPR_20 constexpr
#else
#define HMM_CONSTEXPR_20
#endif

#define HMM_NODISCARD [[nodiscard]]

#ifdef HMM_NO_CHECKS
#define HMM_ASSERT(...)
#else
#define HMM_ASSERT(...) assert(__VA_ARGS__)
#endif

#endif  // HMM_INTERNAL_MACROS_HPP
