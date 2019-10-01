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

#if !(defined H5PB_FRIEND || defined H5PB_MODULE)
#error "Do not include this file outside the H5PB package!"
#endif

#ifndef _H5PBpkg_H
#define _H5PBpkg_H

/* Get package's private header */
#include "H5PBprivate.h"

/* Other private headers needed by this file */

/**************************/
/* Package Private Macros */
/**************************/


/****************************/
/* Package Private Typedefs */
/****************************/

typedef struct H5PB_entry_t {
    void            *page_buf_ptr;      /* Pointer to the buffer containing the data */
    haddr_t	    addr;               /* Address of the page in the file */
    H5F_mem_page_t  type;               /* Type of the page entry (H5F_MEM_PAGE_RAW/META) */
    hbool_t         is_dirty;           /* Flag indicating whether the page has dirty data or not */

    /* Fields supporting replacement policies */
    struct H5PB_entry_t     *next;      /* next pointer in the LRU list */
    struct H5PB_entry_t     *prev;      /* previous pointer in the LRU list */
} H5PB_entry_t;


/*****************************/
/* Package Private Variables */
/*****************************/


/******************************/
/* Package Private Prototypes */
/******************************/


#endif /* _H5PBpkg_H */

