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

#ifndef _H5DOpublic_H
#define _H5DOpublic_H

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------
 *
 * "Optimized dataset" routines.
 *
 *-------------------------------------------------------------------------
 */

H5_HLDLL herr_t H5DOappend(hid_t dset_id, hid_t dxpl_id, unsigned axis,
    size_t extension, hid_t memtype, const void *buf);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/* Compatibility wrappers for functionality moved to H5D */
H5_HLDLL herr_t H5DOwrite_chunk(hid_t dset_id, hid_t dxpl_id, uint32_t filters,
    const hsize_t *offset, size_t data_size, const void *buf);
H5_HLDLL herr_t H5DOread_chunk(hid_t dset_id, hid_t dxpl_id, const hsize_t *offset,
    uint32_t *filters /*out*/, void *buf /*out*/);

#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif

#endif

