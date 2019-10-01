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
 * Programmer:  Scott Wegner <swegner@hdfgroup.org>
 *				Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 * Purpose:	The public header file for the windows driver.
 */
#ifndef H5FDwindows_H
#define H5FDwindows_H

#define H5FD_WINDOWS	(H5FD_sec2_init())

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

H5_DLL herr_t H5Pset_fapl_windows(hid_t fapl_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* H5FDwindows_H */

