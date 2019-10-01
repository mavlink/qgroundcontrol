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
 * Programmer:      Quincey Koziol <koziol@hdfgroup.org>
 *                  Tuesday, June 17, 2008
 *
 * Purpose:         This file contains declarations which are visible only
 *                  within the H5EA package.  Source files outside the H5EA
 *                  package should include H5EAprivate.h instead.
 */
#if !(defined(H5EA_FRIEND) | defined(H5EA_MODULE))
#error "Do not include this file outside the H5EA package!"
#endif

#ifndef _H5EApkg_H
#define _H5EApkg_H

/* Get package's private header */
#include "H5EAprivate.h"

/* Other private headers needed by this file */
#include "H5FLprivate.h"    /* Free Lists                           */


/**************************/
/* Package Private Macros */
/**************************/

/* Fill value for extensible array test class */
#ifdef H5EA_TESTING
#define H5EA_TEST_FILL          ((uint64_t)ULLONG_MAX)
#endif /* H5EA_TESTING */

/* Size of checksum information (on disk) */
#define H5EA_SIZEOF_CHKSUM      4

/* "Standard" size of prefix information for extensible array metadata */
#define H5EA_METADATA_PREFIX_SIZE(c) (                                        \
    H5_SIZEOF_MAGIC   /* Signature */                                         \
    + 1 /* Version */                                                         \
    + 1 /* Array type */                                                      \
    + ((c) ? H5EA_SIZEOF_CHKSUM : 0) /* Metadata checksum */                  \
    )

/* Size of the extensible array header on disk */
#define H5EA_HEADER_SIZE(sizeof_addr, sizeof_size) (                          \
    /* General metadata fields */                                             \
    H5EA_METADATA_PREFIX_SIZE(TRUE)                                           \
                                                                              \
    /* General array information */                                           \
    + 1 /* Element Size */                                                    \
    + 1 /* Max. # of elements bits */                                         \
    + 1 /* # of elements to store in index block */                           \
    + 1 /* Min. # elements per data block */                                  \
    + 1 /* Min. # of data block pointers for a super block */                 \
    + 1 /* Log2(Max. # of elements in data block page) - i.e. # of bits needed to store max. # of elements in data block page */ \
                                                                              \
    /* Extensible Array statistics fields */                                  \
    + (sizeof_size) /* Number of super blocks created */		      \
    + (sizeof_size) /* Size of super blocks created */	        	      \
    + (sizeof_size) /* Number of data blocks created */	        	      \
    + (sizeof_size) /* Size of data blocks created */   		      \
    + (sizeof_size) /* Max. index set */	       			      \
    + (sizeof_size) /* Number of elements 'realized' */	        	      \
                                                                              \
    /* Extensible Array Header specific fields */                             \
    + (sizeof_addr) /* File address of index block */   		      \
    )

/* Size of the extensible array header on disk (via file pointer) */
#define H5EA_HEADER_SIZE_FILE(f)   (                                          \
    H5EA_HEADER_SIZE(H5F_SIZEOF_ADDR(f), H5F_SIZEOF_SIZE(f))                  \
    )

/* Size of the extensible array header on disk (via extensible array header) */
#define H5EA_HEADER_SIZE_HDR(h)   (                                           \
    H5EA_HEADER_SIZE((h)->sizeof_addr, (h)->sizeof_size)                      \
    )

/* Size of the extensible array index block on disk */
#define H5EA_IBLOCK_SIZE(i)     (                                             \
    /* General metadata fields */                                             \
    H5EA_METADATA_PREFIX_SIZE(TRUE)                                           \
                                                                              \
    /* Sanity-checking fields */                                              \
    + (i)->hdr->sizeof_addr          /* File address of array owning the block */ \
                                                                              \
    /* Extensible Array Index Block specific fields */                        \
    + ((size_t)(i)->hdr->cparam.idx_blk_elmts * (size_t)(i)->hdr->cparam.raw_elmt_size) /* Elements in index block  */ \
    + ((i)->ndblk_addrs * (i)->hdr->sizeof_addr) /* Data block addresses in index block  */ \
    + ((i)->nsblk_addrs * (i)->hdr->sizeof_addr) /* Super block addresses in index block  */ \
    )

/* Size of the extensible array super block on disk */
#define H5EA_SBLOCK_SIZE(s)     (                                             \
    /* General metadata fields */                                             \
    H5EA_METADATA_PREFIX_SIZE(TRUE)                                           \
                                                                              \
    /* Sanity-checking fields */                                              \
    + (s)->hdr->sizeof_addr          /* File address of array owning the block */ \
    + (s)->hdr->arr_off_size         /* Offset of the block in the array */   \
                                                                              \
    /* Extensible Array Super Block specific fields */                        \
    + ((s)->ndblks * (s)->dblk_page_init_size) /* Data block 'page init' bitmasks in super block (can be 0 if no pages) */ \
    + ((s)->ndblks * (s)->hdr->sizeof_addr) /* Data block addresses in super block  */ \
    )

/* Size of the extensible array data block prefix on disk */
#define H5EA_DBLOCK_PREFIX_SIZE(d)  (                                         \
    /* General metadata fields */                                             \
    H5EA_METADATA_PREFIX_SIZE(TRUE)                                           \
                                                                              \
    /* Sanity-checking fields */                                              \
    + (d)->hdr->sizeof_addr          /* File address of array owning the block */ \
    + (d)->hdr->arr_off_size         /* Offset of the block in the array */   \
    )

/* Size of the extensible array data block on disk */
#define H5EA_DBLOCK_SIZE(d)     (                                             \
    /* Data block prefix size  */                                             \
    H5EA_DBLOCK_PREFIX_SIZE(d)                                                \
                                                                              \
    /* Extensible Array Data Block specific fields */                         \
    + ((d)->nelmts * (size_t)(d)->hdr->cparam.raw_elmt_size) /* Elements in data block  */  \
    + ((d)->npages * H5EA_SIZEOF_CHKSUM)        /* Checksum for each page */  \
    )

/* Size of the extensible array data block page on disk */
#define H5EA_DBLK_PAGE_SIZE(h)     (					      \
    + ((h)->dblk_page_nelmts * (size_t)(h)->cparam.raw_elmt_size) /* Elements in data block page */  \
    + H5EA_SIZEOF_CHKSUM                        /* Checksum for each page */  \
    )

/* Compute the # of bytes required to store an offset into a given buffer size */
#define H5EA_SIZEOF_OFFSET_BITS(b)      (((b) + 7) / 8)

/* Compute the first super block index that will hold a certain # of data block pointers */
#define H5EA_SBLK_FIRST_IDX(m)          (2 * H5VM_log2_of2((uint32_t)m))

/****************************/
/* Package Private Typedefs */
/****************************/

/* Information for each super block in extensible array */
typedef struct H5EA_sblk_info_t {
    size_t ndblks;              /* Number of data blocks for a super block */
    size_t dblk_nelmts;         /* Number of elements in each data block for super block */
    hsize_t start_idx;          /* Index of first element in super block */
    hsize_t start_dblk;         /* Index of first data block in super block */
} H5EA_sblk_info_t;

/* The extensible array header information */
/* (Each extensible array header has certain information that is shared across
 * all the blocks in that extensible array)
 */
typedef struct H5EA_hdr_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Extensible array configuration/creation parameters (stored in header) */
    H5EA_create_t cparam;               /* Creation parameters for extensible array */

    /* Index block information (stored in header) */
    haddr_t idx_blk_addr;               /* Address of index block in header */

    /* Statistics for array (stored in index block, actually) */
    /* (header and index number/size fields not stored) */
    H5EA_stat_t stats;                  /* Statistics for extensible array */

    /* Data block element buffer factory info (not stored in header) */
    struct {
        size_t nalloc;                  /* Number of factories allocated */
        H5FL_fac_head_t **fac;          /* Array of factories for data block element buffers */
    } elmt_fac;

    /* Computed/cached values (not stored in header) */
    size_t rc;                          /* Reference count of heap's components using heap header */
    haddr_t addr;                       /* Address of header in file */
    size_t size;                        /* Size of header in file */
    H5F_t *f;                           /* Pointer to file for extensible array */
    size_t file_rc;                     /* Reference count of files using array header */
    hbool_t pending_delete;             /* Array is pending deletion */
    size_t sizeof_addr;                 /* Size of file addresses */
    size_t sizeof_size;                 /* Size of file sizes */
    unsigned char arr_off_size;         /* Size of array offsets (in bytes) */

    /* Super block information (not stored) */
    size_t nsblks;                      /* Number of superblocks needed for array */
    H5EA_sblk_info_t *sblk_info;        /* Array of information for each super block */

    /* Data block information (not stored) */
    size_t dblk_page_nelmts;            /* # of elements per data block page */

    /* Client information (not stored) */
    void *cb_ctx;                       /* Callback context */

    /* SWMR / Flush dependency information (not stored) */
    hbool_t swmr_write;                 /* Flag indicating the file is opened with SWMR-write access */
    H5AC_proxy_entry_t *top_proxy;      /* 'Top' proxy cache entry for all array entries */
    void *parent;		        /* Pointer to 'top' proxy flush dependency
                                         * parent, if it exists, otherwise NULL.
                                         * If the extensible array is being used
                                         * to index a chunked dataset and the
                                         * dataset metadata is modified by a
                                         * SWMR writer, this field will be set
                                         * equal to the object header proxy
                                         * that is the flush dependency parent
                                         * of the extensible array header.
 					 *
 					 * The field is used to avoid duplicate
					 * setups of the flush dependency 
					 * relationship, and to allow the 
					 * extensible array header to destroy
					 * the flush dependency on receipt of 
					 * an eviction notification from the
					 * metadata cache.
					 */
} H5EA_hdr_t;

/* The extensible array index block information */
typedef struct H5EA_iblock_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Extensible array information (stored) */
    void        *elmts;         /* Buffer for elements stored in index block  */
    haddr_t     *dblk_addrs;    /* Buffer for addresses of data blocks in index block */
    haddr_t     *sblk_addrs;    /* Buffer for addresses of super blocks in index block */

    /* Internal array information (not stored) */
    H5EA_hdr_t	*hdr;	        /* Shared array header info                     */
    haddr_t     addr;           /* Address of this index block on disk          */
    size_t      size;           /* Size of index block on disk                  */

    /* SWMR / Flush dependency information (not stored) */
    H5AC_proxy_entry_t *top_proxy;      /* "Top" proxy cache entry for all array entries */

    /* Computed/cached values (not stored) */
    size_t      nsblks;         /* # of super blocks whose data block addresses are in index block */
    size_t      ndblk_addrs;    /* Number of pointers to data blocks in index block */
    size_t      nsblk_addrs;    /* Number of pointers to super blocks in index block */
} H5EA_iblock_t;

/* The extensible array super block information */
typedef struct H5EA_sblock_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Extensible array information (stored) */
    hsize_t     block_off;      /* Offset of the block within the array's address space */
    haddr_t     *dblk_addrs;    /* Addresses of data blocks in super block */
    uint8_t     *page_init;     /* Bitmap of whether a data block page is initialized */

    /* Internal array information (not stored) */
    H5EA_hdr_t  *hdr;           /* Shared array header info                     */
    haddr_t     addr;           /* Address of this index block on disk          */
    size_t      size;           /* Size of index block on disk                  */

    /* SWMR / Flush dependency information (not stored) */
    hbool_t     has_hdr_depend; /* Whether this object has a flush dependency on the header */
    H5AC_proxy_entry_t *top_proxy;      /* "Top" proxy cache entry for all array entries */
    H5EA_iblock_t *parent;      /* Parent object for super block (index block)  */

    /* Computed/cached values (not stored) */
    unsigned    idx;            /* Super block index within the extensible array */
    size_t      ndblks;         /* # of data block addresses that are in super block */
    size_t      dblk_nelmts;    /* # of elements for data blocks reachable through this super block */
    size_t      dblk_npages;    /* # of pages in each data block */
    size_t      dblk_page_init_size;    /* Size of 'page init' bitmask for each data block */
    size_t      dblk_page_size; /* Size of a data block page */
} H5EA_sblock_t;

/* The extensible array data block information */
typedef struct H5EA_dblock_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Extensible array information (stored) */
    hsize_t     block_off;      /* Offset of the block within the array's address space */
    void        *elmts;         /* Buffer for elements stored in data block  */

    /* Internal array information (not stored) */
    H5EA_hdr_t  *hdr;           /* Shared array header info                             */
    haddr_t     addr;           /* Address of this data block on disk                   */
    size_t      size;           /* Size of data block on disk                           */

    /* SWMR / Flush dependency information (not stored) */
    hbool_t     has_hdr_depend; /* Whether this object has a flush dependency on the header */
    H5AC_proxy_entry_t *top_proxy;      /* 'Top' proxy cache entry for all array entries */
    void        *parent;        /* Parent object for data block (index or super block)  */

    /* Computed/cached values (not stored) */
    size_t      nelmts;         /* Number of elements in block                */
    size_t      npages;         /* Nummber of pages in a block (zero if not paged) */
} H5EA_dblock_t;

/* The extensible array data block page information */
typedef struct H5EA_dbk_page_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    /* Extensible array information (stored) */
    void        *elmts;         /* Buffer for elements stored in data block page */

    /* Internal array information (not stored) */
    H5EA_hdr_t  *hdr;           /* Shared array header info                         */
    haddr_t     addr;           /* Address of this data block page on disk          */
    size_t      size;           /* Size of data block page on disk                  */

    /* SWMR / Flush dependency information (not stored) */
    hbool_t     has_hdr_depend; /* Whether this object has a flush dependency on the header */
    H5AC_proxy_entry_t *top_proxy;      /* "Top" proxy cache entry for all array entries */
    H5EA_sblock_t *parent;      /* Parent object for data block page (super block)  */

    /* Computed/cached values (not stored) */
    /* <none> */
} H5EA_dblk_page_t;

/* Extensible array */
struct H5EA_t {
    H5EA_hdr_t  *hdr;           /* Pointer to internal extensible array header info */
    H5F_t      *f;              /* Pointer to file for extensible array */
};

/* Metadata cache callback user data types */

/* Info needed for loading header */
typedef struct H5EA_hdr_cache_ud_t {
    H5F_t      *f;              /* Pointer to file for extensible array */
    haddr_t    addr;            /* Address of header on disk */
    void       *ctx_udata;      /* User context for class */
} H5EA_hdr_cache_ud_t;

/* Info needed for loading super block */
typedef struct H5EA_sblock_cache_ud_t {
    H5EA_hdr_t    *hdr;         /* Shared extensible array information */
    H5EA_iblock_t *parent;      /* Pointer to parent object for super block (index block) */
    unsigned sblk_idx;          /* Index of super block */
    haddr_t sblk_addr;          /* Address of super block */
} H5EA_sblock_cache_ud_t;

/* Info needed for loading data block */
typedef struct H5EA_dblock_cache_ud_t {
    H5EA_hdr_t    *hdr;         /* Shared extensible array information */
    void *parent;               /* Pointer to parent object for data block (index or super block) */
    size_t nelmts;              /* Number of elements in data block */
    haddr_t dblk_addr;          /* Address of data block */
} H5EA_dblock_cache_ud_t;

/* Info needed for loading data block page */
typedef struct H5EA_dblk_page_cache_ud_t {
    H5EA_hdr_t    *hdr;         /* Shared extensible array information */
    H5EA_sblock_t *parent;      /* Pointer to parent object for data block page (super block) */
    haddr_t dblk_page_addr;     /* Address of data block page */
} H5EA_dblk_page_cache_ud_t;

#ifdef H5EA_TESTING
typedef struct H5EA__ctx_cb_t {
    herr_t (*encode)(const void *elmt, size_t nelmts, void *udata);   /* Perform action during encode step */
    void *udata;                /* User data for encode action */
} H5EA__ctx_cb_t;
#endif /* H5EA_TESTING */

/*****************************/
/* Package Private Variables */
/*****************************/

/* Internal extensible array testing class */
H5_DLLVAR const H5EA_class_t H5EA_CLS_TEST[1];

/* Array of extensible array client ID -> client class mappings */
H5_DLLVAR const H5EA_class_t *const H5EA_client_class_g[H5EA_NUM_CLS_ID];


/******************************/
/* Package Private Prototypes */
/******************************/

/* Generic routines */
H5_DLL herr_t H5EA__create_flush_depend(H5AC_info_t *parent_entry,
    H5AC_info_t *child_entry);
H5_DLL herr_t H5EA__destroy_flush_depend(H5AC_info_t *parent_entry,
    H5AC_info_t *child_entry);

/* Header routines */
H5_DLL H5EA_hdr_t *H5EA__hdr_alloc(H5F_t *f);
H5_DLL herr_t H5EA__hdr_init(H5EA_hdr_t *hdr, void *ctx_udata);
H5_DLL haddr_t H5EA__hdr_create(H5F_t *f, const H5EA_create_t *cparam,
    void *ctx_udata);
H5_DLL void *H5EA__hdr_alloc_elmts(H5EA_hdr_t *hdr, size_t nelmts);
H5_DLL herr_t H5EA__hdr_free_elmts(H5EA_hdr_t *hdr, size_t nelmts, void *elmts);
H5_DLL herr_t H5EA__hdr_incr(H5EA_hdr_t *hdr);
H5_DLL herr_t H5EA__hdr_decr(H5EA_hdr_t *hdr);
H5_DLL herr_t H5EA__hdr_fuse_incr(H5EA_hdr_t *hdr);
H5_DLL size_t H5EA__hdr_fuse_decr(H5EA_hdr_t *hdr);
H5_DLL herr_t H5EA__hdr_modified(H5EA_hdr_t *hdr);
H5_DLL H5EA_hdr_t *H5EA__hdr_protect(H5F_t *f, haddr_t ea_addr,
    void *ctx_udata, unsigned flags);
H5_DLL herr_t H5EA__hdr_unprotect(H5EA_hdr_t *hdr, unsigned cache_flags);
H5_DLL herr_t H5EA__hdr_delete(H5EA_hdr_t *hdr);
H5_DLL herr_t H5EA__hdr_dest(H5EA_hdr_t *hdr);

/* Index block routines */
H5_DLL H5EA_iblock_t *H5EA__iblock_alloc(H5EA_hdr_t *hdr);
H5_DLL haddr_t H5EA__iblock_create(H5EA_hdr_t *hdr, hbool_t *stats_changed);
H5_DLL H5EA_iblock_t *H5EA__iblock_protect(H5EA_hdr_t *hdr, unsigned flags);
H5_DLL herr_t H5EA__iblock_unprotect(H5EA_iblock_t *iblock, unsigned cache_flags);
H5_DLL herr_t H5EA__iblock_delete(H5EA_hdr_t *hdr);
H5_DLL herr_t H5EA__iblock_dest(H5EA_iblock_t *iblock);

/* Super block routines */
H5_DLL H5EA_sblock_t *H5EA__sblock_alloc(H5EA_hdr_t *hdr, H5EA_iblock_t *parent,
    unsigned sblk_idx);
H5_DLL haddr_t H5EA__sblock_create(H5EA_hdr_t *hdr, H5EA_iblock_t *parent,
    hbool_t *stats_changed, unsigned sblk_idx);
H5_DLL H5EA_sblock_t *H5EA__sblock_protect(H5EA_hdr_t *hdr, H5EA_iblock_t *parent,
    haddr_t sblk_addr, unsigned sblk_idx, unsigned flags);
H5_DLL herr_t H5EA__sblock_unprotect(H5EA_sblock_t *sblock, unsigned cache_flags);
H5_DLL herr_t H5EA__sblock_delete(H5EA_hdr_t *hdr, H5EA_iblock_t *parent,
    haddr_t sblk_addr, unsigned sblk_idx);
H5_DLL herr_t H5EA__sblock_dest(H5EA_sblock_t *sblock);

/* Data block routines */
H5_DLL H5EA_dblock_t *H5EA__dblock_alloc(H5EA_hdr_t *hdr, void *parent,
    size_t nelmts);
H5_DLL haddr_t H5EA__dblock_create(H5EA_hdr_t *hdr, void *parent,
    hbool_t *stats_changed, hsize_t dblk_off, size_t nelmts);
H5_DLL unsigned H5EA__dblock_sblk_idx(const H5EA_hdr_t *hdr, hsize_t idx);
H5_DLL H5EA_dblock_t *H5EA__dblock_protect(H5EA_hdr_t *hdr, void *parent,
    haddr_t dblk_addr, size_t dblk_nelmts, unsigned flags);
H5_DLL herr_t H5EA__dblock_unprotect(H5EA_dblock_t *dblock, unsigned cache_flags);
H5_DLL herr_t H5EA__dblock_delete(H5EA_hdr_t *hdr, void *parent,
    haddr_t dblk_addr, size_t dblk_nelmts);
H5_DLL herr_t H5EA__dblock_dest(H5EA_dblock_t *dblock);

/* Data block page routines */
H5_DLL H5EA_dblk_page_t *H5EA__dblk_page_alloc(H5EA_hdr_t *hdr, H5EA_sblock_t *parent);
H5_DLL herr_t H5EA__dblk_page_create(H5EA_hdr_t *hdr, H5EA_sblock_t *parent,
    haddr_t addr);
H5_DLL H5EA_dblk_page_t *H5EA__dblk_page_protect(H5EA_hdr_t *hdr, 
    H5EA_sblock_t *parent, haddr_t dblk_page_addr, unsigned flags);
H5_DLL herr_t H5EA__dblk_page_unprotect(H5EA_dblk_page_t *dblk_page,
    unsigned cache_flags);
H5_DLL herr_t H5EA__dblk_page_dest(H5EA_dblk_page_t *dblk_page);

/* Debugging routines for dumping file structures */
H5_DLL herr_t H5EA__hdr_debug(H5F_t *f, haddr_t addr,
    FILE *stream, int indent, int fwidth, const H5EA_class_t *cls, haddr_t obj_addr);
H5_DLL herr_t H5EA__iblock_debug(H5F_t *f, haddr_t addr,
    FILE *stream, int indent, int fwidth, const H5EA_class_t *cls,
    haddr_t hdr_addr, haddr_t obj_addr);
H5_DLL herr_t H5EA__sblock_debug(H5F_t *f, haddr_t addr,
    FILE *stream, int indent, int fwidth, const H5EA_class_t *cls,
    haddr_t hdr_addr, unsigned sblk_idx, haddr_t obj_addr);
H5_DLL herr_t H5EA__dblock_debug(H5F_t *f, haddr_t addr,
    FILE *stream, int indent, int fwidth, const H5EA_class_t *cls,
    haddr_t hdr_addr, size_t dblk_nelmts, haddr_t obj_addr);

/* Testing routines */
#ifdef H5EA_TESTING
H5_DLL herr_t H5EA_get_cparam_test(const H5EA_t *ea, H5EA_create_t *cparam);
H5_DLL int H5EA_cmp_cparam_test(const H5EA_create_t *cparam1, const H5EA_create_t *cparam2);
#endif /* H5EA_TESTING */

#endif /* _H5EApkg_H */

