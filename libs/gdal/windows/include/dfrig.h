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
 * File:    dfrig.h
 * Purpose: header file for the Raster Image set
 * Invokes: df.h
 * Contents:
 *  Structure definitions: DFRdr, DFRrig
 * Remarks: This is included with user programs which use RIG
 *---------------------------------------------------------------------------*/

#ifndef DFRIG   /* avoid re-inclusion */
#define DFRIG

/* description record: used to describe image data, palette data etc. */
typedef struct
  {
      int16       ncomponents;  /* Number of components */
      int16       interlace;    /* data ordering: chunky / planar etc */
      int32       xdim;         /* X-dimension of data */
      int32       ydim;         /* Y-dimensionsof data */
      DFdi        nt;           /* number type of data */
      DFdi        compr;        /* compression */
      /* ### Note: compression is currently uniquely described with a tag.
         No data is attached to this tag/ref.  But this capability is
         provided for future expansion, when this tag/ref might point to
         some data needed for decompression, such as the actual encodings */
  }
DFRdr;

/* structure to hold RIG info */
typedef struct
  {
      char       *cf;           /* color format */
      int32       xpos;         /* X position of image on screen */
      int32       ypos;         /* Y position of image on screen */
      float32     aspectratio;  /* ratio of pixel height to width */
      float32     ccngamma;     /* gamma color correction parameters */
      float32     ccnred[3];    /* red color correction parameters */
      float32     ccngrren[3];  /* green color correction parameters */
      float32     ccnblue[3];   /* blue color correction parameters */
      float32     ccnwhite[3];  /* white color correction parameters */
      DFdi        image;        /* image */
      DFRdr       descimage;    /* image data description */
      DFdi        lut;          /* color look-up table (palette) */
      DFRdr       desclut;      /* look-up table description */
      DFdi        mattechannel; /* matte? */
      DFRdr       descmattechannel;     /* Description of matte? */
  }
DFRrig;

/* dimensions of raster-8 image */
typedef struct R8dim
  {
      uint16      xd;
      uint16      yd;
  }
R8dim;

#endif /*DFRIG */
