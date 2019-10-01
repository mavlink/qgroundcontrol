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

#ifndef H5TOOLS_REF_H__
#define H5TOOLS_REF_H__

#include "hdf5.h"

#ifdef __cplusplus
extern "C" {
#endif

H5TOOLS_DLL herr_t      fill_ref_path_table(hid_t fid);
H5TOOLS_DLL const char *lookup_ref_path(haddr_t ref);
H5TOOLS_DLL int         get_next_xid(void);
H5TOOLS_DLL haddr_t     get_fake_xid(void);
H5TOOLS_DLL haddr_t     ref_path_table_lookup(const char *);
H5TOOLS_DLL haddr_t     ref_path_table_gen_fake(const char *);
H5TOOLS_DLL int         term_ref_path_table(void);

#ifdef __cplusplus
}
#endif

#endif

