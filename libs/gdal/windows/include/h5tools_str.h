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
 *              Monday, 19. February 2001
 */
#ifndef H5TOOLS_STR_H__
#define H5TOOLS_STR_H__

typedef struct h5tools_str_t {
    char    *s;     /*allocate string       */
    size_t  len;        /*length of actual value    */
    size_t  nalloc;     /*allocated size of string  */
} h5tools_str_t;

H5TOOLS_DLL void     h5tools_str_close(h5tools_str_t *str);
H5TOOLS_DLL size_t   h5tools_str_len(h5tools_str_t *str);
H5TOOLS_DLL char    *h5tools_str_append(h5tools_str_t *str, const char *fmt, ...);
H5TOOLS_DLL char    *h5tools_str_reset(h5tools_str_t *str);
H5TOOLS_DLL char    *h5tools_str_trunc(h5tools_str_t *str, size_t size);
H5TOOLS_DLL char    *h5tools_str_fmt(h5tools_str_t *str, size_t start, const char *fmt);
H5TOOLS_DLL char    *h5tools_str_prefix(h5tools_str_t *str, const h5tool_format_t *info,
                        hsize_t elmtno, unsigned ndims, h5tools_context_t *ctx);
/*
 * new functions needed to display region reference data
 */
H5TOOLS_DLL char    *h5tools_str_region_prefix(h5tools_str_t *str, const h5tool_format_t *info,
                                   hsize_t elmtno, hsize_t *ptdata, unsigned ndims, 
                                   hsize_t max_idx[], h5tools_context_t *ctx);
H5TOOLS_DLL void     h5tools_str_dump_space_slabs(h5tools_str_t *, hid_t, const h5tool_format_t *, h5tools_context_t *ctx);
H5TOOLS_DLL void     h5tools_str_dump_space_blocks(h5tools_str_t *, hid_t, const h5tool_format_t *);
H5TOOLS_DLL void     h5tools_str_dump_space_points(h5tools_str_t *, hid_t, const h5tool_format_t *);
H5TOOLS_DLL void     h5tools_str_sprint_region(h5tools_str_t *str, const h5tool_format_t *info, hid_t container,
                                   void *vp);
H5TOOLS_DLL char    *h5tools_str_sprint(h5tools_str_t *str, const h5tool_format_t *info,
                                   hid_t container, hid_t type, void *vp,
                                   h5tools_context_t *ctx);
H5TOOLS_DLL char    *h5tools_str_replace ( const char *string, const char *substr, 
									const char *replacement );

#endif  /* H5TOOLS_STR_H__ */
