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
 * Programmer:	Quincey Koziol <koziol@hdfgroup.org>
 *		Thursday, January  3, 2008
 *
 * Purpose:	This file contains declarations which are visible only within
 *		the H5FD package.  Source files outside the H5FD package should
 *		include H5FDprivate.h instead.
 */
#if !(defined H5FD_FRIEND || defined H5FD_MODULE)
#error "Do not include this file outside the H5FD package!"
#endif

#ifndef _H5FDpkg_H
#define _H5FDpkg_H

/* Get package's private header */
#include "H5FDprivate.h"	/* File drivers				*/

/* Other private headers needed by this file */
#include "H5FLprivate.h"	/* Free lists                           */

/**************************/
/* Package Private Macros */
/**************************/


/****************************/
/* Package Private Typedefs */
/****************************/


/*****************************/
/* Package Private Variables */
/*****************************/


/******************************/
/* Package Private Prototypes */
/******************************/
H5_DLL haddr_t H5FD__alloc_real(H5FD_t *file, H5FD_mem_t type, hsize_t size,
    haddr_t *align_addr, hsize_t *align_size);
H5_DLL herr_t H5FD_free_real(H5FD_t *file, H5FD_mem_t type, haddr_t addr,
    hsize_t size);

/* Testing functions */
#ifdef H5FD_TESTING
H5_DLL hbool_t H5FD_supports_swmr_test(const char *vfd_name);
#endif /* H5FD_TESTING */

#endif /* _H5FDpkg_H */

