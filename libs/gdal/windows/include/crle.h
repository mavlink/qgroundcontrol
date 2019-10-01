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
 * File:    crle.h
 * Purpose: Header file for run-length encoding information.
 * Dependencies: should only be included from hcompi.h
 * Invokes: none
 * Contents: Structures & definitions for run-length encoding.  This header
 *              should only be included in hcomp.c and crle.c.
 * Structure definitions:
 * Constant definitions:
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef __CRLE_H
#define __CRLE_H

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

/*
   ** from crle.c
 */

    extern int32 HCPcrle_stread
                (accrec_t * rec);

    extern int32 HCPcrle_stwrite
                (accrec_t * rec);

    extern int32 HCPcrle_seek
                (accrec_t * access_rec, int32 offset, int origin);

    extern int32 HCPcrle_inquire
                (accrec_t * access_rec, int32 *pfile_id, uint16 *ptag, uint16 *pref,
               int32 *plength, int32 *poffset, int32 *pposn, int16 *paccess,
                 int16 *pspecial);

    extern int32 HCPcrle_read
                (accrec_t * access_rec, int32 length, void * data);

    extern int32 HCPcrle_write
                (accrec_t * access_rec, int32 length, const void * data);

    extern intn HCPcrle_endaccess
                (accrec_t * access_rec);

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

/* size of the RLE buffer */
#define RLE_BUF_SIZE    128
/* NIL code for run bytes */
#define RLE_NIL         (-1)
/* minimum length of run */
#define RLE_MIN_RUN     3
/* maximum length of run */
#define RLE_MAX_RUN     (RLE_BUF_SIZE+RLE_MIN_RUN-1)
/* minimum length of mix */
#define RLE_MIN_MIX     1
/*
 * Notes on RLE_MIN_RUN and RLE_MIN_MIX:
 * (excerpt from QAK's email to RA - see bug HDFFR-1261)
 *
 * These are [small] optimizations for improving the compression ratio. The
 * algorithm won't encode a run of identical bytes unless it's at least
 * RLE_MIN_RUN bytes long.  So, we can assume that all runs are at least
 * that many bytes, and subtract RLE_MIN_RUN from the actual run length,
 * allowing encoding of runs that are a little bit longer than otherwise
 * allowed (i.e. runs up to 127+RLE_MIN_RUN bytes, instead of only 127 bytes).
 * Similarly for RLE_MIN_MIX - there must be at least RLE_MIN_MIX bytes in a
 * "mixed" sequence of bytes, so we can encode a little bit longer sequence
 * of mixed bytes (127+RLE_MIN_MIX bytes, instead of only 127 bytes).
 */

/* RLE [en|de]coding information */
typedef struct
{
    int32       offset;         /* offset in the file */
    uint8       buffer[RLE_BUF_SIZE];   /* buffer for storing RLE bytes */
    intn        buf_length;     /* number of bytes in buffer */
    intn        buf_pos;        /* offset into the buffer */
    uintn       last_byte,      /* the last byte stored in the buffer */
                second_byte;    /* the second to last byte stored in the buffer */
    enum
      {
          RLE_INIT,             /* initial state, need to read a byte to
                                   determine the next state */
          RLE_RUN,              /* buffer up to the current position is a run */
          RLE_MIX		/* buffer up to the current position is a mix */
      }
    rle_state;                  /* state of the buffer storage */
}
comp_coder_rle_info_t;

#ifndef CRLE_MASTER
extern funclist_t crle_funcs;   /* functions to perform run-length encoding */
#else
funclist_t  crle_funcs =
{                               /* functions to perform run-length encoding */
    HCPcrle_stread,
    HCPcrle_stwrite,
    HCPcrle_seek,
    HCPcrle_inquire,
    HCPcrle_read,
    HCPcrle_write,
    HCPcrle_endaccess
};
#endif

#endif /* __CRLE_H */
