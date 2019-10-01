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
 * This file contains function prototypes for each exported function in
 * the H5I module.
 */
#ifndef _H5Ipublic_H
#define _H5Ipublic_H

/* Public headers needed by this file */
#include "H5public.h"

/*
 * Library type values.  Start with `1' instead of `0' because it makes the
 * tracing output look better when hid_t values are large numbers.  Change the
 * TYPE_BITS in H5I.c if the MAXID gets larger than 32 (an assertion will
 * fail otherwise).
 *
 * When adding types here, add a section to the 'misc19' test in test/tmisc.c
 * to verify that the H5I{inc|dec|get}_ref() routines work correctly with it.
 *
 * NOTE: H5I_REFERENCE is not used by the library and has been deprecated
 *       with a tentative removal version of 1.12.0. (DER, July 2017)
 */
typedef enum H5I_type_t {
    H5I_UNINIT      = (-2),     /* uninitialized type                           */
    H5I_BADID       = (-1),     /* invalid Type                                 */
    H5I_FILE        = 1,        /* type ID for File objects                     */
    H5I_GROUP,                  /* type ID for Group objects                    */
    H5I_DATATYPE,               /* type ID for Datatype objects                 */
    H5I_DATASPACE,              /* type ID for Dataspace objects                */
    H5I_DATASET,                /* type ID for Dataset objects                  */
    H5I_ATTR,                   /* type ID for Attribute objects                */
    H5I_REFERENCE,              /* *DEPRECATED* type ID for Reference objects   */
    H5I_VFL,                    /* type ID for virtual file layer               */
    H5I_GENPROP_CLS,            /* type ID for generic property list classes    */
    H5I_GENPROP_LST,            /* type ID for generic property lists           */
    H5I_ERROR_CLASS,            /* type ID for error classes                    */
    H5I_ERROR_MSG,              /* type ID for error messages                   */
    H5I_ERROR_STACK,            /* type ID for error stacks                     */
    H5I_NTYPES                  /* number of library types, MUST BE LAST!       */
} H5I_type_t;

/* Type of atoms to return to users */
typedef int64_t hid_t;
#define H5_SIZEOF_HID_T         H5_SIZEOF_INT64_T

/* An invalid object ID. This is also negative for error return. */
#define H5I_INVALID_HID         (-1)

/*
 * Function for freeing objects. This function will be called with an object
 * ID type number and a pointer to the object. The function should free the
 * object and return non-negative to indicate that the object
 * can be removed from the ID type. If the function returns negative
 * (failure) then the object will remain in the ID type.
 */
typedef herr_t (*H5I_free_t)(void*);

/* Type of the function to compare objects & keys */
typedef int (*H5I_search_func_t)(void *obj, hid_t id, void *key);

#ifdef __cplusplus
extern "C" {
#endif

/* Public API functions */

H5_DLL hid_t H5Iregister(H5I_type_t type, const void *object);
H5_DLL void *H5Iobject_verify(hid_t id, H5I_type_t id_type);
H5_DLL void *H5Iremove_verify(hid_t id, H5I_type_t id_type);
H5_DLL H5I_type_t H5Iget_type(hid_t id);
H5_DLL hid_t H5Iget_file_id(hid_t id);
H5_DLL ssize_t H5Iget_name(hid_t id, char *name/*out*/, size_t size);
H5_DLL int H5Iinc_ref(hid_t id);
H5_DLL int H5Idec_ref(hid_t id);
H5_DLL int H5Iget_ref(hid_t id);
H5_DLL H5I_type_t H5Iregister_type(size_t hash_size, unsigned reserved, H5I_free_t free_func);
H5_DLL herr_t H5Iclear_type(H5I_type_t type, hbool_t force);
H5_DLL herr_t H5Idestroy_type(H5I_type_t type);
H5_DLL int H5Iinc_type_ref(H5I_type_t type);
H5_DLL int H5Idec_type_ref(H5I_type_t type);
H5_DLL int H5Iget_type_ref(H5I_type_t type);
H5_DLL void *H5Isearch(H5I_type_t type, H5I_search_func_t func, void *key);
H5_DLL herr_t H5Inmembers(H5I_type_t type, hsize_t *num_members);
H5_DLL htri_t H5Itype_exists(H5I_type_t type);
H5_DLL htri_t H5Iis_valid(hid_t id);

#ifdef __cplusplus
}
#endif
#endif /* _H5Ipublic_H */

