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
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		Monday, May  2, 2005
 *
 * Purpose:	This file contains declarations which are visible only within
 *		the H5MP package.  Source files outside the H5MP package should
 *		include H5MPprivate.h instead.
 */
#if !(defined H5MP_FRIEND || defined H5MP_MODULE)
#error "Do not include this file outside the H5MP package!"
#endif

#ifndef _H5MPpkg_H
#define _H5MPpkg_H

/* Get package's private header */
#include "H5MPprivate.h"	/* Memory Pools				*/

/* Other private headers needed by this file */
#include "H5FLprivate.h"	/* Free Lists                           */

/**************************/
/* Package Private Macros */
/**************************/

/* Alignment macros */
/* (Ideas from Apache APR :-) */

/* Default alignment necessary */
#define H5MP_BLOCK_ALIGNMENT    8

/* General alignment macro */
/* (this only works for aligning to power of 2 boundary) */
#define H5MP_ALIGN(x, a) \
       (((x) + ((size_t)(a)) - 1) & ~(((size_t)(a)) - 1))

/* Default alignment */
#define H5MP_BLOCK_ALIGN(x) H5MP_ALIGN(x, H5MP_BLOCK_ALIGNMENT)


/****************************/
/* Package Private Typedefs */
/****************************/

/* Free block in pool */
typedef struct H5MP_page_blk_t {
    size_t size;                        /* Size of block (includes this H5MP_page_blk_t info) */
    unsigned is_free:1;                 /* Flag to indicate the block is free */
    struct H5MP_page_t *page;           /* Pointer to page block is located in */
    struct H5MP_page_blk_t *prev;       /* Pointer to previous block in page */
    struct H5MP_page_blk_t *next;       /* Pointer to next block in page */
} H5MP_page_blk_t;

/* Memory pool page */
typedef struct H5MP_page_t {
    size_t free_size;                   /* Total amount of free space in page */
    unsigned fac_alloc:1;               /* Flag to indicate the page was allocated by the pool's factory */
    H5MP_page_blk_t *free_blk;          /* Pointer to first free block in page */
    struct H5MP_page_t *next;           /* Pointer to next page in pool */
    struct H5MP_page_t *prev;           /* Pointer to previous page in pool */
} H5MP_page_t;

/* Memory pool header */
struct H5MP_pool_t {
    H5FL_fac_head_t *page_fac;  /* Free-list factory for pages */
    size_t page_size;           /* Page size for pool */
    size_t free_size;           /* Total amount of free space in pool */
    size_t max_size;            /* Maximum block that will fit in a standard page */
    H5MP_page_t *first;         /* Pointer to first page in pool */
    unsigned flags;             /* Bit flags for pool settings */
};


/*****************************************/
/* Package Private Variable Declarations */
/*****************************************/


/******************************/
/* Package Private Prototypes */
/******************************/
#ifdef H5MP_TESTING
H5_DLL herr_t H5MP_get_pool_free_size (const H5MP_pool_t *mp, size_t *free_size);
H5_DLL htri_t H5MP_pool_is_free_size_correct(const H5MP_pool_t *mp);
H5_DLL herr_t H5MP_get_pool_first_page(const H5MP_pool_t *mp, H5MP_page_t **page);
H5_DLL herr_t H5MP_get_page_free_size(const H5MP_page_t *mp, size_t *page);
H5_DLL herr_t H5MP_get_page_next_page(const H5MP_page_t *page, H5MP_page_t **next_page);
#endif /* H5MP_TESTING */

#endif /* _H5MPpkg_H */

