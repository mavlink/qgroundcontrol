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

/*+ hlimits.h
   *** This file contains all hard coded limits for the library
   *** and reserved vdata/vgroup names and classes. 
   *** Also pre-defined attribute names are contained in thie file.
   + */

#ifndef _HLIMITS_H
#define _HLIMITS_H

#ifndef _WIN32
#define HDsetvbuf(F,S,M,Z)	setvbuf(F,S,M,Z)
#endif
/**************************************************************************
*  Generally useful macro definitions
*   (These are copied from hdfi.h and shoudl remain included in both files
*       because hlimits.h is included from netcdf.h which is used in some
*       netCDF utilities which don't need or want the rest of the HDF header
*       files. -QAK - 2/17/99 )
**************************************************************************/
#ifndef MIN
#define MIN(a,b)    (((a)<(b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)    (((a)>(b)) ? (a) : (b))
#endif

/* ------------------------- General Constants hdf.h  --------------------- */
/* tbuf used as a temporary buffer for small jobs.  The size is
   preferably > 512 but MUST be > ~256.  It is advised that if an
   arbitrarily large buffer (> 100 bytes) is require, dynamic space be
   used.  tbuf lives in the hfile.c */

#ifndef TBUF_SZ
#   define TBUF_SZ     1024
#endif

/*  File name max length (old annotations)  */
#define DF_MAXFNLEN     256

/*
   * some max lengths for the Vset interface
   *
   * Except for FIELDNAMELENMAX, change these as you please, they
   * affect memory only, not the file.
   *
 */

#define FIELDNAMELENMAX    128  /* fieldname   : 128 chars max */
#define VSFIELDMAX         256  /* max no of fields per vdata */
#define VSNAMELENMAX        64  /* vdata name  : 64 chars max */
#define VGNAMELENMAX        64  /* vgroup name : 64 chars max */
/* Note: VGNAMELENMAX has been removed from library, test, and tools
   except in mfgr.c and Fortran interface, in favor of dynamic allocation.
   BMR- 1/28/2010 */

/*
 * default max no of objects in a vgroup
 * VGroup will grow dynamically if needed
 */
#define MAXNVELT            64

/*
 * Defaults for linked block operations with Vsets
 */
#define VDEFAULTBLKSIZE    4096
#define VDEFAULTNBLKS        32

/* Max order of a field in a Vdata */
#define MAX_ORDER          65535
#define MAX_FIELD_SIZE     65535


/* ------------------------- Constants for hfile.c --------------------- */
/* Maximum number of files (number of slots for file records) */
#ifndef MAX_FILE
#   define MAX_FILE   32
#endif /* MAX_FILE */

/* Maximum length of external filename(s) (used in hextelt.c) */
#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN     1024
#endif /* MAX_PATH_LEN */

/* ndds (number of dd's in a block) default,
   so user need not specify */
#ifndef DEF_NDDS
#   define DEF_NDDS 16
#endif /* DEF_NDDS */

/* ndds minimum, to prevent excessive overhead of very small dd-blocks */
#ifndef MIN_NDDS
#   define MIN_NDDS 4
#endif /* MIN_NDDS */

/* largest number that will fit into 16-bit word ref variable */
#define MAX_REF ((uint16)65535)

/* length of block and number of blocks for converting 'appendable' data */
/* elements into linked blocks (will eventually be replaced by the newer */
/* variable-length blocks */
#define HDF_APPENDABLE_BLOCK_LEN 4096
#define HDF_APPENDABLE_BLOCK_NUM 16

/* hashing information */
#define HASH_MASK       0xff
#define HASH_BLOCK_SIZE 100

/* ------------------------- Constants for Vxx interface --------------------- */

/*
 * Private conversion buffer stuff
 * VDATA_BUFFER_MAX is the largest buffer that can be allocated for
 *   writing (haven't implemented reading yet).
 * Vtbuf is the buffer
 * Vtbufsize is the buffer size in bytes at any given time.
 * Vtbuf is increased in size as need be
 * BUG: the final Vtbuf never gets freed
 */
#define VDATA_BUFFER_MAX 1000000

/* --------------------- Constants for DFSDxx interface --------------------- */

#define DFS_MAXLEN 255       /*  Max length of label/unit/format strings */
#define DFSD_MAXFILL_LEN 16  /* Current max length for fill_value space */

/* ----------------- Constants for COMPRESSION interface --------------------- */

/* Set the following macro to the value the highest compression scheme is */
#define COMP_MAX_COMP   12
#define COMP_HEADER_LENGTH  14

/* ----------------- Constants for DGROUP interface --------------------- */
#define MAX_GROUPS 8

/* ----------------- Constants for HERROR interface --------------------- */
#define FUNC_NAME_LEN   32

/* error_stack is the error stack.  error_top is the stack top pointer, 
   and points tothe next available slot on the stack */
#ifndef ERR_STACK_SZ
#   define ERR_STACK_SZ 10
#endif

/* max size of a stored error description */
#ifndef ERR_STRING_SIZE
#   define ERR_STRING_SIZE 512
#endif

/* ----------------- Constants for NETCDF interface(netcdf.h) ---------------- */
/*
 * This can be as large as the maximum number of stdio streams
 * you can have open on your system.
 */
#define H4_MAX_NC_OPEN MAX_FILE

/*
 * These maximums are enforced by the interface, to facilitate writing
 * applications and utilities.  However, nothing is statically allocated to
 * these sizes internally.
 */
#define H4_MAX_NC_DIMS 5000	 /* max dimensions per file */
#define H4_MAX_NC_ATTRS 3000	 /* max global or per variable attributes */
#define H4_MAX_NC_VARS 5000	 /* max variables per file */
/* This macro changed the behavior of the SDcreate function in HDF4r1.3
 * SDcreate started to fail if SDS name length was greater than 64, instead of truncating
 * it to 64 characters and creating a dataset. Switched back to the old definition.
 * EP 5/5/2000
#define H4_MAX_NC_NAME MIN(256,MIN(VSNAMELENMAX,VGNAMELENMAX)) */

#define H4_MAX_NC_NAME 256		 /* max length of a name */
#define H4_MAX_NC_CLASS 128         /* max length of a class name - added this
        because 128 was used commonly in SD for class name, and this will help
        changing the class name variable declaration much easier - BMR 4/1/02*/
#define H4_MAX_VAR_DIMS 32          /* max per variable dimensions */

/* These definitions here are for backward/forward compatibiliy since major
   constants were modified with H4 prefix to avoid conflicts with the
   real NetCDF-3 library   - EIP 9/5/07                                     */

#ifdef H4_HAVE_NETCDF
#define MAX_NC_OPEN  H4_MAX_NC_OPEN
#define MAX_NC_DIMS  H4_MAX_NC_DIMS
#define MAX_NC_VARS  H4_MAX_NC_VARS
#define MAX_NC_NAME  H4_MAX_NC_NAME
#define MAX_NC_CLASS H4_MAX_NC_CLASS
#define MAX_VAR_DIMS H4_MAX_VAR_DIMS
#endif

/* ----------------- Constants for MFGR interface --------------------- */
#define H4_MAX_GR_NAME 256		 /* max length of a name */

#endif /* _HLIMITS_H */

/* -----------  Reserved classes and names for vdatas/vgroups -----*/

/* The names of the Vgroups created by the GR interface, from mfgr.h */
#define GR_NAME "RIG0.0"          /* name of the Vgroup containing all the images */
#define RI_NAME "RI0.0"           /* name of a Vgroup containing information a
                                     bout one image */
#define RIGATTRNAME  "RIATTR0.0N" /* name of a Vdata containing an 
                                     attribute */
#define RIGATTRCLASS "RIATTR0.0C" /* class of a Vdata containing an 
                                     attribute */
/* Vdata and Vgroup attributes use the same class as that of SD attr,
 *  _HDF_ATTRIBUTE  "Attr0.0"  8/1/96 */

/* classes of the Vdatas/Vgroups created by the SD interface, 
   from local_nc.h  */
#define _HDF_ATTRIBUTE         "Attr0.0" 
        /* class of a Vdata containing SD interface attribute */
#define _HDF_VARIABLE          "Var0.0"
        /* class of a Vgroup representing an SD NDG */
#define _HDF_SDSVAR            "SDSVar"
        /* class of a Vdata indicating its group is an SDS variable */
	/* - only after hdf4r2 */
#define _HDF_CRDVAR          "CoordVar"
        /* name of a Vdata indicating its group is a coordinate variable */
	/* - only after hdf4r2 */
#define _HDF_DIMENSION         "Dim0.0"
        /* class of a Vgroup representing an SD dimension */
#define _HDF_UDIMENSION        "UDim0.0"
        /* class of a Vgroup representing an SD UNLIMITED dimension*/
#define DIM_VALS          "DimVal0.0"
        /* class of a Vdata containing an SD dimension size and fake values */
#define DIM_VALS01        "DimVal0.1"
             /* class of a Vdata containing an SD dimension size */
#define _HDF_CDF               "CDF0.0"
/* DATA is defined in DTM. Change DATA to DATA0 
  #define DATA              "Data0.0" */
#define DATA0             "Data0.0"
#define ATTR_FIELD_NAME   "VALUES"

/* The following vdata class name is reserved by the Chunking interface.
   originally defined in 'hchunks.h'. The full class name 
   currently is "_HDF_CHK_TBL_0". -GV 9/25/97

   Made the vdata class name available to other interfaces since it is needed
   during hmap project. -BMR 11/11/2010 */
#define _HDF_CHK_TBL_CLASS "_HDF_CHK_TBL_" /* 13 bytes */
#define _HDF_CHK_TBL_CLASS_VER  0          /* zero version number for class */

/*
#define NUM_INTERNAL_VGS	6
char *INTERNAL_HDF_VGS[] = {_HDF_VARIABLE, _HDF_DIMENSION, _HDF_UDIMENSION,
		 _HDF_CDF, GR_NAME, RI_NAME}; 

#define NUM_INTERNAL_VDS	8
char *INTERNAL_HDF_VDS[] = {DIM_VALS, DIM_VALS01, _HDF_ATTRIBUTE, _HDF_SDSVAR,
		 _HDF_CRDVAR, "_HDF_CHK_TBL_", RIGATTRNAME, RIGATTRCLASS};

*/
/* ------------  pre-defined attribute names ---------------- */
/* For MFGR interface */
#define FILL_ATTR    "FillValue"   
          /* name of an attribute containing the fill value */

/* For SD interface  */
#define _FillValue      "_FillValue"
          /* name of an attribute to set fill value for an SDS */
#define _HDF_LongName "long_name" /* data/dimension label string  */
#define _HDF_Units    "units"     /* data/dimension unit string   */
#define _HDF_Format   "format"    /* data/dimension format string */
#define _HDF_CoordSys "coordsys"  /* data coordsys string         */
#define _HDF_ValidRange     "valid_range" /* valid range of data values  */
#define _HDF_ScaleFactor    "scale_factor" /* data calibration factor    */
#define _HDF_ScaleFactorErr "scale_factor_err" /* data calibration factor error */
#define _HDF_AddOffset      "add_offset" /* calibration offset           */
#define _HDF_AddOffsetErr   "add_offset_err" /*  calibration offset error */
#define _HDF_CalibratedNt   "calibrated_nt"  /* data type of uncalibrated data */
#define _HDF_ValidMax       "valid_max"
#define _HDF_ValidMin       "valid_min"
#define _HDF_Remarks        "remarks"        /* annotation, by DFAN */
#define _HDF_AnnoLabel      "anno_label"     /* annotation label, by DFAN */
