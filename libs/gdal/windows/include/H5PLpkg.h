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
 * Purpose: This file contains declarations which are visible only within
 *          the H5PL package.  Source files outside the H5PL package should
 *          include H5PLprivate.h instead.
 */

#if !(defined H5PL_FRIEND || defined H5PL_MODULE)
#error "Do not include this file outside the H5PL package!"
#endif

#ifndef _H5PLpkg_H
#define _H5PLpkg_H

/* Include private header file */
#include "H5PLprivate.h"          /* Filter functions                */

/* Other private headers needed by this file */

/**************************/
/* Package Private Macros */
/**************************/

/* Whether to pre-load pathnames for plugin libraries */
#define H5PL_DEFAULT_PATH       H5_DEFAULT_PLUGINDIR


/****************************/
/* Macros for supporting    */
/* both Windows and POSIX   */
/****************************/

/*******************/
/* Windows support */
/*******************/
/*
 * SPECIAL WINDOWS NOTE
 *
 * Some of the Win32 API functions expand to fooA or fooW depending on
 * whether UNICODE or _UNICODE are defined. You MUST explicitly use
 * the A version of the functions to force char * behavior until we
 * work out a scheme for proper Windows Unicode support.
 *
 * If you do not do this, people will be unable to incorporate our
 * source code into their own CMake builds if they define UNICODE.
 */
#ifdef H5_HAVE_WIN32_API

    /* The path separator on this platform */
#   define H5PL_PATH_SEPARATOR      ";"

    /* Handle for dynamic library */
#   define H5PL_HANDLE HINSTANCE

    /* Get a handle to a plugin library.  Windows: TEXT macro handles Unicode strings */
#   define H5PL_OPEN_DLIB(S) LoadLibraryExA(S, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)

    /* Get the address of a symbol in dynamic library */
#   define H5PL_GET_LIB_FUNC(H,N) GetProcAddress(H,N)

    /* Close dynamic library */
#   define H5PL_CLOSE_LIB(H) FreeLibrary(H)

    /* Clear error - nothing to do */
#   define H5PL_CLR_ERROR

    /* maximum size for expanding env vars */
#   define H5PL_EXPAND_BUFFER_SIZE 32767

    typedef const void *(__cdecl *H5PL_get_plugin_info_t)(void);

#else /* H5_HAVE_WIN32_API */

    /*****************/
    /* POSIX support */
    /*****************/

    /* The path separator on this platform */
#   define H5PL_PATH_SEPARATOR     ":"

    /* Handle for dynamic library */
#   define H5PL_HANDLE void *

    /* Get a handle to a plugin library.  Windows: TEXT macro handles Unicode strings */
#   define H5PL_OPEN_DLIB(S) dlopen(S, RTLD_LAZY)

    /* Get the address of a symbol in dynamic library */
#   define H5PL_GET_LIB_FUNC(H,N) dlsym(H,N)

    /* Close dynamic library */
#   define H5PL_CLOSE_LIB(H) dlclose(H)

    /* Clear error */
#   define H5PL_CLR_ERROR HERROR(H5E_PLUGIN, H5E_CANTGET, "can't dlopen:%s", dlerror())

    typedef const void *(*H5PL_get_plugin_info_t)(void);
#endif /* H5_HAVE_WIN32_API */


/****************************/
/* Package Private Typedefs */
/****************************/

/* Data used to search for plugins */
typedef struct H5PL_search_params_t {
    H5PL_type_t         type;
    H5PL_key_t          key;
} H5PL_search_params_t;


/*****************************/
/* Package Private Variables */
/*****************************/


/******************************/
/* Package Private Prototypes */
/******************************/

/* Accessors to global variables and flags */
H5_DLL herr_t H5PL__get_plugin_control_mask(unsigned int *mask /*out*/);
H5_DLL herr_t H5PL__set_plugin_control_mask(unsigned int mask);

/* Plugin search and manipulation */
H5_DLL herr_t H5PL__open(const char *libname, H5PL_type_t type, H5PL_key_t key, hbool_t *success /*out*/, const void **plugin_info /*out*/);
H5_DLL herr_t H5PL__close(H5PL_HANDLE handle);

/* Plugin cache calls */
H5_DLL herr_t H5PL__create_plugin_cache(void);
H5_DLL herr_t H5PL__close_plugin_cache(hbool_t *already_closed /*out*/);
H5_DLL herr_t H5PL__add_plugin(H5PL_type_t type, H5PL_key_t key, H5PL_HANDLE handle);
H5_DLL herr_t H5PL__find_plugin_in_cache(const H5PL_search_params_t *search_params, hbool_t *found /*out*/, const void **plugin_info /*out*/);

/* Plugin search path calls */
H5_DLL herr_t H5PL__create_path_table(void);
H5_DLL herr_t H5PL__close_path_table(void);
H5_DLL unsigned H5PL__get_num_paths(void);
H5_DLL herr_t H5PL__append_path(const char *path);
H5_DLL herr_t H5PL__prepend_path(const char *path);
H5_DLL herr_t H5PL__replace_path(const char *path, unsigned int index);
H5_DLL herr_t H5PL__insert_path(const char *path, unsigned int index);
H5_DLL herr_t H5PL__remove_path(unsigned int index);
H5_DLL const char *H5PL__get_path(unsigned int index);
H5_DLL herr_t H5PL__find_plugin_in_path_table(const H5PL_search_params_t *search_params, hbool_t *found /*out*/, const void **plugin_info /*out*/);

#endif /* _H5PLpkg_H */

