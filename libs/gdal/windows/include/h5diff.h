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

#ifndef H5DIFF_H__
#define H5DIFF_H__

#include "hdf5.h"
#include "h5trav.h"

/*
 * Debug printf macros. The prefix allows output filtering by test scripts.
 */
#ifdef H5DIFF_DEBUG
#define h5difftrace(x) HDfprintf(stderr, "h5diff debug: " x)
#define h5diffdebug2(x1, x2) HDfprintf(stderr, "h5diff debug: " x1, x2)
#define h5diffdebug3(x1, x2, x3) HDfprintf(stderr, "h5diff debug: " x1, x2, x3)
#define h5diffdebug4(x1, x2, x3, x4) HDfprintf(stderr, "h5diff debug: " x1, x2, x3, x4)
#define h5diffdebug5(x1, x2, x3, x4, x5) HDfprintf(stderr, "h5diff debug: " x1, x2, x3, x4, x5)
#else
#define h5difftrace(x)
#define h5diffdebug2(x1, x2)
#define h5diffdebug3(x1, x2, x3)
#define h5diffdebug4(x1, x2, x3, x4)
#define h5diffdebug5(x1, x2, x3, x4, x5)
#endif

#define MAX_FILENAME 1024

/*-------------------------------------------------------------------------
 * This is used to pass multiple args into diff().
 * Passing this instead of several each arg provides smoother extensibility
 * through its members along with MPI code for ph5diff
 * as it doesn't require interface change.
 *------------------------------------------------------------------------*/
typedef struct {
    h5trav_type_t   type[2];
    hbool_t is_same_trgobj;
} diff_args_t;
/*-------------------------------------------------------------------------
 * command line options
 *-------------------------------------------------------------------------
 */
/* linked list to keep exclude path list */
struct exclude_path_list {
    char  *obj_path;
    h5trav_type_t obj_type;
    struct exclude_path_list * next;
};

typedef struct {
    int      m_quiet;               /* quiet mide: no output at all */
    int      m_report;              /* report mode: print the data */
    int      m_verbose;             /* verbose mode: print the data, list of objcets, warnings */
    int      m_verbose_level;       /* control verbose details */
    int      d;                     /* delta, absolute value to compare */
    double   delta;                 /* delta value */
    int      p;                     /* relative error to compare*/
    int      use_system_epsilon;    /* flag to use system epsilon (1 or 0) */
    double   percent;               /* relative error value */
    int      n;                     /* count, compare up to count */
    hsize_t  count;                 /* count value */
    hbool_t  follow_links;          /* follow symbolic links */
    int      no_dangle_links;       /* return error when find dangling link */
    int      err_stat;              /* an error ocurred (1, error, 0, no error) */
    int      cmn_objs;              /* do we have common objects */
    int      not_cmp;               /* are the objects comparable */
    int      contents;              /* equal contents */
    int      do_nans;               /* consider Nans while diffing floats */
    int      m_list_not_cmp;        /* list not comparable messages */
    int      exclude_path;          /* exclude path to an object */
    struct   exclude_path_list * exclude; /* keep exclude path list */
} diff_opt_t;


/*-------------------------------------------------------------------------
 * public functions
 *-------------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

H5TOOLS_DLL hsize_t  h5diff(const char *fname1,
                const char *fname2,
                const char *objname1,
                const char *objname2,
                diff_opt_t *opts);

H5TOOLS_DLL hsize_t diff( hid_t      file1_id,
              const char *path1,
              hid_t      file2_id,
              const char *path2,
              diff_opt_t *opts,
              diff_args_t *argdata);

#ifdef H5_HAVE_PARALLEL
H5TOOLS_DLL void phdiff_dismiss_workers(void);
H5TOOLS_DLL void print_manager_output(void);
#endif

#ifdef __cplusplus
}
#endif



/*-------------------------------------------------------------------------
 * private functions
 *-------------------------------------------------------------------------
 */


hsize_t diff_dataset( hid_t file1_id,
                      hid_t file2_id,
                      const char *obj1_name,
                      const char *obj2_name,
                      diff_opt_t *opts);

hsize_t diff_datasetid( hid_t dset1_id,
                        hid_t dset2_id,
                        const char *obj1_name,
                        const char *obj2_name,
                        diff_opt_t *opts);


hsize_t diff_match( hid_t file1_id, const char *grp1, trav_info_t *info1,
                    hid_t file2_id, const char *grp2, trav_info_t *info2,
                    trav_table_t *table, diff_opt_t *opts );

hsize_t diff_array( void *_mem1,
                    void *_mem2,
                    hsize_t nelmts,
                    hsize_t hyper_start,
                    int rank,
                    hsize_t *dims,
                    diff_opt_t *opts,
                    const char *name1,
                    const char *name2,
                    hid_t m_type,
                    hid_t container1_id,
                    hid_t container2_id); /* dataset where the reference came from*/


int diff_can_type( hid_t       f_type1, /* file data type */
                   hid_t       f_type2, /* file data type */
                   int         rank1,
                   int         rank2,
                   hsize_t     *dims1,
                   hsize_t     *dims2,
                   hsize_t     *maxdim1,
                   hsize_t     *maxdim2,
                   const char  *obj1_name,
                   const char  *obj2_name,
                   diff_opt_t  *opts,
                   int         is_compound);


hsize_t diff_attr(hid_t loc1_id,
                  hid_t loc2_id,
                  const char *path1,
                  const char *path2,
                  diff_opt_t *opts);


/*-------------------------------------------------------------------------
 * utility functions
 *-------------------------------------------------------------------------
 */

/* in h5diff_util.c */
void        print_found(hsize_t nfound);
void        print_type(hid_t type);
const char* diff_basename(const char *name);
const char* get_type(h5trav_type_t type);
const char* get_class(H5T_class_t tclass);
const char* get_sign(H5T_sign_t sign);
void        print_dimensions (int rank, hsize_t *dims);
herr_t      match_up_memsize (hid_t f_tid1_id, hid_t f_tid2_id,
                              hid_t *m_tid1, hid_t *m_tid2,
                              size_t *m_size1, size_t  *m_size2);
/* in h5diff.c */
int         print_objname(diff_opt_t *opts, hsize_t nfound);
void        do_print_objname (const char *OBJ, const char *path1, const char *path2, diff_opt_t * opts);
void        do_print_attrname (const char *attr, const char *path1, const char *path2);

#endif  /* H5DIFF_H__ */

