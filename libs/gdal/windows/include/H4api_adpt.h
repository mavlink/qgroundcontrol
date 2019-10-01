/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF.  The full HDF copyright notice, including       *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF/releases/.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id: hdfi.h 5435 2010-08-11 17:31:24Z byrn $ */

#ifndef H4API_ADPT_H
#define H4API_ADPT_H

#include "h4config.h"

/**
 * Provide the macros to adapt the HDF public functions to
 * dll entry points.
 * In addition it provides error lines if the configuration is incorrect.
 **/


/* This will only be defined if HDF4 was built with CMake */
#if defined(H4_BUILT_AS_DYNAMIC_LIB)

#if defined(xdr_shared_EXPORTS)
  #if defined (_MSC_VER) || defined(__MINGW32__) /* MSVC Compiler Case */
    #define XDRLIBAPI extern __declspec(dllexport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define XDRLIBAPI extern __attribute__ ((visibility("default")))
  #endif
#endif /* xdr_shared_EXPORTS */

#if defined(hdf_shared_EXPORTS)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFERRPUBLIC __declspec(dllimport)
    #define HDFPUBLIC __declspec(dllexport)
    #define HDFLIBAPI extern __declspec(dllexport)
    #define HDFFCLIBAPI extern __declspec(dllimport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFERRPUBLIC extern __attribute__ ((visibility("default")))
    #define HDFPUBLIC __attribute__ ((visibility("default")))
    #define HDFLIBAPI extern __attribute__ ((visibility("default")))
    #define HDFFCLIBAPI extern __attribute__ ((visibility("default")))
  #endif
#endif /* hdf_shared_EXPORTS */

#if defined(hdf_fcstub_shared_EXPORTS)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFPUBLIC __declspec(dllexport)
    #define HDFLIBAPI extern __declspec(dllimport)
    #define HDFFCLIBAPI extern __declspec(dllexport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFPUBLIC __attribute__ ((visibility("default")))
    #define HDFLIBAPI extern __attribute__ ((visibility("default")))
    #define HDFFCLIBAPI extern __attribute__ ((visibility("default")))
  #endif
#endif /* hdf_fcstub_shared_EXPORTS */

#if defined(mfhdf_shared_EXPORTS)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFERRPUBLIC extern __declspec(dllimport)
    #define HDFPUBLIC __declspec(dllimport)
    #define HDFLIBAPI extern __declspec(dllexport)
    #define HDFFCLIBAPI extern __declspec(dllimport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFERRPUBLIC extern __attribute__ ((visibility("default")))
    #define HDFPUBLIC __attribute__ ((visibility("default")))
    #define HDFLIBAPI extern __attribute__ ((visibility("default")))
    #define HDFFCLIBAPI extern __attribute__ ((visibility("default")))
  #endif
#endif /* mfhdf_shared_EXPORTS */

#if defined(mfhdf_fcstub_shared_EXPORTS)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFPUBLIC __declspec(dllimport)
    #define HDFLIBAPI extern __declspec(dllimport)
    #define HDFFCLIBAPI extern __declspec(dllexport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFPUBLIC __attribute__ ((visibility("default")))
    #define HDFLIBAPI extern __attribute__ ((visibility("default")))
    #define HDFFCLIBAPI extern __attribute__ ((visibility("default")))
  #endif
#endif /* mfhdf_shared_fcstub_EXPORTS */

#if defined(hdf_test_fcstub_shared_EXPORTS)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFFCLIBAPI extern __declspec(dllexport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFFCLIBAPI extern __attribute__ ((visibility("default")))
  #endif
#endif/* hdf_test_fcstub_shared_EXPORTS */

#if defined(mfhdf_hdiff_shared_EXPORTS)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFPUBLIC __declspec(dllimport)
    #define HDFLIBAPI extern __declspec(dllimport)
    #define HDFTOOLSAPI extern __declspec(dllexport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFPUBLIC __attribute__ ((visibility("default")))
    #define HDFLIBAPI extern __attribute__ ((visibility("default")))
    #define HDFTOOLSAPI extern __attribute__ ((visibility("default")))
  #endif
#endif /* mfhdf_hdiff_shared_EXPORTS */

#if defined(mfhdf_hrepack_shared_EXPORTS)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFPUBLIC __declspec(dllimport)
    #define HDFLIBAPI extern __declspec(dllimport)
    #define HDFTOOLSAPI extern __declspec(dllexport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFPUBLIC __attribute__ ((visibility("default")))
    #define HDFLIBAPI extern __attribute__ ((visibility("default")))
    #define HDFTOOLSAPI extern __attribute__ ((visibility("default")))
  #endif
#endif /* mfhdf_hrepack_shared_EXPORTS */

#if !defined(XDRLIBAPI)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define XDRLIBAPI extern __declspec(dllimport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define XDRLIBAPI extern __attribute__ ((visibility("default")))
  #endif
#endif
#if !defined(HDFERRPUBLIC)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFERRPUBLIC extern __declspec(dllimport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFERRPUBLIC extern __attribute__ ((visibility("default")))
  #endif
#endif
#if !defined(HDFPUBLIC)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFPUBLIC __declspec(dllimport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFPUBLIC __attribute__ ((visibility("default")))
  #endif
#endif
#if !defined(HDFLIBAPI)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFLIBAPI extern __declspec(dllimport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFLIBAPI extern __attribute__ ((visibility("default")))
  #endif
#endif
#if !defined(HDFFCLIBAPI)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFFCLIBAPI extern __declspec(dllimport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFFCLIBAPI extern __attribute__ ((visibility("default")))
  #endif
#endif
#if !defined(HDFTOOLSAPI)
  #if defined (_MSC_VER) || defined(__MINGW32__)  /* MSVC Compiler Case */
    #define HDFTOOLSAPI extern __declspec(dllimport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define HDFTOOLSAPI extern __attribute__ ((visibility("default")))
  #endif
#endif

#else
#  define XDRLIBAPI extern
#  define HDFERRPUBLIC extern
#  define HDFPUBLIC
#  define HDFLIBAPI extern
#  define HDFFCLIBAPI extern
#  define HDFTOOLSAPI extern
#endif /*H4_BUILT_AS_DYNAMIC_LIB  */


#endif /* H4API_ADPT_H */
