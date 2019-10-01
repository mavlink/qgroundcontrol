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

#if !(defined H5Z_FRIEND || defined H5Z_MODULE)
#error "Do not include this file outside the H5Z package!"
#endif

#ifndef _H5Zpkg_H
#define _H5Zpkg_H

/* Include private header file */
#include "H5Zprivate.h"          /* Filter functions                */

/********************/
/* Internal filters */
/********************/

/* Shuffle filter */
H5_DLLVAR const H5Z_class2_t H5Z_SHUFFLE[1];

/* Fletcher32 filter */
H5_DLLVAR const H5Z_class2_t H5Z_FLETCHER32[1];

/* n-bit filter */
H5_DLLVAR H5Z_class2_t H5Z_NBIT[1];

/* Scale/offset filter */
H5_DLLVAR H5Z_class2_t H5Z_SCALEOFFSET[1];

/********************/
/* External filters */
/********************/

/* Deflate filter */
#ifdef H5_HAVE_FILTER_DEFLATE
H5_DLLVAR const H5Z_class2_t H5Z_DEFLATE[1];
#endif /* H5_HAVE_FILTER_DEFLATE */

/* szip filter */
#ifdef H5_HAVE_FILTER_SZIP
H5_DLLVAR H5Z_class2_t H5Z_SZIP[1];
#endif /* H5_HAVE_FILTER_SZIP */

/* Package internal routines */
H5_DLL herr_t H5Z__unregister(H5Z_filter_t filter_id);

#endif /* _H5Zpkg_H */

