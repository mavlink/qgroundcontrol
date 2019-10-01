/* port/cpl_config.h.  Generated from cpl_config.h.in by configure.  */
/* port/cpl_config.h.in.  Generated from configure.in by autoheader.  */
/* $Id$ */

/* Define if you want to use pthreads based multiprocessing support */
#define CPL_MULTIPROC_PTHREAD 1

/* Define to 1 if you have the `PTHREAD_MUTEX_RECURSIVE' constant. */
#define HAVE_PTHREAD_MUTEX_RECURSIVE 1

/* Define to 1 if you have the `PTHREAD_MUTEX_ADAPTIVE_NP' constant. */
/* #undef HAVE_PTHREAD_MUTEX_ADAPTIVE_NP */

/* Define to 1 if you have the `pthread_spinlock_t' type. */
/* #undef HAVE_PTHREAD_SPINLOCK */

/* Define to 1 if you have the 5 args `mremap' function. */
#define HAVE_5ARGS_MREMAP 1

/* --prefix directory for GDAL install */
#define GDAL_PREFIX "/external/gdal"

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the `atoll' function. */
#define HAVE_ATOLL 1

/* Define to 1 if you have the `strtoull' function. */
#define HAVE_STRTOULL 1

/* Define to 1 if you have the `getrlimit' function. */
#define HAVE_GETRLIMIT 1

/* Define to 1 if you have the `RLIMIT_AS' constant. */
#define HAVE_RLIMIT_AS 1

/* Define to 1 if you have the <csf.h> header file. */
/* #undef HAVE_CSF_H */

/* Define to 1 if you have the <dbmalloc.h> header file. */
/* #undef HAVE_DBMALLOC_H */

/* Define to 1 if you have the declaration of `strtof', and to 0 if you don't.
   */
#define HAVE_DECL_STRTOF 1

/* Define to 1 if you have the <direct.h> header file. */
/* #undef HAVE_DIRECT_H */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define as 0 or 1 according to the floating point format supported by the
   machine */
#define HAVE_IEEEFP 1

/* Define to 1 if the system has the type `int16'. */
/* #undef HAVE_INT16 */

/* Define to 1 if the system has the type `int32'. */
/* #undef HAVE_INT32 */

/* Define to 1 if the system has the type `int8'. */
/* #undef HAVE_INT8 */

/* Define to 1 if the system has the type `__uint128_t'. */
/* #undef HAVE_UINT128_T */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <jpeglib.h> header file. */
/* #undef HAVE_JPEGLIB_H */

/* Define to 1 if you have the `dl' library (-ldl). */
#define HAVE_LIBDL 1

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `pq' library (-lpq). */
/* #undef HAVE_LIBPQ */

/* Define to 1 if you have the `rt' library (-lrt). */
/* #undef HAVE_LIBRT */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1, if your compiler supports long long data type */
#define HAVE_LONG_LONG 1

/* Define to 1, if your compiler supports uintptr_t data type */
#define HAVE_UINTPTR_T 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <png.h> header file. */
/* #undef HAVE_PNG_H */

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strtof' function. */
#define HAVE_STRTOF 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <values.h> header file. */
/* #undef HAVE_VALUES_H */

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `readlink' function. */
#define HAVE_READLINK 1

/* Define to 1 if you have the `posix_spawnp' function. */
/* #undef HAVE_POSIX_SPAWNP */

/* Define to 1 if you have the `posix_memalign' function. */
#define HAVE_POSIX_MEMALIGN 1

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the `mmap' function. */
#define HAVE_MMAP 1

/* Define to 1 if you have the `sigaction' function. */
#define HAVE_SIGACTION 1

/* Define to 1 if you have the statvfs' function. */
#define HAVE_STATVFS 1

/* Define to 1 if you have the `statvfs64' function. */
#define HAVE_STATVFS64 1

/* Define to 1 if you have the `lstat' function. */
#define HAVE_LSTAT 1

/* Set the native cpu bit order (FILLORDER_LSB2MSB or FILLORDER_MSB2LSB) */
#define HOST_FILLORDER FILLORDER_MSB2LSB

/* Define as const if the declaration of iconv() needs const. */
/* #undef ICONV_CONST */

/* For .cpp files, define as const if the declaration of iconv() needs const. */
/* #undef ICONV_CPP_CONST */

/* Define to 1 if libjvm.so should be dlopen'd */
/* #undef JVM_LIB_DLOPEN */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define for Mac OSX Framework build */
/* #undef MACOSX_FRAMEWORK */

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
/* #undef SIZEOF_LONG */

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG 4

/* The size of `void*', as computed by sizeof. */
#define SIZEOF_VOIDP 4

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you have fseek64, ftell64 */
#define UNIX_STDIO_64 1

/* Define to 1 if you want to use the -fvisibility GCC flag */
/* #undef USE_GCC_VISIBILITY_FLAG */

/* Define to 1 if GCC atomic builtins are available */
#define HAVE_GCC_ATOMIC_BUILTINS 1

/* Define to 1 if GCC bswap builtins are available */
/* #undef HAVE_GCC_BSWAP */

/* Define to name of 64bit fopen function */
#define VSI_FOPEN64 fopen

/* Define to name of 64bit ftruncate function */
#define VSI_FTRUNCATE64 ftruncate64

/* Define to name of 64bit fseek func */
#define VSI_FSEEK64 fseeko

/* Define to name of 64bit ftell func */
#define VSI_FTELL64 ftello

/* Define to 1, if you have 64 bit STDIO API */
#define VSI_LARGE_API_SUPPORTED 1

/* Define to 1, if you have LARGEFILE64_SOURCE */
/* #undef VSI_NEED_LARGEFILE64_SOURCE */

/* Define to name of 64bit stat function */
#define VSI_STAT64 stat64

/* Define to name of 64bit stat structure */
#define VSI_STAT64_T stat64

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the _SC_PHYS_PAGES' constant. */
#define HAVE_SC_PHYS_PAGES 1

/* Use this file to override settings in instances where you're doing FAT compiles
   on Apple.  It is currently off by default because it doesn't seem to work with
   XCode >= 3/28/11 */
/* #include "cpl_config_extras.h" */

/* Define to 1 if you have the `uselocale' function. */
#define HAVE_USELOCALE 1

/* Define to 1 if you have the `std::isnan' function. */
#define HAVE_STD_IS_NAN 1

/* Define to 1 if the compiler supports -Wzero-as-null-pointer-constant */
/* #undef HAVE_GCC_WARNING_ZERO_AS_NULL_POINTER_CONSTANT */
