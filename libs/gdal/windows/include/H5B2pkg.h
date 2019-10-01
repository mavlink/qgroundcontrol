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
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		Monday, January 31, 2005
 *
 * Purpose:	This file contains declarations which are visible only within
 *		the H5B2 package.  Source files outside the H5B2 package should
 *		include H5B2private.h instead.
 */
#if !(defined H5B2_FRIEND || defined H5B2_MODULE)
#error "Do not include this file outside the H5B2 package!"
#endif

#ifndef _H5B2pkg_H
#define _H5B2pkg_H

/* Get package's private header */
#include "H5B2private.h"

/* Other private headers needed by this file */
#include "H5ACprivate.h"	/* Metadata cache			*/
#include "H5FLprivate.h"	/* Free Lists                           */


/**************************/
/* Package Private Macros */
/**************************/

/* Size of storage for number of records per node (on disk) */
#define H5B2_SIZEOF_RECORDS_PER_NODE    (unsigned)2

/* Size of a "tree pointer" (on disk) */
/* (essentially, the largest internal pointer allowed) */
#define H5B2_TREE_POINTER_SIZE(sizeof_addr, sizeof_size)       (              \
    (sizeof_addr) +                                                           \
    H5B2_SIZEOF_RECORDS_PER_NODE +                                            \
    (sizeof_size)                                                             \
    )

/* Size of a internal node pointer (on disk) */
#define H5B2_INT_POINTER_SIZE(h, d) (                                         \
    (unsigned)(h)->sizeof_addr  /* Address of child node */                   \
    + (h)->max_nrec_size        /* # of records in child node */              \
    + (h)->node_info[(d) - 1].cum_max_nrec_size /* Total # of records in child & below */ \
    )

/* Size of checksum information (on disk) */
#define H5B2_SIZEOF_CHKSUM      4

/* Format overhead for all v2 B-tree metadata in the file */
#define H5B2_METADATA_PREFIX_SIZE (                                           \
    (unsigned)H5_SIZEOF_MAGIC   /* Signature */                               \
    + (unsigned)1 /* Version */                                               \
    + (unsigned)1 /* Tree type */                                             \
    + (unsigned)H5B2_SIZEOF_CHKSUM /* Metadata checksum */                    \
    )

/* Size of the v2 B-tree header on disk */
#define H5B2_HEADER_SIZE(sizeof_addr, sizeof_size)   (                        \
    /* General metadata fields */                                             \
    H5B2_METADATA_PREFIX_SIZE                                                 \
                                                                              \
    /* Header specific fields */                                              \
    + (unsigned)4 /* Node size, in bytes */                                   \
    + (unsigned)2 /* Record size, in bytes */                                 \
    + (unsigned)2 /* Depth of tree */                                         \
    + (unsigned)1 /* Split % of full (as integer, ie. "98" means 98%) */      \
    + (unsigned)1 /* Merge % of full (as integer, ie. "98" means 98%) */      \
    + H5B2_TREE_POINTER_SIZE(sizeof_addr, sizeof_size)  /* Node pointer to root node in tree */ \
    )

/* Size of the v2 B-tree header on disk (via file pointer) */
#define H5B2_HEADER_SIZE_FILE(f)   (                                          \
    H5B2_HEADER_SIZE(H5F_SIZEOF_ADDR(f), H5F_SIZEOF_SIZE(f))                  \
    )

/* Size of the v2 B-tree header on disk (via v2 B-tree header) */
#define H5B2_HEADER_SIZE_HDR(h)   (                                           \
    H5B2_HEADER_SIZE((h)->sizeof_addr, (h)->sizeof_size)                      \
    )

/* Size of the v2 B-tree internal node prefix */
#define H5B2_INT_PREFIX_SIZE (                                                \
    /* General metadata fields */                                             \
    H5B2_METADATA_PREFIX_SIZE                                                 \
                                                                              \
    /* Header specific fields */                                              \
    /* <none> */                                                              \
    )

/* Size of the v2 B-tree leaf node prefix */
#define H5B2_LEAF_PREFIX_SIZE (                                               \
    /* General metadata fields */                                             \
    H5B2_METADATA_PREFIX_SIZE                                                 \
                                                                              \
    /* Header specific fields */                                              \
    /* <none> */                                                              \
    )

/* Macro to retrieve pointer to i'th native record for native record buffer */
#define H5B2_NAT_NREC(b, hdr, idx)  ((b) + (hdr)->nat_off[(idx)])

/* Macro to retrieve pointer to i'th native record for internal node */
#define H5B2_INT_NREC(i, hdr, idx)  H5B2_NAT_NREC((i)->int_native, (hdr), (idx))

/* Macro to retrieve pointer to i'th native record for leaf node */
#define H5B2_LEAF_NREC(l, hdr, idx)  H5B2_NAT_NREC((l)->leaf_native, (hdr), (idx))

/* Number of records that fit into internal node */
/* (accounts for extra node pointer by counting it in with the prefix bytes) */
#define H5B2_NUM_INT_REC(h, d) \
    (((h)->node_size - (H5B2_INT_PREFIX_SIZE + H5B2_INT_POINTER_SIZE(h, d))) / ((h)->rrec_size + H5B2_INT_POINTER_SIZE(h, d)))

/* Uncomment this macro to enable extra sanity checking */
/* #define H5B2_DEBUG */


/****************************/
/* Package Private Typedefs */
/****************************/

/* A "node pointer" to another B-tree node */
typedef struct {
    haddr_t     addr;           /* Address of other node */
    uint16_t    node_nrec;      /* Number of records used in node pointed to */
    hsize_t     all_nrec;       /* Number of records in node pointed to and all it's children */
} H5B2_node_ptr_t;

/* Information about a node at a given depth */
typedef struct {
    unsigned    max_nrec;       /* Max. number of records in node */
    unsigned    split_nrec;     /* Number of records to split node at */
    unsigned    merge_nrec;     /* Number of records to merge node at */
    hsize_t     cum_max_nrec;   /* Cumulative max. # of records below this node's depth */
    uint8_t     cum_max_nrec_size; /* Size to store cumulative max. # of records for this node (in bytes) */
    H5FL_fac_head_t *nat_rec_fac;   /* Factory for native record blocks */
    H5FL_fac_head_t *node_ptr_fac;  /* Factory for node pointer blocks */
} H5B2_node_info_t;

/* The B-tree header information */
typedef struct H5B2_hdr_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Internal B-tree information (stored) */
    H5B2_node_ptr_t root;       /* Node pointer to root node in B-tree        */

    /* Information set by user (stored) */
    uint8_t     split_percent;  /* Percent full at which to split the node, when inserting */
    uint8_t     merge_percent;  /* Percent full at which to merge the node, when deleting */
    uint32_t    node_size;      /* Size of B-tree nodes, in bytes             */
    uint32_t    rrec_size;      /* Size of "raw" (on disk) record, in bytes   */

    /* Dynamic information (stored) */
    uint16_t	depth;		/* B-tree's overall depth                     */

    /* Derived information from user's information (not stored) */
    uint8_t     max_nrec_size;  /* Size to store max. # of records in any node (in bytes) */

    /* Shared internal data structures (not stored) */
    H5F_t       *f;             /* Pointer to the file that the B-tree is in */
    haddr_t     addr;           /* Address of B-tree header in the file */
    size_t      hdr_size;       /* Size of the B-tree header on disk */
    size_t      rc;             /* Reference count of nodes using this header */
    size_t      file_rc;        /* Reference count of files using this header */
    hbool_t     pending_delete; /* B-tree is pending deletion */
    uint8_t     sizeof_size;    /* Size of file sizes */
    uint8_t     sizeof_addr;    /* Size of file addresses */
    H5B2_remove_t remove_op;    /* Callback operator for deleting B-tree */
    void        *remove_op_data;/* B-tree deletion callback's context */
    uint8_t	*page;	        /* Common disk page for I/O */
    size_t      *nat_off;       /* Array of offsets of native records */
    H5B2_node_info_t *node_info; /* Table of node info structs for current depth of B-tree */
    void        *min_native_rec; /* Pointer to minimum native record                  */
    void        *max_native_rec; /* Pointer to maximum native record                  */

    /* SWMR / Flush dependency information (not stored) */
    hbool_t     swmr_write;     /* Whether we are doing SWMR writes */
    H5AC_proxy_entry_t *top_proxy;  /* 'Top' proxy cache entry for all B-tree entries */
    void        *parent;        /* Pointer to 'top' proxy flush dependency
                                 * parent, if it exists, otherwise NULL.
                                 * If the v2 B-tree is being used to index a
                                 * chunked dataset and the dataset metadata is
                                 * modified by a SWMR writer, this field will
                                 * be set equal to the object header proxy
                                 * that is the flush dependency parent
                                 * of the v2 B-tree header.
 				 *
 				 * The field is used to avoid duplicate setups
                                 * of the flush dependency relationship, and to
                                 * allow the v2 B-tree header to destroy the
                                 * flush dependency on receipt of an eviction
                                 * notification from the metadata cache.
				 */
    uint64_t    shadow_epoch;   /* Epoch of header, for making shadow copies */

    /* Client information (not stored) */
    const H5B2_class_t *cls;	/* Class of B-tree client */
    void        *cb_ctx;        /* Client callback context */
} H5B2_hdr_t;

/* B-tree leaf node information */
typedef struct H5B2_leaf_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Internal B-tree information */
    H5B2_hdr_t	*hdr;		/* Pointer to the [pinned] v2 B-tree header   */
    uint8_t     *leaf_native;   /* Pointer to native records                  */
    uint16_t    nrec;           /* Number of records in node                  */

    /* SWMR / Flush dependency information (not stored) */
    H5AC_proxy_entry_t *top_proxy;  /* 'Top' proxy cache entry for all B-tree entries */
    void        *parent;        /* Flush dependency parent for leaf           */
    uint64_t    shadow_epoch;   /* Epoch of node, for making shadow copies */
} H5B2_leaf_t;

/* B-tree internal node information */
typedef struct H5B2_internal_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Internal B-tree information */
    H5B2_hdr_t	*hdr;		/* Pointer to the [pinned] v2 B-tree header   */
    uint8_t     *int_native;    /* Pointer to native records                  */
    H5B2_node_ptr_t *node_ptrs; /* Pointer to node pointers                   */
    uint16_t    nrec;           /* Number of records in node                  */
    uint16_t    depth;          /* Depth of this node in the B-tree           */

    /* SWMR / Flush dependency information (not stored) */
    H5AC_proxy_entry_t *top_proxy;  /* 'Top' proxy cache entry for all B-tree entries */
    void        *parent;        /* Flush dependency parent for internal node  */
    uint64_t    shadow_epoch;   /* Epoch of node, for making shadow copies */
} H5B2_internal_t;

/* v2 B-tree */
struct H5B2_t {
    H5B2_hdr_t  *hdr;           /* Pointer to internal v2 B-tree header info */
    H5F_t      *f;              /* Pointer to file for v2 B-tree */
};

/* Node position, for min/max determination */
typedef enum H5B2_nodepos_t {
    H5B2_POS_ROOT,              /* Node is root (i.e. both right & left-most in tree) */
    H5B2_POS_RIGHT,             /* Node is right-most in tree, at a given depth */
    H5B2_POS_LEFT,              /* Node is left-most in tree, at a given depth */
    H5B2_POS_MIDDLE             /* Node is neither right or left-most in tree */
} H5B2_nodepos_t;

/* Update status */
typedef enum H5B2_update_status_t {
    H5B2_UPDATE_UNKNOWN,            /* Unknown update status (initial state) */
    H5B2_UPDATE_MODIFY_DONE,        /* Update successfully modified existing record */
    H5B2_UPDATE_SHADOW_DONE,        /* Update modified existing record and modified node was shadowed */
    H5B2_UPDATE_INSERT_DONE,        /* Update inserted record successfully */
    H5B2_UPDATE_INSERT_CHILD_FULL   /* Update will insert record, but child is full */
} H5B2_update_status_t;

/* Callback info for loading a v2 B-tree header into the cache */
typedef struct H5B2_hdr_cache_ud_t {
    H5F_t *f;                   /* File that v2 b-tree header is within */
    haddr_t addr;               /* Address of B-tree header in the file */
    void *ctx_udata;            /* User-data for protecting */
} H5B2_hdr_cache_ud_t;

/* Callback info for loading a v2 B-tree internal node into the cache */
typedef struct H5B2_internal_cache_ud_t {
    H5F_t *f;                   /* File that v2 b-tree header is within */
    H5B2_hdr_t *hdr;            /* v2 B-tree header */
    void *parent;               /* Flush dependency parent */
    uint16_t nrec;              /* Number of records in node to load */
    uint16_t depth;             /* Depth of node to load */
} H5B2_internal_cache_ud_t;

/* Callback info for loading a v2 B-tree leaf node into the cache */
typedef struct H5B2_leaf_cache_ud_t {
    H5F_t *f;                   /* File that v2 b-tree header is within */
    H5B2_hdr_t *hdr;            /* v2 B-tree header */
    void *parent;               /* Flush dependency parent */
    uint16_t nrec;              /* Number of records in node to load */
} H5B2_leaf_cache_ud_t;

#ifdef H5B2_TESTING
/* Node information for testing */
typedef struct H5B2_node_info_test_t {
    uint16_t depth;             /* Depth of node */
    uint16_t nrec;              /* Number of records in node */
} H5B2_node_info_test_t;
#endif /* H5B2_TESTING */


/*****************************/
/* Package Private Variables */
/*****************************/

/* Declare a free list to manage the H5B2_internal_t struct */
H5FL_EXTERN(H5B2_internal_t);

/* Declare a free list to manage the H5B2_leaf_t struct */
H5FL_EXTERN(H5B2_leaf_t);

/* Internal v2 B-tree testing class */
#ifdef H5B2_TESTING
H5_DLLVAR const H5B2_class_t H5B2_TEST[1];
H5_DLLVAR const H5B2_class_t H5B2_TEST2[1];

/* B-tree record for testing H5B2_TEST2 class */
typedef struct H5B2_test_rec_t {
    hsize_t key;        /* Key for record */
    hsize_t val;        /* Value for record */
} H5B2_test_rec_t;
#endif /* H5B2_TESTING */

/* Array of v2 B-tree client ID -> client class mappings */
extern const H5B2_class_t *const H5B2_client_class_g[H5B2_NUM_BTREE_ID];


/******************************/
/* Package Private Prototypes */
/******************************/

/* Generic routines */
H5_DLL herr_t H5B2__create_flush_depend(H5AC_info_t *parent_entry,
    H5AC_info_t *child_entry);
H5_DLL herr_t H5B2__update_flush_depend(H5B2_hdr_t *hdr, unsigned depth,
    const H5B2_node_ptr_t *node_ptr, void *old_parent, void *new_parent);
H5_DLL herr_t H5B2__destroy_flush_depend(H5AC_info_t *parent_entry,
    H5AC_info_t *child_entry);

/* Internal node management routines */
H5_DLL herr_t H5B2__split1(H5B2_hdr_t *hdr, uint16_t depth, H5B2_node_ptr_t *curr_node_ptr,
    unsigned *parent_cache_info_flags_ptr, H5B2_internal_t *internal,
    unsigned *internal_flags_ptr, unsigned idx);
H5_DLL herr_t H5B2__redistribute2(H5B2_hdr_t *hdr, uint16_t depth,
    H5B2_internal_t *internal, unsigned idx);
H5_DLL herr_t H5B2__redistribute3(H5B2_hdr_t *hdr, uint16_t depth,
    H5B2_internal_t *internal, unsigned *internal_flags_ptr, unsigned idx);
H5_DLL herr_t H5B2__merge2(H5B2_hdr_t *hdr, uint16_t depth, H5B2_node_ptr_t *curr_node_ptr,
    unsigned *parent_cache_info_flags_ptr, H5B2_internal_t *internal,
    unsigned *internal_flags_ptr, unsigned idx);
H5_DLL herr_t H5B2__merge3(H5B2_hdr_t *hdr, uint16_t depth, H5B2_node_ptr_t *curr_node_ptr,
    unsigned *parent_cache_info_flags_ptr, H5B2_internal_t *internal,
    unsigned *internal_flags_ptr, unsigned idx);

/* Routines for managing B-tree header info */
H5_DLL H5B2_hdr_t *H5B2__hdr_alloc(H5F_t *f);
H5_DLL haddr_t H5B2__hdr_create(H5F_t *f, const H5B2_create_t *cparam, void *ctx_udata);
H5_DLL herr_t H5B2__hdr_init(H5B2_hdr_t *hdr, const H5B2_create_t *cparam,
    void *ctx_udata, uint16_t depth);
H5_DLL herr_t H5B2__hdr_incr(H5B2_hdr_t *hdr);
H5_DLL herr_t H5B2__hdr_decr(H5B2_hdr_t *hdr);
H5_DLL herr_t H5B2__hdr_fuse_incr(H5B2_hdr_t *hdr);
H5_DLL size_t H5B2__hdr_fuse_decr(H5B2_hdr_t *hdr);
H5_DLL herr_t H5B2__hdr_dirty(H5B2_hdr_t *hdr);
H5_DLL H5B2_hdr_t *H5B2__hdr_protect(H5F_t *f, haddr_t hdr_addr,
    void *ctx_udata, unsigned flags);
H5_DLL herr_t H5B2__hdr_unprotect(H5B2_hdr_t *hdr, unsigned cache_flags);
H5_DLL herr_t H5B2__hdr_delete(H5B2_hdr_t *hdr);

/* Routines for operating on leaf nodes */
H5_DLL H5B2_leaf_t * H5B2__protect_leaf(H5B2_hdr_t *hdr, void *parent,
    H5B2_node_ptr_t *node_ptr, hbool_t shadow, unsigned flags);
H5_DLL herr_t H5B2__swap_leaf(H5B2_hdr_t *hdr, uint16_t depth, H5B2_internal_t *internal,
    unsigned *internal_flags_ptr, unsigned idx, void *swap_loc);

/* Routines for operating on internal nodes */
H5_DLL H5B2_internal_t *H5B2__protect_internal(H5B2_hdr_t *hdr, void *parent,
    H5B2_node_ptr_t *node_ptr, uint16_t depth, hbool_t shadow, unsigned flags);

/* Routines for allocating nodes */
H5_DLL herr_t H5B2__split_root(H5B2_hdr_t *hdr);
H5_DLL herr_t H5B2__create_leaf(H5B2_hdr_t *hdr, void *parent,
    H5B2_node_ptr_t *node_ptr);
H5_DLL herr_t H5B2__create_internal(H5B2_hdr_t *hdr, void *parent,
    H5B2_node_ptr_t *node_ptr, uint16_t depth);

/* Routines for releasing structures */
H5_DLL herr_t H5B2__hdr_free(H5B2_hdr_t *hdr);
H5_DLL herr_t H5B2__leaf_free(H5B2_leaf_t *l);
H5_DLL herr_t H5B2__internal_free(H5B2_internal_t *i);

/* Routines for inserting records */
H5_DLL herr_t H5B2__insert(H5B2_hdr_t *hdr, void *udata);
H5_DLL herr_t H5B2__insert_internal(H5B2_hdr_t *hdr, uint16_t depth,
    unsigned *parent_cache_info_flags_ptr, H5B2_node_ptr_t *curr_node_ptr,
    H5B2_nodepos_t curr_pos, void *parent, void *udata);
H5_DLL herr_t H5B2__insert_leaf(H5B2_hdr_t *hdr, H5B2_node_ptr_t *curr_node_ptr,
    H5B2_nodepos_t curr_pos, void *parent, void *udata);

/* Routines for update records */
H5_DLL herr_t H5B2__update_internal(H5B2_hdr_t *hdr, uint16_t depth,
    unsigned *parent_cache_info_flags_ptr, H5B2_node_ptr_t *curr_node_ptr,
    H5B2_update_status_t *status, H5B2_nodepos_t curr_pos, void *parent,
    void *udata, H5B2_modify_t op, void *op_data);
H5_DLL herr_t H5B2__update_leaf(H5B2_hdr_t *hdr, H5B2_node_ptr_t *curr_node_ptr,
    H5B2_update_status_t *status, H5B2_nodepos_t curr_pos, void *parent,
    void *udata, H5B2_modify_t op, void *op_data);

/* Routines for iterating over nodes/records */
H5_DLL herr_t H5B2__iterate_node(H5B2_hdr_t *hdr, uint16_t depth,
    const H5B2_node_ptr_t *curr_node, void *parent, H5B2_operator_t op, void *op_data);
H5_DLL herr_t H5B2__node_size(H5B2_hdr_t *hdr, uint16_t depth,
    const H5B2_node_ptr_t *curr_node, void *parent, hsize_t *op_data);

/* Routines for locating records */
H5_DLL herr_t H5B2__locate_record(const H5B2_class_t *type, unsigned nrec,
    size_t *rec_off, const uint8_t *native, const void *udata, unsigned *idx, int *result);
H5_DLL herr_t H5B2__neighbor_internal(H5B2_hdr_t *hdr, uint16_t depth,
    H5B2_node_ptr_t *curr_node_ptr, void *neighbor_loc, H5B2_compare_t comp,
    void *parent, void *udata, H5B2_found_t op, void *op_data);
H5_DLL herr_t H5B2__neighbor_leaf(H5B2_hdr_t *hdr, H5B2_node_ptr_t *curr_node_ptr,
    void *neighbor_loc, H5B2_compare_t comp, void *parent, void *udata,
    H5B2_found_t op, void *op_data);

/* Routines for removing records */
H5_DLL herr_t H5B2__remove_internal(H5B2_hdr_t *hdr, hbool_t *depth_decreased,
    void *swap_loc, void *swap_parent, uint16_t depth, H5AC_info_t *parent_cache_info,
        unsigned *parent_cache_info_flags_ptr, H5B2_nodepos_t curr_pos,
        H5B2_node_ptr_t *curr_node_ptr, void *udata, H5B2_remove_t op,
        void *op_data);
H5_DLL herr_t H5B2__remove_leaf(H5B2_hdr_t *hdr, H5B2_node_ptr_t *curr_node_ptr,
    H5B2_nodepos_t curr_pos, void *parent, void *udata, H5B2_remove_t op,
    void *op_data);
H5_DLL herr_t H5B2__remove_internal_by_idx(H5B2_hdr_t *hdr, 
    hbool_t *depth_decreased, void *swap_loc, void *swap_parent, uint16_t depth,
    H5AC_info_t *parent_cache_info, unsigned *parent_cache_info_flags_ptr,
    H5B2_node_ptr_t *curr_node_ptr, H5B2_nodepos_t curr_pos, hsize_t n,
    H5B2_remove_t op, void *op_data);
H5_DLL herr_t H5B2__remove_leaf_by_idx(H5B2_hdr_t *hdr, H5B2_node_ptr_t *curr_node_ptr,
    H5B2_nodepos_t curr_pos, void *parent, unsigned idx, H5B2_remove_t op,
    void *op_data);

/* Routines for deleting nodes */
H5_DLL herr_t H5B2__delete_node(H5B2_hdr_t *hdr, uint16_t depth,
    const H5B2_node_ptr_t *curr_node, void *parent, H5B2_remove_t op,
    void *op_data);

/* Debugging routines for dumping file structures */
H5_DLL herr_t H5B2__hdr_debug(H5F_t *f, haddr_t addr, FILE *stream, int indent,
    int fwidth, const H5B2_class_t *type, haddr_t obj_addr);
H5_DLL herr_t H5B2__int_debug(H5F_t *f, haddr_t addr, FILE *stream, int indent,
    int fwidth, const H5B2_class_t *type, haddr_t hdr_addr, unsigned nrec, unsigned depth,
    haddr_t obj_addr);
H5_DLL herr_t H5B2__leaf_debug(H5F_t *f, haddr_t addr, FILE *stream, int indent,
    int fwidth, const H5B2_class_t *type, haddr_t hdr_addr, unsigned nrec, haddr_t obj_addr);

/* Sanity checking routines */
#ifdef H5B2_DEBUG
/* Don't label these with H5_ATTR_PURE or you'll get even more warnings... */
H5_DLL herr_t H5B2__assert_internal(hsize_t parent_all_nrec, const H5B2_hdr_t *hdr, const H5B2_internal_t *internal);
H5_DLL herr_t H5B2__assert_internal2(hsize_t parent_all_nrec, const H5B2_hdr_t *hdr, const H5B2_internal_t *internal, const H5B2_internal_t *internal2);
H5_DLL herr_t H5B2__assert_leaf(const H5B2_hdr_t *hdr, const H5B2_leaf_t *leaf);
H5_DLL herr_t H5B2__assert_leaf2(const H5B2_hdr_t *hdr, const H5B2_leaf_t *leaf, const H5B2_leaf_t *leaf2);
#endif /* H5B2_DEBUG */

/* Testing routines */
#ifdef H5B2_TESTING
H5_DLL herr_t H5B2__get_root_addr_test(H5B2_t *bt2, haddr_t *root_addr);
H5_DLL int H5B2__get_node_depth_test(H5B2_t *bt2, void *udata);
H5_DLL herr_t H5B2__get_node_info_test(H5B2_t *bt2, void *udata,
    H5B2_node_info_test_t *ninfo);
#endif /* H5B2_TESTING */

#endif /* _H5B2pkg_H */

