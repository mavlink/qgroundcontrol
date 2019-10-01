/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * This file contains public declarations for the H5T module.
 */
#ifndef _H5Tpublic_H
#define _H5Tpublic_H

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"

#define HOFFSET(S,M)    (offsetof(S,M))

/* These are the various classes of datatypes */
/* If this goes over 16 types (0-15), the file format will need to change) */
typedef enum H5T_class_t {
    H5T_NO_CLASS         = -1,  /*error                                      */
    H5T_INTEGER          = 0,   /*integer types                              */
    H5T_FLOAT            = 1,   /*floating-point types                       */
    H5T_TIME             = 2,   /*date and time types                        */
    H5T_STRING           = 3,   /*character string types                     */
    H5T_BITFIELD         = 4,   /*bit field types                            */
    H5T_OPAQUE           = 5,   /*opaque types                               */
    H5T_COMPOUND         = 6,   /*compound types                             */
    H5T_REFERENCE        = 7,   /*reference types                            */
    H5T_ENUM		 = 8,	/*enumeration types                          */
    H5T_VLEN		 = 9,	/*Variable-Length types                      */
    H5T_ARRAY	         = 10,	/*Array types                                */

    H5T_NCLASSES                /*this must be last                          */
} H5T_class_t;

/* Byte orders */
typedef enum H5T_order_t {
    H5T_ORDER_ERROR      = -1,  /*error                                      */
    H5T_ORDER_LE         = 0,   /*little endian                              */
    H5T_ORDER_BE         = 1,   /*bit endian                                 */
    H5T_ORDER_VAX        = 2,   /*VAX mixed endian                           */
    H5T_ORDER_MIXED      = 3,   /*Compound type with mixed member orders     */
    H5T_ORDER_NONE       = 4    /*no particular order (strings, bits,..)     */
    /*H5T_ORDER_NONE must be last */
} H5T_order_t;

/* Types of integer sign schemes */
typedef enum H5T_sign_t {
    H5T_SGN_ERROR        = -1,  /*error                                      */
    H5T_SGN_NONE         = 0,   /*this is an unsigned type                   */
    H5T_SGN_2            = 1,   /*two's complement                           */

    H5T_NSGN             = 2    /*this must be last!                         */
} H5T_sign_t;

/* Floating-point normalization schemes */
typedef enum H5T_norm_t {
    H5T_NORM_ERROR       = -1,  /*error                                      */
    H5T_NORM_IMPLIED     = 0,   /*msb of mantissa isn't stored, always 1     */
    H5T_NORM_MSBSET      = 1,   /*msb of mantissa is always 1                */
    H5T_NORM_NONE        = 2    /*not normalized                             */
    /*H5T_NORM_NONE must be last */
} H5T_norm_t;

/*
 * Character set to use for text strings.  Do not change these values since
 * they appear in HDF5 files!
 */
typedef enum H5T_cset_t {
    H5T_CSET_ERROR       = -1,  /*error                                      */
    H5T_CSET_ASCII       = 0,   /*US ASCII                                   */
    H5T_CSET_UTF8        = 1,   /*UTF-8 Unicode encoding		     */
    H5T_CSET_RESERVED_2  = 2,   /*reserved for later use		     */
    H5T_CSET_RESERVED_3  = 3,   /*reserved for later use		     */
    H5T_CSET_RESERVED_4  = 4,   /*reserved for later use		     */
    H5T_CSET_RESERVED_5  = 5,   /*reserved for later use		     */
    H5T_CSET_RESERVED_6  = 6,   /*reserved for later use		     */
    H5T_CSET_RESERVED_7  = 7,   /*reserved for later use		     */
    H5T_CSET_RESERVED_8  = 8,   /*reserved for later use		     */
    H5T_CSET_RESERVED_9  = 9,   /*reserved for later use		     */
    H5T_CSET_RESERVED_10 = 10,  /*reserved for later use		     */
    H5T_CSET_RESERVED_11 = 11,  /*reserved for later use		     */
    H5T_CSET_RESERVED_12 = 12,  /*reserved for later use		     */
    H5T_CSET_RESERVED_13 = 13,  /*reserved for later use		     */
    H5T_CSET_RESERVED_14 = 14,  /*reserved for later use		     */
    H5T_CSET_RESERVED_15 = 15   /*reserved for later use		     */
} H5T_cset_t;
#define H5T_NCSET H5T_CSET_RESERVED_2    		/*Number of character sets actually defined  */

/*
 * Type of padding to use in character strings.  Do not change these values
 * since they appear in HDF5 files!
 */
typedef enum H5T_str_t {
    H5T_STR_ERROR        = -1,  /*error                                      */
    H5T_STR_NULLTERM     = 0,   /*null terminate like in C                   */
    H5T_STR_NULLPAD      = 1,   /*pad with nulls                             */
    H5T_STR_SPACEPAD     = 2,   /*pad with spaces like in Fortran            */
    H5T_STR_RESERVED_3   = 3,   /*reserved for later use		     */
    H5T_STR_RESERVED_4   = 4,   /*reserved for later use		     */
    H5T_STR_RESERVED_5   = 5,   /*reserved for later use		     */
    H5T_STR_RESERVED_6   = 6,   /*reserved for later use		     */
    H5T_STR_RESERVED_7   = 7,   /*reserved for later use		     */
    H5T_STR_RESERVED_8   = 8,   /*reserved for later use		     */
    H5T_STR_RESERVED_9   = 9,   /*reserved for later use		     */
    H5T_STR_RESERVED_10  = 10,  /*reserved for later use		     */
    H5T_STR_RESERVED_11  = 11,  /*reserved for later use		     */
    H5T_STR_RESERVED_12  = 12,  /*reserved for later use		     */
    H5T_STR_RESERVED_13  = 13,  /*reserved for later use		     */
    H5T_STR_RESERVED_14  = 14,  /*reserved for later use		     */
    H5T_STR_RESERVED_15  = 15   /*reserved for later use		     */
} H5T_str_t;
#define H5T_NSTR H5T_STR_RESERVED_3		/*num H5T_str_t types actually defined	     */

/* Type of padding to use in other atomic types */
typedef enum H5T_pad_t {
    H5T_PAD_ERROR        = -1,  /*error                                      */
    H5T_PAD_ZERO         = 0,   /*always set to zero                         */
    H5T_PAD_ONE          = 1,   /*always set to one                          */
    H5T_PAD_BACKGROUND   = 2,   /*set to background value                    */

    H5T_NPAD             = 3    /*THIS MUST BE LAST                          */
} H5T_pad_t;

/* Commands sent to conversion functions */
typedef enum H5T_cmd_t {
    H5T_CONV_INIT	= 0,	/*query and/or initialize private data	     */
    H5T_CONV_CONV	= 1, 	/*convert data from source to dest datatype */
    H5T_CONV_FREE	= 2	/*function is being removed from path	     */
} H5T_cmd_t;

/* How is the `bkg' buffer used by the conversion function? */
typedef enum H5T_bkg_t {
    H5T_BKG_NO		= 0, 	/*background buffer is not needed, send NULL */
    H5T_BKG_TEMP	= 1,	/*bkg buffer used as temp storage only       */
    H5T_BKG_YES		= 2	/*init bkg buf with data before conversion   */
} H5T_bkg_t;

/* Type conversion client data */
typedef struct H5T_cdata_t {
    H5T_cmd_t		command;/*what should the conversion function do?    */
    H5T_bkg_t		need_bkg;/*is the background buffer needed?	     */
    hbool_t		recalc;	/*recalculate private data		     */
    void		*priv;	/*private data				     */
} H5T_cdata_t;

/* Conversion function persistence */
typedef enum H5T_pers_t {
    H5T_PERS_DONTCARE	= -1, 	/*wild card				     */
    H5T_PERS_HARD	= 0,	/*hard conversion function		     */
    H5T_PERS_SOFT	= 1 	/*soft conversion function		     */
} H5T_pers_t;

/* The order to retrieve atomic native datatype */
typedef enum H5T_direction_t {
    H5T_DIR_DEFAULT     = 0,    /*default direction is inscendent            */
    H5T_DIR_ASCEND      = 1,    /*in inscendent order                        */
    H5T_DIR_DESCEND     = 2     /*in descendent order                        */
} H5T_direction_t;

/* The exception type passed into the conversion callback function */
typedef enum H5T_conv_except_t {
    H5T_CONV_EXCEPT_RANGE_HI       = 0,   /*source value is greater than destination's range */
    H5T_CONV_EXCEPT_RANGE_LOW      = 1,   /*source value is less than destination's range    */
    H5T_CONV_EXCEPT_PRECISION      = 2,   /*source value loses precision in destination      */
    H5T_CONV_EXCEPT_TRUNCATE       = 3,   /*source value is truncated in destination         */
    H5T_CONV_EXCEPT_PINF           = 4,   /*source value is positive infinity(floating number) */
    H5T_CONV_EXCEPT_NINF           = 5,   /*source value is negative infinity(floating number) */
    H5T_CONV_EXCEPT_NAN            = 6    /*source value is NaN(floating number)             */
} H5T_conv_except_t;

/* The return value from conversion callback function H5T_conv_except_func_t */
typedef enum H5T_conv_ret_t {
    H5T_CONV_ABORT      = -1,   /*abort conversion                           */
    H5T_CONV_UNHANDLED  = 0,    /*callback function failed to handle the exception      */
    H5T_CONV_HANDLED    = 1     /*callback function handled the exception successfully  */
} H5T_conv_ret_t;

/* Variable Length Datatype struct in memory */
/* (This is only used for VL sequences, not VL strings, which are stored in char *'s) */
typedef struct {
    size_t len; /* Length of VL data (in base type units) */
    void *p;    /* Pointer to VL data */
} hvl_t;

/* Variable Length String information */
#define H5T_VARIABLE    ((size_t)(-1))  /* Indicate that a string is variable length (null-terminated in C, instead of fixed length) */

/* Opaque information */
#define H5T_OPAQUE_TAG_MAX      256     /* Maximum length of an opaque tag */
                                        /* This could be raised without too much difficulty */

#ifdef __cplusplus
extern "C" {
#endif

/* All datatype conversion functions are... */
typedef herr_t (*H5T_conv_t) (hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
      size_t nelmts, size_t buf_stride, size_t bkg_stride, void *buf,
      void *bkg, hid_t dset_xfer_plist);

/* Exception handler.  If an exception like overflow happenes during conversion,
 * this function is called if it's registered through H5Pset_type_conv_cb.
 */
typedef H5T_conv_ret_t (*H5T_conv_except_func_t)(H5T_conv_except_t except_type,
    hid_t src_id, hid_t dst_id, void *src_buf, void *dst_buf, void *user_data);

/* When this header is included from a private header, don't make calls to H5open() */
#undef H5OPEN
#ifndef _H5private_H
#define H5OPEN          H5open(),
#else   /* _H5private_H */
#define H5OPEN
#endif  /* _H5private_H */

/*
 * The IEEE floating point types in various byte orders.
 */
#define H5T_IEEE_F32BE		(H5OPEN H5T_IEEE_F32BE_g)
#define H5T_IEEE_F32LE		(H5OPEN H5T_IEEE_F32LE_g)
#define H5T_IEEE_F64BE		(H5OPEN H5T_IEEE_F64BE_g)
#define H5T_IEEE_F64LE		(H5OPEN H5T_IEEE_F64LE_g)
H5_DLLVAR hid_t H5T_IEEE_F32BE_g;
H5_DLLVAR hid_t H5T_IEEE_F32LE_g;
H5_DLLVAR hid_t H5T_IEEE_F64BE_g;
H5_DLLVAR hid_t H5T_IEEE_F64LE_g;

/*
 * These are "standard" types.  For instance, signed (2's complement) and
 * unsigned integers of various sizes and byte orders.
 */
#define H5T_STD_I8BE		(H5OPEN H5T_STD_I8BE_g)
#define H5T_STD_I8LE		(H5OPEN H5T_STD_I8LE_g)
#define H5T_STD_I16BE		(H5OPEN H5T_STD_I16BE_g)
#define H5T_STD_I16LE		(H5OPEN H5T_STD_I16LE_g)
#define H5T_STD_I32BE		(H5OPEN H5T_STD_I32BE_g)
#define H5T_STD_I32LE		(H5OPEN H5T_STD_I32LE_g)
#define H5T_STD_I64BE		(H5OPEN H5T_STD_I64BE_g)
#define H5T_STD_I64LE		(H5OPEN H5T_STD_I64LE_g)
#define H5T_STD_U8BE		(H5OPEN H5T_STD_U8BE_g)
#define H5T_STD_U8LE		(H5OPEN H5T_STD_U8LE_g)
#define H5T_STD_U16BE		(H5OPEN H5T_STD_U16BE_g)
#define H5T_STD_U16LE		(H5OPEN H5T_STD_U16LE_g)
#define H5T_STD_U32BE		(H5OPEN H5T_STD_U32BE_g)
#define H5T_STD_U32LE		(H5OPEN H5T_STD_U32LE_g)
#define H5T_STD_U64BE		(H5OPEN H5T_STD_U64BE_g)
#define H5T_STD_U64LE		(H5OPEN H5T_STD_U64LE_g)
#define H5T_STD_B8BE		(H5OPEN H5T_STD_B8BE_g)
#define H5T_STD_B8LE		(H5OPEN H5T_STD_B8LE_g)
#define H5T_STD_B16BE		(H5OPEN H5T_STD_B16BE_g)
#define H5T_STD_B16LE		(H5OPEN H5T_STD_B16LE_g)
#define H5T_STD_B32BE		(H5OPEN H5T_STD_B32BE_g)
#define H5T_STD_B32LE		(H5OPEN H5T_STD_B32LE_g)
#define H5T_STD_B64BE		(H5OPEN H5T_STD_B64BE_g)
#define H5T_STD_B64LE		(H5OPEN H5T_STD_B64LE_g)
#define H5T_STD_REF_OBJ	        (H5OPEN H5T_STD_REF_OBJ_g)
#define H5T_STD_REF_DSETREG     (H5OPEN H5T_STD_REF_DSETREG_g)
H5_DLLVAR hid_t H5T_STD_I8BE_g;
H5_DLLVAR hid_t H5T_STD_I8LE_g;
H5_DLLVAR hid_t H5T_STD_I16BE_g;
H5_DLLVAR hid_t H5T_STD_I16LE_g;
H5_DLLVAR hid_t H5T_STD_I32BE_g;
H5_DLLVAR hid_t H5T_STD_I32LE_g;
H5_DLLVAR hid_t H5T_STD_I64BE_g;
H5_DLLVAR hid_t H5T_STD_I64LE_g;
H5_DLLVAR hid_t H5T_STD_U8BE_g;
H5_DLLVAR hid_t H5T_STD_U8LE_g;
H5_DLLVAR hid_t H5T_STD_U16BE_g;
H5_DLLVAR hid_t H5T_STD_U16LE_g;
H5_DLLVAR hid_t H5T_STD_U32BE_g;
H5_DLLVAR hid_t H5T_STD_U32LE_g;
H5_DLLVAR hid_t H5T_STD_U64BE_g;
H5_DLLVAR hid_t H5T_STD_U64LE_g;
H5_DLLVAR hid_t H5T_STD_B8BE_g;
H5_DLLVAR hid_t H5T_STD_B8LE_g;
H5_DLLVAR hid_t H5T_STD_B16BE_g;
H5_DLLVAR hid_t H5T_STD_B16LE_g;
H5_DLLVAR hid_t H5T_STD_B32BE_g;
H5_DLLVAR hid_t H5T_STD_B32LE_g;
H5_DLLVAR hid_t H5T_STD_B64BE_g;
H5_DLLVAR hid_t H5T_STD_B64LE_g;
H5_DLLVAR hid_t H5T_STD_REF_OBJ_g;
H5_DLLVAR hid_t H5T_STD_REF_DSETREG_g;

/*
 * Types which are particular to Unix.
 */
#define H5T_UNIX_D32BE		(H5OPEN H5T_UNIX_D32BE_g)
#define H5T_UNIX_D32LE		(H5OPEN H5T_UNIX_D32LE_g)
#define H5T_UNIX_D64BE		(H5OPEN H5T_UNIX_D64BE_g)
#define H5T_UNIX_D64LE		(H5OPEN H5T_UNIX_D64LE_g)
H5_DLLVAR hid_t H5T_UNIX_D32BE_g;
H5_DLLVAR hid_t H5T_UNIX_D32LE_g;
H5_DLLVAR hid_t H5T_UNIX_D64BE_g;
H5_DLLVAR hid_t H5T_UNIX_D64LE_g;

/*
 * Types particular to the C language.  String types use `bytes' instead
 * of `bits' as their size.
 */
#define H5T_C_S1		(H5OPEN H5T_C_S1_g)
H5_DLLVAR hid_t H5T_C_S1_g;

/*
 * Types particular to Fortran.
 */
#define H5T_FORTRAN_S1		(H5OPEN H5T_FORTRAN_S1_g)
H5_DLLVAR hid_t H5T_FORTRAN_S1_g;

/*
 * These types are for Intel CPU's.  They are little endian with IEEE
 * floating point.
 */
#define H5T_INTEL_I8		H5T_STD_I8LE
#define H5T_INTEL_I16		H5T_STD_I16LE
#define H5T_INTEL_I32		H5T_STD_I32LE
#define H5T_INTEL_I64		H5T_STD_I64LE
#define H5T_INTEL_U8		H5T_STD_U8LE
#define H5T_INTEL_U16		H5T_STD_U16LE
#define H5T_INTEL_U32		H5T_STD_U32LE
#define H5T_INTEL_U64		H5T_STD_U64LE
#define H5T_INTEL_B8		H5T_STD_B8LE
#define H5T_INTEL_B16		H5T_STD_B16LE
#define H5T_INTEL_B32		H5T_STD_B32LE
#define H5T_INTEL_B64		H5T_STD_B64LE
#define H5T_INTEL_F32		H5T_IEEE_F32LE
#define H5T_INTEL_F64		H5T_IEEE_F64LE

/*
 * These types are for DEC Alpha CPU's.  They are little endian with IEEE
 * floating point.
 */
#define H5T_ALPHA_I8		H5T_STD_I8LE
#define H5T_ALPHA_I16		H5T_STD_I16LE
#define H5T_ALPHA_I32		H5T_STD_I32LE
#define H5T_ALPHA_I64		H5T_STD_I64LE
#define H5T_ALPHA_U8		H5T_STD_U8LE
#define H5T_ALPHA_U16		H5T_STD_U16LE
#define H5T_ALPHA_U32		H5T_STD_U32LE
#define H5T_ALPHA_U64		H5T_STD_U64LE
#define H5T_ALPHA_B8		H5T_STD_B8LE
#define H5T_ALPHA_B16		H5T_STD_B16LE
#define H5T_ALPHA_B32		H5T_STD_B32LE
#define H5T_ALPHA_B64		H5T_STD_B64LE
#define H5T_ALPHA_F32		H5T_IEEE_F32LE
#define H5T_ALPHA_F64		H5T_IEEE_F64LE

/*
 * These types are for MIPS cpu's commonly used in SGI systems. They are big
 * endian with IEEE floating point.
 */
#define H5T_MIPS_I8		H5T_STD_I8BE
#define H5T_MIPS_I16		H5T_STD_I16BE
#define H5T_MIPS_I32		H5T_STD_I32BE
#define H5T_MIPS_I64		H5T_STD_I64BE
#define H5T_MIPS_U8		H5T_STD_U8BE
#define H5T_MIPS_U16		H5T_STD_U16BE
#define H5T_MIPS_U32		H5T_STD_U32BE
#define H5T_MIPS_U64		H5T_STD_U64BE
#define H5T_MIPS_B8		H5T_STD_B8BE
#define H5T_MIPS_B16		H5T_STD_B16BE
#define H5T_MIPS_B32		H5T_STD_B32BE
#define H5T_MIPS_B64		H5T_STD_B64BE
#define H5T_MIPS_F32		H5T_IEEE_F32BE
#define H5T_MIPS_F64		H5T_IEEE_F64BE

/*
 * The VAX floating point types (i.e. in VAX byte order)
 */
#define H5T_VAX_F32		(H5OPEN H5T_VAX_F32_g)
#define H5T_VAX_F64		(H5OPEN H5T_VAX_F64_g)
H5_DLLVAR hid_t H5T_VAX_F32_g;
H5_DLLVAR hid_t H5T_VAX_F64_g;

/*
 * The predefined native types. These are the types detected by H5detect and
 * they violate the naming scheme a little.  Instead of a class name,
 * precision and byte order as the last component, they have a C-like type
 * name.  If the type begins with `U' then it is the unsigned version of the
 * integer type; other integer types are signed.  The type LLONG corresponds
 * to C's `long long' and LDOUBLE is `long double' (these types might be the
 * same as `LONG' and `DOUBLE' respectively).
 */
#define H5T_NATIVE_CHAR		(CHAR_MIN?H5T_NATIVE_SCHAR:H5T_NATIVE_UCHAR)
#define H5T_NATIVE_SCHAR        (H5OPEN H5T_NATIVE_SCHAR_g)
#define H5T_NATIVE_UCHAR        (H5OPEN H5T_NATIVE_UCHAR_g)
#define H5T_NATIVE_SHORT        (H5OPEN H5T_NATIVE_SHORT_g)
#define H5T_NATIVE_USHORT       (H5OPEN H5T_NATIVE_USHORT_g)
#define H5T_NATIVE_INT          (H5OPEN H5T_NATIVE_INT_g)
#define H5T_NATIVE_UINT         (H5OPEN H5T_NATIVE_UINT_g)
#define H5T_NATIVE_LONG         (H5OPEN H5T_NATIVE_LONG_g)
#define H5T_NATIVE_ULONG        (H5OPEN H5T_NATIVE_ULONG_g)
#define H5T_NATIVE_LLONG        (H5OPEN H5T_NATIVE_LLONG_g)
#define H5T_NATIVE_ULLONG       (H5OPEN H5T_NATIVE_ULLONG_g)
#define H5T_NATIVE_FLOAT        (H5OPEN H5T_NATIVE_FLOAT_g)
#define H5T_NATIVE_DOUBLE       (H5OPEN H5T_NATIVE_DOUBLE_g)
#if H5_SIZEOF_LONG_DOUBLE !=0
#define H5T_NATIVE_LDOUBLE	(H5OPEN H5T_NATIVE_LDOUBLE_g)
#endif
#define H5T_NATIVE_B8		(H5OPEN H5T_NATIVE_B8_g)
#define H5T_NATIVE_B16		(H5OPEN H5T_NATIVE_B16_g)
#define H5T_NATIVE_B32		(H5OPEN H5T_NATIVE_B32_g)
#define H5T_NATIVE_B64		(H5OPEN H5T_NATIVE_B64_g)
#define H5T_NATIVE_OPAQUE       (H5OPEN H5T_NATIVE_OPAQUE_g)
#define H5T_NATIVE_HADDR	(H5OPEN H5T_NATIVE_HADDR_g)
#define H5T_NATIVE_HSIZE	(H5OPEN H5T_NATIVE_HSIZE_g)
#define H5T_NATIVE_HSSIZE	(H5OPEN H5T_NATIVE_HSSIZE_g)
#define H5T_NATIVE_HERR		(H5OPEN H5T_NATIVE_HERR_g)
#define H5T_NATIVE_HBOOL	(H5OPEN H5T_NATIVE_HBOOL_g)
H5_DLLVAR hid_t H5T_NATIVE_SCHAR_g;
H5_DLLVAR hid_t H5T_NATIVE_UCHAR_g;
H5_DLLVAR hid_t H5T_NATIVE_SHORT_g;
H5_DLLVAR hid_t H5T_NATIVE_USHORT_g;
H5_DLLVAR hid_t H5T_NATIVE_INT_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT_g;
H5_DLLVAR hid_t H5T_NATIVE_LONG_g;
H5_DLLVAR hid_t H5T_NATIVE_ULONG_g;
H5_DLLVAR hid_t H5T_NATIVE_LLONG_g;
H5_DLLVAR hid_t H5T_NATIVE_ULLONG_g;
H5_DLLVAR hid_t H5T_NATIVE_FLOAT_g;
H5_DLLVAR hid_t H5T_NATIVE_DOUBLE_g;
#if H5_SIZEOF_LONG_DOUBLE !=0
H5_DLLVAR hid_t H5T_NATIVE_LDOUBLE_g;
#endif
H5_DLLVAR hid_t H5T_NATIVE_B8_g;
H5_DLLVAR hid_t H5T_NATIVE_B16_g;
H5_DLLVAR hid_t H5T_NATIVE_B32_g;
H5_DLLVAR hid_t H5T_NATIVE_B64_g;
H5_DLLVAR hid_t H5T_NATIVE_OPAQUE_g;
H5_DLLVAR hid_t H5T_NATIVE_HADDR_g;
H5_DLLVAR hid_t H5T_NATIVE_HSIZE_g;
H5_DLLVAR hid_t H5T_NATIVE_HSSIZE_g;
H5_DLLVAR hid_t H5T_NATIVE_HERR_g;
H5_DLLVAR hid_t H5T_NATIVE_HBOOL_g;

/* C9x integer types */
#define H5T_NATIVE_INT8			(H5OPEN H5T_NATIVE_INT8_g)
#define H5T_NATIVE_UINT8		(H5OPEN H5T_NATIVE_UINT8_g)
#define H5T_NATIVE_INT_LEAST8		(H5OPEN H5T_NATIVE_INT_LEAST8_g)
#define H5T_NATIVE_UINT_LEAST8		(H5OPEN H5T_NATIVE_UINT_LEAST8_g)
#define H5T_NATIVE_INT_FAST8 		(H5OPEN H5T_NATIVE_INT_FAST8_g)
#define H5T_NATIVE_UINT_FAST8		(H5OPEN H5T_NATIVE_UINT_FAST8_g)
H5_DLLVAR hid_t H5T_NATIVE_INT8_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT8_g;
H5_DLLVAR hid_t H5T_NATIVE_INT_LEAST8_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT_LEAST8_g;
H5_DLLVAR hid_t H5T_NATIVE_INT_FAST8_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT_FAST8_g;

#define H5T_NATIVE_INT16		(H5OPEN H5T_NATIVE_INT16_g)
#define H5T_NATIVE_UINT16		(H5OPEN H5T_NATIVE_UINT16_g)
#define H5T_NATIVE_INT_LEAST16		(H5OPEN H5T_NATIVE_INT_LEAST16_g)
#define H5T_NATIVE_UINT_LEAST16		(H5OPEN H5T_NATIVE_UINT_LEAST16_g)
#define H5T_NATIVE_INT_FAST16		(H5OPEN H5T_NATIVE_INT_FAST16_g)
#define H5T_NATIVE_UINT_FAST16		(H5OPEN H5T_NATIVE_UINT_FAST16_g)
H5_DLLVAR hid_t H5T_NATIVE_INT16_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT16_g;
H5_DLLVAR hid_t H5T_NATIVE_INT_LEAST16_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT_LEAST16_g;
H5_DLLVAR hid_t H5T_NATIVE_INT_FAST16_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT_FAST16_g;

#define H5T_NATIVE_INT32		(H5OPEN H5T_NATIVE_INT32_g)
#define H5T_NATIVE_UINT32		(H5OPEN H5T_NATIVE_UINT32_g)
#define H5T_NATIVE_INT_LEAST32		(H5OPEN H5T_NATIVE_INT_LEAST32_g)
#define H5T_NATIVE_UINT_LEAST32		(H5OPEN H5T_NATIVE_UINT_LEAST32_g)
#define H5T_NATIVE_INT_FAST32		(H5OPEN H5T_NATIVE_INT_FAST32_g)
#define H5T_NATIVE_UINT_FAST32		(H5OPEN H5T_NATIVE_UINT_FAST32_g)
H5_DLLVAR hid_t H5T_NATIVE_INT32_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT32_g;
H5_DLLVAR hid_t H5T_NATIVE_INT_LEAST32_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT_LEAST32_g;
H5_DLLVAR hid_t H5T_NATIVE_INT_FAST32_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT_FAST32_g;

#define H5T_NATIVE_INT64		(H5OPEN H5T_NATIVE_INT64_g)
#define H5T_NATIVE_UINT64		(H5OPEN H5T_NATIVE_UINT64_g)
#define H5T_NATIVE_INT_LEAST64		(H5OPEN H5T_NATIVE_INT_LEAST64_g)
#define H5T_NATIVE_UINT_LEAST64 	(H5OPEN H5T_NATIVE_UINT_LEAST64_g)
#define H5T_NATIVE_INT_FAST64		(H5OPEN H5T_NATIVE_INT_FAST64_g)
#define H5T_NATIVE_UINT_FAST64		(H5OPEN H5T_NATIVE_UINT_FAST64_g)
H5_DLLVAR hid_t H5T_NATIVE_INT64_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT64_g;
H5_DLLVAR hid_t H5T_NATIVE_INT_LEAST64_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT_LEAST64_g;
H5_DLLVAR hid_t H5T_NATIVE_INT_FAST64_g;
H5_DLLVAR hid_t H5T_NATIVE_UINT_FAST64_g;

/* Operations defined on all datatypes */
H5_DLL hid_t H5Tcreate(H5T_class_t type, size_t size);
H5_DLL hid_t H5Tcopy(hid_t type_id);
H5_DLL herr_t H5Tclose(hid_t type_id);
H5_DLL htri_t H5Tequal(hid_t type1_id, hid_t type2_id);
H5_DLL herr_t H5Tlock(hid_t type_id);
H5_DLL herr_t H5Tcommit2(hid_t loc_id, const char *name, hid_t type_id,
    hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id);
H5_DLL hid_t H5Topen2(hid_t loc_id, const char *name, hid_t tapl_id);
H5_DLL herr_t H5Tcommit_anon(hid_t loc_id, hid_t type_id, hid_t tcpl_id, hid_t tapl_id);
H5_DLL hid_t H5Tget_create_plist(hid_t type_id);
H5_DLL htri_t H5Tcommitted(hid_t type_id);
H5_DLL herr_t H5Tencode(hid_t obj_id, void *buf, size_t *nalloc);
H5_DLL hid_t H5Tdecode(const void *buf);
H5_DLL herr_t H5Tflush(hid_t type_id);
H5_DLL herr_t H5Trefresh(hid_t type_id);

/* Operations defined on compound datatypes */
H5_DLL herr_t H5Tinsert(hid_t parent_id, const char *name, size_t offset,
			 hid_t member_id);
H5_DLL herr_t H5Tpack(hid_t type_id);

/* Operations defined on enumeration datatypes */
H5_DLL hid_t H5Tenum_create(hid_t base_id);
H5_DLL herr_t H5Tenum_insert(hid_t type, const char *name, const void *value);
H5_DLL herr_t H5Tenum_nameof(hid_t type, const void *value, char *name/*out*/,
			     size_t size);
H5_DLL herr_t H5Tenum_valueof(hid_t type, const char *name,
			      void *value/*out*/);

/* Operations defined on variable-length datatypes */
H5_DLL hid_t H5Tvlen_create(hid_t base_id);

/* Operations defined on array datatypes */
H5_DLL hid_t H5Tarray_create2(hid_t base_id, unsigned ndims,
            const hsize_t dim[/* ndims */]);
H5_DLL int H5Tget_array_ndims(hid_t type_id);
H5_DLL int H5Tget_array_dims2(hid_t type_id, hsize_t dims[]);

/* Operations defined on opaque datatypes */
H5_DLL herr_t H5Tset_tag(hid_t type, const char *tag);
H5_DLL char *H5Tget_tag(hid_t type);

/* Querying property values */
H5_DLL hid_t H5Tget_super(hid_t type);
H5_DLL H5T_class_t H5Tget_class(hid_t type_id);
H5_DLL htri_t H5Tdetect_class(hid_t type_id, H5T_class_t cls);
H5_DLL size_t H5Tget_size(hid_t type_id);
H5_DLL H5T_order_t H5Tget_order(hid_t type_id);
H5_DLL size_t H5Tget_precision(hid_t type_id);
H5_DLL int H5Tget_offset(hid_t type_id);
H5_DLL herr_t H5Tget_pad(hid_t type_id, H5T_pad_t *lsb/*out*/,
			  H5T_pad_t *msb/*out*/);
H5_DLL H5T_sign_t H5Tget_sign(hid_t type_id);
H5_DLL herr_t H5Tget_fields(hid_t type_id, size_t *spos/*out*/,
			     size_t *epos/*out*/, size_t *esize/*out*/,
			     size_t *mpos/*out*/, size_t *msize/*out*/);
H5_DLL size_t H5Tget_ebias(hid_t type_id);
H5_DLL H5T_norm_t H5Tget_norm(hid_t type_id);
H5_DLL H5T_pad_t H5Tget_inpad(hid_t type_id);
H5_DLL H5T_str_t H5Tget_strpad(hid_t type_id);
H5_DLL int H5Tget_nmembers(hid_t type_id);
H5_DLL char *H5Tget_member_name(hid_t type_id, unsigned membno);
H5_DLL int H5Tget_member_index(hid_t type_id, const char *name);
H5_DLL size_t H5Tget_member_offset(hid_t type_id, unsigned membno);
H5_DLL H5T_class_t H5Tget_member_class(hid_t type_id, unsigned membno);
H5_DLL hid_t H5Tget_member_type(hid_t type_id, unsigned membno);
H5_DLL herr_t H5Tget_member_value(hid_t type_id, unsigned membno, void *value/*out*/);
H5_DLL H5T_cset_t H5Tget_cset(hid_t type_id);
H5_DLL htri_t H5Tis_variable_str(hid_t type_id);
H5_DLL hid_t H5Tget_native_type(hid_t type_id, H5T_direction_t direction);

/* Setting property values */
H5_DLL herr_t H5Tset_size(hid_t type_id, size_t size);
H5_DLL herr_t H5Tset_order(hid_t type_id, H5T_order_t order);
H5_DLL herr_t H5Tset_precision(hid_t type_id, size_t prec);
H5_DLL herr_t H5Tset_offset(hid_t type_id, size_t offset);
H5_DLL herr_t H5Tset_pad(hid_t type_id, H5T_pad_t lsb, H5T_pad_t msb);
H5_DLL herr_t H5Tset_sign(hid_t type_id, H5T_sign_t sign);
H5_DLL herr_t H5Tset_fields(hid_t type_id, size_t spos, size_t epos,
			     size_t esize, size_t mpos, size_t msize);
H5_DLL herr_t H5Tset_ebias(hid_t type_id, size_t ebias);
H5_DLL herr_t H5Tset_norm(hid_t type_id, H5T_norm_t norm);
H5_DLL herr_t H5Tset_inpad(hid_t type_id, H5T_pad_t pad);
H5_DLL herr_t H5Tset_cset(hid_t type_id, H5T_cset_t cset);
H5_DLL herr_t H5Tset_strpad(hid_t type_id, H5T_str_t strpad);

/* Type conversion database */
H5_DLL herr_t H5Tregister(H5T_pers_t pers, const char *name, hid_t src_id,
			   hid_t dst_id, H5T_conv_t func);
H5_DLL herr_t H5Tunregister(H5T_pers_t pers, const char *name, hid_t src_id,
			     hid_t dst_id, H5T_conv_t func);
H5_DLL H5T_conv_t H5Tfind(hid_t src_id, hid_t dst_id, H5T_cdata_t **pcdata);
H5_DLL htri_t H5Tcompiler_conv(hid_t src_id, hid_t dst_id);
H5_DLL herr_t H5Tconvert(hid_t src_id, hid_t dst_id, size_t nelmts,
			  void *buf, void *background, hid_t plist_id);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/* Macros */


/* Typedefs */


/* Function prototypes */
H5_DLL herr_t H5Tcommit1(hid_t loc_id, const char *name, hid_t type_id);
H5_DLL hid_t H5Topen1(hid_t loc_id, const char *name);
H5_DLL hid_t H5Tarray_create1(hid_t base_id, int ndims,
            const hsize_t dim[/* ndims */],
            const int perm[/* ndims */]);
H5_DLL int H5Tget_array_dims1(hid_t type_id, hsize_t dims[], int perm[]);

#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif
#endif /* _H5Tpublic_H */

