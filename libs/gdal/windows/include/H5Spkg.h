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
 *		Thursday, September 28, 2000
 *
 * Purpose:	This file contains declarations which are visible only within
 *		the H5S package.  Source files outside the H5S package should
 *		include H5Sprivate.h instead.
 */
#if !(defined H5S_FRIEND || defined H5S_MODULE)
#error "Do not include this file outside the H5S package!"
#endif

#ifndef _H5Spkg_H
#define _H5Spkg_H

#include "H5Sprivate.h"

/* Flags to indicate special dataspace features are active */
#define H5S_VALID_MAX	0x01
#define H5S_VALID_PERM	0x02

/* Flags for serialization of selections */
#define H5S_HYPER_REGULAR       0x01
#define H5S_SELECT_FLAG_BITS    (H5S_HYPER_REGULAR)

/* Versions for H5S_SEL_HYPER selection info */
#define H5S_HYPER_VERSION_1     1
#define H5S_HYPER_VERSION_2     2

/* Versions for H5S_SEL_POINTS selection info */
#define H5S_POINT_VERSION_1     1

/* Versions for H5S_SEL_NONE selection info */
#define H5S_NONE_VERSION_1      1

/* Versions for H5S_SEL_ALL selection info */
#define H5S_ALL_VERSION_1       1

/* Size of point/offset info for H5S_SEL_POINTS/H5S_SEL_HYPER */
#define H5S_INFO_SIZE_4 0x04        /* 4 bytes: 32 bits */
#define H5S_INFO_SIZE_8 0x08        /* 8 bytes: 64 bits */
#define H5S_SELECT_INFO_SIZE_BITS   (H5S_INFO_SIZE_4|H5S_INFO_SIZE_8)

#define H5S_UINT32_MAX      4294967295  /* 2^32 - 1 */

/* Length of stack-allocated sequences for "project intersect" routines */
#define H5S_PROJECT_INTERSECT_NSEQS 256


/* Initial version of the dataspace information */
#define H5O_SDSPACE_VERSION_1	1

/* This version adds support for "null" dataspaces, encodes the type of the
 *      dataspace in the message and eliminated the rest of the "reserved"
 *      bytes.
 */
#define H5O_SDSPACE_VERSION_2	2

/* The latest version of the format.  Look through the 'encode'
 *      and 'size' callbacks for places to change when updating this. */
#define H5O_SDSPACE_VERSION_LATEST H5O_SDSPACE_VERSION_2

/* Maximum dimension size (highest value that is not a special value e.g.
 * H5S_UNLIMITED) */
#define H5S_MAX_SIZE            ((hsize_t)(hssize_t)(-2))


/*
 * Dataspace extent information
 */
/* Extent container */
struct H5S_extent_t {
    H5O_shared_t sh_loc;        /* Shared message info (must be first) */

    H5S_class_t	type;           /* Type of extent */
    unsigned version;           /* Version of object header message to encode this object with */
    hsize_t nelem;              /* Number of elements in extent */

    unsigned rank;              /* Number of dimensions */
    hsize_t *size;              /* Current size of the dimensions */
    hsize_t *max;               /* Maximum size of the dimensions */
};

/*
 * Dataspace selection information
 */
/* Node in point selection list (typedef'd in H5Sprivate.h) */
struct H5S_pnt_node_t {
    hsize_t *pnt;          /* Pointer to a selected point */
    struct H5S_pnt_node_t *next;  /* pointer to next point in list */
};

/* Information about point selection list */
typedef struct {
    H5S_pnt_node_t *head;   /* Pointer to head of point list */
} H5S_pnt_list_t;

/* Information about new-style hyperslab spans */

/* Information a particular hyperslab span */
struct H5S_hyper_span_t {
    hsize_t low, high;          /* Low & high bounds of span */
    hsize_t nelem;              /* Number of elements in span (only needed during I/O) */
    hsize_t pstride;            /* Pseudo-stride from start of previous span (only used during I/O) */
    struct H5S_hyper_span_info_t *down;     /* Pointer to list of spans in next dimension down */
    struct H5S_hyper_span_t *next;     /* Pointer to next span in list */
};

/* Information about a list of hyperslab spans in one dimension */
struct H5S_hyper_span_info_t {
    unsigned count;                    /* Ref. count of number of spans which share this span */
    struct H5S_hyper_span_info_t *scratch;  /* Scratch pointer
                                             * (used during copies, as mark
                                             * during precomputes for I/O &
                                             * to point to the last span in a
                                             * list during single element adds)
                                             */
    struct H5S_hyper_span_t *head;  /* Pointer to list of spans in next dimension down */
};

/* Information about new-style hyperslab selection */
typedef struct {
    hbool_t diminfo_valid;                      /* Whether the dataset has valid diminfo */
    H5S_hyper_dim_t opt_diminfo[H5S_MAX_RANK];  /* per-dim selection info */
    H5S_hyper_dim_t app_diminfo[H5S_MAX_RANK];  /* per-dim selection info */
	/* 'opt_diminfo' points to a [potentially] optimized version of the user's
         * hyperslab information.  'app_diminfo' points to the actual parameters
         * that the application used for setting the hyperslab selection.  These
         * are only used for re-gurgitating the original values used to set the
         * hyperslab to the application when it queries the hyperslab selection
         * information. */
    int unlim_dim;                      /* Dimension where selection is unlimited, or -1 if none */
    hsize_t num_elem_non_unlim;         /* # of elements in a "slice" excluding the unlimited dimension */
    H5S_hyper_span_info_t *span_lst;    /* List of hyperslab span information */
} H5S_hyper_sel_t;

/* Selection information methods */
/* Method to copy a selection */
typedef herr_t (*H5S_sel_copy_func_t)(H5S_t *dst, const H5S_t *src, hbool_t share_selection);
/* Method to retrieve a list of offset/length sequences for selection */
typedef herr_t (*H5S_sel_get_seq_list_func_t)(const H5S_t *space, unsigned flags,
    H5S_sel_iter_t *iter, size_t maxseq, size_t maxbytes,
    size_t *nseq, size_t *nbytes, hsize_t *off, size_t *len);
/* Method to release current selection */
typedef herr_t (*H5S_sel_release_func_t)(H5S_t *space);
/* Method to determine if current selection is valid for dataspace */
typedef htri_t (*H5S_sel_is_valid_func_t)(const H5S_t *space);
/* Method to determine number of bytes required to store current selection */
typedef hssize_t (*H5S_sel_serial_size_func_t)(const H5S_t *space, H5F_t *f);
/* Method to store current selection in "serialized" form (a byte sequence suitable for storing on disk) */
typedef herr_t (*H5S_sel_serialize_func_t)(const H5S_t *space, uint8_t **p, H5F_t *f);
/* Method to create selection from "serialized" form (a byte sequence suitable for storing on disk) */
typedef herr_t (*H5S_sel_deserialize_func_t)(H5S_t *space, uint32_t version, uint8_t flags,
    const uint8_t **p);
/* Method to determine smallest n-D bounding box containing the current selection */
typedef herr_t (*H5S_sel_bounds_func_t)(const H5S_t *space, hsize_t *start, hsize_t *end);
/* Method to determine linear offset of initial element in selection within dataspace */
typedef herr_t (*H5S_sel_offset_func_t)(const H5S_t *space, hsize_t *offset);
/* Method to get unlimited dimension of selection (or -1 for none) */
typedef int (*H5S_sel_unlim_dim_func_t)(const H5S_t *space);
/* Method to get the number of elements in a slice through the unlimited dimension */
typedef herr_t (*H5S_sel_num_elem_non_unlim_func_t)(const H5S_t *space,
    hsize_t *num_elem_non_unlim);
/* Method to determine if current selection is contiguous */
typedef htri_t (*H5S_sel_is_contiguous_func_t)(const H5S_t *space);
/* Method to determine if current selection is a single block */
typedef htri_t (*H5S_sel_is_single_func_t)(const H5S_t *space);
/* Method to determine if current selection is "regular" */
typedef htri_t (*H5S_sel_is_regular_func_t)(const H5S_t *space);
/* Method to adjust a selection by an offset */
typedef void (*H5S_sel_adjust_u_func_t)(H5S_t *space, const hsize_t *offset);
/* Method to construct single element projection onto scalar dataspace */
typedef herr_t (*H5S_sel_project_scalar)(const H5S_t *space, hsize_t *offset);
/* Method to construct selection projection onto/into simple dataspace */
typedef herr_t (*H5S_sel_project_simple)(const H5S_t *space, H5S_t *new_space, hsize_t *offset);
/* Method to initialize iterator for current selection */
typedef herr_t (*H5S_sel_iter_init_func_t)(H5S_sel_iter_t *sel_iter, const H5S_t *space);

/* Selection class information */
typedef struct {
    H5S_sel_type type;                          /* Type of selection (all, none, points or hyperslab) */

    /* Methods */
    H5S_sel_copy_func_t copy;                   /* Method to make a copy of a selection */
    H5S_sel_get_seq_list_func_t get_seq_list;   /* Method to retrieve a list of offset/length sequences for selection */
    H5S_sel_release_func_t release;             /* Method to release current selection */
    H5S_sel_is_valid_func_t is_valid;           /* Method to determine if current selection is valid for dataspace */
    H5S_sel_serial_size_func_t serial_size;     /* Method to determine number of bytes required to store current selection */
    H5S_sel_serialize_func_t serialize;         /* Method to store current selection in "serialized" form (a byte sequence suitable for storing on disk) */
    H5S_sel_deserialize_func_t deserialize;     /* Method to store create selection from "serialized" form (a byte sequence suitable for storing on disk) */
    H5S_sel_bounds_func_t bounds;               /* Method to determine to smallest n-D bounding box containing the current selection */
    H5S_sel_offset_func_t offset;               /* Method to determine linear offset of initial element in selection within dataspace */
    H5S_sel_unlim_dim_func_t unlim_dim;         /* Method to get unlimited dimension of selection (or -1 for none) */
    H5S_sel_num_elem_non_unlim_func_t num_elem_non_unlim; /* Method to get the number of elements in a slice through the unlimited dimension */
    H5S_sel_is_contiguous_func_t is_contiguous; /* Method to determine if current selection is contiguous */
    H5S_sel_is_single_func_t is_single;         /* Method to determine if current selection is a single block */
    H5S_sel_is_regular_func_t is_regular;       /* Method to determine if current selection is "regular" */
    H5S_sel_adjust_u_func_t adjust_u;           /* Method to adjust a selection by an offset */
    H5S_sel_project_scalar project_scalar;      /* Method to construct scalar dataspace projection */
    H5S_sel_project_simple project_simple;      /* Method to construct simple dataspace projection */
    H5S_sel_iter_init_func_t iter_init;         /* Method to initialize iterator for current selection */
} H5S_select_class_t;

/* Selection information object */
typedef struct {
    const H5S_select_class_t *type;     /* Pointer to selection's class info */

    hbool_t offset_changed;             /* Indicate that the offset for the selection has been changed */
    hssize_t offset[H5S_MAX_RANK];      /* Offset within the extent */

    hsize_t num_elem;                   /* Number of elements in selection */

    union {
        H5S_pnt_list_t *pnt_lst;        /* Info about list of selected points (order is important) */
        H5S_hyper_sel_t *hslab;         /* Info about hyperslab selection */
    } sel_info;
} H5S_select_t;

/* Main dataspace structure (typedef'd in H5Sprivate.h) */
struct H5S_t {
    H5S_extent_t extent;                /* Dataspace extent (must stay first) */
    H5S_select_t select;		/* Dataspace selection */
};

/* Selection iteration methods */
/* Method to retrieve the current coordinates of iterator for current selection */
typedef herr_t (*H5S_sel_iter_coords_func_t)(const H5S_sel_iter_t *iter, hsize_t *coords);
/* Method to retrieve the current block of iterator for current selection */
typedef herr_t (*H5S_sel_iter_block_func_t)(const H5S_sel_iter_t *iter, hsize_t *start, hsize_t *end);
/* Method to determine number of elements left in iterator for current selection */
typedef hsize_t (*H5S_sel_iter_nelmts_func_t)(const H5S_sel_iter_t *iter);
/* Method to determine if there are more blocks left in the current selection */
typedef htri_t (*H5S_sel_iter_has_next_block_func_t)(const H5S_sel_iter_t *iter);
/* Method to move selection iterator to the next element in the selection */
typedef herr_t (*H5S_sel_iter_next_func_t)(H5S_sel_iter_t *iter, hsize_t nelem);
/* Method to move selection iterator to the next block in the selection */
typedef herr_t (*H5S_sel_iter_next_block_func_t)(H5S_sel_iter_t *iter);
/* Method to release iterator for current selection */
typedef herr_t (*H5S_sel_iter_release_func_t)(H5S_sel_iter_t *iter);

/* Selection iteration class */
typedef struct H5S_sel_iter_class_t {
    H5S_sel_type type;                          /* Type of selection (all, none, points or hyperslab) */

    /* Methods on selections */
    H5S_sel_iter_coords_func_t iter_coords;     /* Method to retrieve the current coordinates of iterator for current selection */
    H5S_sel_iter_block_func_t iter_block;       /* Method to retrieve the current block of iterator for current selection */
    H5S_sel_iter_nelmts_func_t iter_nelmts;     /* Method to determine number of elements left in iterator for current selection */
    H5S_sel_iter_has_next_block_func_t iter_has_next_block;         /* Method to query if there is another block left in the selection */
    H5S_sel_iter_next_func_t iter_next;         /* Method to move selection iterator to the next element in the selection */
    H5S_sel_iter_next_block_func_t iter_next_block;     /* Method to move selection iterator to the next block in the selection */
    H5S_sel_iter_release_func_t iter_release;   /* Method to release iterator for current selection */
} H5S_sel_iter_class_t;

/*
 * All selection class methods.
 */
H5_DLLVAR const H5S_select_class_t H5S_sel_all[1];

/*
 * Hyperslab selection class methods.
 */
H5_DLLVAR const H5S_select_class_t H5S_sel_hyper[1];

/*
 * None selection class methods.
 */
H5_DLLVAR const H5S_select_class_t H5S_sel_none[1];

/*
 * Pointer selection class methods.
 */
H5_DLLVAR const H5S_select_class_t H5S_sel_point[1];

/* Array of versions for Dataspace and hyperslab selections */
H5_DLLVAR const unsigned H5O_sdspace_ver_bounds[H5F_LIBVER_NBOUNDS];
H5_DLLVAR const unsigned H5O_sds_hyper_ver_bounds[H5F_LIBVER_NBOUNDS];

/* Extent functions */
H5_DLL herr_t H5S_extent_release(H5S_extent_t *extent);
H5_DLL herr_t H5S_extent_copy_real(H5S_extent_t *dst, const H5S_extent_t *src,
    hbool_t copy_max);

/* Operations on selections */
H5_DLL herr_t H5S__hyper_project_intersection(const H5S_t *src_space,
    const H5S_t *dst_space, const H5S_t *src_intersect_space,
    H5S_t *proj_space);
H5_DLL herr_t H5S__hyper_subtract(H5S_t *space, H5S_t *subtract_space);

/* Testing functions */
#ifdef H5S_TESTING
H5_DLL htri_t H5S_select_shape_same_test(hid_t sid1, hid_t sid2);
H5_DLL htri_t H5S_get_rebuild_status_test(hid_t space_id);
#endif /* H5S_TESTING */

#endif /*_H5Spkg_H*/

