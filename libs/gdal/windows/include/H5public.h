/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * This file contains public declarations for the HDF5 module.
 */
#ifndef _H5public_H
#define _H5public_H

/* Include files for public use... */
/*
 * Since H5pubconf.h is a generated header file, it is messy to try
 * to put a #ifndef _H5pubconf_H ... #endif guard in it.
 * HDF5 has set an internal rule that it is being included here.
 * Source files should NOT include H5pubconf.h directly but include
 * it via H5public.h.  The #ifndef _H5public_H guard above would
 * prevent repeated include.
 */
#include "H5pubconf.h"		/*from configure                             */

/* API Version macro wrapper definitions */
#include "H5version.h"

#ifdef H5_HAVE_FEATURES_H
#include <features.h>           /*for setting POSIX, BSD, etc. compatibility */
#endif
#ifdef H5_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef H5_STDC_HEADERS
#   include <limits.h>		/*for H5T_NATIVE_CHAR defn in H5Tpublic.h    */
#   include <stdarg.h>      /*for variadic functions in H5VLpublic.h     */
#endif
#ifndef __cplusplus
# ifdef H5_HAVE_STDINT_H
#   include <stdint.h>		/*for C9x types				     */
# endif
#else
# ifdef H5_HAVE_STDINT_H_CXX
#   include <stdint.h>		/*for C9x types	when include from C++	     */
# endif
#endif
#ifdef H5_HAVE_INTTYPES_H
#   include <inttypes.h>        /* For uint64_t on some platforms            */
#endif
#ifdef H5_HAVE_STDDEF_H
#   include <stddef.h>
#endif
#ifdef H5_HAVE_PARALLEL
#   include <mpi.h>
#ifndef MPI_FILE_NULL		/*MPIO may be defined in mpi.h already       */
#   include <mpio.h>
#endif
#endif


/* Include the Windows API adapter header early */
#include "H5api_adpt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Macros for enabling/disabling particular GCC warnings */
/* (see the following web-sites for more info:
 *      http://www.dbp-consulting.com/tutorials/SuppressingGCCWarnings.html
 *      http://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html#Diagnostic-Pragmas
 */
/* These pragmas are only implemented usefully in gcc 4.6+ */
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
    #define H5_GCC_DIAG_STR(s) #s
    #define H5_GCC_DIAG_JOINSTR(x,y) H5_GCC_DIAG_STR(x ## y)
    #define H5_GCC_DIAG_DO_PRAGMA(x) _Pragma (#x)
    #define H5_GCC_DIAG_PRAGMA(x) H5_GCC_DIAG_DO_PRAGMA(GCC diagnostic x)

    #define H5_GCC_DIAG_OFF(x) H5_GCC_DIAG_PRAGMA(push) H5_GCC_DIAG_PRAGMA(ignored H5_GCC_DIAG_JOINSTR(-W,x))
    #define H5_GCC_DIAG_ON(x) H5_GCC_DIAG_PRAGMA(pop)
#else
    #define H5_GCC_DIAG_OFF(x)
    #define H5_GCC_DIAG_ON(x)
#endif

/* Version numbers */
#define H5_VERS_MAJOR	1	/* For major interface/format changes  	     */
#define H5_VERS_MINOR	10	/* For minor interface/format changes  	     */
#define H5_VERS_RELEASE	4	/* For tweaks, bug-fixes, or development     */
#define H5_VERS_SUBRELEASE ""	/* For pre-releases like snap0       */
				/* Empty string for real releases.           */
#define H5_VERS_INFO    "HDF5 library version: 1.10.4"      /* Full version string */

#define H5check()	H5check_version(H5_VERS_MAJOR,H5_VERS_MINOR,	      \
				        H5_VERS_RELEASE)

/* macros for comparing the version */
#define H5_VERSION_GE(Maj,Min,Rel) \
       (((H5_VERS_MAJOR==Maj) && (H5_VERS_MINOR==Min) && (H5_VERS_RELEASE>=Rel)) || \
        ((H5_VERS_MAJOR==Maj) && (H5_VERS_MINOR>Min)) || \
        (H5_VERS_MAJOR>Maj))

#define H5_VERSION_LE(Maj,Min,Rel) \
       (((H5_VERS_MAJOR==Maj) && (H5_VERS_MINOR==Min) && (H5_VERS_RELEASE<=Rel)) || \
        ((H5_VERS_MAJOR==Maj) && (H5_VERS_MINOR<Min)) || \
        (H5_VERS_MAJOR<Maj))

/*
 * Status return values.  Failed integer functions in HDF5 result almost
 * always in a negative value (unsigned failing functions sometimes return
 * zero for failure) while successful return is non-negative (often zero).
 * The negative failure value is most commonly -1, but don't bet on it.  The
 * proper way to detect failure is something like:
 *
 * 	if((dset = H5Dopen2(file, name)) < 0)
 *	    fprintf(stderr, "unable to open the requested dataset\n");
 */
typedef int herr_t;


/*
 * Boolean type.  Successful return values are zero (false) or positive
 * (true). The typical true value is 1 but don't bet on it.  Boolean
 * functions cannot fail.  Functions that return `htri_t' however return zero
 * (false), positive (true), or negative (failure). The proper way to test
 * for truth from a htri_t function is:
 *
 * 	if ((retval = H5Tcommitted(type))>0) {
 *	    printf("data type is committed\n");
 *	} else if (!retval) {
 * 	    printf("data type is not committed\n");
 *	} else {
 * 	    printf("error determining whether data type is committed\n");
 *	}
 */
#ifdef H5_HAVE_STDBOOL_H
  #include <stdbool.h>
#else /* H5_HAVE_STDBOOL_H */
  #ifndef __cplusplus
    #if defined(H5_SIZEOF_BOOL) && (H5_SIZEOF_BOOL != 0)
      #define bool    _Bool
    #else
      #define bool    unsigned int
    #endif
    #define true    1
    #define false   0
  #endif /* __cplusplus */
#endif /* H5_HAVE_STDBOOL_H */
typedef bool hbool_t;
typedef int htri_t;

/* Define the ssize_t type if it not is defined */
#if H5_SIZEOF_SSIZE_T==0
/* Undefine this size, we will re-define it in one of the sections below */
#undef H5_SIZEOF_SSIZE_T
#if H5_SIZEOF_SIZE_T==H5_SIZEOF_INT
typedef int ssize_t;
#       define H5_SIZEOF_SSIZE_T H5_SIZEOF_INT
#elif H5_SIZEOF_SIZE_T==H5_SIZEOF_LONG
typedef long ssize_t;
#       define H5_SIZEOF_SSIZE_T H5_SIZEOF_LONG
#elif H5_SIZEOF_SIZE_T==H5_SIZEOF_LONG_LONG
typedef long long ssize_t;
#       define H5_SIZEOF_SSIZE_T H5_SIZEOF_LONG_LONG
#else /* Can't find matching type for ssize_t */
#   error "nothing appropriate for ssize_t"
#endif
#endif

/*
 * The sizes of file objects have their own types defined here, use a 64-bit
 * type.
 */
#if H5_SIZEOF_LONG_LONG >= 8
H5_GCC_DIAG_OFF(long-long)
typedef unsigned long long 	hsize_t;
typedef signed long long	hssize_t;
H5_GCC_DIAG_ON(long-long)
#       define H5_SIZEOF_HSIZE_T H5_SIZEOF_LONG_LONG
#       define H5_SIZEOF_HSSIZE_T H5_SIZEOF_LONG_LONG
#else
#   error "nothing appropriate for hsize_t"
#endif
#define HSIZE_UNDEF             ((hsize_t)(hssize_t)(-1))

/*
 * File addresses have their own types.
 */
#if H5_SIZEOF_INT >= 8
    typedef unsigned                haddr_t;
#   define HADDR_UNDEF              ((haddr_t)(-1))
#   define H5_SIZEOF_HADDR_T        H5_SIZEOF_INT
#   ifdef H5_HAVE_PARALLEL
#       define HADDR_AS_MPI_TYPE    MPI_UNSIGNED
#   endif  /* H5_HAVE_PARALLEL */
#elif H5_SIZEOF_LONG >= 8
    typedef unsigned long           haddr_t;
#   define HADDR_UNDEF              ((haddr_t)(long)(-1))
#   define H5_SIZEOF_HADDR_T        H5_SIZEOF_LONG
#   ifdef H5_HAVE_PARALLEL
#       define HADDR_AS_MPI_TYPE    MPI_UNSIGNED_LONG
#   endif  /* H5_HAVE_PARALLEL */
#elif H5_SIZEOF_LONG_LONG >= 8
    typedef unsigned long long      haddr_t;
#   define HADDR_UNDEF              ((haddr_t)(long long)(-1))
#   define H5_SIZEOF_HADDR_T        H5_SIZEOF_LONG_LONG
#   ifdef H5_HAVE_PARALLEL
#       define HADDR_AS_MPI_TYPE    MPI_LONG_LONG_INT
#   endif  /* H5_HAVE_PARALLEL */
#else
#   error "nothing appropriate for haddr_t"
#endif
#if H5_SIZEOF_HADDR_T == H5_SIZEOF_INT
#   define H5_PRINTF_HADDR_FMT  "%u"
#elif H5_SIZEOF_HADDR_T == H5_SIZEOF_LONG
#   define H5_PRINTF_HADDR_FMT  "%lu"
#elif H5_SIZEOF_HADDR_T == H5_SIZEOF_LONG_LONG
#   define H5_PRINTF_HADDR_FMT  "%" H5_PRINTF_LL_WIDTH "u"
#else
#   error "nothing appropriate for H5_PRINTF_HADDR_FMT"
#endif
#define HADDR_MAX		(HADDR_UNDEF-1)

/* uint32_t type is used for creation order field for messages.  It may be
 * defined in Posix.1g, otherwise it is defined here.
 */
#if H5_SIZEOF_UINT32_T>=4
#elif H5_SIZEOF_SHORT>=4
    typedef short uint32_t;
#   undef H5_SIZEOF_UINT32_T
#   define H5_SIZEOF_UINT32_T H5_SIZEOF_SHORT
#elif H5_SIZEOF_INT>=4
    typedef unsigned int uint32_t;
#   undef H5_SIZEOF_UINT32_T
#   define H5_SIZEOF_UINT32_T H5_SIZEOF_INT
#elif H5_SIZEOF_LONG>=4
    typedef unsigned long uint32_t;
#   undef H5_SIZEOF_UINT32_T
#   define H5_SIZEOF_UINT32_T H5_SIZEOF_LONG
#else
#   error "nothing appropriate for uint32_t"
#endif

/* int64_t type is used for creation order field for links.  It may be
 * defined in Posix.1g, otherwise it is defined here.
 */
#if H5_SIZEOF_INT64_T>=8
#elif H5_SIZEOF_INT>=8
    typedef int int64_t;
#   undef H5_SIZEOF_INT64_T
#   define H5_SIZEOF_INT64_T H5_SIZEOF_INT
#elif H5_SIZEOF_LONG>=8
    typedef long int64_t;
#   undef H5_SIZEOF_INT64_T
#   define H5_SIZEOF_INT64_T H5_SIZEOF_LONG
#elif H5_SIZEOF_LONG_LONG>=8
    typedef long long int64_t;
#   undef H5_SIZEOF_INT64_T
#   define H5_SIZEOF_INT64_T H5_SIZEOF_LONG_LONG
#else
#   error "nothing appropriate for int64_t"
#endif

/* uint64_t type is used for fields for H5O_info_t.  It may be
 * defined in Posix.1g, otherwise it is defined here.
 */
#if H5_SIZEOF_UINT64_T>=8
#elif H5_SIZEOF_INT>=8
    typedef unsigned uint64_t;
#   undef H5_SIZEOF_UINT64_T
#   define H5_SIZEOF_UINT64_T H5_SIZEOF_INT
#elif H5_SIZEOF_LONG>=8
    typedef unsigned long uint64_t;
#   undef H5_SIZEOF_UINT64_T
#   define H5_SIZEOF_UINT64_T H5_SIZEOF_LONG
#elif H5_SIZEOF_LONG_LONG>=8
    typedef unsigned long long uint64_t;
#   undef H5_SIZEOF_UINT64_T
#   define H5_SIZEOF_UINT64_T H5_SIZEOF_LONG_LONG
#else
#   error "nothing appropriate for uint64_t"
#endif

/* Common iteration orders */
typedef enum {
    H5_ITER_UNKNOWN = -1,       /* Unknown order */
    H5_ITER_INC,                /* Increasing order */
    H5_ITER_DEC,                /* Decreasing order */
    H5_ITER_NATIVE,             /* No particular order, whatever is fastest */
    H5_ITER_N		        /* Number of iteration orders */
} H5_iter_order_t;

/* Iteration callback values */
/* (Actually, any positive value will cause the iterator to stop and pass back
 *      that positive value to the function that called the iterator)
 */
#define H5_ITER_ERROR   (-1)
#define H5_ITER_CONT    (0)
#define H5_ITER_STOP    (1)

/*
 * The types of indices on links in groups/attributes on objects.
 * Primarily used for "<do> <foo> by index" routines and for iterating over
 * links in groups/attributes on objects.
 */
typedef enum H5_index_t {
    H5_INDEX_UNKNOWN = -1,	/* Unknown index type			*/
    H5_INDEX_NAME,		/* Index on names 			*/
    H5_INDEX_CRT_ORDER,		/* Index on creation order 		*/
    H5_INDEX_N			/* Number of indices defined 		*/
} H5_index_t;

/*
 * Storage info struct used by H5O_info_t and H5F_info_t
 */
typedef struct H5_ih_info_t {
    hsize_t     index_size;     /* btree and/or list */
    hsize_t     heap_size;
} H5_ih_info_t;

/* Functions in H5.c */
H5_DLL herr_t H5open(void);
H5_DLL herr_t H5close(void);
H5_DLL herr_t H5dont_atexit(void);
H5_DLL herr_t H5garbage_collect(void);
H5_DLL herr_t H5set_free_list_limits (int reg_global_lim, int reg_list_lim,
                int arr_global_lim, int arr_list_lim, int blk_global_lim,
                int blk_list_lim);
H5_DLL herr_t H5get_libversion(unsigned *majnum, unsigned *minnum,
				unsigned *relnum);
H5_DLL herr_t H5check_version(unsigned majnum, unsigned minnum,
			       unsigned relnum);
H5_DLL herr_t H5is_library_threadsafe(hbool_t *is_ts);
H5_DLL herr_t H5free_memory(void *mem);
H5_DLL void *H5allocate_memory(size_t size, hbool_t clear);
H5_DLL void *H5resize_memory(void *mem, size_t size);

#ifdef __cplusplus
}
#endif
#endif /* _H5public_H */
 

