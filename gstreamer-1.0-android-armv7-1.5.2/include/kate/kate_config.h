/* Copyright (C) 2008 Vincent Penquerc'h.
   This file is part of the Kate codec library.
   Written by Vincent Penquerc'h.

   Use, distribution and reproduction of this library is governed
   by a BSD style source license included with this source in the
   file 'COPYING'. Please read these terms before distributing. */

#ifndef KATE_kate_config_h_GUARD
#define KATE_kate_config_h_GUARD

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_FORMAT_MACROS

#include <stddef.h>
#include <limits.h>
#include <sys/types.h>

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef KATE_INTERNAL
#define kate_const
#else
#define kate_const const
#endif

#ifndef kate_malloc
#define kate_malloc malloc
#endif
#ifndef kate_realloc
#define kate_realloc realloc
#endif
#ifndef kate_free
#define kate_free free
#endif

#if defined HAVE_STDINT_H || defined HAVE_INTTYPES_H
typedef int32_t kate_int32_t;
#elif defined int32_t
typedef int32_t kate_int32_t;
#elif defined INT_MAX && INT_MAX==2147483647
typedef int kate_int32_t;
#elif defined SHRT_MAX && SHRT_MAX==2147483647
typedef short int kate_int32_t;
#elif defined LONG_MAX && LONG_MAX==2147483647
typedef long int kate_int32_t;
#elif defined LLONG_MAX && LLONG_MAX==2147483647
typedef long long int kate_int32_t;
#else
#error No 32 bit signed integer found
#endif

#if defined HAVE_STDINT_H || defined HAVE_INTTYPES_H
typedef uint32_t kate_uint32_t;
#elif defined uint32_t
typedef uint32_t kate_uint32_t;
#elif defined UINT_MAX && UINT_MAX==4294967295u
typedef unsigned int kate_uint32_t;
#elif defined USHRT_MAX && USHRT_MAX==4294967295u
typedef short unsigned int kate_uint32_t;
#elif defined ULONG_MAX && ULONG_MAX==4294967295ul
typedef long unsigned int kate_uint32_t;
#elif defined ULLONG_MAX && ULLONG_MAX==4294967295ull
typedef long long unsigned int kate_uint32_t;
#else
#error No 32 bit unsigned integer found
#endif

#if defined HAVE_STDINT_H || defined HAVE_INTTYPES_H
typedef int64_t kate_int64_t;
#elif defined int64_t
typedef int64_t kate_int64_t;
#elif defined INT_MAX && INT_MAX>2147483647
typedef int kate_int64_t;
#elif defined SHRT_MAX && SHRT_MAX>2147483647
typedef short int kate_int64_t;
#elif defined LONG_MAX && LONG_MAX>2147483647
typedef long int kate_int64_t;
#elif defined LLONG_MAX && LLONG_MAX>2147483647
typedef long long int kate_int64_t;
#elif defined LONG_LONG_MAX && LONG_LONG_MAX>2147483647
typedef long long int kate_int64_t;
#elif defined __GNUC__ && __GNUC__>=4 && defined __WORDSIZE && __WORDSIZE==64
/* this case matches glibc, check conservative GCC version just in case */
typedef long int kate_int64_t;
#elif defined __GNUC__ && __GNUC__>=4 && defined __WORDSIZE && __WORDSIZE==32
/* this case matches glibc, check conservative GCC version just in case */
typedef long long int kate_int64_t;
#else
#error No 64 bit signed integer found
#endif

#if defined HAVE_STDINT_H || defined HAVE_INTTYPES_H
typedef uint64_t kate_uint64_t;
#elif defined uint64_t
typedef uint64_t kate_uint64_t;
#elif defined UINT_MAX && UINT_MAX>4294967295u
typedef unsigned int kate_uint64_t;
#elif defined USHRT_MAX && USHRT_MAX>4294967295u
typedef unsigned short int kate_uint64_t;
#elif defined ULONG_MAX && ULONG_MAX>4294967295ul
typedef unsigned long int kate_uint64_t;
#elif defined ULLONG_MAX && ULLONG_MAX>4294967295ull
typedef unsigned long long int kate_uint64_t;
#elif defined ULONG_LONG_MAX && ULONG_LONG_MAX>4294967295ull
typedef unsigned long long int kate_uint64_t;
#elif defined __GNUC__ && __GNUC__>=4 && defined __WORDSIZE && __WORDSIZE==64
/* this case matches glibc, check conservative GCC version just in case */
typedef unsigned long int kate_uint64_t;
#elif defined __GNUC__ && __GNUC__>=4 && defined __WORDSIZE && __WORDSIZE==32
/* this case matches glibc, check conservative GCC version just in case */
typedef unsigned long long int kate_uint64_t;
#else
#error No 64 bit unsigned signed integer found
#endif

#if defined HAVE_STDINT_H || defined HAVE_INTTYPES_H
typedef uintptr_t kate_uintptr_t;
#elif defined uintptr_t
typedef uintptr_t kate_uintptr_t;
#else
typedef size_t kate_uintptr_t;
#endif

typedef float kate_float;

#endif

