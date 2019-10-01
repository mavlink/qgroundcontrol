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
 * This file contains public declarations for the H5S module.
 */
#ifndef _H5Spublic_H
#define _H5Spublic_H

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"

/* Define atomic datatypes */
#define H5S_ALL         (hid_t)0
#define H5S_UNLIMITED   HSIZE_UNDEF

/* Define user-level maximum number of dimensions */
#define H5S_MAX_RANK    32

/* Different types of dataspaces */
typedef enum H5S_class_t {
    H5S_NO_CLASS         = -1,  /*error                                      */
    H5S_SCALAR           = 0,   /*scalar variable                            */
    H5S_SIMPLE           = 1,   /*simple dataspace                           */
    H5S_NULL             = 2    /*null dataspace                             */
} H5S_class_t;

/* Different ways of combining selections */
typedef enum H5S_seloper_t {
    H5S_SELECT_NOOP      = -1,  /* error                                     */
    H5S_SELECT_SET       = 0,   /* Select "set" operation 		     */
    H5S_SELECT_OR,              /* Binary "or" operation for hyperslabs
                                 * (add new selection to existing selection)
                                 * Original region:  AAAAAAAAAA
                                 * New region:             BBBBBBBBBB
                                 * A or B:           CCCCCCCCCCCCCCCC
                                 */
    H5S_SELECT_AND,             /* Binary "and" operation for hyperslabs
                                 * (only leave overlapped regions in selection)
                                 * Original region:  AAAAAAAAAA
                                 * New region:             BBBBBBBBBB
                                 * A and B:                CCCC
                                 */
    H5S_SELECT_XOR,             /* Binary "xor" operation for hyperslabs
                                 * (only leave non-overlapped regions in selection)
                                 * Original region:  AAAAAAAAAA
                                 * New region:             BBBBBBBBBB
                                 * A xor B:          CCCCCC    CCCCCC
                                 */
    H5S_SELECT_NOTB,            /* Binary "not" operation for hyperslabs
                                 * (only leave non-overlapped regions in original selection)
                                 * Original region:  AAAAAAAAAA
                                 * New region:             BBBBBBBBBB
                                 * A not B:          CCCCCC
                                 */
    H5S_SELECT_NOTA,            /* Binary "not" operation for hyperslabs
                                 * (only leave non-overlapped regions in new selection)
                                 * Original region:  AAAAAAAAAA
                                 * New region:             BBBBBBBBBB
                                 * B not A:                    CCCCCC
                                 */
    H5S_SELECT_APPEND,          /* Append elements to end of point selection */
    H5S_SELECT_PREPEND,         /* Prepend elements to beginning of point selection */
    H5S_SELECT_INVALID          /* Invalid upper bound on selection operations */
} H5S_seloper_t;

/* Enumerated type for the type of selection */
typedef enum {
    H5S_SEL_ERROR	= -1, 	/* Error			*/
    H5S_SEL_NONE	= 0,    /* Nothing selected 		*/
    H5S_SEL_POINTS	= 1,    /* Sequence of points selected	*/
    H5S_SEL_HYPERSLABS  = 2,    /* "New-style" hyperslab selection defined	*/
    H5S_SEL_ALL		= 3,    /* Entire extent selected	*/
    H5S_SEL_N			/*THIS MUST BE LAST		*/
}H5S_sel_type;

#ifdef __cplusplus
extern "C" {
#endif

/* Functions in H5S.c */
H5_DLL hid_t H5Screate(H5S_class_t type);
H5_DLL hid_t H5Screate_simple(int rank, const hsize_t dims[],
			       const hsize_t maxdims[]);
H5_DLL herr_t H5Sset_extent_simple(hid_t space_id, int rank,
				    const hsize_t dims[],
				    const hsize_t max[]);
H5_DLL hid_t H5Scopy(hid_t space_id);
H5_DLL herr_t H5Sclose(hid_t space_id);
H5_DLL herr_t H5Sencode(hid_t obj_id, void *buf, size_t *nalloc);
H5_DLL hid_t H5Sdecode(const void *buf);
H5_DLL hssize_t H5Sget_simple_extent_npoints(hid_t space_id);
H5_DLL int H5Sget_simple_extent_ndims(hid_t space_id);
H5_DLL int H5Sget_simple_extent_dims(hid_t space_id, hsize_t dims[],
				      hsize_t maxdims[]);
H5_DLL htri_t H5Sis_simple(hid_t space_id);
H5_DLL hssize_t H5Sget_select_npoints(hid_t spaceid);
H5_DLL herr_t H5Sselect_hyperslab(hid_t space_id, H5S_seloper_t op,
				   const hsize_t start[],
				   const hsize_t _stride[],
				   const hsize_t count[],
				   const hsize_t _block[]);
/* #define NEW_HYPERSLAB_API */
/* Note that these haven't been working for a while and were never
 *      publicly released - QAK */
#ifdef NEW_HYPERSLAB_API
H5_DLL hid_t H5Scombine_hyperslab(hid_t space_id, H5S_seloper_t op,
				   const hsize_t start[],
				   const hsize_t _stride[],
				   const hsize_t count[],
				   const hsize_t _block[]);
H5_DLL herr_t H5Sselect_select(hid_t space1_id, H5S_seloper_t op,
                                  hid_t space2_id);
H5_DLL hid_t H5Scombine_select(hid_t space1_id, H5S_seloper_t op,
                                  hid_t space2_id);
#endif /* NEW_HYPERSLAB_API */
H5_DLL herr_t H5Sselect_elements(hid_t space_id, H5S_seloper_t op,
    size_t num_elem, const hsize_t *coord);
H5_DLL H5S_class_t H5Sget_simple_extent_type(hid_t space_id);
H5_DLL herr_t H5Sset_extent_none(hid_t space_id);
H5_DLL herr_t H5Sextent_copy(hid_t dst_id,hid_t src_id);
H5_DLL htri_t H5Sextent_equal(hid_t sid1, hid_t sid2);
H5_DLL herr_t H5Sselect_all(hid_t spaceid);
H5_DLL herr_t H5Sselect_none(hid_t spaceid);
H5_DLL herr_t H5Soffset_simple(hid_t space_id, const hssize_t *offset);
H5_DLL htri_t H5Sselect_valid(hid_t spaceid);
H5_DLL htri_t H5Sis_regular_hyperslab(hid_t spaceid);
H5_DLL htri_t H5Sget_regular_hyperslab(hid_t spaceid, hsize_t start[],
    hsize_t stride[], hsize_t count[], hsize_t block[]);
H5_DLL hssize_t H5Sget_select_hyper_nblocks(hid_t spaceid);
H5_DLL hssize_t H5Sget_select_elem_npoints(hid_t spaceid);
H5_DLL herr_t H5Sget_select_hyper_blocklist(hid_t spaceid, hsize_t startblock,
    hsize_t numblocks, hsize_t buf[/*numblocks*/]);
H5_DLL herr_t H5Sget_select_elem_pointlist(hid_t spaceid, hsize_t startpoint,
    hsize_t numpoints, hsize_t buf[/*numpoints*/]);
H5_DLL herr_t H5Sget_select_bounds(hid_t spaceid, hsize_t start[],
    hsize_t end[]);
H5_DLL H5S_sel_type H5Sget_select_type(hid_t spaceid);

#ifdef __cplusplus
}
#endif
#endif /* _H5Spublic_H */

