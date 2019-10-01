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
 * Programmer:  James Laird <jlaird@ncsa.uiuc.edu>
 *              Thursday, March 30, 2006
 *
 * Purpose:     This file contains declarations which are visible only within
 *              the H5SM shared object header messages package.  Source files
 *              outside the H5SM package should	include H5SMprivate.h instead.
 */
#if !(defined H5SM_FRIEND || defined H5SM_MODULE)
#error "Do not include this file outside the H5SM package!"
#endif

#ifndef _H5SMpkg_H
#define _H5SMpkg_H

/* Get package's private header */
#include "H5SMprivate.h"	/* Shared Object Header Messages	*/

/* Other private headers needed by this file */
#include "H5ACprivate.h"        /* Metadata Cache		  	*/
#include "H5B2private.h"        /* B-trees                              */
#include "H5HFprivate.h"        /* Fractal heaps		  	*/


/****************************/
/* Package Macros           */
/****************************/

/* Size of checksum information (on disk) */
#define H5SM_SIZEOF_CHECKSUM 4

#define H5SM_HEAP_LOC_SIZE (                                                  \
        (unsigned)4                     /* Reference count */                 \
        + sizeof(H5O_fheap_id_t)        /* size of heap ID on disk */         \
    )

#define H5SM_OH_LOC_SIZE(f) (                                                 \
        (unsigned)1             /* reserved (possible flags?) */              \
        + (unsigned)1           /* message type ID */			      \
        + (unsigned)2           /* creation index of message in OH */         \
        + H5F_SIZEOF_ADDR(f)    /* address of OH */                           \
    )

#define H5SM_SOHM_ENTRY_SIZE(f) (                                             \
        (unsigned)1             /* Message location */                        \
        + (unsigned)4           /* Hash value */                              \
        + MAX(H5SM_HEAP_LOC_SIZE, H5SM_OH_LOC_SIZE(f))  /* Entry */           \
    )

#define H5SM_INDEX_HEADER_SIZE(f) (                                           \
        (unsigned)1             /* Whether index is a list or B-tree */       \
        + (unsigned)1           /* Version of index format */                 \
        + (unsigned)2           /* Type of messages stored in the index */    \
        + (unsigned)4           /* Minimum size of messages to share */       \
        + (unsigned)(3 * 2)     /* B-tree cutoff, list cutoff, # of shared messages */ \
        + H5F_SIZEOF_ADDR(f)    /* Location of list or B-tree */              \
        + H5F_SIZEOF_ADDR(f)    /* Address of heap */                         \
    )

/* Format overhead for all SOHM tree metadata in the file */
#define H5SM_METADATA_PREFIX_SIZE (                                           \
    H5_SIZEOF_MAGIC             /* Signature */                               \
    + H5SM_SIZEOF_CHECKSUM      /* Checksum */                                \
    )

#define H5SM_TABLE_SIZE(f) (                                                  \
    /* General metadata fields */                                             \
    H5SM_METADATA_PREFIX_SIZE                                                 \
                                                                              \
    /* Indices */                                                             \
    + (H5F_SOHM_NINDEXES(f) * H5SM_INDEX_HEADER_SIZE(f))                      \
    )

#define H5SM_LIST_SIZE(f, num_mesg) (                                         \
    /* General metadata fields */                                             \
    H5SM_METADATA_PREFIX_SIZE                                                 \
                                                                              \
    /* Message entries */                                                     \
    + (H5SM_SOHM_ENTRY_SIZE(f) * num_mesg)                                    \
    )

#define H5SM_B2_NODE_SIZE 512
#define H5SM_B2_SPLIT_PERCENT 100
#define H5SM_B2_MERGE_PERCENT 40

#define H5SM_LIST_VERSION	0	/* Version of Shared Object Header Message List Indexes */

/****************************/
/* Package Typedefs         */
/****************************/

/* There are a number of Shared Object Header Message-specific structs here.
 *
 * The H5SM_master_table_t is pointed to by the file superblock.  Since a file
 * can have more than one SOHM index, this table collects all the indexes into
 * one place.  It holds an array of H5SM_index_header_t structs.
 *
 * An H5SM_index_header_t is actually the for a given index.  It holds
 * the number of messages in the index, the types of messages in the index,
 * etc.  It also records whether the index is a list or a b-tree, and has
 * the address of the list or b-tree.
 *
 * If the index is a list, the address in the index header should be given
 * to the cache, which can load it into a H5SM_list_t struct.  This is mostly
 * just a header for the cache information; it contains a pointer back to
 * the index header and an unsorted array of messages.
 *
 * These messages are H5SM_sohm_t structs.  They hold the actual SOHM's
 * address, hash value, and refcount.
 *
 * If the index is a b-tree, the H5SM_index_header_t struct holds the address
 * of the b-tree instead of the address of a H5SM_list_t.  The B-tree's nodes
 * are still 'H5SM_sohm_t's.
 *
 * H5SM_mesg_key_t structs are used to search lists and B-trees for a certain
 * message.  They correspond to a message that hasn't yet been written to
 * disk.
 */

/* Where a message is stored */
typedef enum {
    H5SM_NO_LOC = -1,
    H5SM_IN_HEAP = 0,    	/* Message is stored in the heap */
    H5SM_IN_OH    		/* Message is stored in an object header */
} H5SM_storage_loc_t;

/* Typedef for a record's location if it's stored in the heap */
typedef struct {
    hsize_t ref_count;		/* Number of times this message is used in the file */
    H5O_fheap_id_t fheap_id;    /* ID of the OHM in the fractal heap */
} H5SM_heap_loc_t;

/* Typedef for a SOHM index node */
typedef struct {
    H5SM_storage_loc_t location;        /* Type of message location */
    uint32_t hash;		        /* Hash value for encoded OHM */
    unsigned msg_type_id;               /* Message's type ID */
    union {
        H5O_mesg_loc_t mesg_loc;        /* Location of message in object header */
        H5SM_heap_loc_t heap_loc;       /* Heap ID for message in SOHM heap */
    } u;
} H5SM_sohm_t;

/* Types of message indices */
typedef enum {
    H5SM_BADTYPE = -1,
    H5SM_LIST,    		/* Index is an unsorted list */
    H5SM_BTREE    		/* Index is a sorted B-tree */
} H5SM_index_type_t;

/* Typedef for a SOHM index header */
typedef struct {
/* Stored */
    unsigned mesg_types;	/* Bit flag vector of message types */
    size_t min_mesg_size;	/* number of messages being tracked */
    size_t list_max;		/* >= this many messages, index with a B-tree */
    size_t btree_min;		/* <= this many messages, index with a list again */
    size_t num_messages;	/* number of messages being tracked */
    H5SM_index_type_t index_type; /* Is the index a list or a B-tree? */
    haddr_t index_addr;		/* Address of the actual index (list or B-tree) */
    haddr_t heap_addr;		/* Address of the fheap used to store shared messages */

/* Not stored */
    size_t list_size;           /* Size of list index on disk */
} H5SM_index_header_t;

/* Typedef for a SOHM list */
typedef struct {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    H5SM_index_header_t *header;    /* Pointer to the corresponding index header */
    H5SM_sohm_t *messages;          /* Actual list, stored as an array */
} H5SM_list_t;

/* Typedef for shared object header message master table */
struct H5SM_master_table_t {
    /* Information for H5AC cache functions, _must_ be first field in structure */
    H5AC_info_t cache_info;

    size_t table_size;              /* Size of table on disk */
    unsigned num_indexes;           /* Number of indexes */
    H5SM_index_header_t *indexes;   /* Array of num_indexes indexes */
};

/* Typedef for searching an index (list or B-tree) */
typedef struct {
    H5F_t *file;                        /* File in which sharing is happening */
    H5HF_t *fheap;    			/* The heap for this message type, open. */
    void *encoding; 		        /* The message encoded, or NULL */
    size_t encoding_size; 		/* Size of the encoding, or 0 */
    H5SM_sohm_t message;                /* The message to find/insert.
                                         * If the message doesn't yet have a
                                         * heap ID, the heap ID will be 0. */
} H5SM_mesg_key_t;

/*
 * Data exchange structure to pass through the fractal heap layer for the
 * H5HF_op function when computing a hash value for a message.
 */
typedef struct {
    /* downward (internal) */
    unsigned    type_id;                /* Message type */

    /* upward */
    uint32_t    hash;                   /* Hash value */
} H5SM_fh_ud_gh_t;

/* Typedef to increment a reference count in the B-tree */
typedef struct {
    H5SM_mesg_key_t *key;       /* IN: key for message being incremented */
    H5O_fheap_id_t fheap_id;    /* OUT: fheap ID of record */
} H5SM_incr_ref_opdata;

/* v2 B-tree client callback context */
typedef struct H5SM_bt2_ctx_t {
    uint8_t     sizeof_addr;    /* Size of file addresses */
} H5SM_bt2_ctx_t;

/* Callback info for loading a shared message table index into the cache */
typedef struct H5SM_table_cache_ud_t {
    H5F_t *f;                   /* File that shared message index stored as a table is in */
} H5SM_table_cache_ud_t;

/* Callback info for loading a shared message list index into the cache */
typedef struct H5SM_list_cache_ud_t {
    H5F_t *f;                   /* File that shared message index stored as a table is in */
    H5SM_index_header_t *header; /* Index header for this list */
} H5SM_list_cache_ud_t;


/****************************/
/* Package Variables        */
/****************************/

/* Declare free lists to manage H5SM structs */
H5FL_EXTERN(H5SM_master_table_t);
H5FL_ARR_EXTERN(H5SM_index_header_t);
H5FL_EXTERN(H5SM_list_t);
H5FL_ARR_EXTERN(H5SM_sohm_t);

H5_DLLVAR const H5B2_class_t H5SM_INDEX[1];

/****************************/
/* Package Prototypes       */
/****************************/

/* General routines */
H5_DLL ssize_t H5SM_get_index(const H5SM_master_table_t *table, unsigned type_id);

/* Encode and decode routines, used for B-tree and cache encoding/decoding */
H5_DLL herr_t H5SM__message_compare(const void *rec1, const void *rec2, int *result);
H5_DLL herr_t H5SM__message_encode(uint8_t *raw, const void *native, void *ctx);
H5_DLL herr_t H5SM__message_decode(const uint8_t *raw, void *native, void *ctx);

/* H5B2_remove_t callback to add messages to a list index */
H5_DLL herr_t H5SM_bt2_convert_to_list_op(const void * record, void *op_data);

/* Fractal heap 'op' callback to compute hash value for message "in place" */
H5_DLL herr_t H5SM_get_hash_fh_cb(const void *obj, size_t obj_len, void *_udata);

/* Routines to release data structures */
herr_t H5SM_table_free(H5SM_master_table_t *table);
herr_t H5SM_list_free(H5SM_list_t *list);

/* Testing functions */
#ifdef H5SM_TESTING
H5_DLL herr_t H5SM__get_mesg_count_test(H5F_t *f, unsigned type_id, size_t *mesg_count);
#endif /* H5SM_TESTING */

#endif /* _H5SMpkg_H */

