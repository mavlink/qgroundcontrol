/*
 * This files has neccesary definitions to provide SZIP DLL 
 * support on Windows platfroms, both MSVC and CodeWarrior 
 * compilers
 */

#ifndef SZAPI_ADPT_H
#define SZAPI_ADPT_H

/* This will only be defined if szip was built with CMake shared libs*/
#ifdef SZ_BUILT_AS_DYNAMIC_LIB

#if defined (szip_shared_EXPORTS)
  #define _SZDLL_
  #if defined (_MSC_VER)  /* MSVC Compiler Case */
    #define __SZ_DLL__ __declspec(dllexport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define __SZ_DLL__ __attribute__ ((visibility("default")))
  #endif
#else
  #if defined (_MSC_VER)  /* MSVC Compiler Case */
    #define __SZ_DLL__ __declspec(dllimport)
  #elif (__GNUC__ >= 4)  /* GCC 4.x has support for visibility options */
    #define __SZ_DLL__ __attribute__ ((visibility("default")))
  #endif
#endif

#else /* SZ_BUILT_AS_DYNAMIC_LIB */
  #define __SZ_DLL__
#endif /* SZ_BUILT_AS_DYNAMIC_LIB */

#endif /* H5API_ADPT_H */
