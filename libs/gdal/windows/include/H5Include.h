// C++ informative line for the emacs editor: -*- C++ -*-
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

#include <hdf5.h>

// Define bool type for platforms that don't support bool yet
#ifdef BOOL_NOTDEFINED
#ifdef false
#undef false
#endif
#ifdef true
#undef true
#endif
typedef int bool;
const bool  false = 0;
const bool  true  = 1;
#endif

// These are defined in H5Opkg.h, which should not be included in the C++ API,
// so re-define them here for now.

/* Initial version of the object header format */
#define H5O_VERSION_1    1

/* Revised version - leaves out reserved bytes and alignment padding, and adds
 *      magic number as prefix and checksum as suffix for all chunks.
 */
#define H5O_VERSION_2    2

