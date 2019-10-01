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

/*+ hkit.h
   ***  private header file for hkit routines
   + */

#ifndef __HKIT_H
#include "hdf.h"
#include "hfile.h"

/* tag_messages is the list of tag descriptions in the system, kept as
   tag-description pairs.  To look up a description, a linear search is
   required but efficiency should be okay. */
typedef struct tag_descript_t
  {
      uint16      tag;          /* tag for description ? */
      const char *desc;         /* tag description ? */
      const char *name;         /* tag name ? */
  }
tag_descript_t;

/* stringizing macro */
#define string(x) #x

/*  NOTE:
 *        Please keep tag descriptions <= 30 characters - a
 *        lot of pretty-printing code depends on it.
 */
PRIVATE const tag_descript_t tag_descriptions[] =
{
/* low-level set */
    {DFTAG_NULL, string(DFTAG_NULL), "No Data"},
    {DFTAG_LINKED, string(DFTAG_LINKED), "Linked Blocks Indicator"},
    {DFTAG_VERSION, string(DFTAG_VERSION), "Version Descriptor"},
    {DFTAG_COMPRESSED, string(DFTAG_COMPRESSED), "Compressed Data Indicator"},
    {DFTAG_CHUNK, string(DFTAG_CHUNK), "Data Chunk"},

/* utility set */
    {DFTAG_FID, string(DFTAG_FID), "File Identifier"},
    {DFTAG_FD, string(DFTAG_FD), "File Description"},
    {DFTAG_TID, string(DFTAG_TID), "Tag Identifier"},
    {DFTAG_TD, string(DFTAG_TD), "Tag Description"},
    {DFTAG_DIL, string(DFTAG_DIL), "Data Id Label"},
    {DFTAG_DIA, string(DFTAG_DIA), "Data Id Annotation"},
    {DFTAG_NT, string(DFTAG_NT), "Number type"},
    {DFTAG_MT, string(DFTAG_MT), "Machine type"},
    {DFTAG_FREE, string(DFTAG_FREE), "Free space"},

      /* raster-8 Tags */
    {DFTAG_ID8, string(DFTAG_ID8), "Image Dimensions-8"},
    {DFTAG_IP8, string(DFTAG_IP8), "Image Palette-8"},
    {DFTAG_RI8, string(DFTAG_RI8), "Raster Image-8"},
    {DFTAG_CI8, string(DFTAG_CI8), "RLE Compressed Image-8"},
    {DFTAG_II8, string(DFTAG_II8), "Imcomp Image-8"},

      /* Raster Image Tags */
    {DFTAG_ID, string(DFTAG_ID), "Image Dimensions"},
    {DFTAG_LUT, string(DFTAG_LUT), "Image Palette"},
    {DFTAG_RI, string(DFTAG_RI), "Raster Image Data"},
    {DFTAG_CI, string(DFTAG_CI), "Compressed Image"},
    {DFTAG_RIG, string(DFTAG_RIG), "Raster Image Group"},
    {DFTAG_LD, string(DFTAG_LD), "Palette Dimension"},
    {DFTAG_MD, string(DFTAG_MD), "Matte Dimension"},
    {DFTAG_MA, string(DFTAG_MA), "Matte Data"},
    {DFTAG_CCN, string(DFTAG_CCN), "Color Correction"},
    {DFTAG_CFM, string(DFTAG_CFM), "Color Format"},
    {DFTAG_AR, string(DFTAG_AR), "Aspect Ratio"},
    {DFTAG_DRAW, string(DFTAG_DRAW), "Sequenced images"},
    {DFTAG_RUN, string(DFTAG_RUN), "Runable program / script"},
    {DFTAG_XYP, string(DFTAG_XYP), "X-Y position"},
    {DFTAG_MTO, string(DFTAG_MTO), "M/c-Type override"},

      /* Tektronix */
    {DFTAG_T14, string(DFTAG_T14), "TEK 4014 Data"},
    {DFTAG_T105, string(DFTAG_T105), "TEK 4105 data"},

      /* Scientific / Numeric Data Sets */
    {DFTAG_SDG, string(DFTAG_SDG), "Scientific Data Group"},
    {DFTAG_SDD, string(DFTAG_SDD), "SciData dimension record"},
    {DFTAG_SD, string(DFTAG_SD), "Scientific Data"},
    {DFTAG_SDS, string(DFTAG_SDS), "SciData scales"},
    {DFTAG_SDL, string(DFTAG_SDL), "SciData labels"},
    {DFTAG_SDU, string(DFTAG_SDU), "SciData units"},
    {DFTAG_SDF, string(DFTAG_SDF), "SciData formats"},
    {DFTAG_SDM, string(DFTAG_SDM), "SciData max/min"},
    {DFTAG_SDC, string(DFTAG_SDC), "SciData coordsys"},
    {DFTAG_SDT, string(DFTAG_SDT), "Transpose"},
    {DFTAG_SDLNK, string(DFTAG_SDLNK), "Links related to the dataset"},
    {DFTAG_NDG, string(DFTAG_NDG), "Numeric Data Group"},
    {DFTAG_CAL, string(DFTAG_CAL), "Calibration information"},
    {DFTAG_FV, string(DFTAG_FV), "Fill value information"},

      /* V Group Tags */
    {DFTAG_VG, string(DFTAG_VG), "Vgroup"},
    {DFTAG_VH, string(DFTAG_VH), "Vdata"},
    {DFTAG_VS, string(DFTAG_VS), "Vdata Storage"},

      /* Compression Schemes */
    {DFTAG_RLE, string(DFTAG_RLE), "Run Length Encoding"},
    {DFTAG_IMCOMP, string(DFTAG_IMCOMP), "IMCOMP Encoding"},
    {DFTAG_JPEG, string(DFTAG_JPEG), "24-bit JPEG Encoding"},
    {DFTAG_GREYJPEG, string(DFTAG_GREYJPEG), "8-bit JPEG Encoding"},
    {DFTAG_JPEG5, string(DFTAG_JPEG5), "24-bit JPEG Encoding"},
    {DFTAG_GREYJPEG5, string(DFTAG_GREYJPEG5), "8-bit JPEG Encoding"}

};

/* nt_message is the list of NT descriptions in the system, kept as
   NT-description pairs.  To look up a description, a linear search is
   required but efficiency should be okay. */
typedef struct nt_descript_t
  {
      int32       nt;           /* nt for description */
      const char *name;         /* nt name */
      const char *desc;         /* nt description */
  }
nt_descript_t;

PRIVATE const nt_descript_t nt_descriptions[] =
{

/* Masks for types */
    {DFNT_NATIVE, string(DFNT_NATIVE), "native format"},
    {DFNT_CUSTOM, string(DFNT_CUSTOM), "custom format"},
    {DFNT_LITEND, string(DFNT_LITEND), "little-endian format"},

    {DFNT_NONE, string(DFNT_NONE), "number-type not set"},

/* Floating point types */
    {DFNT_FLOAT32, string(DFNT_FLOAT32), "32-bit floating point"},
    {DFNT_FLOAT64, string(DFNT_FLOAT64), "64-bit floating point"},
    {DFNT_FLOAT128, string(DFNT_FLOAT128), "128-bit floating point"},

/* Integer types */
    {DFNT_INT8, string(DFNT_INT8), "8-bit signed integer"},
    {DFNT_UINT8, string(DFNT_UINT8), "8-bit unsigned integer"},
    {DFNT_INT16, string(DFNT_INT16), "16-bit signed integer"},
    {DFNT_UINT16, string(DFNT_UINT16), "16-bit unsigned integer"},
    {DFNT_INT32, string(DFNT_INT32), "32-bit signed integer"},
    {DFNT_UINT32, string(DFNT_UINT32), "32-bit unsigned integer"},
    {DFNT_INT64, string(DFNT_INT64), "64-bit signed integer"},
    {DFNT_UINT64, string(DFNT_UINT64), "64-bit unsigned integer"},
    {DFNT_INT128, string(DFNT_INT128), "128-bit signed integer"},
    {DFNT_UINT128, string(DFNT_UINT128), "128-bit unsigned integer"},

/* Character types */
    {DFNT_CHAR8, string(DFNT_CHAR8), "8-bit signed char"},
    {DFNT_UCHAR8, string(DFNT_UCHAR8), "8-bit unsigned char"},
    {DFNT_CHAR16, string(DFNT_CHAR16), "16-bit signed char"},
    {DFNT_UCHAR16, string(DFNT_UCHAR16), "16-bit unsigned char"}

};

#endif /* __HKIT_H */
