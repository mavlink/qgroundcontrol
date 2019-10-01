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
 * File:    cskphuff.h
 * Purpose: Header file for skipping huffman encoding information.
 * Dependencies: should only be included from hcompi.h
 * Invokes: none
 * Contents: Structures & definitions for skipping huffman encoding.
 * Structure definitions:
 * Constant definitions:
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef __CSKPHUFF_H
#define __CSKPHUFF_H

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

/*
   ** from cskphuff.c
 */

    extern int32 HCPcskphuff_stread
                (accrec_t * rec);

    extern int32 HCPcskphuff_stwrite
                (accrec_t * rec);

    extern int32 HCPcskphuff_seek
                (accrec_t * access_rec, int32 offset, int origin);

    extern int32 HCPcskphuff_inquire
                (accrec_t * access_rec, int32 *pfile_id, uint16 *ptag, uint16 *pref,
               int32 *plength, int32 *poffset, int32 *pposn, int16 *paccess,
                 int16 *pspecial);

    extern int32 HCPcskphuff_read
                (accrec_t * access_rec, int32 length, void * data);

    extern int32 HCPcskphuff_write
                (accrec_t * access_rec, int32 length, const void * data);

    extern intn HCPcskphuff_endaccess
                (accrec_t * access_rec);

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

/* The maximum source character code: */
#define SKPHUFF_MAX_CHAR     255

/* One greater than the maximum source character code: */
#define SUCCMAX     (SKPHUFF_MAX_CHAR+1)

/* Twice the maximum source character code: */
#define TWICEMAX    (2*SUCCMAX+1)

/* The root node in the tree */
#define ROOT        0

/* Skipping huffman [en|de]coding information */
typedef struct
{
    intn        skip_size;      /* number of bytes in each element */
    uintn     **left,           /* define the left and right pointer arrays */
              **right;
    uint8     **up;             /* define the up pointer array */
    intn        skip_pos;       /* current byte to read or write */
    int32       offset;         /* offset in the de-compressed array */
}
comp_coder_skphuff_info_t;

#ifndef CSKPHUFF_MASTER
extern funclist_t cskphuff_funcs;   /* functions to perform skipping huffman encoding */
#else
funclist_t  cskphuff_funcs =
{                               /* functions to perform skipping huffman encoding */
    HCPcskphuff_stread,
    HCPcskphuff_stwrite,
    HCPcskphuff_seek,
    HCPcskphuff_inquire,
    HCPcskphuff_read,
    HCPcskphuff_write,
    HCPcskphuff_endaccess
};
#endif

#endif /* __CSKPHUFF_H */
