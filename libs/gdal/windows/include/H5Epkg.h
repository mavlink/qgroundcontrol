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
 * Programmer:	Quincey Koziol <koziol@hdfgroup.org>
 *		Wednesday, April 11, 2007
 *
 * Purpose:	This file contains declarations which are visible only within
 *		the H5E package.  Source files outside the H5E package should
 *		include H5Eprivate.h instead.
 */
#if !(defined H5E_FRIEND || defined H5E_MODULE)
#error "Do not include this file outside the H5E package!"
#endif

#ifndef _H5Epkg_H
#define _H5Epkg_H

/* Get package's private header */
#include "H5Eprivate.h"

/* Other private headers needed by this file */


/**************************/
/* Package Private Macros */
/**************************/

/* Amount to indent each error */
#define H5E_INDENT              2

/* Number of slots in an error stack */
#define H5E_NSLOTS	        32

#ifdef H5_HAVE_THREADSAFE
/*
 * The per-thread error stack. pthread_once() initializes a special
 * key that will be used by all threads to create a stack specific to
 * each thread individually. The association of stacks to threads will
 * be handled by the pthread library.
 *
 * In order for this macro to work, H5E_get_my_stack() must be preceeded
 * by "H5E_t *estack =".
 */
#define H5E_get_my_stack()  H5E_get_stack()
#else /* H5_HAVE_THREADSAFE */
/*
 * The current error stack.
 */
#define H5E_get_my_stack() (H5E_stack_g + 0)
#endif /* H5_HAVE_THREADSAFE */


/****************************/
/* Package Private Typedefs */
/****************************/

/* Some syntactic sugar to make the compiler happy with two different kinds of callbacks */
#ifndef H5_NO_DEPRECATED_SYMBOLS
typedef struct {
    unsigned    vers;       /* Which version callback to use */
    hbool_t     is_default; /* If the printing function is the library's own. */
    H5E_auto1_t func1;      /* Old-style callback, NO error stack param. */
    H5E_auto2_t func2;      /* New-style callback, with error stack param. */
    H5E_auto1_t func1_default;      /* The saved library's default function - old style. */
    H5E_auto2_t func2_default;      /* The saved library's default function - new style. */
} H5E_auto_op_t;
#else /* H5_NO_DEPRECATED_SYMBOLS */
typedef struct {
    H5E_auto_t  func2;      /* Only the new style callback function is available. */
} H5E_auto_op_t;
#endif /* H5_NO_DEPRECATED_SYMBOLS */ 

/* Some syntactic sugar to make the compiler happy with two different kinds of callbacks */
typedef struct {
    unsigned vers;              /* Which version callback to use */
    union {
#ifndef H5_NO_DEPRECATED_SYMBOLS
        H5E_walk1_t func1;      /* Old-style callback, NO error stack param. */
#endif /* H5_NO_DEPRECATED_SYMBOLS */
        H5E_walk2_t func2;      /* New-style callback, with error stack param. */
    }u;
} H5E_walk_op_t;

/* Error class */
typedef struct H5E_cls_t {
    char *cls_name;             /* Name of error class */
    char *lib_name;             /* Name of library within class */
    char *lib_vers;             /* Version of library */
} H5E_cls_t;

/* Major or minor message */
typedef struct H5E_msg_t {
    char        *msg;           /* Message for error */
    H5E_type_t   type;          /* Type of error (major or minor) */
    H5E_cls_t   *cls;           /* Which error class this message belongs to */
} H5E_msg_t;

/* Error stack */
struct H5E_t {
    size_t nused;		        /* Num slots currently used in stack  */
    H5E_error2_t slot[H5E_NSLOTS];	/* Array of error records	     */
    H5E_auto_op_t auto_op;              /* Operator for 'automatic' error reporting */
    void *auto_data;                    /* Callback data for 'automatic error reporting */
};


/*****************************/
/* Package Private Variables */
/*****************************/

#ifndef H5_HAVE_THREADSAFE
/*
 * The current error stack.
 */
H5_DLLVAR H5E_t	H5E_stack_g[1];
#endif /* H5_HAVE_THREADSAFE */


/******************************/
/* Package Private Prototypes */
/******************************/
H5_DLL herr_t H5E__term_deprec_interface(void);
#ifdef H5_HAVE_THREADSAFE
H5_DLL H5E_t *H5E_get_stack(void);
#endif /* H5_HAVE_THREADSAFE */
H5_DLL herr_t H5E__push_stack(H5E_t *estack, const char *file, const char *func,
    unsigned line, hid_t cls_id, hid_t maj_id, hid_t min_id, const char *desc);
H5_DLL ssize_t H5E_get_msg(const H5E_msg_t *msg_ptr, H5E_type_t *type,
    char *msg, size_t size);
H5_DLL herr_t H5E_print(const H5E_t *estack, FILE *stream, hbool_t bk_compat);
H5_DLL herr_t H5E_walk(const H5E_t *estack, H5E_direction_t direction,
    const H5E_walk_op_t *op, void *client_data);
H5_DLL herr_t H5E_get_auto(const H5E_t *estack, H5E_auto_op_t *op,
    void **client_data);
H5_DLL herr_t H5E_set_auto(H5E_t *estack, const H5E_auto_op_t *op,
    void *client_data);
H5_DLL herr_t H5E_pop(H5E_t *err_stack, size_t count);

#endif /* _H5Epkg_H */

