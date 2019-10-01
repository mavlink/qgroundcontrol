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
 * This file contains public declarations for the H5D module.
 */
#ifndef _H5Dpublic_H
#define _H5Dpublic_H

/* System headers needed by this file */

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"

/*****************/
/* Public Macros */
/*****************/

/* Macros used to "unset" chunk cache configuration parameters */
#define H5D_CHUNK_CACHE_NSLOTS_DEFAULT      ((size_t) -1)
#define H5D_CHUNK_CACHE_NBYTES_DEFAULT      ((size_t) -1)
#define H5D_CHUNK_CACHE_W0_DEFAULT          (-1.0f)

/* Bit flags for the H5Pset_chunk_opts() and H5Pget_chunk_opts() */
#define H5D_CHUNK_DONT_FILTER_PARTIAL_CHUNKS      (0x0002u)

/*******************/
/* Public Typedefs */
/*******************/

/* Values for the H5D_LAYOUT property */
typedef enum H5D_layout_t {
    H5D_LAYOUT_ERROR	= -1,

    H5D_COMPACT		= 0,	/*raw data is very small		     */
    H5D_CONTIGUOUS	= 1,	/*the default				     */
    H5D_CHUNKED		= 2,	/*slow and fancy			     */
    H5D_VIRTUAL         = 3,    /*actual data is stored in other datasets     */
    H5D_NLAYOUTS	= 4	/*this one must be last!		     */
} H5D_layout_t;

/* Types of chunk index data structures */
typedef enum H5D_chunk_index_t {
    H5D_CHUNK_IDX_BTREE	= 0,    /* v1 B-tree index (default)                */
    H5D_CHUNK_IDX_SINGLE = 1,   /* Single Chunk index (cur dims[]=max dims[]=chunk dims[]; filtered & non-filtered) */
    H5D_CHUNK_IDX_NONE = 2,     /* Implicit: No Index (H5D_ALLOC_TIME_EARLY, non-filtered, fixed dims) */
    H5D_CHUNK_IDX_FARRAY = 3,   /* Fixed array (for 0 unlimited dims)       */
    H5D_CHUNK_IDX_EARRAY = 4,   /* Extensible array (for 1 unlimited dim)   */
    H5D_CHUNK_IDX_BT2 = 5,      /* v2 B-tree index (for >1 unlimited dims)  */
    H5D_CHUNK_IDX_NTYPES        /* This one must be last!                   */
} H5D_chunk_index_t;

/* Values for the space allocation time property */
typedef enum H5D_alloc_time_t {
    H5D_ALLOC_TIME_ERROR	= -1,
    H5D_ALLOC_TIME_DEFAULT  	= 0,
    H5D_ALLOC_TIME_EARLY	= 1,
    H5D_ALLOC_TIME_LATE		= 2,
    H5D_ALLOC_TIME_INCR		= 3
} H5D_alloc_time_t;

/* Values for the status of space allocation */
typedef enum H5D_space_status_t {
    H5D_SPACE_STATUS_ERROR		= -1,
    H5D_SPACE_STATUS_NOT_ALLOCATED	= 0,
    H5D_SPACE_STATUS_PART_ALLOCATED	= 1,
    H5D_SPACE_STATUS_ALLOCATED		= 2
} H5D_space_status_t;

/* Values for time of writing fill value property */
typedef enum H5D_fill_time_t {
    H5D_FILL_TIME_ERROR	= -1,
    H5D_FILL_TIME_ALLOC = 0,
    H5D_FILL_TIME_NEVER	= 1,
    H5D_FILL_TIME_IFSET	= 2
} H5D_fill_time_t;

/* Values for fill value status */
typedef enum H5D_fill_value_t {
    H5D_FILL_VALUE_ERROR        =-1,
    H5D_FILL_VALUE_UNDEFINED    =0,
    H5D_FILL_VALUE_DEFAULT      =1,
    H5D_FILL_VALUE_USER_DEFINED =2
} H5D_fill_value_t;

/* Values for VDS bounds option */
typedef enum H5D_vds_view_t {
    H5D_VDS_ERROR               = -1,
    H5D_VDS_FIRST_MISSING       = 0,
    H5D_VDS_LAST_AVAILABLE      = 1
} H5D_vds_view_t;

/* Callback for H5Pset_append_flush() in a dataset access property list */
typedef herr_t (*H5D_append_cb_t)(hid_t dataset_id, hsize_t *cur_dims, void *op_data);

/********************/
/* Public Variables */
/********************/

/*********************/
/* Public Prototypes */
/*********************/
#ifdef __cplusplus
extern "C" {
#endif

/* Define the operator function pointer for H5Diterate() */
typedef herr_t (*H5D_operator_t)(void *elem, hid_t type_id, unsigned ndim,
				 const hsize_t *point, void *operator_data);

/* Define the operator function pointer for H5Dscatter() */
typedef herr_t (*H5D_scatter_func_t)(const void **src_buf/*out*/,
                                     size_t *src_buf_bytes_used/*out*/,
                                     void *op_data);

/* Define the operator function pointer for H5Dgather() */
typedef herr_t (*H5D_gather_func_t)(const void *dst_buf,
                                    size_t dst_buf_bytes_used, void *op_data);

H5_DLL hid_t H5Dcreate2(hid_t loc_id, const char *name, hid_t type_id,
    hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id);
H5_DLL hid_t H5Dcreate_anon(hid_t file_id, hid_t type_id, hid_t space_id,
    hid_t plist_id, hid_t dapl_id);
H5_DLL hid_t H5Dopen2(hid_t file_id, const char *name, hid_t dapl_id);
H5_DLL herr_t H5Dclose(hid_t dset_id);
H5_DLL hid_t H5Dget_space(hid_t dset_id);
H5_DLL herr_t H5Dget_space_status(hid_t dset_id, H5D_space_status_t *allocation);
H5_DLL hid_t H5Dget_type(hid_t dset_id);
H5_DLL hid_t H5Dget_create_plist(hid_t dset_id);
H5_DLL hid_t H5Dget_access_plist(hid_t dset_id);
H5_DLL hsize_t H5Dget_storage_size(hid_t dset_id);
H5_DLL herr_t H5Dget_chunk_storage_size(hid_t dset_id, const hsize_t *offset, hsize_t *chunk_bytes);
H5_DLL haddr_t H5Dget_offset(hid_t dset_id);
H5_DLL herr_t H5Dread(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
			hid_t file_space_id, hid_t plist_id, void *buf/*out*/);
H5_DLL herr_t H5Dwrite(hid_t dset_id, hid_t mem_type_id, hid_t mem_space_id,
			 hid_t file_space_id, hid_t plist_id, const void *buf);
H5_DLL herr_t H5Dwrite_chunk(hid_t dset_id, hid_t dxpl_id, uint32_t filters, 
            const hsize_t *offset, size_t data_size, const void *buf);
H5_DLL herr_t H5Dread_chunk(hid_t dset_id, hid_t dxpl_id,
            const hsize_t *offset, uint32_t *filters, void *buf);
H5_DLL herr_t H5Diterate(void *buf, hid_t type_id, hid_t space_id,
            H5D_operator_t op, void *operator_data);
H5_DLL herr_t H5Dvlen_reclaim(hid_t type_id, hid_t space_id, hid_t plist_id, void *buf);
H5_DLL herr_t H5Dvlen_get_buf_size(hid_t dataset_id, hid_t type_id, hid_t space_id, hsize_t *size);
H5_DLL herr_t H5Dfill(const void *fill, hid_t fill_type, void *buf,
        hid_t buf_type, hid_t space);
H5_DLL herr_t H5Dset_extent(hid_t dset_id, const hsize_t size[]);
H5_DLL herr_t H5Dflush(hid_t dset_id);
H5_DLL herr_t H5Drefresh(hid_t dset_id);
H5_DLL herr_t H5Dscatter(H5D_scatter_func_t op, void *op_data, hid_t type_id,
    hid_t dst_space_id, void *dst_buf);
H5_DLL herr_t H5Dgather(hid_t src_space_id, const void *src_buf, hid_t type_id,
    size_t dst_buf_size, void *dst_buf, H5D_gather_func_t op, void *op_data);
H5_DLL herr_t H5Ddebug(hid_t dset_id);

/* Internal API routines */
H5_DLL herr_t H5Dformat_convert(hid_t dset_id);
H5_DLL herr_t H5Dget_chunk_index_type(hid_t did, H5D_chunk_index_t *idx_type);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/* Macros */
#define H5D_CHUNK_BTREE H5D_CHUNK_IDX_BTREE

/* Formerly used to support the H5DOread/write_chunk() API calls.
 * These symbols are no longer used in the library.
 */
/* Property names for H5DOwrite_chunk */
#define H5D_XFER_DIRECT_CHUNK_WRITE_FLAG_NAME       "direct_chunk_flag"
#define H5D_XFER_DIRECT_CHUNK_WRITE_FILTERS_NAME    "direct_chunk_filters"
#define H5D_XFER_DIRECT_CHUNK_WRITE_OFFSET_NAME     "direct_chunk_offset"
#define H5D_XFER_DIRECT_CHUNK_WRITE_DATASIZE_NAME   "direct_chunk_datasize"
/* Property names for H5DOread_chunk */
#define H5D_XFER_DIRECT_CHUNK_READ_FLAG_NAME        "direct_chunk_read_flag"
#define H5D_XFER_DIRECT_CHUNK_READ_OFFSET_NAME      "direct_chunk_read_offset"
#define H5D_XFER_DIRECT_CHUNK_READ_FILTERS_NAME     "direct_chunk_read_filters"
 
/* Typedefs */


/* Function prototypes */
H5_DLL hid_t H5Dcreate1(hid_t file_id, const char *name, hid_t type_id,
    hid_t space_id, hid_t dcpl_id);
H5_DLL hid_t H5Dopen1(hid_t file_id, const char *name);
H5_DLL herr_t H5Dextend(hid_t dset_id, const hsize_t size[]);

#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif
#endif /* _H5Dpublic_H */

