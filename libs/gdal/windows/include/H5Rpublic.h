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
 * This file contains public declarations for the H5R module.
 */
#ifndef _H5Rpublic_H
#define _H5Rpublic_H

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Gpublic.h"
#include "H5Ipublic.h"

/*****************/
/* Public Macros */
/*****************/

/* Note! Be careful with the sizes of the references because they should really
 * depend on the run-time values in the file.  Unfortunately, the arrays need
 * to be defined at compile-time, so we have to go with the worst case sizes
 * for them.  -QAK
 */
#define H5R_OBJ_REF_BUF_SIZE        sizeof(haddr_t)

/* 4 is used instead of sizeof(int) to permit portability between the Crays
 * and other machines (the heap ID is always encoded as an int32 anyway).
 */
#define H5R_DSET_REG_REF_BUF_SIZE   (sizeof(haddr_t) + 4)

/*******************/
/* Public Typedefs */
/*******************/

/* Reference types */
typedef enum H5R_type_t {
    H5R_BADTYPE     =   (-1),   /* Invalid Reference Type               */
    H5R_OBJECT,                 /* Object reference                     */
    H5R_DATASET_REGION,         /* Dataset Region Reference             */
    H5R_MAXTYPE                 /* Highest type (Invalid as true type)  */
} H5R_type_t;

/* Object reference structure for user's code
 * This needs to be large enough to store largest haddr_t on a worst case
 * machine (8 bytes currently).
 */
typedef haddr_t hobj_ref_t;

/* Dataset Region reference structure for user's code
 * (Buffer to store heap ID and index)
 * This needs to be large enough to store largest haddr_t in a worst case
 * machine (8 bytes currently) plus an int
 */
typedef unsigned char hdset_reg_ref_t[H5R_DSET_REG_REF_BUF_SIZE];

/********************/
/* Public Variables */
/********************/

/*********************/
/* Public Prototypes */
/*********************/

#ifdef __cplusplus
extern "C" {
#endif

H5_DLL herr_t H5Rcreate(void *ref, hid_t loc_id, const char *name,
			 H5R_type_t ref_type, hid_t space_id);
H5_DLL hid_t H5Rdereference2(hid_t obj_id, hid_t oapl_id, H5R_type_t ref_type, const void *ref);
H5_DLL hid_t H5Rget_region(hid_t dataset, H5R_type_t ref_type, const void *ref);
H5_DLL herr_t H5Rget_obj_type2(hid_t id, H5R_type_t ref_type, const void *_ref,
    H5O_type_t *obj_type);
H5_DLL ssize_t H5Rget_name(hid_t loc_id, H5R_type_t ref_type, const void *ref,
    char *name /*out*/, size_t size);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/* Function prototypes */
H5_DLL H5G_obj_t H5Rget_obj_type1(hid_t id, H5R_type_t ref_type, const void *_ref);
H5_DLL hid_t H5Rdereference1(hid_t obj_id, H5R_type_t ref_type, const void *ref);

#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif

#endif  /* _H5Rpublic_H */

