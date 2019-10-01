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
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, July 26, 1999
 */
#ifndef _H5FDpublic_H
#define _H5FDpublic_H

#include "H5public.h"
#include "H5Fpublic.h"		/*for H5F_close_degree_t */

#define H5_HAVE_VFL 1 /*define a convenient app feature test*/
#define H5FD_VFD_DEFAULT 0   /* Default VFL driver value */

/* Types of allocation requests: see H5Fpublic.h  */
typedef enum H5F_mem_t	H5FD_mem_t;

/* Map "fractal heap" header blocks to 'ohdr' type file memory, since its
 * a fair amount of work to add a new kind of file memory and they are similar
 * enough to object headers and probably too minor to deserve their own type.
 *
 * Map "fractal heap" indirect blocks to 'ohdr' type file memory, since they
 * are similar to fractal heap header blocks.
 *
 * Map "fractal heap" direct blocks to 'lheap' type file memory, since they
 * will be replacing local heaps.
 *
 * Map "fractal heap" 'huge' objects to 'draw' type file memory, since they
 * represent large objects that are directly stored in the file.
 *
 *      -QAK
 */
#define H5FD_MEM_FHEAP_HDR      H5FD_MEM_OHDR
#define H5FD_MEM_FHEAP_IBLOCK   H5FD_MEM_OHDR
#define H5FD_MEM_FHEAP_DBLOCK   H5FD_MEM_LHEAP
#define H5FD_MEM_FHEAP_HUGE_OBJ H5FD_MEM_DRAW

/* Map "free space" header blocks to 'ohdr' type file memory, since its
 * a fair amount of work to add a new kind of file memory and they are similar
 * enough to object headers and probably too minor to deserve their own type.
 *
 * Map "free space" serialized sections to 'lheap' type file memory, since they
 * are similar enough to local heap info.
 *
 *      -QAK
 */
#define H5FD_MEM_FSPACE_HDR     H5FD_MEM_OHDR
#define H5FD_MEM_FSPACE_SINFO   H5FD_MEM_LHEAP

/* Map "shared object header message" master table to 'ohdr' type file memory,
 * since its a fair amount of work to add a new kind of file memory and they are
 * similar enough to object headers and probably too minor to deserve their own
 * type.
 *
 * Map "shared object header message" indices to 'btree' type file memory,
 * since they are similar enough to B-tree nodes.
 *
 *      -QAK
 */
#define H5FD_MEM_SOHM_TABLE     H5FD_MEM_OHDR
#define H5FD_MEM_SOHM_INDEX     H5FD_MEM_BTREE

/* Map "extensible array" header blocks to 'ohdr' type file memory, since its
 * a fair amount of work to add a new kind of file memory and they are similar
 * enough to object headers and probably too minor to deserve their own type.
 *
 * Map "extensible array" index blocks to 'ohdr' type file memory, since they
 * are similar to extensible array header blocks.
 *
 * Map "extensible array" super blocks to 'btree' type file memory, since they
 * are similar enough to B-tree nodes.
 *
 * Map "extensible array" data blocks & pages to 'lheap' type file memory, since
 * they are similar enough to local heap info.
 *
 *      -QAK
 */
#define H5FD_MEM_EARRAY_HDR     H5FD_MEM_OHDR
#define H5FD_MEM_EARRAY_IBLOCK  H5FD_MEM_OHDR
#define H5FD_MEM_EARRAY_SBLOCK  H5FD_MEM_BTREE
#define H5FD_MEM_EARRAY_DBLOCK  H5FD_MEM_LHEAP
#define H5FD_MEM_EARRAY_DBLK_PAGE  H5FD_MEM_LHEAP

/* Map "fixed array" header blocks to 'ohdr' type file memory, since its
 * a fair amount of work to add a new kind of file memory and they are similar
 * enough to object headers and probably too minor to deserve their own type.
 *
 * Map "fixed array" data blocks & pages to 'lheap' type file memory, since
 * they are similar enough to local heap info.
 *
 */
#define H5FD_MEM_FARRAY_HDR     H5FD_MEM_OHDR
#define H5FD_MEM_FARRAY_DBLOCK  H5FD_MEM_LHEAP
#define H5FD_MEM_FARRAY_DBLK_PAGE  H5FD_MEM_LHEAP

/*
 * A free-list map which maps all types of allocation requests to a single
 * free list.  This is useful for drivers that don't really care about
 * keeping different requests segregated in the underlying file and which
 * want to make most efficient reuse of freed memory.  The use of the
 * H5FD_MEM_SUPER free list is arbitrary.
 */
#define H5FD_FLMAP_SINGLE {						      \
    H5FD_MEM_SUPER,			/*default*/			      \
    H5FD_MEM_SUPER,			/*super*/			      \
    H5FD_MEM_SUPER,			/*btree*/			      \
    H5FD_MEM_SUPER,			/*draw*/			      \
    H5FD_MEM_SUPER,			/*gheap*/			      \
    H5FD_MEM_SUPER,			/*lheap*/			      \
    H5FD_MEM_SUPER			/*ohdr*/			      \
}

/*
 * A free-list map which segregates requests into `raw' or `meta' data
 * pools.
 */
#define H5FD_FLMAP_DICHOTOMY {						      \
    H5FD_MEM_SUPER,			/*default*/			      \
    H5FD_MEM_SUPER,			/*super*/			      \
    H5FD_MEM_SUPER,			/*btree*/			      \
    H5FD_MEM_DRAW,			/*draw*/			      \
    H5FD_MEM_DRAW,			/*gheap*/			      \
    H5FD_MEM_SUPER,			/*lheap*/			      \
    H5FD_MEM_SUPER			/*ohdr*/			      \
}

/*
 * The default free list map which causes each request type to use it's own
 * free-list.
 */
#define H5FD_FLMAP_DEFAULT {						      \
    H5FD_MEM_DEFAULT,			/*default*/			      \
    H5FD_MEM_DEFAULT,			/*super*/			      \
    H5FD_MEM_DEFAULT,			/*btree*/			      \
    H5FD_MEM_DEFAULT,			/*draw*/			      \
    H5FD_MEM_DEFAULT,			/*gheap*/			      \
    H5FD_MEM_DEFAULT,			/*lheap*/			      \
    H5FD_MEM_DEFAULT			/*ohdr*/			      \
}


/* Define VFL driver features that can be enabled on a per-driver basis */
/* These are returned with the 'query' function pointer in H5FD_class_t */
    /*
     * Defining H5FD_FEAT_AGGREGATE_METADATA for a VFL driver means that
     * the library will attempt to allocate a larger block for metadata and
     * then sub-allocate each metadata request from that larger block.
     */
#define H5FD_FEAT_AGGREGATE_METADATA    0x00000001
    /*
     * Defining H5FD_FEAT_ACCUMULATE_METADATA for a VFL driver means that
     * the library will attempt to cache metadata as it is written to the file
     * and build up a larger block of metadata to eventually pass to the VFL
     * 'write' routine.
     *
     * Distinguish between updating the metadata accumulator on writes and
     * reads.  This is particularly (perhaps only, even) important for MPI-I/O
     * where we guarantee that writes are collective, but reads may not be.
     * If we were to allow the metadata accumulator to be written during a
     * read operation, the application would hang.
     */
#define H5FD_FEAT_ACCUMULATE_METADATA_WRITE     0x00000002
#define H5FD_FEAT_ACCUMULATE_METADATA_READ      0x00000004
#define H5FD_FEAT_ACCUMULATE_METADATA   (H5FD_FEAT_ACCUMULATE_METADATA_WRITE|H5FD_FEAT_ACCUMULATE_METADATA_READ)
    /*
     * Defining H5FD_FEAT_DATA_SIEVE for a VFL driver means that
     * the library will attempt to cache raw data as it is read from/written to
     * a file in a "data seive" buffer.  See Rajeev Thakur's papers:
     *  http://www.mcs.anl.gov/~thakur/papers/romio-coll.ps.gz
     *  http://www.mcs.anl.gov/~thakur/papers/mpio-high-perf.ps.gz
     */
#define H5FD_FEAT_DATA_SIEVE            0x00000008
    /*
     * Defining H5FD_FEAT_AGGREGATE_SMALLDATA for a VFL driver means that
     * the library will attempt to allocate a larger block for "small" raw data
     * and then sub-allocate "small" raw data requests from that larger block.
     */
#define H5FD_FEAT_AGGREGATE_SMALLDATA   0x00000010
    /*
     * Defining H5FD_FEAT_IGNORE_DRVRINFO for a VFL driver means that
     * the library will ignore the driver info that is encoded in the file
     * for the VFL driver.  (This will cause the driver info to be eliminated
     * from the file when it is flushed/closed, if the file is opened R/W).
     */
#define H5FD_FEAT_IGNORE_DRVRINFO       0x00000020
    /*
     * Defining the H5FD_FEAT_DIRTY_DRVRINFO_LOAD for a VFL driver means that
     * the library will mark the driver info dirty when the file is opened
     * R/W.  This will cause the driver info to be re-encoded when the file
     * is flushed/closed.
     */
#define H5FD_FEAT_DIRTY_DRVRINFO_LOAD   0x00000040
    /*
     * Defining H5FD_FEAT_POSIX_COMPAT_HANDLE for a VFL driver means that
     * the handle for the VFD (returned with the 'get_handle' callback) is
     * of type 'int' and is compatible with POSIX I/O calls.
     */
#define H5FD_FEAT_POSIX_COMPAT_HANDLE   0x00000080    
    /*
     * Defining H5FD_FEAT_HAS_MPI for a VFL driver means that
     * the driver makes use of MPI communication and code may retrieve
     * communicator/rank information from it
     */
#define H5FD_FEAT_HAS_MPI               0x00000100
    /*
     * Defining the H5FD_FEAT_ALLOCATE_EARLY for a VFL driver will force
     * the library to use the H5D_ALLOC_TIME_EARLY on dataset create
     * instead of the default H5D_ALLOC_TIME_LATE
     */
#define H5FD_FEAT_ALLOCATE_EARLY        0x00000200
    /* 
     * Defining H5FD_FEAT_ALLOW_FILE_IMAGE for a VFL driver means that
     * the driver is able to use a file image in the fapl as the initial
     * contents of a file.
     */
#define H5FD_FEAT_ALLOW_FILE_IMAGE      0x00000400
    /*
     * Defining H5FD_FEAT_CAN_USE_FILE_IMAGE_CALLBACKS for a VFL driver
     * means that the driver is able to use callbacks to make a copy of the
     * image to store in memory.
     */
#define H5FD_FEAT_CAN_USE_FILE_IMAGE_CALLBACKS 0x00000800
    /*
     * Defining H5FD_FEAT_SUPPORTS_SWMR_IO for a VFL driver means that the
     * driver supports the single-writer/multiple-readers I/O pattern.
     */
#define H5FD_FEAT_SUPPORTS_SWMR_IO      0x00001000
    /*
     * Defining H5FD_FEAT_USE_ALLOC_SIZE for a VFL driver
     * means that the library will just pass the allocation size to the
     * the driver's allocation callback which will eventually handle alignment.
     * This is specifically used for the multi/split driver.
     */
#define H5FD_FEAT_USE_ALLOC_SIZE	0x00002000
    /*
     * Defining H5FD_FEAT_PAGED_AGGR for a VFL driver
     * means that the driver needs special file space mapping for paged aggregation.
     * This is specifically used for the multi/split driver.
     */
#define H5FD_FEAT_PAGED_AGGR		0x00004000
    /*
     * Defining H5FD_FEAT_DEFAULT_VFD_COMPATIBLE for a VFL driver
     * that creates a file which is compatible with the default VFD.
     * Generally, this means that the VFD creates a single file that follows
     * the canonical HDF5 file format.
     */
#define H5FD_FEAT_DEFAULT_VFD_COMPATIBLE        0x00008000


/* Forward declaration */
typedef struct H5FD_t H5FD_t;

/* Class information for each file driver */
typedef struct H5FD_class_t {
    const char *name;
    haddr_t maxaddr;
    H5F_close_degree_t fc_degree;
    herr_t  (*terminate)(void);
    hsize_t (*sb_size)(H5FD_t *file);
    herr_t  (*sb_encode)(H5FD_t *file, char *name/*out*/,
                         unsigned char *p/*out*/);
    herr_t  (*sb_decode)(H5FD_t *f, const char *name, const unsigned char *p);
    size_t  fapl_size;
    void *  (*fapl_get)(H5FD_t *file);
    void *  (*fapl_copy)(const void *fapl);
    herr_t  (*fapl_free)(void *fapl);
    size_t  dxpl_size;
    void *  (*dxpl_copy)(const void *dxpl);
    herr_t  (*dxpl_free)(void *dxpl);
    H5FD_t *(*open)(const char *name, unsigned flags, hid_t fapl,
                    haddr_t maxaddr);
    herr_t  (*close)(H5FD_t *file);
    int     (*cmp)(const H5FD_t *f1, const H5FD_t *f2);
    herr_t  (*query)(const H5FD_t *f1, unsigned long *flags);
    herr_t  (*get_type_map)(const H5FD_t *file, H5FD_mem_t *type_map);
    haddr_t (*alloc)(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
    herr_t  (*free)(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                    haddr_t addr, hsize_t size);
    haddr_t (*get_eoa)(const H5FD_t *file, H5FD_mem_t type);
    herr_t  (*set_eoa)(H5FD_t *file, H5FD_mem_t type, haddr_t addr);
    haddr_t (*get_eof)(const H5FD_t *file, H5FD_mem_t type);
    herr_t  (*get_handle)(H5FD_t *file, hid_t fapl, void**file_handle);
    herr_t  (*read)(H5FD_t *file, H5FD_mem_t type, hid_t dxpl,
                    haddr_t addr, size_t size, void *buffer);
    herr_t  (*write)(H5FD_t *file, H5FD_mem_t type, hid_t dxpl,
                     haddr_t addr, size_t size, const void *buffer);
    herr_t  (*flush)(H5FD_t *file, hid_t dxpl_id, hbool_t closing);
    herr_t  (*truncate)(H5FD_t *file, hid_t dxpl_id, hbool_t closing);
    herr_t  (*lock)(H5FD_t *file, hbool_t rw);
    herr_t  (*unlock)(H5FD_t *file);
    H5FD_mem_t fl_map[H5FD_MEM_NTYPES];
} H5FD_class_t;

/* A free list is a singly-linked list of address/size pairs. */
typedef struct H5FD_free_t {
    haddr_t		addr;
    hsize_t		size;
    struct H5FD_free_t	*next;
} H5FD_free_t;

/*
 * The main datatype for each driver. Public fields common to all drivers
 * are declared here and the driver appends private fields in memory.
 */
struct H5FD_t {
    hid_t               driver_id;      /*driver ID for this file   */
    const H5FD_class_t *cls;            /*constant class info       */
    unsigned long       fileno;         /* File 'serial' number     */
    unsigned            access_flags;   /* File access flags (from create or open) */
    unsigned long       feature_flags;  /* VFL Driver feature Flags */
    haddr_t             maxaddr;        /* For this file, overrides class */
    haddr_t             base_addr;      /* Base address for HDF5 data w/in file */

    /* Space allocation management fields */
    hsize_t             threshold;      /* Threshold for alignment  */
    hsize_t             alignment;      /* Allocation alignment     */
    hbool_t             paged_aggr;     /* Paged aggregation for file space is enabled or not */
};

/* Define enum for the source of file image callbacks */
typedef enum {
    H5FD_FILE_IMAGE_OP_NO_OP,
    H5FD_FILE_IMAGE_OP_PROPERTY_LIST_SET,     
    H5FD_FILE_IMAGE_OP_PROPERTY_LIST_COPY,
    H5FD_FILE_IMAGE_OP_PROPERTY_LIST_GET,
    H5FD_FILE_IMAGE_OP_PROPERTY_LIST_CLOSE,
    H5FD_FILE_IMAGE_OP_FILE_OPEN,
    H5FD_FILE_IMAGE_OP_FILE_RESIZE,
    H5FD_FILE_IMAGE_OP_FILE_CLOSE
} H5FD_file_image_op_t;

/* Define structure to hold file image callbacks */
typedef struct {
    void   *(*image_malloc)(size_t size, H5FD_file_image_op_t file_image_op, 
                            void *udata);
    void   *(*image_memcpy)(void *dest, const void *src, size_t size,
                            H5FD_file_image_op_t file_image_op, void *udata);
    void   *(*image_realloc)(void *ptr, size_t size, 
                            H5FD_file_image_op_t file_image_op, void *udata);
    herr_t  (*image_free)(void *ptr, H5FD_file_image_op_t file_image_op, 
                          void *udata);
    void   *(*udata_copy)(void *udata);
    herr_t  (*udata_free)(void *udata);
    void *udata;
} H5FD_file_image_callbacks_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Function prototypes */
H5_DLL hid_t H5FDregister(const H5FD_class_t *cls);
H5_DLL herr_t H5FDunregister(hid_t driver_id);
H5_DLL H5FD_t *H5FDopen(const char *name, unsigned flags, hid_t fapl_id,
                        haddr_t maxaddr);
H5_DLL herr_t H5FDclose(H5FD_t *file);
H5_DLL int H5FDcmp(const H5FD_t *f1, const H5FD_t *f2);
H5_DLL int H5FDquery(const H5FD_t *f, unsigned long *flags);
H5_DLL haddr_t H5FDalloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
H5_DLL herr_t H5FDfree(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                       haddr_t addr, hsize_t size);
H5_DLL haddr_t H5FDget_eoa(H5FD_t *file, H5FD_mem_t type);
H5_DLL herr_t H5FDset_eoa(H5FD_t *file, H5FD_mem_t type, haddr_t eoa);
H5_DLL haddr_t H5FDget_eof(H5FD_t *file, H5FD_mem_t type);
H5_DLL herr_t H5FDget_vfd_handle(H5FD_t *file, hid_t fapl, void**file_handle);
H5_DLL herr_t H5FDread(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                       haddr_t addr, size_t size, void *buf/*out*/);
H5_DLL herr_t H5FDwrite(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                        haddr_t addr, size_t size, const void *buf);
H5_DLL herr_t H5FDflush(H5FD_t *file, hid_t dxpl_id, hbool_t closing);
H5_DLL herr_t H5FDtruncate(H5FD_t *file, hid_t dxpl_id, hbool_t closing);
H5_DLL herr_t H5FDlock(H5FD_t *file, hbool_t rw);
H5_DLL herr_t H5FDunlock(H5FD_t *file);

/* Allows querying a VFD ID for features before the file is opened */
H5_DLL herr_t H5FDdriver_query(hid_t driver_id, unsigned long *flags/*out*/);

#ifdef __cplusplus
}
#endif
#endif

