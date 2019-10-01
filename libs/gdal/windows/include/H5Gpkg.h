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
 * Programmer: Robb Matzke <matzke@llnl.gov>
 *             Thursday, September 18, 1997
 *
 * Purpose:     This file contains declarations which are visible
 *              only within the H5G package. Source files outside the
 *              H5G package should include H5Gprivate.h instead.
 */
#if !(defined H5G_FRIEND || defined H5G_MODULE)
#error "Do not include this file outside the H5G package!"
#endif

#ifndef _H5Gpkg_H
#define _H5Gpkg_H

/* Get package's private header */
#include "H5Gprivate.h"

/* Other private headers needed by this file */
#include "H5B2private.h"	/* v2 B-trees				*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5HFprivate.h"	/* Fractal heaps			*/
#include "H5HLprivate.h"	/* Local Heaps				*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5SLprivate.h"	/* Skip lists				*/

/**************************/
/* Package Private Macros */
/**************************/

/* Standard length of fractal heap ID for link */
#define H5G_DENSE_FHEAP_ID_LEN  7

/* Size of a symbol table node on disk */
#define H5G_NODE_SIZE(f)     (                                                \
    /* General metadata fields */                                             \
    H5_SIZEOF_MAGIC                                                           \
    + 1         /* Version */                                                 \
    + 1         /* Reserved */                                                \
    + 2         /* Number of symbols */                                       \
                                                                              \
    /* Entries */                                                             \
    + ((2 * H5F_SYM_LEAF_K(f)) * (unsigned)H5G_SIZEOF_ENTRY_FILE(f))                    \
    )


/****************************/
/* Package Private Typedefs */
/****************************/

/*
 * Various types of object header information can be cached in a symbol
 * table entry (it's normal home is the object header to which the entry
 * points).  This datatype determines what (if anything) is cached in the
 * symbol table entry.
 */
typedef enum H5G_cache_type_t {
    H5G_CACHED_ERROR	= -1, 	/*force enum to be signed		     */
    H5G_NOTHING_CACHED  = 0,    /*nothing is cached, must be 0               */
    H5G_CACHED_STAB     = 1,    /*symbol table, `stab'                       */
    H5G_CACHED_SLINK	= 2, 	/*symbolic link				     */

    H5G_NCACHED                 /*THIS MUST BE LAST                          */
} H5G_cache_type_t;

/*
 * A symbol table entry caches these parameters from object header
 * messages...  The values are entered into the symbol table when an object
 * header is created (by hand) and are extracted from the symbol table with a
 * callback function registered in H5O_init_interface().  Be sure to update
 * H5G_ent_decode(), H5G_ent_encode(), and H5G__ent_debug() as well.
 */
typedef union H5G_cache_t {
    struct {
        haddr_t btree_addr;             /*file address of symbol table B-tree*/
        haddr_t heap_addr;              /*file address of stab name heap     */
    } stab;

    struct {
	size_t	lval_offset;		/*link value offset		     */
    } slink;
} H5G_cache_t;

/*
 * A symbol table entry.  The two important fields are `name_off' and
 * `header'.  The remaining fields are used for caching information that
 * also appears in the object header to which this symbol table entry
 * points.
 */
struct H5G_entry_t {
    H5G_cache_type_t type;              /*type of information cached         */
    H5G_cache_t cache;                  /*cached data from object header     */
    size_t      name_off;               /*offset of name within name heap    */
    haddr_t     header;                 /*file address of object header      */
};

/*
 * A symbol table node is a collection of symbol table entries.  It can
 * be thought of as the lowest level of the B-link tree that points to
 * a collection of symbol table entries that belong to a specific symbol
 * table or group.
 */
typedef struct H5G_node_t {
    H5AC_info_t cache_info;     /* Information for H5AC cache functions, _must_ be */
                                /* first field in structure */
    size_t node_size;           /* Size of node on disk              */
    unsigned nsyms;             /* Number of symbols                 */
    H5G_entry_t *entry;         /* Array of symbol table entries     */
} H5G_node_t;

/*
 * Shared information for all open group objects
 */
struct H5G_shared_t {
    int fo_count;                   /* open file object count */
    hbool_t mounted;                /* Group is mount point */
};

/*
 * A group handle passed around through layers of the library within and
 * above the H5G layer.
 */
struct H5G_t {
    H5G_shared_t *shared;               /* Shared file object data */
    H5O_loc_t oloc;                     /* Object location for group */
    H5G_name_t path;                    /* Group hierarchy path   */
};

/* Link iteration operator for internal library callbacks */
typedef herr_t (*H5G_lib_iterate_t)(const H5O_link_t *lnk, void *op_data);

/* Data structure to hold table of links for a group */
typedef struct {
    size_t      nlinks;         /* # of links in table */
    H5O_link_t *lnks;           /* Pointer to array of links */
} H5G_link_table_t;

/*
 * Common data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the methods for the objects
 * to which the B-link tree points.
 *
 * It's also used for B-tree iterators which require no additional info.
 *
 */
typedef struct H5G_bt_common_t {
    /* downward */
    const char  *name;                  /*points to temporary memory         */
    H5HL_t *heap;                       /*symbol table heap		     */
} H5G_bt_common_t;

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the insert method for entries.
 */
typedef struct H5G_bt_ins_t {
    /* downward */
    H5G_bt_common_t common;             /* Common info for B-tree user data (must be first) */
    const H5O_link_t *lnk;              /* Link to insert into table         */
    H5O_type_t obj_type;                /* Type of object being inserted */
    const void *crt_info;               /* Creation info for object being inserted */
} H5G_bt_ins_t;

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the remove method for entries.
 */
typedef struct H5G_bt_rm_t {
    /* downward */
    H5G_bt_common_t common;         /* Common info for B-tree user data (must be first) */
    H5RS_str_t *grp_full_path_r;    /* Full path of group where link is removed */
} H5G_bt_rm_t;

/* Typedef for B-tree 'find' operation */
typedef herr_t (*H5G_bt_find_op_t)(const H5G_entry_t *ent/*in*/, void *operator_data/*in,out*/);

/*
 * Data exchange structure for symbol table nodes.  This structure is
 * passed through the B-link tree layer to the 'find' method for entries.
 */
typedef struct H5G_bt_lkp_t {
    /* downward */
    H5G_bt_common_t common;         /* Common info for B-tree user data (must be first) */
    H5G_bt_find_op_t op;            /* Operator to call when correct entry is found */
    void *op_data;                  /* Data to pass to operator */

    /* upward */
} H5G_bt_lkp_t;

/*
 * Data exchange structure to pass through the B-tree layer for the
 * H5B_iterate function.
 */
typedef struct H5G_bt_it_it_t {
    /* downward */
    H5HL_t      *heap;          /*symbol table heap 			     */
    hsize_t	skip;		/*initial entries to skip		     */
    H5G_lib_iterate_t op;	/*iteration operator			     */
    void	*op_data;	/*user-defined operator data		     */

    /* upward */
    hsize_t	*final_ent;	/*final entry looked at                      */
} H5G_bt_it_it_t;

/* Data passed through B-tree iteration for copying copy symbol table content */
typedef struct H5G_bt_it_cpy_t {
    const H5O_loc_t *src_oloc;  /* Source object location */
    haddr_t     src_heap_addr;  /* Heap address of the source symbol table  */
    H5F_t       *dst_file;      /* File of destination group */
    const H5O_stab_t *dst_stab; /* Symbol table message for destination group */
    H5O_copy_t  *cpy_info;      /* Information for copy operation */
} H5G_bt_it_cpy_t;

/* Common information for "by index" lookups in symbol tables */
typedef struct H5G_bt_it_idx_common_t {
    /* downward */
    hsize_t     idx;            /* Index of group member to be queried */
    hsize_t     num_objs;       /* The number of objects having been traversed */
    H5G_bt_find_op_t op;        /* Operator to call when correct entry is found */
} H5G_bt_it_idx_common_t;

/* Data passed through B-tree iteration for building a table of the links */
typedef struct H5G_bt_it_bt_t {
    /* downward */
    size_t alloc_nlinks;        /* Number of links allocated in table */
    H5HL_t *heap;               /* Symbol table heap */

    /* upward */
    H5G_link_table_t *ltable;   /* Link table to add information to */
} H5G_bt_it_bt_t;

/* Typedefs for "new format" groups */
/* (fractal heap & v2 B-tree info) */

/* Typedef for native 'name' field index records in the v2 B-tree */
/* (Keep 'id' field first so generic record handling in callbacks works) */
typedef struct H5G_dense_bt2_name_rec_t {
    uint8_t id[H5G_DENSE_FHEAP_ID_LEN]; /* Heap ID for link */
    uint32_t hash;                      /* Hash of 'name' field value */
} H5G_dense_bt2_name_rec_t;

/* Typedef for native 'creation order' field index records in the v2 B-tree */
/* (Keep 'id' field first so generic record handling in callbacks works) */
typedef struct H5G_dense_bt2_corder_rec_t {
    uint8_t id[H5G_DENSE_FHEAP_ID_LEN]; /* Heap ID for link */
    int64_t corder;                     /* 'creation order' field value */
} H5G_dense_bt2_corder_rec_t;

/*
 * Common data exchange structure for dense link storage.  This structure is
 * passed through the v2 B-tree layer to the methods for the objects
 * to which the v2 B-tree points.
 */
typedef struct H5G_bt2_ud_common_t {
    /* downward */
    H5F_t       *f;                     /* Pointer to file that fractal heap is in */
    H5HF_t      *fheap;                 /* Fractal heap handle               */
    const char  *name;                  /* Name of link to compare           */
    uint32_t    name_hash;              /* Hash of name of link to compare   */
    int64_t     corder;                 /* Creation order value of link to compare   */
    H5B2_found_t found_op;              /* Callback when correct link is found */
    void        *found_op_data;         /* Callback data when correct link is found */
} H5G_bt2_ud_common_t;

/*
 * Data exchange structure for dense link storage.  This structure is
 * passed through the v2 B-tree layer when inserting links.
 */
typedef struct H5G_bt2_ud_ins_t {
    /* downward */
    H5G_bt2_ud_common_t common;         /* Common info for B-tree user data (must be first) */
    uint8_t id[H5G_DENSE_FHEAP_ID_LEN]; /* Heap ID of link to insert         */
} H5G_bt2_ud_ins_t;

/* Typedef for group creation operation */
typedef struct H5G_obj_create_t{
    hid_t gcpl_id;              /* Group creation property list */
    H5G_cache_type_t cache_type; /* Type of symbol table entry cache */
    H5G_cache_t cache;          /* Cached data for symbol table entry */
} H5G_obj_create_t;

/* Callback information for copying groups */
typedef struct H5G_copy_file_ud_t {
    H5O_copy_file_ud_common_t common;   /* Shared information (must be first) */
    H5G_cache_type_t cache_type;        /* Type of symbol table entry cache */
    H5G_cache_t cache;                  /* Cached data for symbol table entry */
} H5G_copy_file_ud_t;


/*****************************/
/* Package Private Variables */
/*****************************/

/*
 * This is the class identifier to give to the B-tree functions.
 */
H5_DLLVAR H5B_class_t H5B_SNODE[1];

/* The v2 B-tree class for indexing 'name' field on links */
H5_DLLVAR const H5B2_class_t H5G_BT2_NAME[1];

/* The v2 B-tree class for indexing 'creation order' field on links */
H5_DLLVAR const H5B2_class_t H5G_BT2_CORDER[1];

/* Free list for managing H5G_t structs */
H5FL_EXTERN(H5G_t);

/* Free list for managing H5G_shared_t structs */
H5FL_EXTERN(H5G_shared_t);

/******************************/
/* Package Private Prototypes */
/******************************/

/*
 * General group routines
 */
H5_DLL H5G_t *H5G__create(H5F_t *file, H5G_obj_create_t *gcrt_info);
H5_DLL H5G_t *H5G__create_named(const H5G_loc_t *loc, const char *name,
    hid_t lcpl_id, hid_t gcpl_id);
H5_DLL H5G_t *H5G__create_anon(H5F_t *file, H5G_obj_create_t *gcrt_info);
H5_DLL H5G_t *H5G__open_name(const H5G_loc_t *loc, const char *name);
H5_DLL hid_t H5G__get_create_plist(const H5G_t *grp);
H5_DLL herr_t H5G__get_info(const H5G_loc_t *loc, H5G_info_t *grp_info);
H5_DLL herr_t H5G__get_info_by_name(const H5G_loc_t *loc, const char *name,
    H5G_info_t *grp_info);
H5_DLL herr_t H5G__get_info_by_idx(const H5G_loc_t *loc, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5G_info_t *grp_info);
H5_DLL herr_t H5G__flush(H5G_t *grp, hid_t group_id);
H5_DLL herr_t H5G__refresh(H5G_t *grp, hid_t group_id);
H5_DLL herr_t H5G__close_cb(H5G_t *grp);

/*
 * Group hierarchy traversal routines
 */
H5_DLL herr_t H5G__traverse_special(const H5G_loc_t *grp_loc,
    const H5O_link_t *lnk, unsigned target, hbool_t last_comp,
    H5G_loc_t *obj_loc, hbool_t *obj_exists);

/*
 * Utility functions
 */
H5_DLL const char *H5G__component(const char *name, size_t *size_p);

/*
 * Functions that understand symbol tables but not names.  The
 * functions that understand names are exported to the rest of
 * the library and appear in H5Gprivate.h.
 */
H5_DLL herr_t H5G__stab_create(H5O_loc_t *grp_oloc, const H5O_ginfo_t *ginfo,
    H5O_stab_t *stab);
H5_DLL herr_t H5G__stab_create_components(H5F_t *f, H5O_stab_t *stab, size_t size_hint);
H5_DLL herr_t H5G__stab_insert(const H5O_loc_t *grp_oloc, const char *name,
    H5O_link_t *obj_lnk, H5O_type_t obj_type, const void *crt_info);
H5_DLL herr_t H5G__stab_insert_real(H5F_t *f, const H5O_stab_t *stab,
    const char *name, H5O_link_t *obj_lnk, H5O_type_t obj_type,
    const void *crt_info);
H5_DLL herr_t H5G__stab_delete(H5F_t *f, const H5O_stab_t *stab);
H5_DLL herr_t H5G__stab_iterate(const H5O_loc_t *oloc, H5_iter_order_t order,
    hsize_t skip, hsize_t *last_lnk, H5G_lib_iterate_t op, void *op_data);
H5_DLL herr_t H5G__stab_count(const struct H5O_loc_t *oloc, hsize_t *num_objs);
H5_DLL herr_t H5G__stab_bh_size(H5F_t *f, const H5O_stab_t *stab,
    H5_ih_info_t *bh_info);
H5_DLL ssize_t H5G__stab_get_name_by_idx(const H5O_loc_t *oloc, H5_iter_order_t order,
    hsize_t n, char* name, size_t size);
H5_DLL herr_t H5G__stab_remove(const H5O_loc_t *oloc, H5RS_str_t *grp_full_path_r,
    const char *name);
H5_DLL herr_t H5G__stab_remove_by_idx(const H5O_loc_t *oloc, 
    H5RS_str_t *grp_full_path_r, H5_iter_order_t order, hsize_t n);
H5_DLL herr_t H5G__stab_lookup(const H5O_loc_t *grp_oloc, const char *name,
    H5O_link_t *lnk);
H5_DLL herr_t H5G__stab_lookup_by_idx(const H5O_loc_t *grp_oloc, H5_iter_order_t order,
    hsize_t n, H5O_link_t *lnk);
#ifndef H5_STRICT_FORMAT_CHECKS
H5_DLL herr_t H5G__stab_valid(H5O_loc_t *grp_oloc, H5O_stab_t *alt_stab);
#endif /* H5_STRICT_FORMAT_CHECKS */
#ifndef H5_NO_DEPRECATED_SYMBOLS
H5_DLL H5G_obj_t H5G__stab_get_type_by_idx(H5O_loc_t *oloc, hsize_t idx);
#endif /* H5_NO_DEPRECATED_SYMBOLS */


/*
 * Functions that understand symbol table entries.
 */
H5_DLL void H5G__ent_copy(H5G_entry_t *dst, const H5G_entry_t *src,
            H5_copy_depth_t depth);
H5_DLL void H5G__ent_reset(H5G_entry_t *ent);
H5_DLL herr_t H5G__ent_decode_vec(const H5F_t *f, const uint8_t **pp,
				  const uint8_t *p_end, H5G_entry_t *ent, unsigned n);
H5_DLL herr_t H5G__ent_encode_vec(const H5F_t *f, uint8_t **pp,
				  const H5G_entry_t *ent, unsigned n);
H5_DLL herr_t H5G__ent_convert(H5F_t *f, H5HL_t *heap, const char *name,
    const H5O_link_t *lnk, H5O_type_t obj_type, const void *crt_info,
    H5G_entry_t *ent);
H5_DLL herr_t H5G__ent_debug(const H5G_entry_t *ent, FILE * stream, int indent,
    int fwidth, const H5HL_t *heap);

/* Functions that understand symbol table nodes */
H5_DLL herr_t H5G__node_init(H5F_t *f);
H5_DLL int H5G__node_iterate(H5F_t *f, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL int H5G__node_sumup(H5F_t *f, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL int H5G__node_by_idx(H5F_t *f, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL int H5G__node_copy(H5F_t *f, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL int H5G__node_build_table(H5F_t *f, const void *_lt_key, haddr_t addr,
		     const void *_rt_key, void *_udata);
H5_DLL herr_t H5G__node_iterate_size(H5F_t *f, const void *_lt_key, haddr_t addr,
                     const void *_rt_key, void *_udata);
H5_DLL herr_t H5G__node_free(H5G_node_t *sym);

/* Functions that understand links in groups */
H5_DLL herr_t H5G__ent_to_link(H5O_link_t *lnk, const H5HL_t *heap,
    const H5G_entry_t *ent, const char *name);
H5_DLL herr_t H5G__link_to_loc(const H5G_loc_t *grp_loc, const H5O_link_t *lnk,
    H5G_loc_t *obj_loc);
H5_DLL herr_t H5G__link_sort_table(H5G_link_table_t *ltable, H5_index_t idx_type,
    H5_iter_order_t order);
H5_DLL herr_t H5G__link_iterate_table(const H5G_link_table_t *ltable,
    hsize_t skip, hsize_t *last_lnk, const H5G_lib_iterate_t op, void *op_data);
H5_DLL herr_t H5G__link_release_table(H5G_link_table_t *ltable);
H5_DLL herr_t H5G__link_name_replace(H5F_t *file, H5RS_str_t *grp_full_path_r,
    const H5O_link_t *lnk);

/* Functions that understand "compact" link storage */
H5_DLL herr_t H5G__compact_insert(const H5O_loc_t *grp_oloc, H5O_link_t *obj_lnk);
H5_DLL ssize_t H5G__compact_get_name_by_idx(const H5O_loc_t *oloc,
    const H5O_linfo_t *linfo, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t idx, char *name, size_t size);
H5_DLL herr_t H5G__compact_remove(const H5O_loc_t *oloc, H5RS_str_t *grp_full_path_r,
    const char *name);
H5_DLL herr_t H5G__compact_remove_by_idx(const H5O_loc_t *oloc, 
    const H5O_linfo_t *linfo, H5RS_str_t *grp_full_path_r, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n);
H5_DLL herr_t H5G__compact_iterate(const H5O_loc_t *oloc,
    const H5O_linfo_t *linfo, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t skip, hsize_t *last_lnk, H5G_lib_iterate_t op, void *op_data);
H5_DLL htri_t H5G__compact_lookup(const H5O_loc_t *grp_oloc, const char *name,
    H5O_link_t *lnk);
H5_DLL herr_t H5G__compact_lookup_by_idx(const H5O_loc_t *oloc,
    const H5O_linfo_t *linfo, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t n, H5O_link_t *lnk);
#ifndef H5_NO_DEPRECATED_SYMBOLS
H5_DLL H5G_obj_t H5G__compact_get_type_by_idx(H5O_loc_t *oloc, 
    const H5O_linfo_t *linfo, hsize_t idx);
#endif /* H5_NO_DEPRECATED_SYMBOLS */

/* Functions that understand "dense" link storage */
H5_DLL herr_t H5G__dense_build_table(H5F_t *f, const H5O_linfo_t *linfo,
    H5_index_t idx_type, H5_iter_order_t order, H5G_link_table_t *ltable);
H5_DLL herr_t H5G__dense_create(H5F_t *f, H5O_linfo_t *linfo,
    const H5O_pline_t *pline);
H5_DLL herr_t H5G__dense_insert(H5F_t *f, const H5O_linfo_t *linfo,
    const H5O_link_t *lnk);
H5_DLL htri_t H5G__dense_lookup(H5F_t *f, const H5O_linfo_t *linfo,
    const char *name, H5O_link_t *lnk);
H5_DLL herr_t H5G__dense_lookup_by_idx(H5F_t *f, const H5O_linfo_t *linfo,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, H5O_link_t *lnk);
H5_DLL herr_t H5G__dense_iterate(H5F_t *f, const H5O_linfo_t *linfo,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t skip, hsize_t *last_lnk,
    H5G_lib_iterate_t op, void *op_data);
H5_DLL ssize_t H5G__dense_get_name_by_idx(H5F_t  *f, H5O_linfo_t *linfo,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, char *name,
    size_t size);
H5_DLL herr_t H5G__dense_remove(H5F_t *f, const H5O_linfo_t *linfo,
    H5RS_str_t *grp_full_path_r, const char *name);
H5_DLL herr_t H5G__dense_remove_by_idx(H5F_t *f, const H5O_linfo_t *linfo,
    H5RS_str_t *grp_full_path_r, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t n);
H5_DLL herr_t H5G__dense_delete(H5F_t *f, H5O_linfo_t *linfo, hbool_t adj_link);
#ifndef H5_NO_DEPRECATED_SYMBOLS
H5_DLL H5G_obj_t H5G__dense_get_type_by_idx(H5F_t  *f, H5O_linfo_t *linfo,
    hsize_t idx);
#endif /* H5_NO_DEPRECATED_SYMBOLS */

/* Functions that understand group objects */
H5_DLL herr_t H5G__obj_create(H5F_t *f, H5G_obj_create_t *gcrt_info,
    H5O_loc_t *oloc/*out*/);
H5_DLL herr_t H5G__obj_create_real(H5F_t *f, const H5O_ginfo_t *ginfo,
    const H5O_linfo_t *linfo, const H5O_pline_t *pline, H5G_obj_create_t *gcrt_info,
    H5O_loc_t *oloc/*out*/);
H5_DLL htri_t H5G__obj_get_linfo(const H5O_loc_t *grp_oloc, H5O_linfo_t *linfo);
H5_DLL herr_t H5G__obj_iterate(const H5O_loc_t *grp_oloc,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t skip, hsize_t *last_lnk,
    H5G_lib_iterate_t op, void *op_data);
H5_DLL herr_t H5G__obj_info(const H5O_loc_t *oloc, H5G_info_t *grp_info);
H5_DLL htri_t H5G__obj_lookup(const H5O_loc_t *grp_oloc, const char *name,
    H5O_link_t *lnk);

/*
 * These functions operate on group hierarchy names.
 */
H5_DLL herr_t H5G__name_init(H5G_name_t *name, const char *path);

/*
 * These functions operate on group "locations"
 */
H5_DLL herr_t H5G__loc_insert(H5G_loc_t *grp_loc, const char *name,
    H5G_loc_t *obj_loc, H5O_type_t obj_type, const void *crt_info);

/* Testing functions */
#ifdef H5G_TESTING
H5_DLL htri_t H5G__is_empty_test(hid_t gid);
H5_DLL htri_t H5G__has_links_test(hid_t gid, unsigned *nmsgs);
H5_DLL htri_t H5G__has_stab_test(hid_t gid);
H5_DLL htri_t H5G__is_new_dense_test(hid_t gid);
H5_DLL herr_t H5G__new_dense_info_test(hid_t gid, hsize_t *name_count, hsize_t *corder_count);
H5_DLL herr_t H5G__lheap_size_test(hid_t gid, size_t *lheap_size);
H5_DLL herr_t H5G__user_path_test(hid_t obj_id, char *user_path, size_t *user_path_len, unsigned *user_path_hidden);
H5_DLL herr_t H5G__verify_cached_stab_test(H5O_loc_t *grp_oloc, H5G_entry_t *ent);
H5_DLL herr_t H5G__verify_cached_stabs_test(hid_t gid);
#endif /* H5G_TESTING */

#endif /* _H5Gpkg_H */

