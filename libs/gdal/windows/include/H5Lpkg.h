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
 * Programmer: James Laird <matzke@llnl.gov>
 *             Friday, December 1, 2005
 *
 * Purpose:     This file contains declarations which are visible
 *              only within the H5L package. Source files outside the
 *              H5L package should include H5Lprivate.h instead.
 */
#if !(defined H5L_FRIEND || defined H5L_MODULE)
#error "Do not include this file outside the H5L package!"
#endif

#ifndef _H5Lpkg_H
#define _H5Lpkg_H

/* Get package's private header */
#include "H5Lprivate.h"

/* Other private headers needed by this file */


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

H5_DLL herr_t H5L__create_ud(const H5G_loc_t *link_loc, const char *link_name,
    const void * ud_data, size_t ud_data_size, H5L_type_t type,
    hid_t lcpl_id);
H5_DLL herr_t H5L__link_copy_file(H5F_t *dst_file, const H5O_link_t *_src_lnk,
    const H5O_loc_t *src_oloc, H5O_link_t *dst_lnk, H5O_copy_t *cpy_info);

#endif /* _H5Lpkg_H */

