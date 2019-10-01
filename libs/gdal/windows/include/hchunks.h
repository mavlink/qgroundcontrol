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
 * File:         hchunks.h
 * Purpose:      Header file for Chunked elements
 * Dependencies: tbbt.c mcache.c
 * Invokes:      none
 * Contents:     Structures & definitions for chunked elements
 * Structure definitions: DIM_DEF, HCHUNK_DEF
 * Constant definitions:
 * Author: -GeorgeV -  9/3/96
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef __HCHUNKS_H
#define __HCHUNKS_H

#include "H4api_adpt.h"

/* required includes */
#include "hfile.h"  /* special info stuff */

#ifdef   _HCHUNKS_MAIN_
/* Private to 'hchunks.c' */

#include "tbbt.h"   /* TBBT stuff */
#include "mcache.h" /* caching routines */
#include "hcomp.h"  /* For Compression */

/* Define class, class version and name(partial) for chunk table i.e. Vdata */
#if 0 /* moved definition of class of vdata to hlimits.h */
#define _HDF_CHK_TBL_CLASS "_HDF_CHK_TBL_" /* 13 bytes */
#define _HDF_CHK_TBL_CLASS_VER  0          /* zero version number for class */
#endif /* moved definition of class of vdata to hlimits.h */ 
#define _HDF_CHK_TBL_NAME  "_HDF_CHK_TBL_" /* 13 bytes */

/* Define field name for each chunk record i.e. Vdata record */
#define _HDF_CHK_FIELD_1   "origin"  /* 6 bytes */
#define _HDF_CHK_FIELD_2   "chk_tag" /* 7 bytes */
#define _HDF_CHK_FIELD_3   "chk_ref" /* 7 bytes */
#define _HDF_CHK_FIELD_NAMES   "origin,chk_tag,chk_ref" /* 22 bytes */

/* Define version number for chunked header format */
#define _HDF_CHK_HDR_VER   0  /* zero version for format header */

#endif /* _HCHUNKS_MAIN_ */

/* Public structures */

/* Structure for each Data array dimension Defintion */
typedef struct dim_def_struct {
    int32 dim_length;          /* length of this dimension */
    int32 chunk_length;        /* chunk length along this dimension */
    int32 distrib_type;        /* Data distribution along this dimension */
} DIM_DEF, * DIM_DEF_PTR;

/* Structure for each Chunk Definition*/
typedef struct hchunk_def_struct {
    int32    chunk_size;     /* size of this chunk*/
    int32    nt_size;        /* number type size i.e. size of data type */
    int32    num_dims;       /* number of actual dimensions */
    DIM_DEF *pdims;          /* ptr to array of dimension records for this chunk*/
    int32   chunk_flag;      /* multiply specialness? SPECIAL_COMP */

    /* For Compression info */
    comp_coder_t comp_type;     /* Compression type */
    comp_model_t model_type;    /* Compression model type */
    comp_info  *cinfo;        /* Compression info struct */
    model_info *minfo;        /* Compression model info struct */
}HCHUNK_DEF, * HCHUNK_DEF_PTR;

/* Private structues */
#ifdef _HCHUNKS_MAIN_
/* Private to 'hchunks.c' */

/* Structure for each Data array dimension */
typedef struct dim_rec_struct {
    /* fields stored in chunked header */
    int32 flag;                /* distrib_type(low 8 bits 0-7)
                                  - Data distribution along this dimension 
                                  other(medium low 8 bits 8-15)
                                  - regular/unlimited dimension? */
    int32 dim_length;          /* length of this dimension */
    int32 chunk_length;        /* chunk length along this dimension */
    
    /* info determined from 'flag' field */
    int32 distrib_type;        /* Data distribution along this dimension */
    int32 unlimited;           /* regular(0) or unlimited dimension(1) */

    /* computed fields */
    int32 last_chunk_length;   /* last chunk length along this dimension */
    int32 num_chunks;          /* i.e. "dim_length / chunk_length" */
} DIM_REC, * DIM_REC_PTR;

/* Structure for each Chunk */
typedef struct chunk_rec_struct {
    int32 chunk_number;      /* chunk number from coordinates i.e. origin */
    int32 chk_vnum;          /* chunk vdata record number i.e. position in table*/

    /* chunk record fields stored in Vdata Table */
    int32  *origin;          /* origin -> position of chunk */
    uint16 chk_tag;          /* DFTAG_CHUNK or another Chunked element? */
    uint16 chk_ref;          /* reference number of this chunk */
}CHUNK_REC, * CHUNK_REC_PTR;

/* information on this special chunk data elt */
typedef struct chunkinfo_t
{
    intn        attached;     /* how many access records refer to this elt */
    int32       aid;          /* Access id of chunk table i.e. Vdata */

    /* chunked element format header  fields */
    int32       sp_tag_header_len; /* length of the special element header */
    uint8       version;      /* Version of this Chunked element */
    int32       flag;         /* flag for multiply specialness ...*/
    int32       length;       /* the actual length of the data elt */
    int32       chunk_size;   /* the logical size of the chunks */
    int32       nt_size;      /* number type size i.e. size of data type */
    uint16      chktbl_tag;   /* DFTAG_VH - Vdata header */
    uint16      chktbl_ref;   /* ref of the first chunk table structure(VDATA) */
    uint16      sp_tag;       /* For future use.. */
    uint16      sp_ref;       /* For future use.. */
    int32       ndims;        /* number of dimensions of chunk */
    DIM_REC     *ddims;       /* array of dimension records */
    int32       fill_val_len; /* fill value number of bytes */
    VOID        *fill_val;    /* fill value */
    /* For each specialness, only one for now SPECIAL_COMP */
    int32       comp_sp_tag_head_len; /* Compression header length */
    VOID        *comp_sp_tag_header;  /* compression header */

    /* For Compression info */
    comp_coder_t comp_type;            /* Compression type */
    comp_model_t model_type;           /* Compression model type */
    comp_info   *cinfo;               /* Compression info struct */
    model_info  *minfo;               /* Compression model info struct */

    /* additional memory resident data structures to be used */
    int32       *seek_chunk_indices; /* chunk array indicies relative
                                        to the other chunks */
    int32       *seek_pos_chunk;     /* postion within the current chunk */
    int32       *seek_user_indices;  /* user postion within the element  */
    TBBT_TREE   *chk_tree;    /* TBBT tree of all accessed table entries 
                                 i.e. CHUNK_REC's read/written/modified */
    MCACHE      *chk_cache;   /* chunk cache */
    int32       num_recs;     /* number of Table(Vdata) records */
}
chunkinfo_t;
#endif /* _HCHUNKS_MAIN_ */

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

/*
** from hchunks.c
*/

/* User Public */
    HDFLIBAPI int32 HMCcreate
        (int32 file_id,       /* IN: file to put linked chunk element in */
         uint16 tag,          /* IN: tag of element */
         uint16 ref,          /* IN: ref of element */
         uint8 nlevels,       /* IN: number of levels of chunks */
         int32 fill_val_len,  /* IN: fill value length in bytes */
         VOID  *fill_val,     /* IN: fill value */
         HCHUNK_DEF *chk_array /* IN: structure describing chunk distribution
                                 can be an array? but we only handle 1 level */);

    HDFLIBAPI intn HMCgetcompress
        (accrec_t* access_rec,    /* IN: access record */
         comp_coder_t* comp_type, /* OUT: compression type */
         comp_info* c_info        /* OUT: retrieved compression info */);

    HDFLIBAPI intn HMCgetcomptype
        (int32 access_id,         /* IN: access record */
         comp_coder_t* comp_type  /* OUT: compression type */);

    HDFLIBAPI intn HMCgetdatainfo
        (int32 file_id,    /* IN: file in which element is located */
         uint16 data_tag,
         uint16 data_ref,
	 int32 *chk_coord, /* IN: chunk coord array or NULL for non-chunk SDS */
         uintn start_block,/* IN: data block to start at, 0 base */
         uintn info_count, /* IN: size of offset/length lists */
         int32 *offsetarray,     /* OUT: array to hold offsets */
         int32 *lengtharray);    /* OUT: array to hold lengths */

    HDFLIBAPI intn HMCgetdatasize
        (int32 file_id,    /* IN: file in which element is located */
         uint8 *p,         /* IN: buffer of special info header */
         int32 *comp_size, /* OUT: size of compressed data */
         int32 *orig_size  /* OUT: size of non-compressed data */);

    HDFLIBAPI int32 HMCsetMaxcache
        (int32 access_id,  /* IN: access aid to mess with */
         int32 maxcache,   /* IN: max number of pages to cache */
         int32 flags       /* IN: flags = 0, HMC_PAGEALL */);

    HDFLIBAPI int32 HMCwriteChunk
        (int32 access_id,  /* IN: access aid to mess with */
         int32 *origin,    /* IN: origin of chunk to write */
         const VOID *datap /* IN: buffer for data */);

    HDFLIBAPI int32 HMCreadChunk
        (int32 access_id,  /* IN: access aid to mess with */
         int32 *origin,    /* IN: origin of chunk to read */
         VOID *datap       /* IN: buffer for data */);

    HDFLIBAPI int32 HMCPcloseAID
        (accrec_t *access_rec /* IN:  access record of file to close */);

    HDFLIBAPI int32 HMCPgetnumrecs /* has to be here because used in hfile.c */
        (accrec_t * access_rec, /* IN:  access record to return info about */
         int32 *num_recs        /* OUT: length of the chunked elt */);

/* Library Private */
#ifdef _HCHUNKS_MAIN_
    /* tbbt.h helper routines */
    intn chkcompare(void * k1,   /* IN: first key */
           void * k2,   /* IN: second key */
           intn cmparg /* IN: not sure? */);
    void chkfreekey(void * key /*IN: chunk key */ );
    void chkdestroynode(void * n /* IN: chunk record */ );

/* Private to 'hchunks.c' */
    extern int32 HMCPstread
        (accrec_t *access_rec  /* IN: access record to fill in */);

    extern int32 HMCPstwrite
        (accrec_t *access_rec  /* IN: access record to fill in */);

    extern int32 HMCPseek
        (accrec_t *access_rec, /* IN: access record to mess with */
         int32 offset,         /* IN: seek offset */
         int   origin          /* IN: where we should calc the offset from */);

    extern int32 HMCPchunkread
        (VOID  *cookie,    /* IN: access record to mess with */
         int32 chunk_num,  /* IN: chunk to read */
         VOID  *datap      /* OUT: buffer for data */);

    extern int32 HMCPread
        (accrec_t * access_rec, /* IN: access record to mess with */
         int32 length,          /* IN: number of bytes to read */
         void * data             /* OUT: buffer for data */);

    extern int32 HMCPchunkwrite
        (VOID  *cookie,    /* IN: access record to mess with */
         int32 chunk_num,  /* IN: chunk number */
         const VOID *datap /* IN: buffer for data */);

    extern int32 HMCPwrite
        (accrec_t *access_rec, /* IN: access record to mess with */
         int32 length,         /* IN: number of bytes to write */
         const void * data      /* IN: buffer for data */);

    extern intn HMCPendaccess
        (accrec_t *access_rec /* IN:  access record to close */);

    extern int32 HMCPinfo
        (accrec_t *access_rec,       /* IN: access record of access elemement */
         sp_info_block_t *info_chunk /* OUT: information about the special element */);

    extern int32 HMCPinquire
        (accrec_t * access_rec, /* IN:  access record to return info about */
         int32 *pfile_id,       /* OUT: file ID; */
         uint16 *ptag,          /* OUT: tag of info record; */
         uint16 *pref,          /* OUT: ref of info record; */
         int32 *plength,        /* OUT: length of element; */
         int32 *poffset,        /* OUT: offset of element -- meaningless */
         int32 *pposn,          /* OUT: current position in element; */
         int16 *paccess,        /* OUT: access mode; */
         int16 *pspecial        /* OUT: special code; */);

#endif /* _HCHUNKS_MAIN_ */

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

#ifndef _HCHUNKS_MAIN_
/* not in master file hchunk.c */
extern funclist_t chunked_funcs;  /* functions to perform chunking */

#else /* in hchunks.c */

/* the accessing special function table for chunks */
funclist_t  chunked_funcs =
{
    HMCPstread,
    HMCPstwrite,
    HMCPseek,
    HMCPinquire,
    HMCPread,
    HMCPwrite,
    HMCPendaccess,
    HMCPinfo,
    NULL         /* no routine registerd */
};

#endif

#endif /* __HCHUNKS_H */
