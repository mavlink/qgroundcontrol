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
 * This file contains function prototypes for each exported function in the
 * H5P module.
 */
#ifndef _H5Ppublic_H
#define _H5Ppublic_H

/* System headers needed by this file */

/* Public headers needed by this file */
#include "H5public.h"
#include "H5ACpublic.h"
#include "H5Dpublic.h"
#include "H5Fpublic.h"
#include "H5FDpublic.h"
#include "H5Ipublic.h"
#include "H5Lpublic.h"
#include "H5Opublic.h"
#include "H5MMpublic.h"
#include "H5Tpublic.h"
#include "H5Zpublic.h"


/*****************/
/* Public Macros */
/*****************/

/* When this header is included from a private HDF5 header, don't make calls to H5open() */
#undef H5OPEN
#ifndef _H5private_H
#define H5OPEN        H5open(),
#else   /* _H5private_H */
#define H5OPEN
#endif  /* _H5private_H */

/*
 * The library's property list classes
 */

#define H5P_ROOT                (H5OPEN H5P_CLS_ROOT_ID_g)
#define H5P_OBJECT_CREATE       (H5OPEN H5P_CLS_OBJECT_CREATE_ID_g)
#define H5P_FILE_CREATE         (H5OPEN H5P_CLS_FILE_CREATE_ID_g)
#define H5P_FILE_ACCESS         (H5OPEN H5P_CLS_FILE_ACCESS_ID_g)
#define H5P_DATASET_CREATE      (H5OPEN H5P_CLS_DATASET_CREATE_ID_g)
#define H5P_DATASET_ACCESS      (H5OPEN H5P_CLS_DATASET_ACCESS_ID_g)
#define H5P_DATASET_XFER        (H5OPEN H5P_CLS_DATASET_XFER_ID_g)
#define H5P_FILE_MOUNT          (H5OPEN H5P_CLS_FILE_MOUNT_ID_g)
#define H5P_GROUP_CREATE        (H5OPEN H5P_CLS_GROUP_CREATE_ID_g)
#define H5P_GROUP_ACCESS        (H5OPEN H5P_CLS_GROUP_ACCESS_ID_g)
#define H5P_DATATYPE_CREATE     (H5OPEN H5P_CLS_DATATYPE_CREATE_ID_g)
#define H5P_DATATYPE_ACCESS     (H5OPEN H5P_CLS_DATATYPE_ACCESS_ID_g)
#define H5P_STRING_CREATE       (H5OPEN H5P_CLS_STRING_CREATE_ID_g)
#define H5P_ATTRIBUTE_CREATE    (H5OPEN H5P_CLS_ATTRIBUTE_CREATE_ID_g)
#define H5P_ATTRIBUTE_ACCESS    (H5OPEN H5P_CLS_ATTRIBUTE_ACCESS_ID_g)
#define H5P_OBJECT_COPY         (H5OPEN H5P_CLS_OBJECT_COPY_ID_g)
#define H5P_LINK_CREATE         (H5OPEN H5P_CLS_LINK_CREATE_ID_g)
#define H5P_LINK_ACCESS         (H5OPEN H5P_CLS_LINK_ACCESS_ID_g)

/*
 * The library's default property lists
 */
#define H5P_FILE_CREATE_DEFAULT        (H5OPEN H5P_LST_FILE_CREATE_ID_g)
#define H5P_FILE_ACCESS_DEFAULT        (H5OPEN H5P_LST_FILE_ACCESS_ID_g)
#define H5P_DATASET_CREATE_DEFAULT     (H5OPEN H5P_LST_DATASET_CREATE_ID_g)
#define H5P_DATASET_ACCESS_DEFAULT     (H5OPEN H5P_LST_DATASET_ACCESS_ID_g)
#define H5P_DATASET_XFER_DEFAULT       (H5OPEN H5P_LST_DATASET_XFER_ID_g)
#define H5P_FILE_MOUNT_DEFAULT         (H5OPEN H5P_LST_FILE_MOUNT_ID_g)
#define H5P_GROUP_CREATE_DEFAULT       (H5OPEN H5P_LST_GROUP_CREATE_ID_g)
#define H5P_GROUP_ACCESS_DEFAULT       (H5OPEN H5P_LST_GROUP_ACCESS_ID_g)
#define H5P_DATATYPE_CREATE_DEFAULT    (H5OPEN H5P_LST_DATATYPE_CREATE_ID_g)
#define H5P_DATATYPE_ACCESS_DEFAULT    (H5OPEN H5P_LST_DATATYPE_ACCESS_ID_g)
#define H5P_ATTRIBUTE_CREATE_DEFAULT   (H5OPEN H5P_LST_ATTRIBUTE_CREATE_ID_g)
#define H5P_ATTRIBUTE_ACCESS_DEFAULT   (H5OPEN H5P_LST_ATTRIBUTE_ACCESS_ID_g)
#define H5P_OBJECT_COPY_DEFAULT        (H5OPEN H5P_LST_OBJECT_COPY_ID_g)
#define H5P_LINK_CREATE_DEFAULT        (H5OPEN H5P_LST_LINK_CREATE_ID_g)
#define H5P_LINK_ACCESS_DEFAULT        (H5OPEN H5P_LST_LINK_ACCESS_ID_g)

/* Common creation order flags (for links in groups and attributes on objects) */
#define H5P_CRT_ORDER_TRACKED           0x0001
#define H5P_CRT_ORDER_INDEXED           0x0002

/* Default value for all property list classes */
#define H5P_DEFAULT     (hid_t)0

#ifdef __cplusplus
extern "C" {
#endif

/*******************/
/* Public Typedefs */
/*******************/


/* Define property list class callback function pointer types */
typedef herr_t (*H5P_cls_create_func_t)(hid_t prop_id, void *create_data);
typedef herr_t (*H5P_cls_copy_func_t)(hid_t new_prop_id, hid_t old_prop_id,
                                      void *copy_data);
typedef herr_t (*H5P_cls_close_func_t)(hid_t prop_id, void *close_data);

/* Define property list callback function pointer types */
typedef herr_t (*H5P_prp_cb1_t)(const char *name, size_t size, void *value);
typedef herr_t (*H5P_prp_cb2_t)(hid_t prop_id, const char *name, size_t size, void *value);
typedef H5P_prp_cb1_t H5P_prp_create_func_t;
typedef H5P_prp_cb2_t H5P_prp_set_func_t;
typedef H5P_prp_cb2_t H5P_prp_get_func_t;
typedef H5P_prp_cb2_t H5P_prp_delete_func_t;
typedef H5P_prp_cb1_t H5P_prp_copy_func_t;
typedef int (*H5P_prp_compare_func_t)(const void *value1, const void *value2, size_t size);
typedef H5P_prp_cb1_t H5P_prp_close_func_t;

/* Define property list iteration function type */
typedef herr_t (*H5P_iterate_t)(hid_t id, const char *name, void *iter_data);

/* Actual IO mode property */
typedef enum H5D_mpio_actual_chunk_opt_mode_t {
    /* The default value, H5D_MPIO_NO_CHUNK_OPTIMIZATION, is used for all I/O
     * operations that do not use chunk optimizations, including non-collective
     * I/O and contiguous collective I/O.
     */
    H5D_MPIO_NO_CHUNK_OPTIMIZATION = 0,
    H5D_MPIO_LINK_CHUNK,
    H5D_MPIO_MULTI_CHUNK
}  H5D_mpio_actual_chunk_opt_mode_t;

typedef enum H5D_mpio_actual_io_mode_t {
    /* The following four values are conveniently defined as a bit field so that
     * we can switch from the default to independent or collective and then to
     * mixed without having to check the original value.
     *
     * NO_COLLECTIVE means that either collective I/O wasn't requested or that
     * no I/O took place.
     *
     * CHUNK_INDEPENDENT means that collective I/O was requested, but the
     * chunk optimization scheme chose independent I/O for each chunk.
     */
    H5D_MPIO_NO_COLLECTIVE = 0x0,
    H5D_MPIO_CHUNK_INDEPENDENT = 0x1,
    H5D_MPIO_CHUNK_COLLECTIVE = 0x2,
    H5D_MPIO_CHUNK_MIXED = 0x1 | 0x2,

    /* The contiguous case is separate from the bit field. */
    H5D_MPIO_CONTIGUOUS_COLLECTIVE = 0x4
} H5D_mpio_actual_io_mode_t;

/* Broken collective IO property */
typedef enum H5D_mpio_no_collective_cause_t {
    H5D_MPIO_COLLECTIVE = 0x00,
    H5D_MPIO_SET_INDEPENDENT = 0x01,
    H5D_MPIO_DATATYPE_CONVERSION = 0x02,
    H5D_MPIO_DATA_TRANSFORMS = 0x04,
    H5D_MPIO_MPI_OPT_TYPES_ENV_VAR_DISABLED = 0x08,
    H5D_MPIO_NOT_SIMPLE_OR_SCALAR_DATASPACES = 0x10,
    H5D_MPIO_NOT_CONTIGUOUS_OR_CHUNKED_DATASET = 0x20,
    H5D_MPIO_PARALLEL_FILTERED_WRITES_DISABLED = 0x40,
    H5D_MPIO_NO_COLLECTIVE_MAX_CAUSE = 0x80
} H5D_mpio_no_collective_cause_t;

/********************/
/* Public Variables */
/********************/

/* Property list class IDs */
/* (Internal to library, do not use!  Use macros above) */
H5_DLLVAR hid_t H5P_CLS_ROOT_ID_g;
H5_DLLVAR hid_t H5P_CLS_OBJECT_CREATE_ID_g;
H5_DLLVAR hid_t H5P_CLS_FILE_CREATE_ID_g;
H5_DLLVAR hid_t H5P_CLS_FILE_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_CLS_DATASET_CREATE_ID_g;
H5_DLLVAR hid_t H5P_CLS_DATASET_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_CLS_DATASET_XFER_ID_g;
H5_DLLVAR hid_t H5P_CLS_FILE_MOUNT_ID_g;
H5_DLLVAR hid_t H5P_CLS_GROUP_CREATE_ID_g;
H5_DLLVAR hid_t H5P_CLS_GROUP_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_CLS_DATATYPE_CREATE_ID_g;
H5_DLLVAR hid_t H5P_CLS_DATATYPE_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_CLS_STRING_CREATE_ID_g;
H5_DLLVAR hid_t H5P_CLS_ATTRIBUTE_CREATE_ID_g;
H5_DLLVAR hid_t H5P_CLS_ATTRIBUTE_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_CLS_OBJECT_COPY_ID_g;
H5_DLLVAR hid_t H5P_CLS_LINK_CREATE_ID_g;
H5_DLLVAR hid_t H5P_CLS_LINK_ACCESS_ID_g;

/* Default roperty list IDs */
/* (Internal to library, do not use!  Use macros above) */
H5_DLLVAR hid_t H5P_LST_FILE_CREATE_ID_g;
H5_DLLVAR hid_t H5P_LST_FILE_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_LST_DATASET_CREATE_ID_g;
H5_DLLVAR hid_t H5P_LST_DATASET_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_LST_DATASET_XFER_ID_g;
H5_DLLVAR hid_t H5P_LST_FILE_MOUNT_ID_g;
H5_DLLVAR hid_t H5P_LST_GROUP_CREATE_ID_g;
H5_DLLVAR hid_t H5P_LST_GROUP_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_LST_DATATYPE_CREATE_ID_g;
H5_DLLVAR hid_t H5P_LST_DATATYPE_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_LST_ATTRIBUTE_CREATE_ID_g;
H5_DLLVAR hid_t H5P_LST_ATTRIBUTE_ACCESS_ID_g;
H5_DLLVAR hid_t H5P_LST_OBJECT_COPY_ID_g;
H5_DLLVAR hid_t H5P_LST_LINK_CREATE_ID_g;
H5_DLLVAR hid_t H5P_LST_LINK_ACCESS_ID_g;

/*********************/
/* Public Prototypes */
/*********************/

/* Generic property list routines */
H5_DLL hid_t H5Pcreate_class(hid_t parent, const char *name,
    H5P_cls_create_func_t cls_create, void *create_data,
    H5P_cls_copy_func_t cls_copy, void *copy_data,
    H5P_cls_close_func_t cls_close, void *close_data);
H5_DLL char *H5Pget_class_name(hid_t pclass_id);
H5_DLL hid_t H5Pcreate(hid_t cls_id);
H5_DLL herr_t H5Pregister2(hid_t cls_id, const char *name, size_t size,
    void *def_value, H5P_prp_create_func_t prp_create,
    H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get,
    H5P_prp_delete_func_t prp_del, H5P_prp_copy_func_t prp_copy,
    H5P_prp_compare_func_t prp_cmp, H5P_prp_close_func_t prp_close);
H5_DLL herr_t H5Pinsert2(hid_t plist_id, const char *name, size_t size,
    void *value, H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get,
    H5P_prp_delete_func_t prp_delete, H5P_prp_copy_func_t prp_copy,
    H5P_prp_compare_func_t prp_cmp, H5P_prp_close_func_t prp_close);
H5_DLL herr_t H5Pset(hid_t plist_id, const char *name, const void *value);
H5_DLL htri_t H5Pexist(hid_t plist_id, const char *name);
H5_DLL herr_t H5Pencode(hid_t plist_id, void *buf, size_t *nalloc);
H5_DLL hid_t  H5Pdecode(const void *buf);
H5_DLL herr_t H5Pget_size(hid_t id, const char *name, size_t *size);
H5_DLL herr_t H5Pget_nprops(hid_t id, size_t *nprops);
H5_DLL hid_t H5Pget_class(hid_t plist_id);
H5_DLL hid_t H5Pget_class_parent(hid_t pclass_id);
H5_DLL herr_t H5Pget(hid_t plist_id, const char *name, void * value);
H5_DLL htri_t H5Pequal(hid_t id1, hid_t id2);
H5_DLL htri_t H5Pisa_class(hid_t plist_id, hid_t pclass_id);
H5_DLL int H5Piterate(hid_t id, int *idx, H5P_iterate_t iter_func,
            void *iter_data);
H5_DLL herr_t H5Pcopy_prop(hid_t dst_id, hid_t src_id, const char *name);
H5_DLL herr_t H5Premove(hid_t plist_id, const char *name);
H5_DLL herr_t H5Punregister(hid_t pclass_id, const char *name);
H5_DLL herr_t H5Pclose_class(hid_t plist_id);
H5_DLL herr_t H5Pclose(hid_t plist_id);
H5_DLL hid_t H5Pcopy(hid_t plist_id);

/* Object creation property list (OCPL) routines */
H5_DLL herr_t H5Pset_attr_phase_change(hid_t plist_id, unsigned max_compact, unsigned min_dense);
H5_DLL herr_t H5Pget_attr_phase_change(hid_t plist_id, unsigned *max_compact, unsigned *min_dense);
H5_DLL herr_t H5Pset_attr_creation_order(hid_t plist_id, unsigned crt_order_flags);
H5_DLL herr_t H5Pget_attr_creation_order(hid_t plist_id, unsigned *crt_order_flags);
H5_DLL herr_t H5Pset_obj_track_times(hid_t plist_id, hbool_t track_times);
H5_DLL herr_t H5Pget_obj_track_times(hid_t plist_id, hbool_t *track_times);
H5_DLL herr_t H5Pmodify_filter(hid_t plist_id, H5Z_filter_t filter,
        unsigned int flags, size_t cd_nelmts,
        const unsigned int cd_values[/*cd_nelmts*/]);
H5_DLL herr_t H5Pset_filter(hid_t plist_id, H5Z_filter_t filter,
        unsigned int flags, size_t cd_nelmts,
        const unsigned int c_values[]);
H5_DLL int H5Pget_nfilters(hid_t plist_id);
H5_DLL H5Z_filter_t H5Pget_filter2(hid_t plist_id, unsigned filter,
       unsigned int *flags/*out*/,
       size_t *cd_nelmts/*out*/,
       unsigned cd_values[]/*out*/,
       size_t namelen, char name[],
       unsigned *filter_config /*out*/);
H5_DLL herr_t H5Pget_filter_by_id2(hid_t plist_id, H5Z_filter_t id,
       unsigned int *flags/*out*/, size_t *cd_nelmts/*out*/,
       unsigned cd_values[]/*out*/, size_t namelen, char name[]/*out*/,
       unsigned *filter_config/*out*/);
H5_DLL htri_t H5Pall_filters_avail(hid_t plist_id);
H5_DLL herr_t H5Premove_filter(hid_t plist_id, H5Z_filter_t filter);
H5_DLL herr_t H5Pset_deflate(hid_t plist_id, unsigned aggression);
H5_DLL herr_t H5Pset_fletcher32(hid_t plist_id);

/* File creation property list (FCPL) routines */
H5_DLL herr_t H5Pset_userblock(hid_t plist_id, hsize_t size);
H5_DLL herr_t H5Pget_userblock(hid_t plist_id, hsize_t *size);
H5_DLL herr_t H5Pset_sizes(hid_t plist_id, size_t sizeof_addr,
       size_t sizeof_size);
H5_DLL herr_t H5Pget_sizes(hid_t plist_id, size_t *sizeof_addr/*out*/,
       size_t *sizeof_size/*out*/);
H5_DLL herr_t H5Pset_sym_k(hid_t plist_id, unsigned ik, unsigned lk);
H5_DLL herr_t H5Pget_sym_k(hid_t plist_id, unsigned *ik/*out*/, unsigned *lk/*out*/);
H5_DLL herr_t H5Pset_istore_k(hid_t plist_id, unsigned ik);
H5_DLL herr_t H5Pget_istore_k(hid_t plist_id, unsigned *ik/*out*/);
H5_DLL herr_t H5Pset_shared_mesg_nindexes(hid_t plist_id, unsigned nindexes);
H5_DLL herr_t H5Pget_shared_mesg_nindexes(hid_t plist_id, unsigned *nindexes);
H5_DLL herr_t H5Pset_shared_mesg_index(hid_t plist_id, unsigned index_num, unsigned mesg_type_flags, unsigned min_mesg_size);
H5_DLL herr_t H5Pget_shared_mesg_index(hid_t plist_id, unsigned index_num, unsigned *mesg_type_flags, unsigned *min_mesg_size);
H5_DLL herr_t H5Pset_shared_mesg_phase_change(hid_t plist_id, unsigned max_list, unsigned min_btree);
H5_DLL herr_t H5Pget_shared_mesg_phase_change(hid_t plist_id, unsigned *max_list, unsigned *min_btree);
H5_DLL herr_t H5Pset_file_space_strategy(hid_t plist_id, H5F_fspace_strategy_t strategy, hbool_t persist, hsize_t threshold);
H5_DLL herr_t H5Pget_file_space_strategy(hid_t plist_id, H5F_fspace_strategy_t *strategy, hbool_t *persist, hsize_t *threshold);
H5_DLL herr_t H5Pset_file_space_page_size(hid_t plist_id, hsize_t fsp_size);
H5_DLL herr_t H5Pget_file_space_page_size(hid_t plist_id, hsize_t *fsp_size);

/* File access property list (FAPL) routines */
H5_DLL herr_t H5Pset_alignment(hid_t fapl_id, hsize_t threshold,
    hsize_t alignment);
H5_DLL herr_t H5Pget_alignment(hid_t fapl_id, hsize_t *threshold/*out*/,
    hsize_t *alignment/*out*/);
H5_DLL herr_t H5Pset_driver(hid_t plist_id, hid_t driver_id,
        const void *driver_info);
H5_DLL hid_t H5Pget_driver(hid_t plist_id);
H5_DLL const void *H5Pget_driver_info(hid_t plist_id);
H5_DLL herr_t H5Pset_family_offset(hid_t fapl_id, hsize_t offset);
H5_DLL herr_t H5Pget_family_offset(hid_t fapl_id, hsize_t *offset);
H5_DLL herr_t H5Pset_multi_type(hid_t fapl_id, H5FD_mem_t type);
H5_DLL herr_t H5Pget_multi_type(hid_t fapl_id, H5FD_mem_t *type);
H5_DLL herr_t H5Pset_cache(hid_t plist_id, int mdc_nelmts,
       size_t rdcc_nslots, size_t rdcc_nbytes,
       double rdcc_w0);
H5_DLL herr_t H5Pget_cache(hid_t plist_id,
       int *mdc_nelmts, /* out */
       size_t *rdcc_nslots/*out*/,
       size_t *rdcc_nbytes/*out*/, double *rdcc_w0);
H5_DLL herr_t H5Pset_mdc_config(hid_t    plist_id,
       H5AC_cache_config_t * config_ptr);
H5_DLL herr_t H5Pget_mdc_config(hid_t     plist_id,
       H5AC_cache_config_t * config_ptr);    /* out */
H5_DLL herr_t H5Pset_gc_references(hid_t fapl_id, unsigned gc_ref);
H5_DLL herr_t H5Pget_gc_references(hid_t fapl_id, unsigned *gc_ref/*out*/);
H5_DLL herr_t H5Pset_fclose_degree(hid_t fapl_id, H5F_close_degree_t degree);
H5_DLL herr_t H5Pget_fclose_degree(hid_t fapl_id, H5F_close_degree_t *degree);
H5_DLL herr_t H5Pset_meta_block_size(hid_t fapl_id, hsize_t size);
H5_DLL herr_t H5Pget_meta_block_size(hid_t fapl_id, hsize_t *size/*out*/);
H5_DLL herr_t H5Pset_sieve_buf_size(hid_t fapl_id, size_t size);
H5_DLL herr_t H5Pget_sieve_buf_size(hid_t fapl_id, size_t *size/*out*/);
H5_DLL herr_t H5Pset_small_data_block_size(hid_t fapl_id, hsize_t size);
H5_DLL herr_t H5Pget_small_data_block_size(hid_t fapl_id, hsize_t *size/*out*/);
H5_DLL herr_t H5Pset_libver_bounds(hid_t plist_id, H5F_libver_t low,
    H5F_libver_t high);
H5_DLL herr_t H5Pget_libver_bounds(hid_t plist_id, H5F_libver_t *low,
    H5F_libver_t *high);
H5_DLL herr_t H5Pset_elink_file_cache_size(hid_t plist_id, unsigned efc_size);
H5_DLL herr_t H5Pget_elink_file_cache_size(hid_t plist_id, unsigned *efc_size);
H5_DLL herr_t H5Pset_file_image(hid_t fapl_id, void *buf_ptr, size_t buf_len);
H5_DLL herr_t H5Pget_file_image(hid_t fapl_id, void **buf_ptr_ptr, size_t *buf_len_ptr);
H5_DLL herr_t H5Pset_file_image_callbacks(hid_t fapl_id,
       H5FD_file_image_callbacks_t *callbacks_ptr);
H5_DLL herr_t H5Pget_file_image_callbacks(hid_t fapl_id,
       H5FD_file_image_callbacks_t *callbacks_ptr);
H5_DLL herr_t H5Pset_core_write_tracking(hid_t fapl_id, hbool_t is_enabled, size_t page_size);
H5_DLL herr_t H5Pget_core_write_tracking(hid_t fapl_id, hbool_t *is_enabled, size_t *page_size);
H5_DLL herr_t H5Pset_metadata_read_attempts(hid_t plist_id, unsigned attempts);
H5_DLL herr_t H5Pget_metadata_read_attempts(hid_t plist_id, unsigned *attempts);
H5_DLL herr_t H5Pset_object_flush_cb(hid_t plist_id, H5F_flush_cb_t func, void *udata);
H5_DLL herr_t H5Pget_object_flush_cb(hid_t plist_id, H5F_flush_cb_t *func, void **udata);
H5_DLL herr_t H5Pset_mdc_log_options(hid_t plist_id, hbool_t is_enabled, const char *location, hbool_t start_on_access);
H5_DLL herr_t H5Pget_mdc_log_options(hid_t plist_id, hbool_t *is_enabled, char *location, size_t *location_size, hbool_t *start_on_access);
H5_DLL herr_t H5Pset_evict_on_close(hid_t fapl_id, hbool_t evict_on_close);
H5_DLL herr_t H5Pget_evict_on_close(hid_t fapl_id, hbool_t *evict_on_close);
#ifdef H5_HAVE_PARALLEL
H5_DLL herr_t H5Pset_all_coll_metadata_ops(hid_t plist_id, hbool_t is_collective);
H5_DLL herr_t H5Pget_all_coll_metadata_ops(hid_t plist_id, hbool_t *is_collective);
H5_DLL herr_t H5Pset_coll_metadata_write(hid_t plist_id, hbool_t is_collective);
H5_DLL herr_t H5Pget_coll_metadata_write(hid_t plist_id, hbool_t *is_collective);
#endif /* H5_HAVE_PARALLEL */
H5_DLL herr_t H5Pset_mdc_image_config(hid_t plist_id, H5AC_cache_image_config_t *config_ptr);
H5_DLL herr_t H5Pget_mdc_image_config(hid_t plist_id, H5AC_cache_image_config_t *config_ptr /*out*/);
H5_DLL herr_t H5Pset_page_buffer_size(hid_t plist_id, size_t buf_size, unsigned min_meta_per, unsigned min_raw_per);
H5_DLL herr_t H5Pget_page_buffer_size(hid_t plist_id, size_t *buf_size, unsigned *min_meta_per, unsigned *min_raw_per);

/* Dataset creation property list (DCPL) routines */
H5_DLL herr_t H5Pset_layout(hid_t plist_id, H5D_layout_t layout);
H5_DLL H5D_layout_t H5Pget_layout(hid_t plist_id);
H5_DLL herr_t H5Pset_chunk(hid_t plist_id, int ndims, const hsize_t dim[/*ndims*/]);
H5_DLL int H5Pget_chunk(hid_t plist_id, int max_ndims, hsize_t dim[]/*out*/);
H5_DLL herr_t H5Pset_virtual(hid_t dcpl_id, hid_t vspace_id,
    const char *src_file_name, const char *src_dset_name, hid_t src_space_id);
H5_DLL herr_t H5Pget_virtual_count(hid_t dcpl_id, size_t *count/*out*/);
H5_DLL hid_t H5Pget_virtual_vspace(hid_t dcpl_id, size_t index);
H5_DLL hid_t H5Pget_virtual_srcspace(hid_t dcpl_id, size_t index);
H5_DLL ssize_t H5Pget_virtual_filename(hid_t dcpl_id, size_t index,
    char *name/*out*/, size_t size);
H5_DLL ssize_t H5Pget_virtual_dsetname(hid_t dcpl_id, size_t index,
    char *name/*out*/, size_t size);
H5_DLL herr_t H5Pset_external(hid_t plist_id, const char *name, off_t offset,
          hsize_t size);
H5_DLL herr_t H5Pset_chunk_opts(hid_t plist_id, unsigned opts);
H5_DLL herr_t H5Pget_chunk_opts(hid_t plist_id, unsigned *opts);
H5_DLL int H5Pget_external_count(hid_t plist_id);
H5_DLL herr_t H5Pget_external(hid_t plist_id, unsigned idx, size_t name_size,
          char *name/*out*/, off_t *offset/*out*/,
          hsize_t *size/*out*/);
H5_DLL herr_t H5Pset_szip(hid_t plist_id, unsigned options_mask, unsigned pixels_per_block);
H5_DLL herr_t H5Pset_shuffle(hid_t plist_id);
H5_DLL herr_t H5Pset_nbit(hid_t plist_id);
H5_DLL herr_t H5Pset_scaleoffset(hid_t plist_id, H5Z_SO_scale_type_t scale_type, int scale_factor);
H5_DLL herr_t H5Pset_fill_value(hid_t plist_id, hid_t type_id,
     const void *value);
H5_DLL herr_t H5Pget_fill_value(hid_t plist_id, hid_t type_id,
     void *value/*out*/);
H5_DLL herr_t H5Pfill_value_defined(hid_t plist, H5D_fill_value_t *status);
H5_DLL herr_t H5Pset_alloc_time(hid_t plist_id, H5D_alloc_time_t
    alloc_time);
H5_DLL herr_t H5Pget_alloc_time(hid_t plist_id, H5D_alloc_time_t
    *alloc_time/*out*/);
H5_DLL herr_t H5Pset_fill_time(hid_t plist_id, H5D_fill_time_t fill_time);
H5_DLL herr_t H5Pget_fill_time(hid_t plist_id, H5D_fill_time_t
    *fill_time/*out*/);

/* Dataset access property list (DAPL) routines */
H5_DLL herr_t H5Pset_chunk_cache(hid_t dapl_id, size_t rdcc_nslots,
       size_t rdcc_nbytes, double rdcc_w0);
H5_DLL herr_t H5Pget_chunk_cache(hid_t dapl_id,
       size_t *rdcc_nslots/*out*/,
       size_t *rdcc_nbytes/*out*/,
       double *rdcc_w0/*out*/);
H5_DLL herr_t H5Pset_virtual_view(hid_t plist_id, H5D_vds_view_t view);
H5_DLL herr_t H5Pget_virtual_view(hid_t plist_id, H5D_vds_view_t *view);
H5_DLL herr_t H5Pset_virtual_printf_gap(hid_t plist_id, hsize_t gap_size);
H5_DLL herr_t H5Pget_virtual_printf_gap(hid_t plist_id, hsize_t *gap_size);
H5_DLL herr_t H5Pset_virtual_prefix(hid_t dapl_id, const char* prefix);
H5_DLL ssize_t H5Pget_virtual_prefix(hid_t dapl_id, char* prefix /*out*/, size_t size);
H5_DLL herr_t H5Pset_append_flush(hid_t plist_id, unsigned ndims,
    const hsize_t boundary[], H5D_append_cb_t func, void *udata);
H5_DLL herr_t H5Pget_append_flush(hid_t plist_id, unsigned dims,
    hsize_t boundary[], H5D_append_cb_t *func, void **udata);
H5_DLL herr_t H5Pset_efile_prefix(hid_t dapl_id, const char* prefix);
H5_DLL ssize_t H5Pget_efile_prefix(hid_t dapl_id, char* prefix /*out*/, size_t size);

/* Dataset xfer property list (DXPL) routines */
H5_DLL herr_t H5Pset_data_transform(hid_t plist_id, const char* expression);
H5_DLL ssize_t H5Pget_data_transform(hid_t plist_id, char* expression /*out*/, size_t size);
H5_DLL herr_t H5Pset_buffer(hid_t plist_id, size_t size, void *tconv,
        void *bkg);
H5_DLL size_t H5Pget_buffer(hid_t plist_id, void **tconv/*out*/,
        void **bkg/*out*/);
H5_DLL herr_t H5Pset_preserve(hid_t plist_id, hbool_t status);
H5_DLL int H5Pget_preserve(hid_t plist_id);
H5_DLL herr_t H5Pset_edc_check(hid_t plist_id, H5Z_EDC_t check);
H5_DLL H5Z_EDC_t H5Pget_edc_check(hid_t plist_id);
H5_DLL herr_t H5Pset_filter_callback(hid_t plist_id, H5Z_filter_func_t func,
                                     void* op_data);
H5_DLL herr_t H5Pset_btree_ratios(hid_t plist_id, double left, double middle,
       double right);
H5_DLL herr_t H5Pget_btree_ratios(hid_t plist_id, double *left/*out*/,
       double *middle/*out*/,
       double *right/*out*/);
H5_DLL herr_t H5Pset_vlen_mem_manager(hid_t plist_id,
                                       H5MM_allocate_t alloc_func,
                                       void *alloc_info, H5MM_free_t free_func,
                                       void *free_info);
H5_DLL herr_t H5Pget_vlen_mem_manager(hid_t plist_id,
                                       H5MM_allocate_t *alloc_func,
                                       void **alloc_info,
                                       H5MM_free_t *free_func,
                                       void **free_info);
H5_DLL herr_t H5Pset_hyper_vector_size(hid_t fapl_id, size_t size);
H5_DLL herr_t H5Pget_hyper_vector_size(hid_t fapl_id, size_t *size/*out*/);
H5_DLL herr_t H5Pset_type_conv_cb(hid_t dxpl_id, H5T_conv_except_func_t op, void* operate_data);
H5_DLL herr_t H5Pget_type_conv_cb(hid_t dxpl_id, H5T_conv_except_func_t *op, void** operate_data);
#ifdef H5_HAVE_PARALLEL
H5_DLL herr_t H5Pget_mpio_actual_chunk_opt_mode(hid_t plist_id, H5D_mpio_actual_chunk_opt_mode_t *actual_chunk_opt_mode);
H5_DLL herr_t H5Pget_mpio_actual_io_mode(hid_t plist_id, H5D_mpio_actual_io_mode_t *actual_io_mode);
H5_DLL herr_t H5Pget_mpio_no_collective_cause(hid_t plist_id, uint32_t *local_no_collective_cause, uint32_t *global_no_collective_cause);
#endif /* H5_HAVE_PARALLEL */

/* Link creation property list (LCPL) routines */
H5_DLL herr_t H5Pset_create_intermediate_group(hid_t plist_id, unsigned crt_intmd);
H5_DLL herr_t H5Pget_create_intermediate_group(hid_t plist_id, unsigned *crt_intmd /*out*/);

/* Group creation property list (GCPL) routines */
H5_DLL herr_t H5Pset_local_heap_size_hint(hid_t plist_id, size_t size_hint);
H5_DLL herr_t H5Pget_local_heap_size_hint(hid_t plist_id, size_t *size_hint /*out*/);
H5_DLL herr_t H5Pset_link_phase_change(hid_t plist_id, unsigned max_compact, unsigned min_dense);
H5_DLL herr_t H5Pget_link_phase_change(hid_t plist_id, unsigned *max_compact /*out*/, unsigned *min_dense /*out*/);
H5_DLL herr_t H5Pset_est_link_info(hid_t plist_id, unsigned est_num_entries, unsigned est_name_len);
H5_DLL herr_t H5Pget_est_link_info(hid_t plist_id, unsigned *est_num_entries /* out */, unsigned *est_name_len /* out */);
H5_DLL herr_t H5Pset_link_creation_order(hid_t plist_id, unsigned crt_order_flags);
H5_DLL herr_t H5Pget_link_creation_order(hid_t plist_id, unsigned *crt_order_flags /* out */);

/* String creation property list (STRCPL) routines */
H5_DLL herr_t H5Pset_char_encoding(hid_t plist_id, H5T_cset_t encoding);
H5_DLL herr_t H5Pget_char_encoding(hid_t plist_id, H5T_cset_t *encoding /*out*/);

/* Link access property list (LAPL) routines */
H5_DLL herr_t H5Pset_nlinks(hid_t plist_id, size_t nlinks);
H5_DLL herr_t H5Pget_nlinks(hid_t plist_id, size_t *nlinks);
H5_DLL herr_t H5Pset_elink_prefix(hid_t plist_id, const char *prefix);
H5_DLL ssize_t H5Pget_elink_prefix(hid_t plist_id, char *prefix, size_t size);
H5_DLL hid_t H5Pget_elink_fapl(hid_t lapl_id);
H5_DLL herr_t H5Pset_elink_fapl(hid_t lapl_id, hid_t fapl_id);
H5_DLL herr_t H5Pset_elink_acc_flags(hid_t lapl_id, unsigned flags);
H5_DLL herr_t H5Pget_elink_acc_flags(hid_t lapl_id, unsigned *flags);
H5_DLL herr_t H5Pset_elink_cb(hid_t lapl_id, H5L_elink_traverse_t func, void *op_data);
H5_DLL herr_t H5Pget_elink_cb(hid_t lapl_id, H5L_elink_traverse_t *func, void **op_data);

/* Object copy property list (OCPYPL) routines */
H5_DLL herr_t H5Pset_copy_object(hid_t plist_id, unsigned crt_intmd);
H5_DLL herr_t H5Pget_copy_object(hid_t plist_id, unsigned *crt_intmd /*out*/);
H5_DLL herr_t H5Padd_merge_committed_dtype_path(hid_t plist_id, const char *path);
H5_DLL herr_t H5Pfree_merge_committed_dtype_paths(hid_t plist_id);
H5_DLL herr_t H5Pset_mcdt_search_cb(hid_t plist_id, H5O_mcdt_search_cb_t func, void *op_data);
H5_DLL herr_t H5Pget_mcdt_search_cb(hid_t plist_id, H5O_mcdt_search_cb_t *func, void **op_data);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/* Macros */

/* We renamed the "root" of the property list class hierarchy */
#define H5P_NO_CLASS            H5P_ROOT


/* Typedefs */


/* Function prototypes */
H5_DLL herr_t H5Pregister1(hid_t cls_id, const char *name, size_t size,
    void *def_value, H5P_prp_create_func_t prp_create,
    H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get,
    H5P_prp_delete_func_t prp_del, H5P_prp_copy_func_t prp_copy,
    H5P_prp_close_func_t prp_close);
H5_DLL herr_t H5Pinsert1(hid_t plist_id, const char *name, size_t size,
    void *value, H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get,
    H5P_prp_delete_func_t prp_delete, H5P_prp_copy_func_t prp_copy,
    H5P_prp_close_func_t prp_close);
H5_DLL H5Z_filter_t H5Pget_filter1(hid_t plist_id, unsigned filter,
    unsigned int *flags/*out*/, size_t *cd_nelmts/*out*/,
    unsigned cd_values[]/*out*/, size_t namelen, char name[]);
H5_DLL herr_t H5Pget_filter_by_id1(hid_t plist_id, H5Z_filter_t id,
    unsigned int *flags/*out*/, size_t *cd_nelmts/*out*/,
    unsigned cd_values[]/*out*/, size_t namelen, char name[]/*out*/);
H5_DLL herr_t H5Pget_version(hid_t plist_id, unsigned *boot/*out*/,
         unsigned *freelist/*out*/, unsigned *stab/*out*/,
         unsigned *shhdr/*out*/);
H5_DLL herr_t H5Pset_file_space(hid_t plist_id, H5F_file_space_type_t strategy, hsize_t threshold);
H5_DLL herr_t H5Pget_file_space(hid_t plist_id, H5F_file_space_type_t *strategy, hsize_t *threshold);
#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif
#endif /* _H5Ppublic_H */

