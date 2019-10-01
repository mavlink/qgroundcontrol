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
 * Programmer:  Bill Wendling <wendling@ncsa.uiuc.edu>
 *              Tuesday, 6. March 2001
 *
 * Purpose:     Support functions for the various tools.
 */
#ifndef H5TOOLS_UTILS_H__
#define H5TOOLS_UTILS_H__

#include "hdf5.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ``parallel_print'' information */
#define PRINT_DATA_MAX_SIZE     512
#define OUTBUFF_SIZE        (PRINT_DATA_MAX_SIZE*4)

H5TOOLS_DLLVAR int  g_nTasks;
H5TOOLS_DLLVAR unsigned char g_Parallel;
H5TOOLS_DLLVAR char    outBuff[];
H5TOOLS_DLLVAR unsigned outBuffOffset;
H5TOOLS_DLLVAR FILE *   overflow_file;

/* Maximum size used in a call to malloc for a dataset */
H5TOOLS_DLLVAR hsize_t H5TOOLS_MALLOCSIZE;
/* size of hyperslab buffer when a dataset is bigger than H5TOOLS_MALLOCSIZE */
H5TOOLS_DLLVAR hsize_t H5TOOLS_BUFSIZE;
/*
 * begin get_option section
 */
H5TOOLS_DLLVAR int         opt_err;     /* getoption prints errors if this is on    */
H5TOOLS_DLLVAR int         opt_ind;     /* token pointer                            */
H5TOOLS_DLLVAR const char *opt_arg;     /* flag argument (or value)                 */

enum {
    no_arg = 0,         /* doesn't take an argument     */
    require_arg,        /* requires an argument          */
    optional_arg        /* argument is optional         */
};

/*
 * get_option determines which options are specified on the command line and
 * returns a pointer to any arguments possibly associated with the option in
 * the ``opt_arg'' variable. get_option returns the shortname equivalent of
 * the option. The long options are specified in the following way:
 *
 * struct long_options foo[] = {
 *   { "filename", require_arg, 'f' },
 *   { "append", no_arg, 'a' },
 *   { "width", require_arg, 'w' },
 *   { NULL, 0, 0 }
 * };
 *
 * Long named options can have arguments specified as either:
 *
 *   ``--param=arg'' or ``--param arg''
 *
 * Short named options can have arguments specified as either:
 *
 *   ``-w80'' or ``-w 80''
 *
 * and can have more than one short named option specified at one time:
 *
 *   -aw80
 *
 * in which case those options which expect an argument need to come at the
 * end.
 */
typedef struct long_options {
    const char  *name;          /* name of the long option              */
    int          has_arg;       /* whether we should look for an arg    */
    char         shortval;      /* the shortname equivalent of long arg
                                 * this gets returned from get_option   */
} long_options;

H5TOOLS_DLL int    get_option(int argc, const char **argv, const char *opt,
                         const struct long_options *l_opt);
/*
 * end get_option section
 */

/*struct taken from the dumper. needed in table struct*/
typedef struct obj_t {
    haddr_t objno;
    char *objname;
    hbool_t displayed;          /* Flag to indicate that the object has been displayed */
    hbool_t recorded;           /* Flag for named datatypes to indicate they were found in the group hierarchy */
} obj_t;

/*struct for the tables that the find_objs function uses*/
typedef struct table_t {
    size_t size;
    size_t nobjs;
    obj_t *objs;
} table_t;

/*this struct stores the information that is passed to the find_objs function*/
typedef struct find_objs_t {
    hid_t fid;
    table_t *group_table;
    table_t *type_table;
    table_t *dset_table;
} find_objs_t;

H5TOOLS_DLLVAR unsigned h5tools_nCols;               /*max number of columns for outputting  */

/* Definitions of useful routines */
H5TOOLS_DLL void     indentation(unsigned);
H5TOOLS_DLL void     print_version(const char *progname);
H5TOOLS_DLL void     parallel_print(const char* format, ... );
H5TOOLS_DLL void     error_msg(const char *fmt, ...);
H5TOOLS_DLL void     warn_msg(const char *fmt, ...);
H5TOOLS_DLL void     help_ref_msg(FILE *output);
H5TOOLS_DLL void     free_table(table_t *table);
#ifdef H5DUMP_DEBUG
H5TOOLS_DLL void     dump_tables(find_objs_t *info)
#endif  /* H5DUMP_DEBUG */
H5TOOLS_DLL herr_t init_objs(hid_t fid, find_objs_t *info, table_t **group_table,
    table_t **dset_table, table_t **type_table);
H5TOOLS_DLL obj_t   *search_obj(table_t *temp, haddr_t objno);
#ifndef H5_HAVE_TMPFILE
H5TOOLS_DLL FILE *  tmpfile(void);
#endif

/*************************************************************
 *
 * candidate functions to be public
 *
 *************************************************************/

/* This code is layout for common code among tools */
typedef enum toolname_t {
    TOOL_H5DIFF, TOOL_H5LS, TOOL__H5DUMP /* add as necessary */
} h5tool_toolname_t;

/* this struct can be used to differntiate among tools */
typedef struct {
    h5tool_toolname_t toolname;
    int msg_mode;
} h5tool_opt_t;

/* obtain link info from H5tools_get_symlink_info() */
typedef struct {
    H5O_type_t  trg_type;  /* OUT: target type */
    char *trg_path;        /* OUT: target obj path. This must be freed
                            *      when used with H5tools_get_symlink_info() */
    haddr_t     objno;     /* OUT: target object address */
    unsigned long  fileno; /* OUT: File number that target object is located in */
    H5L_info_t linfo;      /* OUT: link info */
    h5tool_opt_t opt;      /* IN: options */
} h5tool_link_info_t;


/* Definitions of routines */
H5TOOLS_DLL int H5tools_get_symlink_info(hid_t file_id, const char * linkpath,
    h5tool_link_info_t *link_info, hbool_t get_obj_type);
H5TOOLS_DLL const char *h5tools_getprogname(void);
H5TOOLS_DLL void     h5tools_setprogname(const char*progname);
H5TOOLS_DLL int      h5tools_getstatus(void);
H5TOOLS_DLL void     h5tools_setstatus(int d_status);
H5TOOLS_DLL int h5tools_getenv_update_hyperslab_bufsize(void);
#ifdef __cplusplus
}
#endif

#endif  /* H5TOOLS_UTILS_H__ */
