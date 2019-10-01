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
 * Purpose:	The public header file for the mpio driver.
 */
#ifndef H5FDmpio_H
#define H5FDmpio_H

/* Macros */

#ifdef H5_HAVE_PARALLEL
#   define H5FD_MPIO	(H5FD_mpio_init())
#else
#   define H5FD_MPIO	(-1)
#endif /* H5_HAVE_PARALLEL */

#ifdef H5_HAVE_PARALLEL
/*Turn on H5FDmpio_debug if H5F_DEBUG is on */
#ifdef H5F_DEBUG
#ifndef H5FDmpio_DEBUG
#define H5FDmpio_DEBUG
#endif
#endif

/* Global var whose value comes from environment variable */
/* (Defined in H5FDmpio.c) */
H5_DLLVAR hbool_t H5FD_mpi_opt_types_g;

/* Function prototypes */
#ifdef __cplusplus
extern "C" {
#endif
H5_DLL hid_t H5FD_mpio_init(void);
H5_DLL herr_t H5Pset_fapl_mpio(hid_t fapl_id, MPI_Comm comm, MPI_Info info);
H5_DLL herr_t H5Pget_fapl_mpio(hid_t fapl_id, MPI_Comm *comm/*out*/,
			MPI_Info *info/*out*/);
H5_DLL herr_t H5Pset_dxpl_mpio(hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode);
H5_DLL herr_t H5Pget_dxpl_mpio(hid_t dxpl_id, H5FD_mpio_xfer_t *xfer_mode/*out*/);
H5_DLL herr_t H5Pset_dxpl_mpio_collective_opt(hid_t dxpl_id, H5FD_mpio_collective_opt_t opt_mode);
H5_DLL herr_t H5Pset_dxpl_mpio_chunk_opt(hid_t dxpl_id, H5FD_mpio_chunk_opt_t opt_mode);
H5_DLL herr_t H5Pset_dxpl_mpio_chunk_opt_num(hid_t dxpl_id, unsigned num_chunk_per_proc);
H5_DLL herr_t H5Pset_dxpl_mpio_chunk_opt_ratio(hid_t dxpl_id, unsigned percent_num_proc_per_chunk);
#ifdef __cplusplus
}
#endif

#endif /* H5_HAVE_PARALLEL */

#endif

