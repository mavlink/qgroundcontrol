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
 * Created:             H5Opublic.h
 *                      Aug  5 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Public declarations for the H5O (object header)
 *                      package.
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5Opublic_H
#define _H5Opublic_H

/* Public headers needed by this file */
#include "H5public.h"        /* Generic Functions            */
#include "H5Ipublic.h"        /* IDs                      */
#include "H5Lpublic.h"        /* Links                  */

/*****************/
/* Public Macros */
/*****************/

/* Flags for object copy (H5Ocopy) */
#define H5O_COPY_SHALLOW_HIERARCHY_FLAG (0x0001u)   /* Copy only immediate members */
#define H5O_COPY_EXPAND_SOFT_LINK_FLAG  (0x0002u)   /* Expand soft links into new objects */
#define H5O_COPY_EXPAND_EXT_LINK_FLAG   (0x0004u)   /* Expand external links into new objects */
#define H5O_COPY_EXPAND_REFERENCE_FLAG    (0x0008u)   /* Copy objects that are pointed by references */
#define H5O_COPY_WITHOUT_ATTR_FLAG      (0x0010u)   /* Copy object without copying attributes */
#define H5O_COPY_PRESERVE_NULL_FLAG     (0x0020u)   /* Copy NULL messages (empty space) */
#define H5O_COPY_MERGE_COMMITTED_DTYPE_FLAG (0x0040u)   /* Merge committed datatypes in dest file */
#define H5O_COPY_ALL                    (0x007Fu)   /* All object copying flags (for internal checking) */

/* Flags for shared message indexes.
 * Pass these flags in using the mesg_type_flags parameter in
 * H5P_set_shared_mesg_index.
 * (Developers: These flags correspond to object header message type IDs,
 * but we need to assign each kind of message to a different bit so that
 * one index can hold multiple types.)
 */
#define H5O_SHMESG_NONE_FLAG    0x0000          /* No shared messages */
#define H5O_SHMESG_SDSPACE_FLAG ((unsigned)1 << 0x0001) /* Simple Dataspace Message.  */
#define H5O_SHMESG_DTYPE_FLAG   ((unsigned)1 << 0x0003) /* Datatype Message.  */
#define H5O_SHMESG_FILL_FLAG    ((unsigned)1 << 0x0005) /* Fill Value Message. */
#define H5O_SHMESG_PLINE_FLAG   ((unsigned)1 << 0x000b) /* Filter pipeline message.  */
#define H5O_SHMESG_ATTR_FLAG    ((unsigned)1 << 0x000c) /* Attribute Message.  */
#define H5O_SHMESG_ALL_FLAG     (H5O_SHMESG_SDSPACE_FLAG | H5O_SHMESG_DTYPE_FLAG | H5O_SHMESG_FILL_FLAG | H5O_SHMESG_PLINE_FLAG | H5O_SHMESG_ATTR_FLAG)

/* Object header status flag definitions */
#define H5O_HDR_CHUNK0_SIZE             0x03    /* 2-bit field indicating # of bytes to store the size of chunk 0's data */
#define H5O_HDR_ATTR_CRT_ORDER_TRACKED  0x04    /* Attribute creation order is tracked */
#define H5O_HDR_ATTR_CRT_ORDER_INDEXED  0x08    /* Attribute creation order has index */
#define H5O_HDR_ATTR_STORE_PHASE_CHANGE 0x10    /* Non-default attribute storage phase change values stored */
#define H5O_HDR_STORE_TIMES             0x20    /* Store access, modification, change & birth times for object */
#define H5O_HDR_ALL_FLAGS       (H5O_HDR_CHUNK0_SIZE | H5O_HDR_ATTR_CRT_ORDER_TRACKED | H5O_HDR_ATTR_CRT_ORDER_INDEXED | H5O_HDR_ATTR_STORE_PHASE_CHANGE | H5O_HDR_STORE_TIMES)

/* Maximum shared message values.  Number of indexes is 8 to allow room to add
 * new types of messages.
 */
#define H5O_SHMESG_MAX_NINDEXES 8
#define H5O_SHMESG_MAX_LIST_SIZE 5000

/* Flags for H5Oget_info.
 * Theses flags determine which fields will be filled in in the H5O_info_t
 * struct.
 */
#define H5O_INFO_BASIC          0x0001u         /* Fill in the fileno, addr, type, and rc fields */
#define H5O_INFO_TIME           0x0002u         /* Fill in the atime, mtime, ctime, and btime fields */
#define H5O_INFO_NUM_ATTRS      0x0004u         /* Fill in the num_attrs field */
#define H5O_INFO_HDR            0x0008u         /* Fill in the hdr field */
#define H5O_INFO_META_SIZE      0x0010u         /* Fill in the meta_size field */
#define H5O_INFO_ALL            (H5O_INFO_BASIC | H5O_INFO_TIME | H5O_INFO_NUM_ATTRS | H5O_INFO_HDR | H5O_INFO_META_SIZE)

/*******************/
/* Public Typedefs */
/*******************/

/* Types of objects in file */
typedef enum H5O_type_t {
    H5O_TYPE_UNKNOWN = -1,    /* Unknown object type        */
    H5O_TYPE_GROUP,            /* Object is a group        */
    H5O_TYPE_DATASET,        /* Object is a dataset        */
    H5O_TYPE_NAMED_DATATYPE,     /* Object is a named data type    */
    H5O_TYPE_NTYPES             /* Number of different object types (must be last!) */
} H5O_type_t;

/* Information struct for object header metadata (for H5Oget_info/H5Oget_info_by_name/H5Oget_info_by_idx) */
typedef struct H5O_hdr_info_t {
    unsigned version;        /* Version number of header format in file */
    unsigned nmesgs;        /* Number of object header messages */
    unsigned nchunks;        /* Number of object header chunks */
    unsigned flags;             /* Object header status flags */
    struct {
        hsize_t total;        /* Total space for storing object header in file */
        hsize_t meta;        /* Space within header for object header metadata information */
        hsize_t mesg;        /* Space within header for actual message information */
        hsize_t free;        /* Free space within object header */
    } space;
    struct {
        uint64_t present;    /* Flags to indicate presence of message type in header */
        uint64_t shared;    /* Flags to indicate message type is shared in header */
    } mesg;
} H5O_hdr_info_t;

/* Information struct for object (for H5Oget_info/H5Oget_info_by_name/H5Oget_info_by_idx) */
typedef struct H5O_info_t {
    unsigned long     fileno;        /* File number that object is located in */
    haddr_t         addr;        /* Object address in file    */
    H5O_type_t         type;        /* Basic object type (group, dataset, etc.) */
    unsigned         rc;        /* Reference count of object    */
    time_t        atime;        /* Access time            */
    time_t        mtime;        /* Modification time        */
    time_t        ctime;        /* Change time            */
    time_t        btime;        /* Birth time            */
    hsize_t         num_attrs;    /* # of attributes attached to object */
    H5O_hdr_info_t      hdr;            /* Object header information */
    /* Extra metadata storage for obj & attributes */
    struct {
        H5_ih_info_t   obj;             /* v1/v2 B-tree & local/fractal heap for groups, B-tree for chunked datasets */
        H5_ih_info_t   attr;            /* v2 B-tree & heap for attributes */
    } meta_size;
} H5O_info_t;

/* Typedef for message creation indexes */
typedef uint32_t H5O_msg_crt_idx_t;

/* Prototype for H5Ovisit/H5Ovisit_by_name() operator */
typedef herr_t (*H5O_iterate_t)(hid_t obj, const char *name, const H5O_info_t *info,
    void *op_data);

typedef enum H5O_mcdt_search_ret_t {
    H5O_MCDT_SEARCH_ERROR = -1,    /* Abort H5Ocopy */
    H5O_MCDT_SEARCH_CONT,    /* Continue the global search of all committed datatypes in the destination file */
    H5O_MCDT_SEARCH_STOP    /* Stop the search, but continue copying.  The committed datatype will be copied but not merged. */
} H5O_mcdt_search_ret_t;

/* Callback to invoke when completing the search for a matching committed datatype from the committed dtype list */
typedef H5O_mcdt_search_ret_t (*H5O_mcdt_search_cb_t)(void *op_data);

/********************/
/* Public Variables */
/********************/


#ifdef __cplusplus
extern "C" {
#endif

/*********************/
/* Public Prototypes */
/*********************/
H5_DLL hid_t H5Oopen(hid_t loc_id, const char *name, hid_t lapl_id);
H5_DLL hid_t H5Oopen_by_addr(hid_t loc_id, haddr_t addr);
H5_DLL hid_t H5Oopen_by_idx(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t lapl_id);
H5_DLL htri_t H5Oexists_by_name(hid_t loc_id, const char *name, hid_t lapl_id);
H5_DLL herr_t H5Oget_info2(hid_t loc_id, H5O_info_t *oinfo, unsigned fields);
H5_DLL herr_t H5Oget_info_by_name2(hid_t loc_id, const char *name, H5O_info_t *oinfo,
    unsigned fields, hid_t lapl_id);
H5_DLL herr_t H5Oget_info_by_idx2(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5O_info_t *oinfo,
    unsigned fields, hid_t lapl_id);
H5_DLL herr_t H5Olink(hid_t obj_id, hid_t new_loc_id, const char *new_name,
    hid_t lcpl_id, hid_t lapl_id);
H5_DLL herr_t H5Oincr_refcount(hid_t object_id);
H5_DLL herr_t H5Odecr_refcount(hid_t object_id);
H5_DLL herr_t H5Ocopy(hid_t src_loc_id, const char *src_name, hid_t dst_loc_id,
    const char *dst_name, hid_t ocpypl_id, hid_t lcpl_id);
H5_DLL herr_t H5Oset_comment(hid_t obj_id, const char *comment);
H5_DLL herr_t H5Oset_comment_by_name(hid_t loc_id, const char *name,
    const char *comment, hid_t lapl_id);
H5_DLL ssize_t H5Oget_comment(hid_t obj_id, char *comment, size_t bufsize);
H5_DLL ssize_t H5Oget_comment_by_name(hid_t loc_id, const char *name,
    char *comment, size_t bufsize, hid_t lapl_id);
H5_DLL herr_t H5Ovisit2(hid_t obj_id, H5_index_t idx_type, H5_iter_order_t order,
    H5O_iterate_t op, void *op_data, unsigned fields);
H5_DLL herr_t H5Ovisit_by_name2(hid_t loc_id, const char *obj_name,
    H5_index_t idx_type, H5_iter_order_t order, H5O_iterate_t op,
    void *op_data, unsigned fields, hid_t lapl_id);
H5_DLL herr_t H5Oclose(hid_t object_id);
H5_DLL herr_t H5Oflush(hid_t obj_id);
H5_DLL herr_t H5Orefresh(hid_t oid);
H5_DLL herr_t H5Odisable_mdc_flushes(hid_t object_id);
H5_DLL herr_t H5Oenable_mdc_flushes(hid_t object_id);
H5_DLL herr_t H5Oare_mdc_flushes_disabled(hid_t object_id, hbool_t *are_disabled);

/* Future function prototypes to be deprecated in next version */
H5_DLL herr_t H5Oget_info1(hid_t loc_id, H5O_info_t *oinfo);
H5_DLL herr_t H5Oget_info_by_name1(hid_t loc_id, const char *name, H5O_info_t *oinfo,
    hid_t lapl_id);
H5_DLL herr_t H5Oget_info_by_idx1(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5O_info_t *oinfo,
    hid_t lapl_id);

H5_DLL herr_t H5Ovisit1(hid_t obj_id, H5_index_t idx_type, H5_iter_order_t order,
    H5O_iterate_t op, void *op_data);
H5_DLL herr_t H5Ovisit_by_name1(hid_t loc_id, const char *obj_name,
    H5_index_t idx_type, H5_iter_order_t order, H5O_iterate_t op,
    void *op_data, hid_t lapl_id);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/* Macros */

/* Typedefs */

/* A struct that's part of the H5G_stat_t structure (deprecated) */
typedef struct H5O_stat_t {
    hsize_t size;               /* Total size of object header in file */
    hsize_t free;               /* Free space within object header */
    unsigned nmesgs;            /* Number of object header messages */
    unsigned nchunks;           /* Number of object header chunks */
} H5O_stat_t;

/* Function prototypes */
#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif
#endif /* _H5Opublic_H */

