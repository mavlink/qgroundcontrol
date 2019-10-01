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
 * Programmer:
 *
 * Purpose:    This file contains declarations which are visible only within
 *        the H5FA package.  Source files outside the H5FA package should
 *        include H5FAprivate.h instead.
 */
#if !(defined(H5FA_FRIEND) | defined(H5FA_MODULE))
#error "Do not include this file outside the H5FA package!"
#endif

#ifndef _H5FApkg_H
#define _H5FApkg_H

/* Get package's private header */
#include "H5FAprivate.h"

/* Other private headers needed by this file */
#include "H5ACprivate.h"    /* Metadata cache                       */
#include "H5FLprivate.h"    /* Free Lists                           */


/**************************/
/* Package Private Macros */
/**************************/

/* Define this to display debugging information for the Fixed Array layer */
/* #define H5FA_DEBUG */


/* Fill value for fixed array test class */
#ifdef H5FA_TESTING
#define H5FA_TEST_FILL          ((uint64_t)ULLONG_MAX)
#endif /* H5FA_TESTING */

/* Size of checksum information (on disk) */
#define H5FA_SIZEOF_CHKSUM      4

/* "Standard" size of prefix information for fixed array metadata */
#define H5FA_METADATA_PREFIX_SIZE(c) (                                      \
    H5_SIZEOF_MAGIC                     /* Signature            */          \
    + 1                                 /* Version              */          \
    + 1                                 /* Array type           */          \
    + ((c) ? H5FA_SIZEOF_CHKSUM : 0)    /* Metadata checksum    */          \
    )

/* Size of the Fixed Array header on disk */
#define H5FA_HEADER_SIZE(sizeof_addr, sizeof_size) (                          \
    /* General metadata fields */                                             \
    H5FA_METADATA_PREFIX_SIZE(TRUE)                                           \
                                                                              \
    /* General array information */                                           \
    + 1 /* Element Size */                                                    \
    + 1 /* Log2(Max. # of elements in data block page) - i.e. # of bits needed to store max. # of elements in data block page */ \
                                                                              \
    /* Fixed Array statistics fields */                                       \
    + (sizeof_size) /* # of elements in the fixed array */    		      \
                                                                              \
    /* Fixed Array Header specific fields */                                  \
    + (sizeof_addr) /* File address of Fixed Array data block */  	      \
    )

/* Size of the fixed array header on disk (via file pointer) */
#define H5FA_HEADER_SIZE_FILE(f)   (                                          \
    H5FA_HEADER_SIZE(H5F_SIZEOF_ADDR(f), H5F_SIZEOF_SIZE(f))                  \
    )

/* Size of the fixed array header on disk (via fixed array header) */
#define H5FA_HEADER_SIZE_HDR(h)   (                                           \
    H5FA_HEADER_SIZE((h)->sizeof_addr, (h)->sizeof_size)                      \
    )

/* Size of the Fixed Array data block prefix on disk */
#define H5FA_DBLOCK_PREFIX_SIZE(d)  (                                       \
    /* General metadata fields */                                           \
    H5FA_METADATA_PREFIX_SIZE(TRUE)                                         \
                                                                            \
    /* Sanity-checking fields */                                            \
    + (d)->hdr->sizeof_addr        /* File address of Fixed Array header owning the data block */   \
                                                                            \
    /* Fixed Array Data Block specific fields */                            \
    + (d)->dblk_page_init_size /* Fixed array data block 'page init' bitmasks (can be 0 if no pages) */ \
    )

/* Size of the Fixed Array data block on disk */
#define H5FA_DBLOCK_SIZE(d)  (                                              \
    /* Data block prefix size  */                                           \
    H5FA_DBLOCK_PREFIX_SIZE(d)                                              \
                                                                            \
    /* Fixed Array Elements|Pages of Elements*/                             \
    + ((d)->hdr->cparam.nelmts * (size_t)(d)->hdr->cparam.raw_elmt_size)    \
    + ((d)->npages * H5FA_SIZEOF_CHKSUM)        /* Checksum */              \
    )

/* Size of the Fixed Array data block page on disk */
#define H5FA_DBLK_PAGE_SIZE(h, nelmts)     (                                  \
    /* Fixed Array Data Block Page */					      \
    + (nelmts * (size_t)(h)->cparam.raw_elmt_size) /* Elements in data block page */  \
    + H5FA_SIZEOF_CHKSUM                        	/* Checksum for each page */  	   \
    )

/****************************/
/* Package Private Typedefs */
/****************************/

/* The Fixed Array header information */
typedef struct H5FA_hdr_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Fixed array configuration/creation parameters (stored in header) */
    H5FA_create_t cparam;               /* Creation parameters for Fixed Array */

    /* Fixed Array data block information (stored in header) */
    haddr_t dblk_addr;                   /* Address of Fixed Array Data block */

    /* Statistics for Fixed Array (stored in header) */
    H5FA_stat_t stats;                  /* Statistcs for Fixed Array */

    /* Computed/cached values (not stored in header) */
    size_t rc;                          /* Reference count of the header                                */
    haddr_t addr;                       /* Address of header in file                                    */
    size_t size;                        /* Size of header in file                                       */
    H5F_t *f;                           /* Pointer to file for fixed array                              */
    size_t file_rc;                     /* Reference count of files using array header                  */
    hbool_t pending_delete;             /* Array is pending deletion                                    */
    size_t sizeof_addr;                 /* Size of file addresses                                       */
    size_t sizeof_size;                 /* Size of file sizes                                           */

    /* Client information (not stored) */
    void *cb_ctx;                       /* Callback context */

    /* SWMR / Flush dependency information (not stored) */
    hbool_t swmr_write;                 /* Flag indicating the file is opened with SWMR-write access    */
    H5AC_proxy_entry_t *top_proxy;      /* 'Top' proxy cache entry for all array entries */
    void *parent;		        /* Pointer to 'top' proxy flush dependency
                                         * parent, if it exists, otherwise NULL.
                                         * If the fixed array is being used
                                         * to index a chunked dataset and the
                                         * dataset metadata is modified by a
                                         * SWMR writer, this field will be set
                                         * equal to the object header proxy
                                         * that is the flush dependency parent
                                         * of the fixed array header.
 					 *
 					 * The field is used to avoid duplicate
					 * setups of the flush dependency 
					 * relationship, and to allow the 
					 * fixed array header to destroy
					 * the flush dependency on receipt of 
					 * an eviction notification from the
					 * metadata cache.
					 */
} H5FA_hdr_t;

/* The fixed array data block information */
typedef struct H5FA_dblock_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Fixed array information (stored) */
    uint8_t     *dblk_page_init;/* Bitmap of whether a data block page is initialized       */
    void        *elmts;         /* Buffer for elements stored in data block                 */

    /* Internal array information (not stored) */
    H5FA_hdr_t    *hdr;            /* Shared array header info                              */

    /* SWMR / Flush dependency information (not stored) */
    H5AC_proxy_entry_t *top_proxy;      /* 'Top' proxy cache entry for all array entries */

    /* Computed/cached values (not stored) */
    haddr_t     addr;               /* Address of this data block on disk                   */
    hsize_t     size;               /* Size of data block on disk                           */
    size_t      npages;             /* Nummber of pages in data block (zero if not paged)   */
    size_t      last_page_nelmts;   /* Nummber of elements in last page, if paged           */

    /* Fixed Array data block information (not stored) */
    size_t dblk_page_nelmts;        /* # of elements per data block page                    */
    size_t dblk_page_size;          /* Size of a data block page                            */
    size_t dblk_page_init_size;     /* Size of 'page init' bitmask                          */
} H5FA_dblock_t;

/* The fixed array data block page information */
typedef struct H5FA_dbk_page_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Fixed array information (stored) */
    void        *elmts;         /* Buffer for elements stored in data block page */

    /* Internal array information (not stored) */
    H5FA_hdr_t    *hdr;         /* Shared array header info                     */

    /* SWMR / Flush dependency information (not stored) */
    H5AC_proxy_entry_t *top_proxy;      /* 'Top' proxy cache entry for all array entries */

    /* Computed/cached values (not stored) */
    haddr_t     addr;           /* Address of this data block page on disk      */
    size_t      size;           /* Size of data block page on disk              */
    size_t      nelmts;         /* Number of elements in data block page        */
} H5FA_dblk_page_t;

/* Fixed array */
struct H5FA_t {
    H5FA_hdr_t  *hdr;           /* Pointer to internal fixed array header info  */
    H5F_t      *f;              /* Pointer to file for fixed array              */
};


/* Metadata cache callback user data types */

/* Info needed for loading header */
typedef struct H5FA_hdr_cache_ud_t {
    H5F_t      *f;              /* Pointer to file for fixed array */
    haddr_t    addr;            /* Address of header on disk */
    void       *ctx_udata;      /* User context for class */
} H5FA_hdr_cache_ud_t;

/* Info needed for loading data block */
typedef struct H5FA_dblock_cache_ud_t {
    H5FA_hdr_t *hdr;            /* Shared fixed array information       */
    haddr_t     dblk_addr;      /* Address of data block on disk        */
} H5FA_dblock_cache_ud_t;

/* Info needed for loading data block page */
typedef struct H5FA_dblk_page_cache_ud_t {
    H5FA_hdr_t *hdr;            /* Shared fixed array information           */
    size_t nelmts;              /* Number of elements in data block page    */
    haddr_t     dblk_page_addr; /* Address of data block page on disk */
} H5FA_dblk_page_cache_ud_t;


/*****************************/
/* Package Private Variables */
/*****************************/

/* Internal fixed array testing class */
H5_DLLVAR const H5FA_class_t H5FA_CLS_TEST[1];

/* Array of fixed array client ID -> client class mappings */
H5_DLLVAR const H5FA_class_t *const H5FA_client_class_g[H5FA_NUM_CLS_ID];


/******************************/
/* Package Private Prototypes */
/******************************/

/* Generic routines */
H5_DLL herr_t H5FA__create_flush_depend(H5AC_info_t *parent_entry,
    H5AC_info_t *child_entry);
H5_DLL herr_t H5FA__destroy_flush_depend(H5AC_info_t *parent_entry,
    H5AC_info_t *child_entry);

/* Header routines */
H5_DLL H5FA_hdr_t *H5FA__hdr_alloc(H5F_t *f);
H5_DLL herr_t H5FA__hdr_init(H5FA_hdr_t *hdr, void *ctx_udata);
H5_DLL haddr_t H5FA__hdr_create(H5F_t *f, const H5FA_create_t *cparam, void *ctx_udata);
H5_DLL void *H5FA__hdr_alloc_elmts(H5FA_hdr_t *hdr, size_t nelmts);
H5_DLL herr_t H5FA__hdr_free_elmts(H5FA_hdr_t *hdr, size_t nelmts, void *elmts);
H5_DLL herr_t H5FA__hdr_incr(H5FA_hdr_t *hdr);
H5_DLL herr_t H5FA__hdr_decr(H5FA_hdr_t *hdr);
H5_DLL herr_t H5FA__hdr_fuse_incr(H5FA_hdr_t *hdr);
H5_DLL size_t H5FA__hdr_fuse_decr(H5FA_hdr_t *hdr);
H5_DLL herr_t H5FA__hdr_modified(H5FA_hdr_t *hdr);
H5_DLL H5FA_hdr_t *H5FA__hdr_protect(H5F_t *f, haddr_t fa_addr, void *ctx_udata,
    unsigned flags);
H5_DLL herr_t H5FA__hdr_unprotect(H5FA_hdr_t *hdr, unsigned cache_flags);
H5_DLL herr_t H5FA__hdr_delete(H5FA_hdr_t *hdr);
H5_DLL herr_t H5FA__hdr_dest(H5FA_hdr_t *hdr);

/* Data block routines */
H5_DLL H5FA_dblock_t *H5FA__dblock_alloc(H5FA_hdr_t *hdr);
H5_DLL haddr_t H5FA__dblock_create(H5FA_hdr_t *hdr, hbool_t *hdr_dirty);
H5_DLL unsigned H5FA__dblock_sblk_idx(const H5FA_hdr_t *hdr, hsize_t idx);
H5_DLL H5FA_dblock_t *H5FA__dblock_protect(H5FA_hdr_t *hdr, haddr_t dblk_addr,
    unsigned flags);
H5_DLL herr_t H5FA__dblock_unprotect(H5FA_dblock_t *dblock, unsigned cache_flags);
H5_DLL herr_t H5FA__dblock_delete(H5FA_hdr_t *hdr, haddr_t dblk_addr);
H5_DLL herr_t H5FA__dblock_dest(H5FA_dblock_t *dblock);

/* Data block page routines */
H5_DLL herr_t H5FA__dblk_page_create(H5FA_hdr_t *hdr, haddr_t addr, size_t nelmts);
H5_DLL H5FA_dblk_page_t *H5FA__dblk_page_alloc(H5FA_hdr_t *hdr, size_t nelmts);
H5_DLL H5FA_dblk_page_t *H5FA__dblk_page_protect(H5FA_hdr_t *hdr, haddr_t dblk_page_addr,
    size_t dblk_page_nelmts, unsigned flags);
H5_DLL herr_t H5FA__dblk_page_unprotect(H5FA_dblk_page_t *dblk_page, unsigned cache_flags);
H5_DLL herr_t H5FA__dblk_page_dest(H5FA_dblk_page_t *dblk_page);

/* Debugging routines for dumping file structures */
H5_DLL herr_t H5FA__hdr_debug(H5F_t *f, haddr_t addr, FILE *stream, int indent,
    int fwidth, const H5FA_class_t *cls, haddr_t obj_addr);
H5_DLL herr_t H5FA__dblock_debug(H5F_t *f, haddr_t addr, FILE *stream,
    int indent, int fwidth, const H5FA_class_t *cls, haddr_t hdr_addr, haddr_t obj_addr);

/* Testing routines */
#ifdef H5FA_TESTING
H5_DLL herr_t H5FA_get_cparam_test(const H5FA_t *ea, H5FA_create_t *cparam);
H5_DLL int H5FA_cmp_cparam_test(const H5FA_create_t *cparam1, const H5FA_create_t *cparam2);
#endif /* H5FA_TESTING */

#endif /* _H5FApkg_H */

