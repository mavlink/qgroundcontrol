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


#ifndef _H5Einit_H
#define _H5Einit_H

/*********************/
/* Major error codes */
/*********************/

assert(H5E_REFERENCE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "References"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_REFERENCE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_ARGS_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Invalid arguments to routine"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_ARGS_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_FUNC_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Function entry/exit"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_FUNC_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_PLINE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Data filters"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_PLINE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_ATOM_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Object atom"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_ATOM_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_IO_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Low-level I/O"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_IO_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_SOHM_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Shared Object Header Messages"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_SOHM_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_VFL_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Virtual File Layer"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_VFL_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_PLIST_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Property lists"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_PLIST_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_ATTR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Attribute"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_ATTR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_PAGEBUF_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Page Buffering"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_PAGEBUF_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_DATATYPE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Datatype"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_DATATYPE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_EFL_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "External file list"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_EFL_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_RESOURCE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Resource unavailable"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_RESOURCE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_EARRAY_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Extensible Array"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_EARRAY_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_OHDR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Object header"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_OHDR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_FILE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "File accessibilty"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_FILE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_STORAGE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Data storage"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_STORAGE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CACHE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Object cache"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CACHE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_NONE_MAJOR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "No error"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NONE_MAJOR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_PLUGIN_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Plugin for dynamically loaded library"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_PLUGIN_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_TST_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Ternary Search Trees"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_TST_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CONTEXT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "API Context"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CONTEXT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_INTERNAL_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Internal error (too specific to document in detail)"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_INTERNAL_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_HEAP_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Heap"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_HEAP_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_RS_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Reference Counted Strings"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_RS_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_DATASET_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Dataset"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_DATASET_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_LINK_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Links"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_LINK_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_ERROR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Error API"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_ERROR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_FARRAY_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Fixed Array"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_FARRAY_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_SLIST_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Skip Lists"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_SLIST_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_SYM_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Symbol table"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_SYM_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_DATASPACE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Dataspace"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_DATASPACE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_FSPACE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "Free Space Manager"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_FSPACE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BTREE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MAJOR, "B-Tree node"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BTREE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/*********************/
/* Minor error codes */
/*********************/


/* Object atom related errors */
assert(H5E_BADATOM_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to find atom information (already closed?)"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADATOM_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BADGROUP_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to find ID group information"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADGROUP_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTREGISTER_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to register new atom"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTREGISTER_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTINC_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to increment reference count"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTINC_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTDEC_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to decrement reference count"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTDEC_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_NOIDS_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Out of IDs for group"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NOIDS_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* File accessibilty errors */
assert(H5E_FILEEXISTS_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "File already exists"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_FILEEXISTS_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_FILEOPEN_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "File already open"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_FILEOPEN_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTCREATE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to create file"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCREATE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTOPENFILE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to open file"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTOPENFILE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTCLOSEFILE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to close file"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCLOSEFILE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_NOTHDF5_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Not an HDF5 file"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NOTHDF5_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BADFILE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Bad file ID accessed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADFILE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_TRUNCATED_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "File has been truncated"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_TRUNCATED_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_MOUNT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "File mount error"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_MOUNT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Parallel MPI errors */
assert(H5E_MPI_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Some MPI function failed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_MPI_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_MPIERRSTR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "MPI Error String"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_MPIERRSTR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTRECV_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't receive data"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTRECV_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTGATHER_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't gather data"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTGATHER_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_NO_INDEPENDENT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't perform independent IO"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NO_INDEPENDENT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Heap errors */
assert(H5E_CANTRESTORE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't restore condition"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTRESTORE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTCOMPUTE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't compute value"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCOMPUTE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTEXTEND_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't extend heap's space"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTEXTEND_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTATTACH_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't attach object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTATTACH_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTUPDATE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't update object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTUPDATE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTOPERATE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't operate on object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTOPERATE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* B-tree related errors */
assert(H5E_NOTFOUND_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Object not found"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NOTFOUND_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_EXISTS_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Object already exists"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_EXISTS_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTENCODE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to encode value"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTENCODE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTDECODE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to decode value"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTDECODE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTSPLIT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to split node"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTSPLIT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTREDISTRIBUTE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to redistribute records"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTREDISTRIBUTE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTSWAP_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to swap records"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTSWAP_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTINSERT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to insert object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTINSERT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTLIST_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to list node"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTLIST_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTMODIFY_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to modify record"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTMODIFY_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTREMOVE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to remove object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTREMOVE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Function entry/exit interface errors */
assert(H5E_CANTINIT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to initialize object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTINIT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_ALREADYINIT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Object already initialized"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_ALREADYINIT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTRELEASE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to release object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTRELEASE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Plugin errors */
assert(H5E_OPENERROR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't open directory or file"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_OPENERROR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* I/O pipeline errors */
assert(H5E_NOFILTER_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Requested filter is not available"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NOFILTER_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CALLBACK_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Callback failed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CALLBACK_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANAPPLY_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Error from filter 'can apply' callback"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANAPPLY_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_SETLOCAL_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Error from filter 'set local' callback"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_SETLOCAL_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_NOENCODER_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Filter present but encoding disabled"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NOENCODER_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTFILTER_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Filter operation failed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTFILTER_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Argument errors */
assert(H5E_UNINITIALIZED_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Information is uinitialized"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_UNINITIALIZED_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_UNSUPPORTED_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Feature is unsupported"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_UNSUPPORTED_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BADTYPE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Inappropriate type"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADTYPE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BADRANGE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Out of range"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADRANGE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BADVALUE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Bad value"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADVALUE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Generic low-level file I/O errors */
assert(H5E_SEEKERROR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Seek failed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_SEEKERROR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_READERROR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Read failed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_READERROR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_WRITEERROR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Write failed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_WRITEERROR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CLOSEERROR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Close failed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CLOSEERROR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_OVERFLOW_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Address overflowed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_OVERFLOW_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_FCNTL_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "File control (fcntl) failed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_FCNTL_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Resource errors */
assert(H5E_NOSPACE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "No space available for allocation"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NOSPACE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTALLOC_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't allocate space"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTALLOC_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTCOPY_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to copy object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCOPY_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTFREE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to free object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTFREE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_ALREADYEXISTS_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Object already exists"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_ALREADYEXISTS_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTLOCK_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to lock object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTLOCK_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTUNLOCK_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to unlock object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTUNLOCK_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTGC_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to garbage collect"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTGC_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTGETSIZE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to compute size"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTGETSIZE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_OBJOPEN_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Object is already open"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_OBJOPEN_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Group related errors */
assert(H5E_CANTOPENOBJ_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't open object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTOPENOBJ_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTCLOSEOBJ_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't close object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCLOSEOBJ_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_COMPLEN_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Name component is too long"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_COMPLEN_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_PATH_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Problem with path to object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_PATH_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Datatype conversion errors */
assert(H5E_CANTCONVERT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't convert datatypes"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCONVERT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BADSIZE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Bad size for object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADSIZE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* System level errors */
assert(H5E_SYSERRSTR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "System error message"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_SYSERRSTR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Dataspace errors */
assert(H5E_CANTCLIP_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't clip hyperslab region"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCLIP_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTCOUNT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't count elements"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCOUNT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTSELECT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't select hyperslab"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTSELECT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTNEXT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't move to next iterator location"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTNEXT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BADSELECT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Invalid selection"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADSELECT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTCOMPARE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't compare objects"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCOMPARE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTAPPEND_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't append object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTAPPEND_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Cache related errors */
assert(H5E_CANTFLUSH_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to flush data from cache"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTFLUSH_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTUNSERIALIZE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to mark metadata as unserialized"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTUNSERIALIZE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTSERIALIZE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to serialize data from cache"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTSERIALIZE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTTAG_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to tag metadata in the cache"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTTAG_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTLOAD_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to load metadata into cache"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTLOAD_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_PROTECT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Protected metadata error"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_PROTECT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_NOTCACHED_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Metadata not currently cached"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NOTCACHED_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_SYSTEM_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Internal error detected"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_SYSTEM_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTINS_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to insert metadata into cache"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTINS_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTPROTECT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to protect metadata"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTPROTECT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTUNPROTECT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to unprotect metadata"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTUNPROTECT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTPIN_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to pin cache entry"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTPIN_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTUNPIN_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to un-pin cache entry"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTUNPIN_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTMARKDIRTY_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to mark a pinned entry as dirty"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTMARKDIRTY_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTMARKCLEAN_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to mark a pinned entry as clean"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTMARKCLEAN_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTMARKUNSERIALIZED_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to mark an entry as unserialized"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTMARKUNSERIALIZED_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTMARKSERIALIZED_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to mark an entry as serialized"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTMARKSERIALIZED_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTDIRTY_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to mark metadata as dirty"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTDIRTY_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTCLEAN_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to mark metadata as clean"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCLEAN_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTEXPUNGE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to expunge a metadata cache entry"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTEXPUNGE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTRESIZE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to resize a metadata cache entry"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTRESIZE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTDEPEND_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to create a flush dependency"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTDEPEND_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTUNDEPEND_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to destroy a flush dependency"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTUNDEPEND_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTNOTIFY_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to notify object about action"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTNOTIFY_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_LOGFAIL_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Failure in the cache logging framework"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_LOGFAIL_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTCORK_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to cork an object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTCORK_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTUNCORK_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to uncork an object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTUNCORK_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Link related errors */
assert(H5E_TRAVERSE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Link traversal failure"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_TRAVERSE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_NLINKS_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Too many soft links in path"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NLINKS_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_NOTREGISTERED_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Link class not registered"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NOTREGISTERED_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTMOVE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't move object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTMOVE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTSORT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't sort objects"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTSORT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Free space errors */
assert(H5E_CANTMERGE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't merge objects"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTMERGE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTREVIVE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't revive object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTREVIVE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTSHRINK_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't shrink container"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTSHRINK_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Property list errors */
assert(H5E_CANTGET_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't get value"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTGET_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTSET_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't set value"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTSET_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_DUPCLASS_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Duplicate class name in parent class"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_DUPCLASS_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_SETDISALLOWED_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Disallowed operation"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_SETDISALLOWED_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* No error */
assert(H5E_NONE_MINOR_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "No error"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_NONE_MINOR_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

/* Object header related errors */
assert(H5E_LINKCOUNT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Bad object header link count"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_LINKCOUNT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_VERSION_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Wrong version number"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_VERSION_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_ALIGNMENT_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Alignment error"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_ALIGNMENT_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BADMESG_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unrecognized message"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADMESG_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTDELETE_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't delete message"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTDELETE_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_BADITER_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Iteration failed"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_BADITER_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTPACK_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't pack messages"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTPACK_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTRESET_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Can't reset object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTRESET_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")
assert(H5E_CANTRENAME_g==(-1));
if((msg = H5E_create_msg(cls, H5E_MINOR, "Unable to rename object"))==NULL)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTINIT, FAIL, "error message initialization failed")
if((H5E_CANTRENAME_g = H5I_register(H5I_ERROR_MSG, msg, FALSE))<0)
    HGOTO_ERROR(H5E_ERROR, H5E_CANTREGISTER, FAIL, "can't register error message")

#endif /* H5Einit_H */
