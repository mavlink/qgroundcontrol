/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF.  The full HDF copyright notice, including       *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF/releases/.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id$ */

/*-----------------------------------------------------------------------------
 * File:    mstdio.h
 * Purpose: Header file for stdio-like modeling information.
 * Dependencies: should be included after hdf.h
 * Invokes:
 * Contents: Structures & definitions for stdio modeling.  This header
 *              should only be included in hcomp.c and mstdio.c.
 * Structure definitions:
 * Constant definitions:
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef __MSTDIO_H
#define __MSTDIO_H

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

/*
   ** from mstdio.c
 */

    extern int32 HCPmstdio_stread
                (accrec_t * rec);

    extern int32 HCPmstdio_stwrite
                (accrec_t * rec);

    extern int32 HCPmstdio_seek
                (accrec_t * access_rec, int32 offset, int origin);

    extern int32 HCPmstdio_inquire
                (accrec_t * access_rec, int32 *pfile_id, uint16 *ptag, uint16 *pref,
               int32 *plength, int32 *poffset, int32 *pposn, int16 *paccess,
                 int16 *pspecial);

    extern int32 HCPmstdio_read
                (accrec_t * access_rec, int32 length, void * data);

    extern int32 HCPmstdio_write
                (accrec_t * access_rec, int32 length, const void * data);

    extern intn HCPmstdio_endaccess
                (accrec_t * access_rec);

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

/* model information about stdio model */
typedef struct
{
    int32      pos;            /* postion ? */
}
comp_model_stdio_info_t;

#ifndef MSTDIO_MASTER
extern funclist_t mstdio_funcs; /* functions to perform run-length encoding */
#else
funclist_t  mstdio_funcs =
{                               /* functions to perform run-length encoding */
    HCPmstdio_stread,
    HCPmstdio_stwrite,
    HCPmstdio_seek,
    HCPmstdio_inquire,
    HCPmstdio_read,
    HCPmstdio_write,
    HCPmstdio_endaccess
};
#endif

#endif /* __MSTDIO_H */
