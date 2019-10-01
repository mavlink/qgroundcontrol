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

/*+ htags.h
   *** This file contains all the tag definitions for HDF
   + */

#ifndef _HTAGS_H
#define _HTAGS_H

/* wild-card tags and refs. Should only be used in interface calls
   and never stored in the file i.e. in DD's. */
#define DFREF_WILDCARD      0
#define DFTAG_WILDCARD      0

#define DFREF_NONE          0  /* used by mfhdf/libsrc/putget.c */
/* tags and refs */
#define DFTAG_NULL          1
#define DFTAG_LINKED        20  /* linked-block special element */
#define DFTAG_VERSION       30
#define DFTAG_COMPRESSED    40  /* compressed special element */
#define DFTAG_VLINKED       50  /* variable-len linked-block header */
#define DFTAG_VLINKED_DATA  51  /* variable-len linked-block data */
#define DFTAG_CHUNKED       60  /* chunked special element header
                                   (for expansion, not used )*/
#define DFTAG_CHUNK         61  /* chunk element */

/* utility set */
#define DFTAG_FID   ((uint16)100)   /* File identifier */
#define DFTAG_FD    ((uint16)101)   /* File description */
#define DFTAG_TID   ((uint16)102)   /* Tag identifier */
#define DFTAG_TD    ((uint16)103)   /* Tag descriptor */
#define DFTAG_DIL   ((uint16)104)   /* data identifier label */
#define DFTAG_DIA   ((uint16)105)   /* data identifier annotation */
#define DFTAG_NT    ((uint16)106)   /* number type */
#define DFTAG_MT    ((uint16)107)   /* machine type */
#define DFTAG_FREE  ((uint16)108)   /* free space in the file */

/* raster-8 set */
#define DFTAG_ID8   ((uint16)200)   /* 8-bit Image dimension */
#define DFTAG_IP8   ((uint16)201)   /* 8-bit Image palette */
#define DFTAG_RI8   ((uint16)202)   /* Raster-8 image */
#define DFTAG_CI8   ((uint16)203)   /* RLE compressed 8-bit image */
#define DFTAG_II8   ((uint16)204)   /* IMCOMP compressed 8-bit image */

/* Raster Image set */
#define DFTAG_ID    ((uint16)300)   /* Image DimRec */
#define DFTAG_LUT   ((uint16)301)   /* Image Palette */
#define DFTAG_RI    ((uint16)302)   /* Raster Image */
#define DFTAG_CI    ((uint16)303)   /* Compressed Image */
#define DFTAG_NRI   ((uint16)304)   /* New-format Raster Image */

#define DFTAG_RIG   ((uint16)306)   /* Raster Image Group */
#define DFTAG_LD    ((uint16)307)   /* Palette DimRec */
#define DFTAG_MD    ((uint16)308)   /* Matte DimRec */
#define DFTAG_MA    ((uint16)309)   /* Matte Data */
#define DFTAG_CCN   ((uint16)310)   /* color correction */
#define DFTAG_CFM   ((uint16)311)   /* color format */
#define DFTAG_AR    ((uint16)312)   /* aspect ratio */

#define DFTAG_DRAW  ((uint16)400)   /* Draw these images in sequence */
#define DFTAG_RUN   ((uint16)401)   /* run this as a program/script */

#define DFTAG_XYP   ((uint16)500)   /* x-y position */
#define DFTAG_MTO   ((uint16)501)   /* machine-type override */

/* Tektronix */
#define DFTAG_T14   ((uint16)602)   /* TEK 4014 data */
#define DFTAG_T105  ((uint16)603)   /* TEK 4105 data */

/* Scientific Data set */
/*
   Objects of tag 721 are never actually written to the file.  The tag is
   needed to make things easier mixing DFSD and SD style objects in the
   same file
 */
#define DFTAG_SDG   ((uint16)700)   /* Scientific Data Group */
#define DFTAG_SDD   ((uint16)701)   /* Scientific Data DimRec */
#define DFTAG_SD    ((uint16)702)   /* Scientific Data */
#define DFTAG_SDS   ((uint16)703)   /* Scales */
#define DFTAG_SDL   ((uint16)704)   /* Labels */
#define DFTAG_SDU   ((uint16)705)   /* Units */
#define DFTAG_SDF   ((uint16)706)   /* Formats */
#define DFTAG_SDM   ((uint16)707)   /* Max/Min */
#define DFTAG_SDC   ((uint16)708)   /* Coord sys */
#define DFTAG_SDT   ((uint16)709)   /* Transpose */
#define DFTAG_SDLNK ((uint16)710)   /* Links related to the dataset */
#define DFTAG_NDG   ((uint16)720)   /* Numeric Data Group */
                  /* tag 721 reserved chouck 24-Nov-93 */
#define DFTAG_CAL   ((uint16)731)   /* Calibration information */
#define DFTAG_FV    ((uint16)732)   /* Fill Value information */
#define DFTAG_BREQ  ((uint16)799)   /* Beginning of required tags   */
#define DFTAG_SDRAG ((uint16)781)   /* List of ragged array line lengths */
#define DFTAG_EREQ  ((uint16)780)   /* Current end of the range   */

/* VSets */
#define DFTAG_VG     ((uint16)1965)     /* Vgroup */
#define DFTAG_VH     ((uint16)1962)     /* Vdata Header */
#define DFTAG_VS     ((uint16)1963)     /* Vdata Storage */

/* compression schemes */
#define DFTAG_RLE       ((uint16)11)    /* run length encoding */
#define DFTAG_IMC       ((uint16)12)    /* IMCOMP compression alias */
#define DFTAG_IMCOMP    ((uint16)12)    /* IMCOMP compression */
#define DFTAG_JPEG      ((uint16)13)    /* JPEG compression (24-bit data) */
#define DFTAG_GREYJPEG  ((uint16)14)    /* JPEG compression (8-bit data) */
#define DFTAG_JPEG5     ((uint16)15)    /* JPEG compression (24-bit data) */
#define DFTAG_GREYJPEG5 ((uint16)16)    /* JPEG compression (8-bit data) */

/* Interlace schemes */
#define DFIL_PIXEL   0  /* Pixel Interlacing */
#define DFIL_LINE    1  /* Scan Line Interlacing */
#define DFIL_PLANE   2  /* Scan Plane Interlacing */

/* SPECIAL CODES */
#define SPECIAL_LINKED 1    /* Fixed-size Linked blocks */
#define SPECIAL_EXT 2       /* External */
#define SPECIAL_COMP 3      /* Compressed */
#define SPECIAL_VLINKED 4   /* Variable-length linked blocks */
#define SPECIAL_CHUNKED 5   /* chunked element */
#define SPECIAL_BUFFERED 6  /* Buffered element */
#define SPECIAL_COMPRAS 7   /* Compressed Raster element */

#endif /* _HTAGS_H */

