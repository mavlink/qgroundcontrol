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
 * Purpose:     Support h5dump functions for the various tools.
 */
#ifndef H5TOOLS_DUMP_H__
#define H5TOOLS_DUMP_H__

#include "h5tools_utils.h"

/* 3 private values: can't be set, but can be read.
   Note: these are defined in H5Zprivate, they are
   duplicated here.
 */
#define H5_SZIP_LSB_OPTION_MASK         8
#define H5_SZIP_MSB_OPTION_MASK         16
#define H5_SZIP_RAW_OPTION_MASK         128

#ifdef __cplusplus
extern "C" {
#endif

H5TOOLS_DLLVAR table_t *h5dump_type_table;  /*type table reference for datatype dump  */

/* Definitions of useful routines */
H5TOOLS_DLL void    h5tools_dump_init(void);

H5TOOLS_DLL int     h5tools_dump_dset(FILE *stream, const h5tool_format_t *info,
                            h5tools_context_t *ctx/*in,out*/, hid_t dset,
                            struct subset_t *sset);
H5TOOLS_DLL int     h5tools_dump_mem(FILE *stream, const h5tool_format_t *info,
                            h5tools_context_t *ctx/*in,out*/, hid_t obj_id,
                            hid_t type, hid_t space, void *mem);
H5TOOLS_DLL int     h5tools_dump_simple_data(FILE *stream, const h5tool_format_t *info, hid_t container,
                            h5tools_context_t *ctx/*in,out*/, unsigned flags,
                            hsize_t nelmts, hid_t type, void *_mem);
H5TOOLS_DLL void    h5tools_dump_datatype(FILE *stream, const h5tool_format_t *info,
                            h5tools_context_t *ctx/*in,out*/, hid_t type);
H5TOOLS_DLL void    h5tools_dump_dataspace(FILE *stream, const h5tool_format_t *info,
                            h5tools_context_t *ctx/*in,out*/, hid_t space);
H5TOOLS_DLL void    h5tools_dump_attribute(FILE *stream, const h5tool_format_t *info,
                            h5tools_context_t *ctx/*in,out*/, const char *attr_name, 
                            hid_t attr_id, int display_index, int display_char);
H5TOOLS_DLL void    h5tools_dump_oid(FILE *stream, const h5tool_format_t *info,
                            h5tools_context_t *ctx/*in,out*/, hid_t oid);
H5TOOLS_DLL void    h5tools_dump_dcpl(FILE *stream, const h5tool_format_t *info,
                            h5tools_context_t *ctx/*in,out*/, hid_t dcpl, hid_t type_id, hid_t obj_id);
H5TOOLS_DLL void    h5tools_dump_comment(FILE *stream, const h5tool_format_t *info,
                            h5tools_context_t *ctx/*in,out*/, hid_t obj_id);
H5TOOLS_DLL void    h5tools_dump_data(FILE *stream, const h5tool_format_t *info,
                            h5tools_context_t *ctx, hid_t obj_id,
                            int obj_data, struct subset_t *sset, int display_index, int display_char);

H5TOOLS_DLL int     h5tools_print_datatype(FILE *stream, h5tools_str_t *buffer/*in,out*/,
                            const h5tool_format_t *info, h5tools_context_t *ctx/*in,out*/,
                            hid_t type, int object_search);
H5TOOLS_DLL int     h5tools_print_dataspace(h5tools_str_t *buffer/*in,out*/,
                            hid_t space);
H5TOOLS_DLL int     h5tools_print_enum(FILE *stream, h5tools_str_t *buffer/*in,out*/,
                            const h5tool_format_t *info, h5tools_context_t *ctx/*in,out*/,
                            hid_t type);
H5TOOLS_DLL void    h5tools_print_fill_value(h5tools_str_t *buffer/*in,out*/,
                            const h5tool_format_t *info, h5tools_context_t *ctx/*in,out*/, 
                            hid_t dcpl, hid_t type_id, hid_t obj_id);
H5TOOLS_DLL void    h5tools_print_packed_bits(h5tools_str_t *buffer/*in,out*/, hid_t type);
#ifdef __cplusplus
}
#endif

#endif /* H5TOOLS_DUMP_H__ */

