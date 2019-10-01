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
 * Programmer: Quincey Koziol <koziol@ncsa.uiuc.edu>
 *             Wednesday, July 9, 2003
 *
 * Purpose:     This file contains declarations which are visible
 *              only within the H5HG package. Source files outside the
 *              H5HG package should include H5HGprivate.h instead.
 */
#if !(defined H5HG_FRIEND || defined H5HG_MODULE)
#error "Do not include this file outside the H5HG package!"
#endif

#ifndef _H5HGpkg_H
#define _H5HGpkg_H

/* Get package's private header */
#include "H5HGprivate.h"

/* Other private headers needed by this file */
#include "H5ACprivate.h"	/* Metadata cache			*/
#include "H5FLprivate.h"	/* Free lists                           */


/*****************************/
/* Package Private Variables */
/*****************************/

/* Declare extern the free list to manage the H5HG_t struct */
H5FL_EXTERN(H5HG_heap_t);

/* Declare extern the free list to manage sequences of H5HG_obj_t's */
H5FL_SEQ_EXTERN(H5HG_obj_t);

/* Declare extern the PQ free list to manage heap chunks */
H5FL_BLK_EXTERN(gheap_chunk);


/**************************/
/* Package Private Macros */
/**************************/

/*
 * Global heap collection version.
 */
#define H5HG_VERSION	1

/*
 * All global heap collections are at least this big.  This allows us to read
 * most collections with a single read() since we don't have to read a few
 * bytes of header to figure out the size.  If the heap is larger than this
 * then a second read gets the rest after we've decoded the header.
 */
#define H5HG_MINSIZE	4096

/*
 * Pad all global heap messages to a multiple of eight bytes so we can load
 * the entire collection into memory and operate on it there.  Eight should
 * be sufficient for machines that have alignment constraints because our
 * largest data type is eight bytes.
 */
#define H5HG_ALIGNMENT	8
#define H5HG_ALIGN(X)	(H5HG_ALIGNMENT*(((X)+H5HG_ALIGNMENT-1)/H5HG_ALIGNMENT))
#define H5HG_ISALIGNED(X) ((X)==H5HG_ALIGN(X))

/*
 * The size of the collection header, always a multiple of the alignment so
 * that the stuff that follows the header is aligned.
 */
#define H5HG_SIZEOF_HDR(f)						      \
    (size_t)H5HG_ALIGN(4 +			/*magic number		*/ \
                       1 +			/*version number	*/ \
                       3 +			/*reserved		*/ \
                       H5F_SIZEOF_SIZE(f))	/*collection size	*/

/*
 * The overhead associated with each object in the heap, always a multiple of
 * the alignment so that the stuff that follows the header is aligned.
 */
#define H5HG_SIZEOF_OBJHDR(f)						      \
    (size_t)H5HG_ALIGN(2 +			/*object id number	*/ \
                       2 +			/*reference count	*/ \
                       4 +			/*reserved		*/ \
                       H5F_SIZEOF_SIZE(f))	/*object data size	*/

/*
 * The initial guess for the number of messages in a collection.  We assume
 * that all objects in that collection are zero length, giving the maximum
 * possible number of objects in the collection.  The collection itself has
 * some overhead and each message has some overhead.  The `+2' accounts for
 * rounding and for the free space object.
 */
#define H5HG_NOBJS(f,z) ((((z)-H5HG_SIZEOF_HDR(f))/		      \
                          H5HG_SIZEOF_OBJHDR(f)+2))


/****************************/
/* Package Private Typedefs */
/****************************/

typedef struct H5HG_obj_t {
    int         nrefs;      /* reference count                  */
    size_t      size;       /* total size of object             */
    uint8_t     *begin;     /* ptr to object into heap->chunk   */
} H5HG_obj_t;

/* Forward declarations for fields */
struct H5F_file_t;

struct H5HG_heap_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    haddr_t		addr;		/*collection address		*/
    size_t		size;		/*total size of collection	*/
    uint8_t		*chunk;		/*the collection, incl. header	*/
    size_t		nalloc;		/*numb object slots allocated	*/
    size_t		nused;		/*number of slots used		*/
                                        /* If this value is >65535 then all indices */
                                        /* have been used at some time and the */
                                        /* correct new index should be searched for */
    struct H5F_file_t   *shared;        /* shared file */
    H5HG_obj_t	*obj;		/*array of object descriptions	*/
};

/******************************/
/* Package Private Prototypes */
/******************************/
H5_DLL herr_t H5HG_free(H5HG_heap_t *heap);
H5_DLL H5HG_heap_t *H5HG__protect(H5F_t *f, haddr_t addr, unsigned flags);

#endif /* _H5HGpkg_H */

