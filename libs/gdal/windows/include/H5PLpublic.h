/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * This file contains public declarations for the H5PL module.
 */

#ifndef _H5PLpublic_H
#define _H5PLpublic_H

/* Public headers needed by this file */
#include "H5public.h"          /* Generic Functions                    */

/*******************/
/* Public Typedefs */
/*******************/

/* Special string to indicate no plugin loading.
 */
#define H5PL_NO_PLUGIN          "::"

/* Plugin type used by the plugin library */
typedef enum H5PL_type_t {
    H5PL_TYPE_ERROR         = -1,   /* Error                */
    H5PL_TYPE_FILTER        =  0,   /* Filter               */
    H5PL_TYPE_NONE          =  1    /* This must be last!   */
} H5PL_type_t;

/* Common dynamic plugin type flags used by the set/get_loading_state functions */
#define H5PL_FILTER_PLUGIN      0x0001
#define H5PL_ALL_PLUGIN         0xFFFF

#ifdef __cplusplus
extern "C" {
#endif

/* plugin state */
H5_DLL herr_t H5PLset_loading_state(unsigned int plugin_control_mask);
H5_DLL herr_t H5PLget_loading_state(unsigned int *plugin_control_mask /*out*/);
H5_DLL herr_t H5PLappend(const char *search_path);
H5_DLL herr_t H5PLprepend(const char *search_path);
H5_DLL herr_t H5PLreplace(const char *search_path, unsigned int index);
H5_DLL herr_t H5PLinsert(const char *search_path, unsigned int index);
H5_DLL herr_t H5PLremove(unsigned int index);
H5_DLL ssize_t H5PLget(unsigned int index, char *path_buf /*out*/, size_t buf_size);
H5_DLL herr_t H5PLsize(unsigned int *num_paths /*out*/);

#ifdef __cplusplus
}
#endif

#endif /* _H5PLpublic_H */

