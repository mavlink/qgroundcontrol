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

/*-------------------------------------------------------------------------
 *
 * Created:             H5MMproto.h
 *                      Jul 10 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Public declarations for the H5MM (memory management)
 *                      package.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5MMpublic_H
#define _H5MMpublic_H

/* Public headers needed by this file */
#include "H5public.h"

/* These typedefs are currently used for VL datatype allocation/freeing */
typedef void *(*H5MM_allocate_t)(size_t size, void *alloc_info);
typedef void (*H5MM_free_t)(void *mem, void *free_info);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif /* _H5MMpublic_H */

