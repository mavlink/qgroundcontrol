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
 *		Thursday, September 28, 2000
 *
 * Purpose:	This file contains declarations which are visible only within
 *		the H5F package.  Source files outside the H5F package should
 *		include H5Fprivate.h instead.
 */
#if !(defined H5F_FRIEND || defined H5F_MODULE)
#error "Do not include this file outside the H5F package!"
#endif

#ifndef _H5Fpkg_H
#define _H5Fpkg_H

/* Get package's private header */
#include "H5Fprivate.h"

/* Other public headers needed by this file */
#include "H5Bpublic.h"          /* B-tree header, for H5B_NUM_BTREE_ID      */

/* Other private headers needed by this file */
#include "H5private.h"		    /* Generic Functions                        */
#include "H5ACprivate.h"        /* Metadata cache                           */
#include "H5FLprivate.h"        /* Free Lists                               */
#include "H5FOprivate.h"        /* File objects                             */
#include "H5FSprivate.h"        /* File free space                          */
#include "H5Gprivate.h"         /* Groups                                   */
#include "H5Oprivate.h"         /* Object header messages                   */
#include "H5PBprivate.h"        /* Page buffer                              */
#include "H5UCprivate.h"        /* Reference counted object functions       */


/*
 * Feature: Define this constant on the compiler command-line if you want to
 *	    see some debugging messages on the debug stream.
 */
#ifdef NDEBUG
#  undef H5F_DEBUG
#endif

/* Superblock status flags */
#define H5F_SUPER_WRITE_ACCESS          0x01
#define H5F_SUPER_FILE_OK               0x02
#define H5F_SUPER_SWMR_WRITE_ACCESS     0x04
#define H5F_SUPER_ALL_FLAGS             (H5F_SUPER_WRITE_ACCESS | H5F_SUPER_FILE_OK | H5F_SUPER_SWMR_WRITE_ACCESS)

/* Mask for removing private file access flags */
#define H5F_ACC_PUBLIC_FLAGS 	        0x007fu

/* Free space section+aggregator merge flags */
#define H5F_FS_MERGE_METADATA           0x01    /* Section can merge with metadata aggregator */
#define H5F_FS_MERGE_RAWDATA            0x02    /* Section can merge with small 'raw' data aggregator */

/* Macro to abstract checking whether file is using a free space manager */
#define H5F_HAVE_FREE_SPACE_MANAGER(F)  \
    ((F)->shared->fs_strategy == H5F_FSPACE_STRATEGY_FSM_AGGR ||                        \
     (F)->shared->fs_strategy == H5F_FSPACE_STRATEGY_PAGE)

/* Macros for encoding/decoding superblock */
#define H5F_MAX_DRVINFOBLOCK_SIZE  1024         /* Maximum size of superblock driver info buffer */
#define H5F_DRVINFOBLOCK_HDR_SIZE 16            /* Size of superblock driver info header */

/* Superblock sizes for various versions */
#define H5F_SIZEOF_CHKSUM 4     /* Checksum size in the file */

/* Fixed-size portion at the beginning of all superblocks */
#define H5F_SUPERBLOCK_FIXED_SIZE ( H5F_SIGNATURE_LEN                   \
        + 1) /* superblock version */

/* The H5F_SUPERBLOCK_MINIMAL_VARLEN_SIZE is the minimal amount of super block
 * variable length data guarnateed to load the sizeof offsets and the sizeof 
 * lengths fields in all versions of the superblock.
 *
 * This is necessary in the V3 cache, as on the initial load, we need to 
 * get enough of the superblock to determine its version and size so that
 * the metadata cache can load the correct amount of data from file to 
 * allow the second deserialization attempt to succeed.
 *
 * The value selected will have to be revisited for each new version 
 * of the super block.  Note that the current value is one byte larger
 * than it needs to be. 
 */
#define H5F_SUPERBLOCK_MINIMAL_VARLEN_SIZE	7

/* Macros for computing variable-size superblock size */
#define H5F_SUPERBLOCK_VARLEN_SIZE_COMMON                               \
        (2  /* freespace, and root group versions */			\
        + 1 /* reserved */                                              \
        + 3 /* shared header vers, size of address, size of lengths */  \
        + 1 /* reserved */                                              \
        + 4 /* group leaf k, group internal k */                        \
        + 4) /* consistency flags */
#define H5F_SUPERBLOCK_VARLEN_SIZE_V0(sizeof_addr, sizeof_size)         \
        ( H5F_SUPERBLOCK_VARLEN_SIZE_COMMON /* Common variable-length info */ \
        + (sizeof_addr) /* base address */                              \
        + (sizeof_addr) /* <unused> */                                  \
        + (sizeof_addr) /* EOF address */                               \
        + (sizeof_addr) /* driver block address */                      \
        + H5G_SIZEOF_ENTRY(sizeof_addr, sizeof_size)) /* root group ptr */
#define H5F_SUPERBLOCK_VARLEN_SIZE_V1(sizeof_addr, sizeof_size)         \
        ( H5F_SUPERBLOCK_VARLEN_SIZE_COMMON /* Common variable-length info */ \
        + 2 /* indexed B-tree internal k */                             \
        + 2 /* reserved */                                              \
        + (sizeof_addr) /* base address */                              \
        + (sizeof_addr) /* <unused> */                                  \
        + (sizeof_addr) /* EOF address */                               \
        + (sizeof_addr) /* driver block address */                      \
        + H5G_SIZEOF_ENTRY(sizeof_addr, sizeof_size)) /* root group ptr */
#define H5F_SUPERBLOCK_VARLEN_SIZE_V2(sizeof_addr)                      \
        ( 2 /* size of address, size of lengths */                      \
        + 1 /* consistency flags */                                     \
        + (sizeof_addr) /* base address */                              \
        + (sizeof_addr) /* superblock extension address */              \
        + (sizeof_addr) /* EOF address */                               \
        + (sizeof_addr) /* root group object header address */          \
        + H5F_SIZEOF_CHKSUM) /* superblock checksum (keep this last) */
#define H5F_SUPERBLOCK_VARLEN_SIZE(v, sizeof_addr, sizeof_size) (	\
        (v == 0 ? H5F_SUPERBLOCK_VARLEN_SIZE_V0(sizeof_addr, sizeof_size) : 0) \
        + (v == 1 ? H5F_SUPERBLOCK_VARLEN_SIZE_V1(sizeof_addr, sizeof_size) : 0) \
        + (v >= 2 ? H5F_SUPERBLOCK_VARLEN_SIZE_V2(sizeof_addr) : 0))

/* Total size of superblock, depends on superblock version */
#define H5F_SUPERBLOCK_SIZE(s) ( H5F_SUPERBLOCK_FIXED_SIZE              \
        + H5F_SUPERBLOCK_VARLEN_SIZE((s)->super_vers, (s)->sizeof_addr, (s)->sizeof_size))

/* For superblock version 0 & 1:
   Offset to the file consistency flags (status_flags) in the superblock (excluding H5F_SUPERBLOCK_FIXED_SIZE) */
#define H5F_SUPER_STATUS_OFF_V01                                                \
        (2  /* freespace, and root group versions */                    \
        + 1 /* reserved */                                              \
        + 3 /* shared header vers, size of address, size of lengths */  \
        + 1 /* reserved */                                              \
        + 4) /* group leaf k, group internal k */

#define H5F_SUPER_STATUS_OFF(v)   (v >= 2 ? 2 : H5F_SUPER_STATUS_OFF_V01)

/* Offset to the file consistency flags (status_flags) in the superblock */
#define H5F_SUPER_STATUS_FLAGS_OFF(v) (H5F_SUPERBLOCK_FIXED_SIZE + H5F_SUPER_STATUS_OFF(v))

/* Size of file consistency flags (status_flags) in the superblock */
#define H5F_SUPER_STATUS_FLAGS_SIZE(v)        (v >= 2 ? 1 : 4)

/* Forward declaration external file cache struct used below (defined in
 * H5Fefc.c) */
typedef struct H5F_efc_t H5F_efc_t;

/* Structure for passing 'user data' to superblock cache callbacks */
typedef struct H5F_superblock_cache_ud_t {
/* IN: */
    H5F_t *f;               /* Pointer to file */
    hbool_t ignore_drvrinfo; /* Indicate if the driver info should be ignored */
/* OUT: */
    unsigned sym_leaf_k;    /* Symbol table leaf node's 'K' value */
    unsigned btree_k[H5B_NUM_BTREE_ID]; /* B-tree key values for each type */
    haddr_t stored_eof;     /* End-of-file in file */
    hbool_t drvrinfo_removed; /* Indicate if the driver info was removed */
    unsigned    super_vers;   /* Superblock version obtained in get_load_size callback. 
			       * It will be used later in verify_chksum callback 
			       */
} H5F_superblock_cache_ud_t;

/* Structure for passing 'user data' to driver info block cache callbacks */
typedef struct H5F_drvrinfo_cache_ud_t {
    H5F_t   *f;               /* Pointer to file */
    haddr_t driver_addr;      /* address of driver info block */
} H5F_drvrinfo_cache_ud_t;

/* Structure for metadata & "small [raw] data" block aggregation fields */
struct H5F_blk_aggr_t {
    unsigned long       feature_flag;   /* Feature flag type */
    hsize_t             alloc_size;     /* Size for allocating new blocks */
    hsize_t             tot_size;       /* Total amount of bytes aggregated into block */
    hsize_t             size;           /* Current size of block left */
    haddr_t             addr;           /* Location of block left */
};

/* Structure for metadata accumulator fields */
typedef struct H5F_meta_accum_t {
    unsigned char      *buf;            /* Buffer to hold the accumulated metadata */
    haddr_t             loc;            /* File location (offset) of the accumulated metadata */
    size_t              size;           /* Size of the accumulated metadata buffer used (in bytes) */
    size_t              alloc_size;     /* Size of the accumulated metadata buffer allocated (in bytes) */
    size_t              dirty_off;      /* Offset of the dirty region in the accumulator buffer */
    size_t              dirty_len;      /* Length of the dirty region in the accumulator buffer */
    hbool_t             dirty;          /* Flag to indicate that the accumulated metadata is dirty */
} H5F_meta_accum_t;

/* A record of the mount table */
typedef struct H5F_mount_t {
    struct H5G_t	*group;	/* Mount point group held open		*/
    struct H5F_t	*file;	/* File mounted at that point		*/
} H5F_mount_t;

/*
 * The mount table describes what files are attached to (mounted on) the file
 * to which this table belongs.
 */
typedef struct H5F_mtab_t {
    unsigned		nmounts;/* Number of children which are mounted	*/
    unsigned		nalloc;	/* Number of mount slots allocated	*/
    H5F_mount_t		*child;	/* An array of mount records		*/
} H5F_mtab_t;

/* Structure specifically to store superblock. This was originally
 * maintained entirely within H5F_file_t, but is now extracted
 * here because the superblock is now handled by the cache */
typedef struct H5F_super_t {
    H5AC_info_t cache_info;     /* Cache entry information structure          */
    unsigned    super_vers;     /* Superblock version                         */
    uint8_t	sizeof_addr;	/* Size of addresses in file                  */
    uint8_t	sizeof_size;	/* Size of offsets in file                    */
    uint8_t     status_flags;   /* File status flags                          */
    unsigned    sym_leaf_k;     /* Size of leaves in symbol tables            */
    unsigned    btree_k[H5B_NUM_BTREE_ID]; /* B-tree key values for each type */
    haddr_t     base_addr;      /* Absolute base address for rel.addrs.       */
                                /* (superblock for file is at this offset)    */
    haddr_t     ext_addr;       /* Relative address of superblock extension   */
    haddr_t     driver_addr;    /* File driver information block address      */
    haddr_t     root_addr;      /* Root group address                         */
    H5G_entry_t *root_ent;      /* Root group symbol table entry              */
} H5F_super_t;

/*
 * Define the structure to store the file information for HDF5 files. One of
 * these structures is allocated per file, not per H5Fopen(). That is, set of
 * H5F_t structs can all point to the same H5F_file_t struct. The `nrefs'
 * count in this struct indicates the number of H5F_t structs which are
 * pointing to this struct.
 */
struct H5F_file_t {
    H5FD_t	*lf; 		/* Lower level file handle for I/O	*/
    H5F_super_t *sblock;        /* Pointer to (pinned) superblock for file */
    H5O_drvinfo_t *drvinfo;	/* Pointer to the (pinned) driver info 
                                 * cache entry.  This field is only defined
                                 * for older versions of the super block,
                                 * and then only when a driver information
                                 * block is present.  At all other times
                                 * it should be NULL.
                                 */
    hbool_t drvinfo_sb_msg_exists;  /* Convenience field used to track 
                                     * whether the driver info superblock 
                                     * extension message has been created 
                                     * yet. This field should be TRUE iff the
                                     * superblock extension exists and contains
                                     * a driver info message.  Under all other
                                     * circumstances, it must be set to FALSE.
                                     */
    unsigned	nrefs;		/* Ref count for times file is opened	*/
    unsigned	flags;		/* Access Permissions for file          */
    H5F_mtab_t	mtab;		/* File mount table                     */
    H5F_efc_t   *efc;           /* External file cache                  */

    /* Cached values from FCPL/superblock */
    uint8_t	sizeof_addr;	/* Size of addresses in file            */
    uint8_t	sizeof_size;	/* Size of offsets in file              */
    haddr_t	sohm_addr;	/* Relative address of shared object header message table */
    unsigned	sohm_vers;	/* Version of shared message table on disk */
    unsigned	sohm_nindexes;	/* Number of shared messages indexes in the table */
    unsigned long feature_flags; /* VFL Driver feature Flags            */
    haddr_t	maxaddr;	/* Maximum address for file             */

    H5PB_t      *page_buf;                  /* The page buffer cache                */
    H5AC_t      *cache;		/* The object cache	 		*/
    H5AC_cache_config_t
		mdc_initCacheCfg; /* initial configuration for the      */
                                /* metadata cache.  This structure is   */
                                /* fixed at creation time and should    */
                                /* not change thereafter.               */
    H5AC_cache_image_config_t 
		mdc_initCacheImageCfg;  /* initial configuration for the */
                                        /* generate metadata cache image on     */
                                        /* close option.  This structure is     */
                                        /* fixed at creation time and should    */
                                        /* not change thereafter.               */
    hbool_t     use_mdc_logging; /* Set when metadata logging is desired */
    hbool_t     start_mdc_log_on_access; /* set when mdc logging should  */
                                /* begin on file access/create          */
    char        *mdc_log_location; /* location of mdc log               */
    hid_t       fcpl_id;	/* File creation property list ID 	*/
    H5F_close_degree_t fc_degree;   /* File close behavior degree	*/
    hbool_t evict_on_close; /* If the file's objects should be evicted from the metadata cache on close */
    size_t	rdcc_nslots;	/* Size of raw data chunk cache (slots)	*/
    size_t	rdcc_nbytes;	/* Size of raw data chunk cache	(bytes)	*/
    double	rdcc_w0;	/* Preempt read chunks first? [0.0..1.0]*/
    size_t      sieve_buf_size; /* Size of the data sieve buffer allocated (in bytes) */
    hsize_t	threshold;	/* Threshold for alignment		*/
    hsize_t	alignment;	/* Alignment				*/
    unsigned	gc_ref;		/* Garbage-collect references?		*/
    H5F_libver_t    low_bound;  /* The 'low' bound of library format versions */
    H5F_libver_t    high_bound; /* The 'high' bound of library format versions */
    hbool_t	store_msg_crt_idx;  /* Store creation index for object header messages?	*/
    unsigned	ncwfs;		/* Num entries on cwfs list		*/
    struct H5HG_heap_t **cwfs;	/* Global heap cache			*/
    struct H5G_t *root_grp;	/* Open root group			*/
    H5FO_t *open_objs;          /* Open objects in file                 */
    H5UC_t *grp_btree_shared;   /* Ref-counted group B-tree node info   */

    /* File space allocation information */
    H5F_fspace_strategy_t fs_strategy; /* File space handling strategy	*/
    hsize_t     fs_threshold;	/* Free space section threshold 	*/
    hbool_t     fs_persist;     /* Free-space persist or not */
    hbool_t     use_tmp_space;  /* Whether temp. file space allocation is allowed */
    haddr_t	tmp_addr;       /* Next address to use for temp. space in the file */
    hbool_t     point_of_no_return; /* Flag to indicate that we can't go back and delete a freespace header when it's used up */

    H5F_fs_state_t fs_state[H5F_MEM_PAGE_NTYPES];   /* State of free space manager for each type */
    haddr_t fs_addr[H5F_MEM_PAGE_NTYPES];   /* Address of free space manager info for each type */
    H5FS_t *fs_man[H5F_MEM_PAGE_NTYPES];    /* Free space manager for each file space type */
    hbool_t first_alloc_dealloc;            /* TRUE iff free space managers   */
                                            /* are persistent and have not    */
                                            /* been used accessed for either  */
                                            /* allocation or deallocation     */
                                            /* since file open.               */
    haddr_t eoa_pre_fsm_fsalloc;	    /* eoa pre file space allocation  */
                                            /* for self referential FSMs      */
    haddr_t eoa_post_fsm_fsalloc;           /* eoa post file space allocation */
                                            /* for self referential FSMs      */
    haddr_t eoa_post_mdci_fsalloc;          /* eoa past file space allocation */
                                            /* for metadata cache image, or   */
                                            /* HADDR_UNDEF if no cache image. */

    /* Free-space aggregation info */
    unsigned fs_aggr_merge[H5FD_MEM_NTYPES]; /* Flags for whether free space can merge with aggregator(s) */
    H5FD_mem_t fs_type_map[H5FD_MEM_NTYPES]; /* Mapping of "real" file space type into tracked type */
    H5F_blk_aggr_t meta_aggr;   	    /* Metadata aggregation info (if aggregating metadata allocations) */
    H5F_blk_aggr_t sdata_aggr;              /* "Small data" aggregation info (if aggregating "small data" allocations) */

    /* Paged aggregation info */
    hsize_t fs_page_size;                   /* File space page size */
    size_t pgend_meta_thres;                /* Do not track page end meta section <= this threshold */

    /* Metadata accumulator information */
    H5F_meta_accum_t accum;                 /* Metadata accumulator info */

    /* Metadata retry info */
    unsigned 		read_attempts;	    /* The # of reads to try when reading metadata with checksum */
    unsigned		retries_nbins;	    /* # of bins for each retries[] */
    uint32_t		*retries[H5AC_NTYPES];  /* Track # of read retries for metdata items with checksum */

    /* Object flush info */
    H5F_object_flush_t 	object_flush;	    /* Information for object flush callback */
};

/*
 * This is the top-level file descriptor.  One of these structures is
 * allocated every time H5Fopen() is called although they may contain pointers
 * to shared H5F_file_t structs.
 */
struct H5F_t {
    char		       *open_name;      /* Name used to open file                                       */
    char		       *actual_name;    /* Actual name of the file, after resolving symlinks, etc.      */
    char               *extpath;        /* Path for searching target external link file                 */
    H5F_file_t		   *shared;         /* The shared file info                                         */
    unsigned		    nopen_objs;     /* Number of open object headers                                */
    H5FO_t             *obj_count;      /* # of time each object is opened through top file structure   */
    hid_t               file_id;        /* ID of this file                                              */
    hbool_t             closing;        /* File is in the process of being closed                       */
    struct H5F_t       *parent;         /* Parent file that this file is mounted to                     */
    unsigned            nmounts;        /* Number of children mounted to this file                      */
#ifdef H5_HAVE_PARALLEL
    H5P_coll_md_read_flag_t coll_md_read;  /* Do all metadata reads collectively */
    hbool_t             coll_md_write;  /* Do all metadata writes collectively */
#endif /* H5_HAVE_PARALLEL */
};


/*****************************/
/* Package Private Variables */
/*****************************/

/* Declare a free list to manage the H5F_t struct */
H5FL_EXTERN(H5F_t);

/* Declare a free list to manage the H5F_file_t struct */
H5FL_EXTERN(H5F_file_t);


/******************************/
/* Package Private Prototypes */
/******************************/

/* General routines */
H5_DLL H5F_t *H5F__new(H5F_file_t *shared, unsigned flags, hid_t fcpl_id, hid_t fapl_id, H5FD_t *lf);
H5_DLL herr_t H5F__dest(H5F_t *f, hbool_t flush);
H5_DLL H5F_t *H5F__create(const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id);
H5_DLL H5F_t *H5F__open(const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id);
H5_DLL herr_t H5F__flush(H5F_t *f, H5F_scope_t scope);
H5_DLL herr_t H5F__flush_real(H5F_t *f);
H5_DLL htri_t H5F__is_hdf5(const char *name);
H5_DLL herr_t H5F_get_objects(const H5F_t *f, unsigned types, size_t max_index, hid_t *obj_id_list, hbool_t app_ref, size_t *obj_id_count_ptr);
H5_DLL herr_t H5F__get_freespace(H5F_t *f, hsize_t *tot_space);
H5_DLL ssize_t H5F__get_file_image(H5F_t *f, void *buf_ptr, size_t buf_len);
H5_DLL herr_t H5F__get_info(H5F_t *f, H5F_info2_t *finfo);
H5_DLL ssize_t H5F__get_free_sections(H5F_t *f, H5FD_mem_t type, size_t nsects, H5F_sect_info_t *sect_info);
H5_DLL herr_t H5F__get_metadata_read_retry_info(H5F_t *file, H5F_retry_info_t *info);
H5_DLL herr_t H5F__format_convert(H5F_t *f);
H5_DLL herr_t H5F__start_swmr_write(H5F_t *f);
H5_DLL herr_t H5F__close(hid_t file_id);
H5_DLL herr_t H5F__close_cb(H5F_t *f);
H5_DLL herr_t H5F__set_libver_bounds(H5F_t *f, H5F_libver_t low, H5F_libver_t high);

/* File mount related routines */
H5_DLL herr_t H5F__mount(H5G_loc_t *loc, const char *name, H5F_t *child, hid_t plist_id);
H5_DLL herr_t H5F__unmount(H5G_loc_t *loc, const char *name);
H5_DLL herr_t H5F__close_mounts(H5F_t *f);
H5_DLL herr_t H5F__mount_count_ids(H5F_t *f, unsigned *nopen_files, unsigned *nopen_objs);

/* Superblock related routines */
H5_DLL herr_t H5F__super_init(H5F_t *f);
H5_DLL herr_t H5F__super_read(H5F_t *f, H5P_genplist_t *fa_plist, hbool_t initial_read);
H5_DLL herr_t H5F__super_size(H5F_t *f, hsize_t *super_size, hsize_t *super_ext_size);
H5_DLL herr_t H5F__super_free(H5F_super_t *sblock);

/* Superblock extension related routines */
H5_DLL herr_t H5F_super_ext_open(H5F_t *f, haddr_t ext_addr, H5O_loc_t *ext_ptr);
H5_DLL herr_t H5F__super_ext_write_msg(H5F_t *f, unsigned id, void *mesg, hbool_t may_create, unsigned mesg_flags);
H5_DLL herr_t H5F__super_ext_remove_msg(H5F_t *f, unsigned id);
H5_DLL herr_t H5F__super_ext_close(H5F_t *f, H5O_loc_t *ext_ptr, hbool_t was_created);

/* Metadata accumulator routines */
H5_DLL herr_t H5F__accum_read(H5F_t *f, H5FD_mem_t type, haddr_t addr, size_t size, void *buf);
H5_DLL herr_t H5F__accum_write(H5F_t *f, H5FD_mem_t type, haddr_t addr, size_t size, const void *buf);
H5_DLL herr_t H5F__accum_free(H5F_t *f, H5FD_mem_t type, haddr_t addr, hsize_t size);
H5_DLL herr_t H5F__accum_flush(H5F_t *f);
H5_DLL herr_t H5F__accum_reset(H5F_t *f, hbool_t flush);

/* Shared file list related routines */
H5_DLL herr_t H5F_sfile_add(H5F_file_t *shared);
H5_DLL H5F_file_t * H5F_sfile_search(H5FD_t *lf);
H5_DLL herr_t H5F_sfile_remove(H5F_file_t *shared);

/* External file cache routines */
H5_DLL H5F_efc_t *H5F__efc_create(unsigned max_nfiles);
H5_DLL H5F_t *H5F__efc_open(H5F_t *parent, const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id);
H5_DLL unsigned H5F__efc_max_nfiles(H5F_efc_t *efc);
H5_DLL herr_t H5F__efc_release(H5F_efc_t *efc);
H5_DLL herr_t H5F__efc_destroy(H5F_efc_t *efc);
H5_DLL herr_t H5F__efc_try_close(H5F_t *f);

/* Space allocation routines */
H5_DLL haddr_t H5F__alloc(H5F_t *f, H5F_mem_t type, hsize_t size, haddr_t *frag_addr, hsize_t *frag_size);
H5_DLL herr_t H5F__free(H5F_t *f, H5F_mem_t type, haddr_t addr, hsize_t size);
H5_DLL htri_t H5F__try_extend(H5F_t *f, H5FD_mem_t type, haddr_t blk_end, hsize_t extra_requested);

/* Functions that get/retrieve values from VFD layer */
H5_DLL herr_t H5F__set_eoa(const H5F_t *f, H5F_mem_t type, haddr_t addr);
H5_DLL herr_t H5F__set_base_addr(const H5F_t *f, haddr_t addr);
H5_DLL herr_t H5F__set_paged_aggr(const H5F_t *f, hbool_t paged);
H5_DLL herr_t H5F__get_max_eof_eoa(const H5F_t *f, haddr_t *max_eof_eoa);

/* Functions that flush or evict */
H5_DLL herr_t H5F__evict_cache_entries(H5F_t *f);

/* Testing functions */
#ifdef H5F_TESTING
H5_DLL herr_t H5F_get_sohm_mesg_count_test(hid_t fid, unsigned type_id, size_t *mesg_count);
H5_DLL herr_t H5F_check_cached_stab_test(hid_t file_id);
H5_DLL herr_t H5F_get_maxaddr_test(hid_t file_id, haddr_t *maxaddr);
H5_DLL herr_t H5F_get_sbe_addr_test(hid_t file_id, haddr_t *sbe_addr);
#endif /* H5F_TESTING */

#endif /* _H5Fpkg_H */

