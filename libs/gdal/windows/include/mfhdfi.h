/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF.  The full HDF copyright notice, including       *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF/releases/.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id: mfhdfi.h 5009 2007-12-27 16:49:20Z bmribler $ */

#ifndef _MFHDFI_H
#define _MFHDFI_H

/* enumerated type used to specify whether a variable is an SDS, coordinate
   variable, or its type is unknown because it was created before HDF4.2r2 */
typedef enum
{
    IS_SDSVAR=0,        /* variable is an actual SDS */
    IS_CRDVAR=1,        /* variable is a coordinate variable */
    UNKNOWN=2           /* variable is created before HDF4.2r2, unknown type */
} hdf_vartype_t;

#endif /* _MFHDFI_H */
