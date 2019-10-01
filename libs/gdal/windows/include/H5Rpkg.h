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

/* Purpose:     This file contains declarations which are visible
 *              only within the H5R package. Source files outside the
 *              H5R package should include H5Rprivate.h instead.
 */
#if !(defined H5R_FRIEND || defined H5R_MODULE)
#error "Do not include this file outside the H5R package!"
#endif

#ifndef _H5Rpkg_H
#define _H5Rpkg_H

/* Get package's private header */
#include "H5Rprivate.h"

/* Other private headers needed by this file */
#include "H5Fprivate.h"         /* Files                                    */
#include "H5Gprivate.h"         /* Groups                                   */
#include "H5Oprivate.h"         /* Object headers                           */
#include "H5Sprivate.h"         /* Dataspaces                               */


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
H5_DLL herr_t H5R__create(void *ref, H5G_loc_t *loc, const char *name,
    H5R_type_t ref_type, H5S_t *space);
H5_DLL hid_t H5R__dereference(H5F_t *file, hid_t dapl_id, H5R_type_t ref_type,
    const void *_ref);
H5_DLL H5S_t *H5R__get_region(H5F_t *file, const void *_ref);
H5_DLL herr_t H5R__get_obj_type(H5F_t *file, H5R_type_t ref_type,
    const void *_ref, H5O_type_t *obj_type);
H5_DLL ssize_t H5R__get_name(H5F_t *file, hid_t id, H5R_type_t ref_type,
    const void *_ref, char *name, size_t size);

#endif /* _H5Rpkg_H */

