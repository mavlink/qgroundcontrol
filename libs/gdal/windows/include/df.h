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
 * File:    df.h
 * Purpose: header file for HDF routines
 * Invokes: dfi.h
 * Contents:
 *  Structure definitions: DFddh, DFdd, DFdesc, DFdle, DF, DFdi, DFdata
 *  Procedure type definitions
 *  Global variables
 *  Tag definitions
 *  Error return codes
 *  Logical constants
 * Remarks: This file is included with user programs
 *          Since it includes stdio.h etc., do not include these after df.h
 *---------------------------------------------------------------------------*/

#ifndef DF_H    /* avoid re-inclusion */
#define DF_H

#include "H4api_adpt.h"

/* include DF (internal) header information */
#include "hdf.h"

/*-------------------------------------------------------------------------*/
/*                      Type declarations                                   */

typedef struct DFddh
  {                             /*format of data descriptor headers in file */
      int16       dds;          /* number of dds in header block */
      int32       next;         /* offset of next header block */
  }
DFddh;

typedef struct DFdd
  {                             /* format of data descriptors as in file */
      uint16      tag;          /* data tag */
      uint16      ref;          /* data reference number */
      int32       offset;       /* offset of data element in file */
      int32       length;       /* number of bytes */
  }
DFdd;

/* descriptor structure is same as dd structure.  ###Note: may be changed */
typedef DFdd DFdesc;

/* DLE is the internal structure which stores data descriptor information */
/* It is a linked list of DDs */
typedef struct DFdle
  {                             /* Data List element */
      struct DFdle *next;       /* link to next dle */
      DFddh       ddh;          /* To store headers */
      DFdd        dd[1];        /* dummy size */
  }
DFdle;

/* DF is the internal structure associated with each DF file */
/* It holds information associated with the file as a whole */
/* ### Note: there are hooks for having multiple DF files open at a time */
typedef struct DF
  {
      DFdle      *list;         /* Pointer to the DLE list */
      DFdle      *last_dle;     /* last_dle and last_dd are used in searches */
      /* to indicate element returned */
      /* by previous call to DFfind */
      DFdd       *up_dd;        /* DD of element being read/updated, */
      /* used by DFstart */
      uint16      last_tag;     /* Last tag searched for by DFfind */
      uint16      last_ref;     /* Last reference number searched for */
      intn        type;         /* 0= not in use, 1= normal, -1 = multiple */
      /* this is a hook for when */
      /* multiple files are open */
      intn        access;       /* permitted access types: */
      /* 0=none, 1=r, 2=w, 3=r/w */
      intn        changed;      /* True if anything in DDs modified */
      /* since last write */
      intn        last_dd;      /* see last_dle */
      intn        defdds;       /* default numer of DD's in each block */
      intn        up_access;    /* access permissions to element being */
      /* read/updated. Used by DFstart */
      /* File handle is a file pointer or file descriptor depending on whether */
      /* we use buffered or unbuffered I/O.  But, since this structure is a */
      /* fake, it doesn't matter whether I/O is buffered or not. */
      intn        file;         /* file descriptor */
  }
DF;

typedef struct DFdata
  {                             /* structure for returning status information */
      int32       version;      /* version number of program */
  }
DFdata;

/*--------------------------------------------------------------------------*/
/*                          Procedure types                                 */

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

/* prototypes for dfstubs.c */
    HDFLIBAPI DF  *DFopen
                (char *name, int acc_mode, int ndds);

    HDFLIBAPI int  DFclose
                (DF * dfile);

    HDFLIBAPI int  DFdescriptors
                (DF * dfile, DFdesc ptr[], int begin, int num);

    HDFLIBAPI int  DFnumber
                (DF * dfile, uint16 tag);

    HDFLIBAPI int  DFsetfind
                (DF * dfile, uint16 tag, uint16 ref);

    HDFLIBAPI int  DFfind
                (DF * dfile, DFdesc * ptr);

    HDFLIBAPI int  DFaccess
                (DF * dfile, uint16 tag, uint16 ref, char *acc_mode);

    HDFLIBAPI int  DFstart
                (DF * dfile, uint16 tag, uint16 ref, char *acc_mode);

    HDFLIBAPI int32 DFread
                (DF * dfile, char *ptr, int32 len);

    HDFLIBAPI int32 DFseek
                (DF * dfile, int32 offset);

    HDFLIBAPI int32 DFwrite
                (DF * dfile, char *ptr, int32 len);

    HDFLIBAPI int  DFupdate
                (DF * dfile);

    HDFLIBAPI int  DFstat
                (DF * dfile, DFdata * dfinfo);

    HDFLIBAPI int32 DFgetelement
                (DF * dfile, uint16 tag, uint16 ref, char *ptr);

    HDFLIBAPI int32 DFputelement
                (DF * dfile, uint16 tag, uint16 ref, char *ptr, int32 len);

    HDFLIBAPI int  DFdup
                (DF * dfile, uint16 itag, uint16 iref, uint16 otag, uint16 oref);

    HDFLIBAPI int  DFdel
                (DF * dfile, uint16 tag, uint16 ref);

    HDFLIBAPI uint16 DFnewref
                (DF * dfile);

    HDFLIBAPI int  DFishdf
                (char *filename);

    HDFLIBAPI int  DFerrno
                (void);

    HDFLIBAPI int  DFIerr
                (DF * dfile);

    HDFLIBAPI int  DFImemcopy
                (char *from, char *to, int length);

    HDFLIBAPI void *DFIgetspace
                (uint32 qty);

    HDFLIBAPI void *DFIfreespace
                (void *ptr);

    HDFLIBAPI int  DFIc2fstr
                (char *str, int len);

    HDFLIBAPI char *DFIf2cstring
                (_fcd fdesc, intn len);

/* prototypes for dfconv.c */
    HDFLIBAPI int  DFconvert
                (uint8 *source, uint8 *dest, int ntype, int sourcetype, int desttype, int32 size);

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

/*--------------------------------------------------------------------------*/
/*                          Global Variables                                */

#ifndef DFMASTER
HDFLIBAPI
#endif                          /*DFMASTER */
int DFerror;            /* Error code for DF routines */

#define DFSETERR(error) (DFerror=(DFerror?DFerror:error))

#define DFTOFID(df) (int32)(df->list)

#endif /* DF_H */
