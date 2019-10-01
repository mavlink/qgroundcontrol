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

/* Generated automatically by bin/make_err -- do not edit */
/* Add new errors to H5err.txt file */


#ifndef _H5Edefin_H
#define _H5Edefin_H

/* Major error IDs */
hid_t H5E_REFERENCE_g      = FAIL;      /* References */
hid_t H5E_ARGS_g           = FAIL;      /* Invalid arguments to routine */
hid_t H5E_FUNC_g           = FAIL;      /* Function entry/exit */
hid_t H5E_PLINE_g          = FAIL;      /* Data filters */
hid_t H5E_ATOM_g           = FAIL;      /* Object atom */
hid_t H5E_IO_g             = FAIL;      /* Low-level I/O */
hid_t H5E_SOHM_g           = FAIL;      /* Shared Object Header Messages */
hid_t H5E_VFL_g            = FAIL;      /* Virtual File Layer */
hid_t H5E_PLIST_g          = FAIL;      /* Property lists */
hid_t H5E_ATTR_g           = FAIL;      /* Attribute */
hid_t H5E_PAGEBUF_g        = FAIL;      /* Page Buffering */
hid_t H5E_DATATYPE_g       = FAIL;      /* Datatype */
hid_t H5E_EFL_g            = FAIL;      /* External file list */
hid_t H5E_RESOURCE_g       = FAIL;      /* Resource unavailable */
hid_t H5E_EARRAY_g         = FAIL;      /* Extensible Array */
hid_t H5E_OHDR_g           = FAIL;      /* Object header */
hid_t H5E_FILE_g           = FAIL;      /* File accessibilty */
hid_t H5E_STORAGE_g        = FAIL;      /* Data storage */
hid_t H5E_CACHE_g          = FAIL;      /* Object cache */
hid_t H5E_NONE_MAJOR_g     = FAIL;      /* No error */
hid_t H5E_PLUGIN_g         = FAIL;      /* Plugin for dynamically loaded library */
hid_t H5E_TST_g            = FAIL;      /* Ternary Search Trees */
hid_t H5E_CONTEXT_g        = FAIL;      /* API Context */
hid_t H5E_INTERNAL_g       = FAIL;      /* Internal error (too specific to document in detail) */
hid_t H5E_HEAP_g           = FAIL;      /* Heap */
hid_t H5E_RS_g             = FAIL;      /* Reference Counted Strings */
hid_t H5E_DATASET_g        = FAIL;      /* Dataset */
hid_t H5E_LINK_g           = FAIL;      /* Links */
hid_t H5E_ERROR_g          = FAIL;      /* Error API */
hid_t H5E_FARRAY_g         = FAIL;      /* Fixed Array */
hid_t H5E_SLIST_g          = FAIL;      /* Skip Lists */
hid_t H5E_SYM_g            = FAIL;      /* Symbol table */
hid_t H5E_DATASPACE_g      = FAIL;      /* Dataspace */
hid_t H5E_FSPACE_g         = FAIL;      /* Free Space Manager */
hid_t H5E_BTREE_g          = FAIL;      /* B-Tree node */

/* Minor error IDs */

/* Object atom related errors */
hid_t H5E_BADATOM_g        = FAIL;      /* Unable to find atom information (already closed?) */
hid_t H5E_BADGROUP_g       = FAIL;      /* Unable to find ID group information */
hid_t H5E_CANTREGISTER_g   = FAIL;      /* Unable to register new atom */
hid_t H5E_CANTINC_g        = FAIL;      /* Unable to increment reference count */
hid_t H5E_CANTDEC_g        = FAIL;      /* Unable to decrement reference count */
hid_t H5E_NOIDS_g          = FAIL;      /* Out of IDs for group */

/* File accessibilty errors */
hid_t H5E_FILEEXISTS_g     = FAIL;      /* File already exists */
hid_t H5E_FILEOPEN_g       = FAIL;      /* File already open */
hid_t H5E_CANTCREATE_g     = FAIL;      /* Unable to create file */
hid_t H5E_CANTOPENFILE_g   = FAIL;      /* Unable to open file */
hid_t H5E_CANTCLOSEFILE_g  = FAIL;      /* Unable to close file */
hid_t H5E_NOTHDF5_g        = FAIL;      /* Not an HDF5 file */
hid_t H5E_BADFILE_g        = FAIL;      /* Bad file ID accessed */
hid_t H5E_TRUNCATED_g      = FAIL;      /* File has been truncated */
hid_t H5E_MOUNT_g          = FAIL;      /* File mount error */

/* Parallel MPI errors */
hid_t H5E_MPI_g            = FAIL;      /* Some MPI function failed */
hid_t H5E_MPIERRSTR_g      = FAIL;      /* MPI Error String */
hid_t H5E_CANTRECV_g       = FAIL;      /* Can't receive data */
hid_t H5E_CANTGATHER_g     = FAIL;      /* Can't gather data */
hid_t H5E_NO_INDEPENDENT_g = FAIL;      /* Can't perform independent IO */

/* Heap errors */
hid_t H5E_CANTRESTORE_g    = FAIL;      /* Can't restore condition */
hid_t H5E_CANTCOMPUTE_g    = FAIL;      /* Can't compute value */
hid_t H5E_CANTEXTEND_g     = FAIL;      /* Can't extend heap's space */
hid_t H5E_CANTATTACH_g     = FAIL;      /* Can't attach object */
hid_t H5E_CANTUPDATE_g     = FAIL;      /* Can't update object */
hid_t H5E_CANTOPERATE_g    = FAIL;      /* Can't operate on object */

/* B-tree related errors */
hid_t H5E_NOTFOUND_g       = FAIL;      /* Object not found */
hid_t H5E_EXISTS_g         = FAIL;      /* Object already exists */
hid_t H5E_CANTENCODE_g     = FAIL;      /* Unable to encode value */
hid_t H5E_CANTDECODE_g     = FAIL;      /* Unable to decode value */
hid_t H5E_CANTSPLIT_g      = FAIL;      /* Unable to split node */
hid_t H5E_CANTREDISTRIBUTE_g = FAIL;      /* Unable to redistribute records */
hid_t H5E_CANTSWAP_g       = FAIL;      /* Unable to swap records */
hid_t H5E_CANTINSERT_g     = FAIL;      /* Unable to insert object */
hid_t H5E_CANTLIST_g       = FAIL;      /* Unable to list node */
hid_t H5E_CANTMODIFY_g     = FAIL;      /* Unable to modify record */
hid_t H5E_CANTREMOVE_g     = FAIL;      /* Unable to remove object */

/* Function entry/exit interface errors */
hid_t H5E_CANTINIT_g       = FAIL;      /* Unable to initialize object */
hid_t H5E_ALREADYINIT_g    = FAIL;      /* Object already initialized */
hid_t H5E_CANTRELEASE_g    = FAIL;      /* Unable to release object */

/* Plugin errors */
hid_t H5E_OPENERROR_g      = FAIL;      /* Can't open directory or file */

/* I/O pipeline errors */
hid_t H5E_NOFILTER_g       = FAIL;      /* Requested filter is not available */
hid_t H5E_CALLBACK_g       = FAIL;      /* Callback failed */
hid_t H5E_CANAPPLY_g       = FAIL;      /* Error from filter 'can apply' callback */
hid_t H5E_SETLOCAL_g       = FAIL;      /* Error from filter 'set local' callback */
hid_t H5E_NOENCODER_g      = FAIL;      /* Filter present but encoding disabled */
hid_t H5E_CANTFILTER_g     = FAIL;      /* Filter operation failed */

/* Argument errors */
hid_t H5E_UNINITIALIZED_g  = FAIL;      /* Information is uinitialized */
hid_t H5E_UNSUPPORTED_g    = FAIL;      /* Feature is unsupported */
hid_t H5E_BADTYPE_g        = FAIL;      /* Inappropriate type */
hid_t H5E_BADRANGE_g       = FAIL;      /* Out of range */
hid_t H5E_BADVALUE_g       = FAIL;      /* Bad value */

/* Generic low-level file I/O errors */
hid_t H5E_SEEKERROR_g      = FAIL;      /* Seek failed */
hid_t H5E_READERROR_g      = FAIL;      /* Read failed */
hid_t H5E_WRITEERROR_g     = FAIL;      /* Write failed */
hid_t H5E_CLOSEERROR_g     = FAIL;      /* Close failed */
hid_t H5E_OVERFLOW_g       = FAIL;      /* Address overflowed */
hid_t H5E_FCNTL_g          = FAIL;      /* File control (fcntl) failed */

/* Resource errors */
hid_t H5E_NOSPACE_g        = FAIL;      /* No space available for allocation */
hid_t H5E_CANTALLOC_g      = FAIL;      /* Can't allocate space */
hid_t H5E_CANTCOPY_g       = FAIL;      /* Unable to copy object */
hid_t H5E_CANTFREE_g       = FAIL;      /* Unable to free object */
hid_t H5E_ALREADYEXISTS_g  = FAIL;      /* Object already exists */
hid_t H5E_CANTLOCK_g       = FAIL;      /* Unable to lock object */
hid_t H5E_CANTUNLOCK_g     = FAIL;      /* Unable to unlock object */
hid_t H5E_CANTGC_g         = FAIL;      /* Unable to garbage collect */
hid_t H5E_CANTGETSIZE_g    = FAIL;      /* Unable to compute size */
hid_t H5E_OBJOPEN_g        = FAIL;      /* Object is already open */

/* Group related errors */
hid_t H5E_CANTOPENOBJ_g    = FAIL;      /* Can't open object */
hid_t H5E_CANTCLOSEOBJ_g   = FAIL;      /* Can't close object */
hid_t H5E_COMPLEN_g        = FAIL;      /* Name component is too long */
hid_t H5E_PATH_g           = FAIL;      /* Problem with path to object */

/* Datatype conversion errors */
hid_t H5E_CANTCONVERT_g    = FAIL;      /* Can't convert datatypes */
hid_t H5E_BADSIZE_g        = FAIL;      /* Bad size for object */

/* System level errors */
hid_t H5E_SYSERRSTR_g      = FAIL;      /* System error message */

/* Dataspace errors */
hid_t H5E_CANTCLIP_g       = FAIL;      /* Can't clip hyperslab region */
hid_t H5E_CANTCOUNT_g      = FAIL;      /* Can't count elements */
hid_t H5E_CANTSELECT_g     = FAIL;      /* Can't select hyperslab */
hid_t H5E_CANTNEXT_g       = FAIL;      /* Can't move to next iterator location */
hid_t H5E_BADSELECT_g      = FAIL;      /* Invalid selection */
hid_t H5E_CANTCOMPARE_g    = FAIL;      /* Can't compare objects */
hid_t H5E_CANTAPPEND_g     = FAIL;      /* Can't append object */

/* Cache related errors */
hid_t H5E_CANTFLUSH_g      = FAIL;      /* Unable to flush data from cache */
hid_t H5E_CANTUNSERIALIZE_g = FAIL;      /* Unable to mark metadata as unserialized */
hid_t H5E_CANTSERIALIZE_g  = FAIL;      /* Unable to serialize data from cache */
hid_t H5E_CANTTAG_g        = FAIL;      /* Unable to tag metadata in the cache */
hid_t H5E_CANTLOAD_g       = FAIL;      /* Unable to load metadata into cache */
hid_t H5E_PROTECT_g        = FAIL;      /* Protected metadata error */
hid_t H5E_NOTCACHED_g      = FAIL;      /* Metadata not currently cached */
hid_t H5E_SYSTEM_g         = FAIL;      /* Internal error detected */
hid_t H5E_CANTINS_g        = FAIL;      /* Unable to insert metadata into cache */
hid_t H5E_CANTPROTECT_g    = FAIL;      /* Unable to protect metadata */
hid_t H5E_CANTUNPROTECT_g  = FAIL;      /* Unable to unprotect metadata */
hid_t H5E_CANTPIN_g        = FAIL;      /* Unable to pin cache entry */
hid_t H5E_CANTUNPIN_g      = FAIL;      /* Unable to un-pin cache entry */
hid_t H5E_CANTMARKDIRTY_g  = FAIL;      /* Unable to mark a pinned entry as dirty */
hid_t H5E_CANTMARKCLEAN_g  = FAIL;      /* Unable to mark a pinned entry as clean */
hid_t H5E_CANTMARKUNSERIALIZED_g = FAIL;      /* Unable to mark an entry as unserialized */
hid_t H5E_CANTMARKSERIALIZED_g = FAIL;      /* Unable to mark an entry as serialized */
hid_t H5E_CANTDIRTY_g      = FAIL;      /* Unable to mark metadata as dirty */
hid_t H5E_CANTCLEAN_g      = FAIL;      /* Unable to mark metadata as clean */
hid_t H5E_CANTEXPUNGE_g    = FAIL;      /* Unable to expunge a metadata cache entry */
hid_t H5E_CANTRESIZE_g     = FAIL;      /* Unable to resize a metadata cache entry */
hid_t H5E_CANTDEPEND_g     = FAIL;      /* Unable to create a flush dependency */
hid_t H5E_CANTUNDEPEND_g   = FAIL;      /* Unable to destroy a flush dependency */
hid_t H5E_CANTNOTIFY_g     = FAIL;      /* Unable to notify object about action */
hid_t H5E_LOGFAIL_g        = FAIL;      /* Failure in the cache logging framework */
hid_t H5E_CANTCORK_g       = FAIL;      /* Unable to cork an object */
hid_t H5E_CANTUNCORK_g     = FAIL;      /* Unable to uncork an object */

/* Link related errors */
hid_t H5E_TRAVERSE_g       = FAIL;      /* Link traversal failure */
hid_t H5E_NLINKS_g         = FAIL;      /* Too many soft links in path */
hid_t H5E_NOTREGISTERED_g  = FAIL;      /* Link class not registered */
hid_t H5E_CANTMOVE_g       = FAIL;      /* Can't move object */
hid_t H5E_CANTSORT_g       = FAIL;      /* Can't sort objects */

/* Free space errors */
hid_t H5E_CANTMERGE_g      = FAIL;      /* Can't merge objects */
hid_t H5E_CANTREVIVE_g     = FAIL;      /* Can't revive object */
hid_t H5E_CANTSHRINK_g     = FAIL;      /* Can't shrink container */

/* Property list errors */
hid_t H5E_CANTGET_g        = FAIL;      /* Can't get value */
hid_t H5E_CANTSET_g        = FAIL;      /* Can't set value */
hid_t H5E_DUPCLASS_g       = FAIL;      /* Duplicate class name in parent class */
hid_t H5E_SETDISALLOWED_g  = FAIL;      /* Disallowed operation */

/* No error */
hid_t H5E_NONE_MINOR_g     = FAIL;      /* No error */

/* Object header related errors */
hid_t H5E_LINKCOUNT_g      = FAIL;      /* Bad object header link count */
hid_t H5E_VERSION_g        = FAIL;      /* Wrong version number */
hid_t H5E_ALIGNMENT_g      = FAIL;      /* Alignment error */
hid_t H5E_BADMESG_g        = FAIL;      /* Unrecognized message */
hid_t H5E_CANTDELETE_g     = FAIL;      /* Can't delete message */
hid_t H5E_BADITER_g        = FAIL;      /* Iteration failed */
hid_t H5E_CANTPACK_g       = FAIL;      /* Can't pack messages */
hid_t H5E_CANTRESET_g      = FAIL;      /* Can't reset object */
hid_t H5E_CANTRENAME_g     = FAIL;      /* Unable to rename object */

#endif /* H5Edefin_H */
