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
 * This file contains public declarations for the H5E module.
 */
#ifndef _H5Epublic_H
#define _H5Epublic_H

#include <stdio.h>              /*FILE arg of H5Eprint()                     */

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"

/* Value for the default error stack */
#define H5E_DEFAULT             (hid_t)0

/* Different kinds of error information */
typedef enum H5E_type_t {
    H5E_MAJOR,
    H5E_MINOR
} H5E_type_t;

/* Information about an error; element of error stack */
typedef struct H5E_error2_t {
    hid_t       cls_id;         /*class ID                           */
    hid_t       maj_num;	/*major error ID		     */
    hid_t       min_num;	/*minor error number		     */
    unsigned	line;		/*line in file where error occurs    */
    const char	*func_name;   	/*function in which error occurred   */
    const char	*file_name;	/*file in which error occurred       */
    const char	*desc;		/*optional supplied description      */
} H5E_error2_t;

/* When this header is included from a private header, don't make calls to H5open() */
#undef H5OPEN
#ifndef _H5private_H
#define H5OPEN          H5open(),
#else   /* _H5private_H */
#define H5OPEN
#endif  /* _H5private_H */

/* HDF5 error class */
#define H5E_ERR_CLS		(H5OPEN H5E_ERR_CLS_g)
H5_DLLVAR hid_t H5E_ERR_CLS_g;

/* Include the automatically generated public header information */
/* (This includes the list of major and minor error codes for the library) */
#include "H5Epubgen.h"

/*
 * One often needs to temporarily disable automatic error reporting when
 * trying something that's likely or expected to fail.  The code to try can
 * be nested between calls to H5Eget_auto() and H5Eset_auto(), but it's
 * easier just to use this macro like:
 * 	H5E_BEGIN_TRY {
 *	    ...stuff here that's likely to fail...
 *      } H5E_END_TRY;
 *
 * Warning: don't break, return, or longjmp() from the body of the loop or
 *	    the error reporting won't be properly restored!
 *
 * These two macros still use the old API functions for backward compatibility
 * purpose.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS
#define H5E_BEGIN_TRY {							      \
    unsigned H5E_saved_is_v2;					              \
    union {								      \
        H5E_auto1_t efunc1;						      \
        H5E_auto2_t efunc2;					              \
    } H5E_saved;							      \
    void *H5E_saved_edata;						      \
								    	      \
    (void)H5Eauto_is_v2(H5E_DEFAULT, &H5E_saved_is_v2);		              \
    if(H5E_saved_is_v2) {						      \
        (void)H5Eget_auto2(H5E_DEFAULT, &H5E_saved.efunc2, &H5E_saved_edata); \
        (void)H5Eset_auto2(H5E_DEFAULT, NULL, NULL);		              \
    } else {								      \
        (void)H5Eget_auto1(&H5E_saved.efunc1, &H5E_saved_edata);		      \
        (void)H5Eset_auto1(NULL, NULL);					      \
    }

#define H5E_END_TRY							      \
    if(H5E_saved_is_v2)							      \
        (void)H5Eset_auto2(H5E_DEFAULT, H5E_saved.efunc2, H5E_saved_edata);   \
    else								      \
        (void)H5Eset_auto1(H5E_saved.efunc1, H5E_saved_edata);		      \
}
#else /* H5_NO_DEPRECATED_SYMBOLS */
#define H5E_BEGIN_TRY {							      \
    H5E_auto_t saved_efunc;						      \
    void *H5E_saved_edata;						      \
								    	      \
    (void)H5Eget_auto(H5E_DEFAULT, &saved_efunc, &H5E_saved_edata);	      \
    (void)H5Eset_auto(H5E_DEFAULT, NULL, NULL);

#define H5E_END_TRY							      \
    (void)H5Eset_auto(H5E_DEFAULT, saved_efunc, H5E_saved_edata);	      \
}
#endif /* H5_NO_DEPRECATED_SYMBOLS */

/*
 * Public API Convenience Macros for Error reporting - Documented
 */
/* Use the Standard C __FILE__ & __LINE__ macros instead of typing them in */
#define H5Epush_sim(func, cls, maj, min, str) H5Epush2(H5E_DEFAULT, __FILE__, func, __LINE__, cls, maj, min, str)

/*
 * Public API Convenience Macros for Error reporting - Undocumented
 */
/* Use the Standard C __FILE__ & __LINE__ macros instead of typing them in */
/*  And return after pushing error onto stack */
#define H5Epush_ret(func, cls, maj, min, str, ret) {			      \
    H5Epush2(H5E_DEFAULT, __FILE__, func, __LINE__, cls, maj, min, str);      \
    return(ret);							      \
}

/* Use the Standard C __FILE__ & __LINE__ macros instead of typing them in
 * And goto a label after pushing error onto stack.
 */
#define H5Epush_goto(func, cls, maj, min, str, label) {			      \
    H5Epush2(H5E_DEFAULT, __FILE__, func, __LINE__, cls, maj, min, str);      \
    goto label;								      \
}

/* Error stack traversal direction */
typedef enum H5E_direction_t {
    H5E_WALK_UPWARD	= 0,		/*begin deep, end at API function    */
    H5E_WALK_DOWNWARD	= 1		/*begin at API function, end deep    */
} H5E_direction_t;


#ifdef __cplusplus
extern "C" {
#endif

/* Error stack traversal callback function pointers */
typedef herr_t (*H5E_walk2_t)(unsigned n, const H5E_error2_t *err_desc,
    void *client_data);
typedef herr_t (*H5E_auto2_t)(hid_t estack, void *client_data);

/* Public API functions */
H5_DLL hid_t  H5Eregister_class(const char *cls_name, const char *lib_name,
    const char *version);
H5_DLL herr_t H5Eunregister_class(hid_t class_id);
H5_DLL herr_t H5Eclose_msg(hid_t err_id);
H5_DLL hid_t  H5Ecreate_msg(hid_t cls, H5E_type_t msg_type, const char *msg);
H5_DLL hid_t  H5Ecreate_stack(void);
H5_DLL hid_t  H5Eget_current_stack(void);
H5_DLL herr_t H5Eclose_stack(hid_t stack_id);
H5_DLL ssize_t H5Eget_class_name(hid_t class_id, char *name, size_t size);
H5_DLL herr_t H5Eset_current_stack(hid_t err_stack_id);
H5_DLL herr_t H5Epush2(hid_t err_stack, const char *file, const char *func, unsigned line,
    hid_t cls_id, hid_t maj_id, hid_t min_id, const char *msg, ...);
H5_DLL herr_t H5Epop(hid_t err_stack, size_t count);
H5_DLL herr_t H5Eprint2(hid_t err_stack, FILE *stream);
H5_DLL herr_t H5Ewalk2(hid_t err_stack, H5E_direction_t direction, H5E_walk2_t func,
    void *client_data);
H5_DLL herr_t H5Eget_auto2(hid_t estack_id, H5E_auto2_t *func, void **client_data);
H5_DLL herr_t H5Eset_auto2(hid_t estack_id, H5E_auto2_t func, void *client_data);
H5_DLL herr_t H5Eclear2(hid_t err_stack);
H5_DLL herr_t H5Eauto_is_v2(hid_t err_stack, unsigned *is_stack);
H5_DLL ssize_t H5Eget_msg(hid_t msg_id, H5E_type_t *type, char *msg,
    size_t size);
H5_DLL ssize_t H5Eget_num(hid_t error_stack_id);


/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/* Typedefs */

/* Alias major & minor error types to hid_t's, for compatibility with new
 *      error API in v1.8
 */
typedef hid_t   H5E_major_t;
typedef hid_t   H5E_minor_t;

/* Information about an error element of error stack. */
typedef struct H5E_error1_t {
    H5E_major_t maj_num;                /*major error number                 */
    H5E_minor_t min_num;                /*minor error number                 */
    const char  *func_name;             /*function in which error occurred   */
    const char  *file_name;             /*file in which error occurred       */
    unsigned    line;                   /*line in file where error occurs    */
    const char  *desc;                  /*optional supplied description      */
} H5E_error1_t;

/* Error stack traversal callback function pointers */
typedef herr_t (*H5E_walk1_t)(int n, H5E_error1_t *err_desc, void *client_data);
typedef herr_t (*H5E_auto1_t)(void *client_data);

/* Function prototypes */
H5_DLL herr_t H5Eclear1(void);
H5_DLL herr_t H5Eget_auto1(H5E_auto1_t *func, void **client_data);
H5_DLL herr_t H5Epush1(const char *file, const char *func, unsigned line,
    H5E_major_t maj, H5E_minor_t min, const char *str);
H5_DLL herr_t H5Eprint1(FILE *stream);
H5_DLL herr_t H5Eset_auto1(H5E_auto1_t func, void *client_data);
H5_DLL herr_t H5Ewalk1(H5E_direction_t direction, H5E_walk1_t func,
    void *client_data);
H5_DLL char *H5Eget_major(H5E_major_t maj);
H5_DLL char *H5Eget_minor(H5E_minor_t min);
#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif

#endif /* end _H5Epublic_H */

