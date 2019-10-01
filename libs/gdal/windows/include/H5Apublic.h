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
 * This file contains public declarations for the H5A module.
 */
#ifndef _H5Apublic_H
#define _H5Apublic_H

/* Public headers needed by this file */
#include "H5Ipublic.h"		/* IDs			  		*/
#include "H5Opublic.h"		/* Object Headers			*/
#include "H5Tpublic.h"		/* Datatypes				*/

#ifdef __cplusplus
extern "C" {
#endif

/* Information struct for attribute (for H5Aget_info/H5Aget_info_by_idx) */
typedef struct {
    hbool_t             corder_valid;   /* Indicate if creation order is valid */
    H5O_msg_crt_idx_t   corder;         /* Creation order                 */
    H5T_cset_t          cset;           /* Character set of attribute name */
    hsize_t             data_size;      /* Size of raw data		  */
} H5A_info_t;

/* Typedef for H5Aiterate2() callbacks */
typedef herr_t (*H5A_operator2_t)(hid_t location_id/*in*/,
    const char *attr_name/*in*/, const H5A_info_t *ainfo/*in*/, void *op_data/*in,out*/);

/* Public function prototypes */
H5_DLL hid_t   H5Acreate2(hid_t loc_id, const char *attr_name, hid_t type_id,
    hid_t space_id, hid_t acpl_id, hid_t aapl_id);
H5_DLL hid_t   H5Acreate_by_name(hid_t loc_id, const char *obj_name, const char *attr_name,
    hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t lapl_id);
H5_DLL hid_t   H5Aopen(hid_t obj_id, const char *attr_name, hid_t aapl_id);
H5_DLL hid_t   H5Aopen_by_name(hid_t loc_id, const char *obj_name,
    const char *attr_name, hid_t aapl_id, hid_t lapl_id);
H5_DLL hid_t   H5Aopen_by_idx(hid_t loc_id, const char *obj_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t aapl_id,
    hid_t lapl_id);
H5_DLL herr_t  H5Awrite(hid_t attr_id, hid_t type_id, const void *buf);
H5_DLL herr_t  H5Aread(hid_t attr_id, hid_t type_id, void *buf);
H5_DLL herr_t  H5Aclose(hid_t attr_id);
H5_DLL hid_t   H5Aget_space(hid_t attr_id);
H5_DLL hid_t   H5Aget_type(hid_t attr_id);
H5_DLL hid_t   H5Aget_create_plist(hid_t attr_id);
H5_DLL ssize_t H5Aget_name(hid_t attr_id, size_t buf_size, char *buf);
H5_DLL ssize_t H5Aget_name_by_idx(hid_t loc_id, const char *obj_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
    char *name /*out*/, size_t size, hid_t lapl_id);
H5_DLL hsize_t H5Aget_storage_size(hid_t attr_id);
H5_DLL herr_t  H5Aget_info(hid_t attr_id, H5A_info_t *ainfo /*out*/);
H5_DLL herr_t  H5Aget_info_by_name(hid_t loc_id, const char *obj_name,
    const char *attr_name, H5A_info_t *ainfo /*out*/, hid_t lapl_id);
H5_DLL herr_t  H5Aget_info_by_idx(hid_t loc_id, const char *obj_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
    H5A_info_t *ainfo /*out*/, hid_t lapl_id);
H5_DLL herr_t  H5Arename(hid_t loc_id, const char *old_name, const char *new_name);
H5_DLL herr_t  H5Arename_by_name(hid_t loc_id, const char *obj_name,
    const char *old_attr_name, const char *new_attr_name, hid_t lapl_id);
H5_DLL herr_t  H5Aiterate2(hid_t loc_id, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t *idx, H5A_operator2_t op, void *op_data);
H5_DLL herr_t  H5Aiterate_by_name(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t *idx, H5A_operator2_t op, void *op_data,
    hid_t lapd_id);
H5_DLL herr_t  H5Adelete(hid_t loc_id, const char *name);
H5_DLL herr_t  H5Adelete_by_name(hid_t loc_id, const char *obj_name,
    const char *attr_name, hid_t lapl_id);
H5_DLL herr_t  H5Adelete_by_idx(hid_t loc_id, const char *obj_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t lapl_id);
H5_DLL htri_t H5Aexists(hid_t obj_id, const char *attr_name);
H5_DLL htri_t H5Aexists_by_name(hid_t obj_id, const char *obj_name,
    const char *attr_name, hid_t lapl_id);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/* Macros */


/* Typedefs */

/* Typedef for H5Aiterate1() callbacks */
typedef herr_t (*H5A_operator1_t)(hid_t location_id/*in*/,
    const char *attr_name/*in*/, void *operator_data/*in,out*/);


/* Function prototypes */
H5_DLL hid_t   H5Acreate1(hid_t loc_id, const char *name, hid_t type_id,
    hid_t space_id, hid_t acpl_id);
H5_DLL hid_t   H5Aopen_name(hid_t loc_id, const char *name);
H5_DLL hid_t   H5Aopen_idx(hid_t loc_id, unsigned idx);
H5_DLL int     H5Aget_num_attrs(hid_t loc_id);
H5_DLL herr_t  H5Aiterate1(hid_t loc_id, unsigned *attr_num, H5A_operator1_t op,
    void *op_data);

#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif

#endif /* _H5Apublic_H */

