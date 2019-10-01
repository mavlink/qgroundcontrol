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
 * File:    mfgr.h
 * Purpose: header file for multi-file general raster information
 * Dependencies: 
 * Invokes:
 * Contents:
 * Structure definitions: 
 * Constant definitions: 
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef __MFGR_H
#define __MFGR_H

#include "H4api_adpt.h"

/* Interlace types available */
typedef int16 gr_interlace_t;
#define MFGR_INTERLACE_PIXEL		0    /* pixel interlacing scheme */
#define MFGR_INTERLACE_LINE		    1    /* line interlacing scheme */
#define MFGR_INTERLACE_COMPONENT 	2    /* component interlacing scheme */

#if defined MFGR_MASTER | defined MFGR_TESTER

#include "hfile.h"
#include "tbbt.h"       /* Get tbbt routines */

/* This is the size of the hash tables used for GR & RI IDs */
#define GRATOM_HASH_SIZE    32

/* The tag of the attribute data */
#define RI_TAG      DFTAG_VG    /* Current RI groups are stored in Vgroups */
#define ATTR_TAG    DFTAG_VH    /* Current GR attributes are stored in VDatas */

/* The default threshhold for attributes which will be cached */
#define GR_ATTR_THRESHHOLD  2048    

#define VALIDRIINDEX(i,gp) ((i)>=0 && (i)<(gp)->gr_count)

/*
 * Each gr_info_t maintains 2 threaded-balanced-binary-tress: one of
 * raster images and one of global attributes
 */

typedef struct gr_info {
    int32       hdf_file_id;    /* the corresponding HDF file ID (must be first in the structure) */
    uint16      gr_ref;         /* ref # of the Vgroup of the GR in the file */

    int32       gr_count;       /* # of image entries in gr_tab so far */
    TBBT_TREE  *grtree;         /* Root of image B-Tree */
    uintn       gr_modified;    /* whether any images have been modified */

    int32       gattr_count;    /* # of global attr entries in gr_tab so far */
    TBBT_TREE  *gattree;        /* Root of global attribute B-Tree */
    uintn       gattr_modified; /* whether any global attributes have been modified */

    intn        access;         /* the number of active pointers to this file's GRstuff */
    uint32      attr_cache;     /* the threshhold for the attribute sizes to cache */
} gr_info_t;

typedef struct at_info {
    int32 index;            /* index of the attribute (needs to be first in the struct) */
    int32 nt;               /* number type of the attribute */
    int32 len;              /* length/order of the attribute */
    uint16 ref;             /* ref of the attribute (stored in VData) */
    uintn data_modified;    /* flag to indicate whether the attribute data has been modified */
    uintn new_at;           /* flag to indicate whether the attribute was added to the Vgroup */
    char *name;             /* name of the attribute */
    void * data;             /* data for the attribute */
} at_info_t;

typedef struct dim_info {
    uint16  dim_ref;            /* reference # of the Dim record */
    int32   xdim,ydim,          /* dimensions of the image */
            ncomps,             /* number of components of each pixel in image */
            nt,                 /* number type of the components */
            file_nt_subclass;   /* number type subclass of data on disk */
    gr_interlace_t il;          /* interlace of the components (stored on disk) */
    uint16  nt_tag,nt_ref;      /* tag & ref of the number-type info */
    uint16  comp_tag,comp_ref;  /* tag & ref of the compression info */
} dim_info_t;

typedef struct ri_info {
    int32   index;              /* index of this image (needs to be first in the struct) */
    uint16  ri_ref;             /* ref # of the RI Vgroup */
    uint16  rig_ref;            /* ref # of the RIG group */
    gr_info_t *gr_ptr;          /* ptr to the GR info that this ri_info applies to */
    dim_info_t img_dim;         /* image dimension information */
    dim_info_t lut_dim;         /* palette dimension information */
    uint16  img_tag,img_ref;    /* tag & ref of the image data */
    int32   img_aid;            /* AID for the image data */
    intn    acc_perm;           /* Access permission (read/write) for image AID */
    uint16  lut_tag,lut_ref;    /* tag & ref of the palette data */
    gr_interlace_t im_il;       /* interlace of image when next read (default PIXEL) */
    gr_interlace_t lut_il;      /* interlace of LUT when next read */
    uintn data_modified;        /* whether the image or palette data has been modified */
    uintn meta_modified;        /* whether the image or palette meta-info has been modified */
    uintn attr_modified;        /* whether the attributes have been modified */
    char   *name;               /* name of the image */
    int32   lattr_count;        /* # of local attr entries in ri_info so far */
    TBBT_TREE *lattree;         /* Root of the local attribute B-Tree */
    intn access;                /* the number of times this image has been selected */
    uintn use_buf_drvr;         /* access to image needs to be through the buffered special element driver */
    uintn use_cr_drvr;          /* access to image needs to be through the compressed raster special element driver */
    uintn comp_img;             /* whether to compress image data */
    comp_coder_t comp_type;     /* compression type */
    comp_info cinfo;            /* compression information */
    uintn ext_img;              /* whether to make image data external */
    char *ext_name;             /* name of the external file */
    int32 ext_offset;           /* offset in the external file */
    uintn acc_img;              /* whether to make image data a different access type */
    uintn acc_type;             /* type of access-mode to get image data with */
    uintn fill_img;             /* whether to fill image, or just store fill value */
    void * fill_value;           /* pointer to the fill value (NULL means use default fill value of 0) */
    uintn store_fill;           /* whether to add fill value attribute or not */
    intn   name_generated;      /* whether the image has name that was given by app. or was generated by the library like the DFR8 images (added for hmap)*/
} ri_info_t;

/* Useful raster routines for generally private use */

HDFLIBAPI intn GRIil_convert(const void * inbuf,gr_interlace_t inil,void * outbuf,
        gr_interlace_t outil,int32 dims[2],int32 ncomp,int32 nt);

extern VOID GRIgrdestroynode(void * n);

extern VOID GRIattrdestroynode(void * n);

extern VOID GRIridestroynode(void * n);

#endif /* MFGR_MASTER | MFGR_TESTER */

#endif /* __MFGR_H */
