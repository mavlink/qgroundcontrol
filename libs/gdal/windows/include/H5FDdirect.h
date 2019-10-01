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
 * Programmer:  Raymond Lu <slu@hdfgroup.uiuc.edu>
 *              Wednesday, 20 September 2006
 *
 * Purpose:	The public header file for the direct driver.
 */
#ifndef H5FDdirect_H
#define H5FDdirect_H

#ifdef H5_HAVE_DIRECT
#       define H5FD_DIRECT	(H5FD_direct_init())
#else
#       define H5FD_DIRECT      (-1)
#endif /* H5_HAVE_DIRECT */

#ifdef H5_HAVE_DIRECT
#ifdef __cplusplus
extern "C" {
#endif

/* Default values for memory boundary, file block size, and maximal copy buffer size.
 * Application can set these values through the function H5Pset_fapl_direct. */
#define MBOUNDARY_DEF		4096
#define FBSIZE_DEF		4096
#define CBSIZE_DEF		16*1024*1024

H5_DLL hid_t H5FD_direct_init(void);
H5_DLL herr_t H5Pset_fapl_direct(hid_t fapl_id, size_t alignment, size_t block_size,
			size_t cbuf_size);
H5_DLL herr_t H5Pget_fapl_direct(hid_t fapl_id, size_t *boundary/*out*/,
			size_t *block_size/*out*/, size_t *cbuf_size/*out*/);

#ifdef __cplusplus
}
#endif

#endif /* H5_HAVE_DIRECT */

#endif

