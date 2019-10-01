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

#ifndef H5TRAV_H__
#define H5TRAV_H__

#include "hdf5.h"

/* Typedefs for visiting objects */
typedef herr_t (*h5trav_obj_func_t)(const char *path_name, const H5O_info_t *oinfo,
        const char *first_seen, void *udata);
typedef herr_t (*h5trav_lnk_func_t)(const char *path_name, const H5L_info_t *linfo,
        void *udata);

/*-------------------------------------------------------------------------
 * public enum to specify type of an object
 * the TYPE can be:
 *    H5TRAV_TYPE_UNKNOWN = -1,
 *    H5TRAV_TYPE_GROUP,            Object is a group
 *    H5TRAV_TYPE_DATASET,          Object is a dataset
 *    H5TRAV_TYPE_TYPE,             Object is a named datatype
 *    H5TRAV_TYPE_LINK,             Object is a symbolic link
 *    H5TRAV_TYPE_UDLINK,           Object is a user-defined link
 *-------------------------------------------------------------------------
 */
typedef enum {
    H5TRAV_TYPE_UNKNOWN = -1,        /* Unknown object type */
    H5TRAV_TYPE_GROUP,          /* Object is a group */
    H5TRAV_TYPE_DATASET,        /* Object is a dataset */
    H5TRAV_TYPE_NAMED_DATATYPE, /* Object is a named datatype */
    H5TRAV_TYPE_LINK,           /* Object is a symbolic link */
    H5TRAV_TYPE_UDLINK          /* Object is a user-defined link */
} h5trav_type_t;

/*-------------------------------------------------------------------------
 * public struct to store name and type of an object
 *-------------------------------------------------------------------------
 */
/* Struct to keep track of symbolic link targets visited.
 * Functions: symlink_visit_add() and symlink_is_visited()
 */
typedef struct symlink_trav_path_t {
    H5L_type_t  type;
    char *file;
    char *path;
} symlink_trav_path_t;

typedef struct symlink_trav_t {
    size_t      nalloc;
    size_t      nused;
    symlink_trav_path_t *objs;
    hbool_t dangle_link;
} symlink_trav_t;

typedef struct trav_path_t {
    char      *path;
    h5trav_type_t type;
    haddr_t     objno;     /* object address */
    unsigned long 	fileno; /* File number that object is located in */
} trav_path_t;

typedef struct trav_info_t {
    size_t      nalloc;
    size_t      nused;
    const char *fname;
    hid_t fid;                          /* File ID */
    trav_path_t *paths;
    symlink_trav_t symlink_visited;     /* already visited symbolic links */
    void * opts;                        /* optional data passing */
} trav_info_t;


/*-------------------------------------------------------------------------
 * keep record of hard link information
 *-------------------------------------------------------------------------
 */
typedef struct trav_link_t {
    char      *new_name;
} trav_link_t;


/*-------------------------------------------------------------------------
 * struct to store basic info needed for the h5trav table traversal algorythm
 *-------------------------------------------------------------------------
 */

typedef struct trav_obj_t {
    haddr_t     objno;     /* object address */
    unsigned    flags[2];  /* h5diff.object is present or not in both files*/
    hbool_t     is_same_trgobj; /* same target object? no need to compare */
    char        *name;     /* name */
    h5trav_type_t type;    /* type of object */
    trav_link_t *links;    /* array of possible link names */
    size_t      sizelinks; /* size of links array */
    size_t      nlinks;    /* number of links */
} trav_obj_t;


/*-------------------------------------------------------------------------
 * private struct that stores all objects
 *-------------------------------------------------------------------------
 */

typedef struct trav_table_t {
    size_t      size;
    size_t      nobjs;
    trav_obj_t *objs;
} trav_table_t;


/*-------------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------
 * "h5trav general" public functions
 *-------------------------------------------------------------------------
 */
H5TOOLS_DLL void h5trav_set_index(H5_index_t print_index_by, H5_iter_order_t print_index_order);
H5TOOLS_DLL int h5trav_visit(hid_t file_id, const char *grp_name, 
    hbool_t visit_start, hbool_t recurse, h5trav_obj_func_t visit_obj, 
    h5trav_lnk_func_t visit_lnk, void *udata, unsigned fields);
H5TOOLS_DLL herr_t symlink_visit_add(symlink_trav_t *visited, H5L_type_t type, const char *file, const char *path);
H5TOOLS_DLL hbool_t symlink_is_visited(symlink_trav_t *visited, H5L_type_t type, const char *file, const char *path);

/*-------------------------------------------------------------------------
 * "h5trav info" public functions
 *-------------------------------------------------------------------------
 */
H5TOOLS_DLL int h5trav_getinfo(hid_t file_id, trav_info_t *info);
H5TOOLS_DLL ssize_t h5trav_getindex(const trav_info_t *info, const char *obj);
H5TOOLS_DLL int trav_info_visit_obj (const char *path, const H5O_info_t *oinfo, const char *already_visited, void *udata);
H5TOOLS_DLL int trav_info_visit_lnk (const char *path, const H5L_info_t *linfo, void *udata);

/*-------------------------------------------------------------------------
 * "h5trav table" public functions
 *-------------------------------------------------------------------------
 */

H5TOOLS_DLL int  h5trav_gettable(hid_t fid, trav_table_t *travt);
H5TOOLS_DLL int  h5trav_getindext(const char *obj, const trav_table_t *travt);

/*-------------------------------------------------------------------------
 * "h5trav print" public functions
 *-------------------------------------------------------------------------
 */
H5TOOLS_DLL int h5trav_print(hid_t fid);
H5TOOLS_DLL void h5trav_set_verbose(int print_verbose);

#ifdef __cplusplus
}
#endif

/*-------------------------------------------------------------------------
 * info private functions
 *-------------------------------------------------------------------------
 */

H5TOOLS_DLL void trav_info_init(const char *filename, hid_t fileid, trav_info_t **info);

H5TOOLS_DLL void trav_info_free(trav_info_t *info);

H5TOOLS_DLL void trav_info_add(trav_info_t *info, const char *path, h5trav_type_t obj_type);

H5TOOLS_DLL void trav_fileinfo_add(trav_info_t *info, hid_t loc_id);

/*-------------------------------------------------------------------------
 * table private functions
 *-------------------------------------------------------------------------
 */

H5TOOLS_DLL void trav_table_init(trav_table_t **table);

H5TOOLS_DLL void trav_table_free(trav_table_t *table);

H5TOOLS_DLL void trav_table_addflags(unsigned *flags,
                         char *objname,
                         h5trav_type_t type,
                         trav_table_t *table);

#endif  /* H5TRAV_H__ */

