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
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, August  2, 1999
 *
 * Purpose:	The public header file for the sec2 driver.
 */
#ifndef H5FDsec2_H
#define H5FDsec2_H

#define H5FD_SEC2	(H5FD_sec2_init())

#ifdef __cplusplus
extern "C" {
#endif

H5_DLL hid_t H5FD_sec2_init(void);
H5_DLL herr_t H5Pset_fapl_sec2(hid_t fapl_id);

#ifdef __cplusplus
}
#endif

#endif

