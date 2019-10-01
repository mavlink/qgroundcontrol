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

/*-------------------------------------------------------------------------
 *
 * Created:             H5Lpublic.h
 *                      Dec 1 2005
 *                      James Laird
 *
 * Purpose:             Public declarations for the H5L package (links)
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5Lpublic_H
#define _H5Lpublic_H

/* Public headers needed by this file */
#include "H5public.h"		/* Generic Functions			*/
#include "H5Ipublic.h"		/* IDs			  		*/
#include "H5Tpublic.h"		/* Datatypes				*/

/*****************/
/* Public Macros */
/*****************/

/* Maximum length of a link's name */
/* (encoded in a 32-bit unsigned integer) */
#define H5L_MAX_LINK_NAME_LEN   ((uint32_t)(-1))  /* (4GB - 1) */

/* Macro to indicate operation occurs on same location */
#define H5L_SAME_LOC (hid_t)0

/* Current version of the H5L_class_t struct */
#define H5L_LINK_CLASS_T_VERS 1

/* Previous versions of the H5L_class_t struct */
#define H5L_LINK_CLASS_T_VERS_0 0

#ifdef __cplusplus
extern "C" {
#endif

/*******************/
/* Public Typedefs */
/*******************/

/* Link class types.
 * Values less than 64 are reserved for the HDF5 library's internal use.
 * Values 64 to 255 are for "user-defined" link class types; these types are
 * defined by HDF5 but their behavior can be overridden by users.
 * Users who want to create new classes of links should contact the HDF5
 * development team at hdfhelp@ncsa.uiuc.edu .
 * These values can never change because they appear in HDF5 files.
 */
typedef enum {
    H5L_TYPE_ERROR = (-1),      /* Invalid link type id         */
    H5L_TYPE_HARD = 0,          /* Hard link id                 */
    H5L_TYPE_SOFT = 1,          /* Soft link id                 */
    H5L_TYPE_EXTERNAL = 64,     /* External link id             */
    H5L_TYPE_MAX = 255	        /* Maximum link type id         */
} H5L_type_t;
#define H5L_TYPE_BUILTIN_MAX H5L_TYPE_SOFT      /* Maximum value link value for "built-in" link types */
#define H5L_TYPE_UD_MIN      H5L_TYPE_EXTERNAL  /* Link ids at or above this value are "user-defined" link types. */

/* Information struct for link (for H5Lget_info/H5Lget_info_by_idx) */
typedef struct {
    H5L_type_t          type;           /* Type of link                   */
    hbool_t             corder_valid;   /* Indicate if creation order is valid */
    int64_t             corder;         /* Creation order                 */
    H5T_cset_t          cset;           /* Character set of link name     */
    union {
        haddr_t         address;        /* Address hard link points to    */
        size_t          val_size;       /* Size of a soft link or UD link value */
    } u;
} H5L_info_t;

/* The H5L_class_t struct can be used to override the behavior of a
 * "user-defined" link class. Users should populate the struct with callback
 * functions defined below.
 */
/* Callback prototypes for user-defined links */
/* Link creation callback */
typedef herr_t (*H5L_create_func_t)(const char *link_name, hid_t loc_group,
    const void *lnkdata, size_t lnkdata_size, hid_t lcpl_id);

/* Callback for when the link is moved */
typedef herr_t (*H5L_move_func_t)(const char *new_name, hid_t new_loc,
    const void *lnkdata, size_t lnkdata_size);

/* Callback for when the link is copied */
typedef herr_t (*H5L_copy_func_t)(const char *new_name, hid_t new_loc,
    const void *lnkdata, size_t lnkdata_size);

/* Callback during link traversal */
typedef hid_t (*H5L_traverse_0_func_t)(const char *link_name, hid_t cur_group,
    const void *lnkdata, size_t lnkdata_size, hid_t lapl_id);
typedef hid_t (*H5L_traverse_func_t)(const char *link_name, hid_t cur_group,
    const void *lnkdata, size_t lnkdata_size, hid_t lapl_id, hid_t dxpl_id);

/* Callback for when the link is deleted */
typedef herr_t (*H5L_delete_func_t)(const char *link_name, hid_t file,
    const void *lnkdata, size_t lnkdata_size);

/* Callback for querying the link */
/* Returns the size of the buffer needed */
typedef ssize_t (*H5L_query_func_t)(const char *link_name, const void *lnkdata,
    size_t lnkdata_size, void *buf /*out*/, size_t buf_size);

/* User-defined link types */
typedef struct {
    int version;                    /* Version number of this struct        */
    H5L_type_t id;                  /* Link type ID                         */
    const char *comment;            /* Comment for debugging                */
    H5L_create_func_t create_func;  /* Callback during link creation        */
    H5L_move_func_t move_func;      /* Callback after moving link           */
    H5L_copy_func_t copy_func;      /* Callback after copying link          */
    H5L_traverse_0_func_t trav_func; /* Callback during link traversal       */
    H5L_delete_func_t del_func;     /* Callback for link deletion           */
    H5L_query_func_t query_func;    /* Callback for queries                 */
} H5L_class_0_t;

typedef struct {
    int version;                    /* Version number of this struct        */
    H5L_type_t id;                  /* Link type ID                         */
    const char *comment;            /* Comment for debugging                */
    H5L_create_func_t create_func;  /* Callback during link creation        */
    H5L_move_func_t move_func;      /* Callback after moving link           */
    H5L_copy_func_t copy_func;      /* Callback after copying link          */
    H5L_traverse_func_t trav_func;  /* Callback during link traversal       */
    H5L_delete_func_t del_func;     /* Callback for link deletion           */
    H5L_query_func_t query_func;    /* Callback for queries                 */
} H5L_class_t;

/* Prototype for H5Literate/H5Literate_by_name() operator */
typedef herr_t (*H5L_iterate_t)(hid_t group, const char *name, const H5L_info_t *info,
    void *op_data);

/* Callback for external link traversal */
typedef herr_t (*H5L_elink_traverse_t)(const char *parent_file_name,
    const char *parent_group_name, const char *child_file_name,
    const char *child_object_name, unsigned *acc_flags, hid_t fapl_id,
    void *op_data);


/********************/
/* Public Variables */
/********************/


/*********************/
/* Public Prototypes */
/*********************/
H5_DLL herr_t H5Lmove(hid_t src_loc, const char *src_name, hid_t dst_loc,
    const char *dst_name, hid_t lcpl_id, hid_t lapl_id);
H5_DLL herr_t H5Lcopy(hid_t src_loc, const char *src_name, hid_t dst_loc,
    const char *dst_name, hid_t lcpl_id, hid_t lapl_id);
H5_DLL herr_t H5Lcreate_hard(hid_t cur_loc, const char *cur_name,
    hid_t dst_loc, const char *dst_name, hid_t lcpl_id, hid_t lapl_id);
H5_DLL herr_t H5Lcreate_soft(const char *link_target, hid_t link_loc_id,
    const char *link_name, hid_t lcpl_id, hid_t lapl_id);
H5_DLL herr_t H5Ldelete(hid_t loc_id, const char *name, hid_t lapl_id);
H5_DLL herr_t H5Ldelete_by_idx(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t lapl_id);
H5_DLL herr_t H5Lget_val(hid_t loc_id, const char *name, void *buf/*out*/,
    size_t size, hid_t lapl_id);
H5_DLL herr_t H5Lget_val_by_idx(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
    void *buf/*out*/, size_t size, hid_t lapl_id);
H5_DLL htri_t H5Lexists(hid_t loc_id, const char *name, hid_t lapl_id);
H5_DLL herr_t H5Lget_info(hid_t loc_id, const char *name,
    H5L_info_t *linfo /*out*/, hid_t lapl_id);
H5_DLL herr_t H5Lget_info_by_idx(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
    H5L_info_t *linfo /*out*/, hid_t lapl_id);
H5_DLL ssize_t H5Lget_name_by_idx(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
    char *name /*out*/, size_t size, hid_t lapl_id);
H5_DLL herr_t H5Literate(hid_t grp_id, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t *idx, H5L_iterate_t op, void *op_data);
H5_DLL herr_t H5Literate_by_name(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t *idx,
    H5L_iterate_t op, void *op_data, hid_t lapl_id);
H5_DLL herr_t H5Lvisit(hid_t grp_id, H5_index_t idx_type, H5_iter_order_t order,
    H5L_iterate_t op, void *op_data);
H5_DLL herr_t H5Lvisit_by_name(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, H5L_iterate_t op,
    void *op_data, hid_t lapl_id);

/* UD link functions */
H5_DLL herr_t H5Lcreate_ud(hid_t link_loc_id, const char *link_name,
    H5L_type_t link_type, const void *udata, size_t udata_size, hid_t lcpl_id,
    hid_t lapl_id);
H5_DLL herr_t H5Lregister(const H5L_class_t *cls);
H5_DLL herr_t H5Lunregister(H5L_type_t id);
H5_DLL htri_t H5Lis_registered(H5L_type_t id);

/* External link functions */
H5_DLL herr_t H5Lunpack_elink_val(const void *ext_linkval/*in*/, size_t link_size,
   unsigned *flags, const char **filename/*out*/, const char **obj_path /*out*/);
H5_DLL herr_t H5Lcreate_external(const char *file_name, const char *obj_name,
    hid_t link_loc_id, const char *link_name, hid_t lcpl_id, hid_t lapl_id);

#ifdef __cplusplus
}
#endif
#endif /* _H5Lpublic_H */

