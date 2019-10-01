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

/*+ hnt.h
   *** This file contains all the number-type definitions for HDF
   + */

#ifndef _HNT_H
#define _HNT_H

/* masks for types */
#define DFNT_HDF      0x00000000    /* standard HDF format  */
#define DFNT_NATIVE   0x00001000    /* native format        */
#define DFNT_CUSTOM   0x00002000    /* custom format        */
#define DFNT_LITEND   0x00004000    /* Little Endian format */
#define DFNT_MASK     0x00000fff    /* format mask */

/* type info codes */

#define DFNT_NONE        0  /* indicates that number type not set */
#define DFNT_QUERY       0  /* use this code to find the current type */
#define DFNT_VERSION     1  /* current version of NT info */

#define DFNT_FLOAT32     5
#define DFNT_FLOAT       5  /* For backward compat; don't use */
#define DFNT_FLOAT64     6
#define DFNT_DOUBLE      6  /* For backward compat; don't use */
#define DFNT_FLOAT128    7  /* No current plans for support */

#define DFNT_INT8       20
#define DFNT_UINT8      21

#define DFNT_INT16      22
#define DFNT_UINT16     23
#define DFNT_INT32      24
#define DFNT_UINT32     25
#define DFNT_INT64      26
#define DFNT_UINT64     27
#define DFNT_INT128     28  /* No current plans for support */
#define DFNT_UINT128    30  /* No current plans for support */

#define DFNT_UCHAR8      3  /* 3 chosen for backward compatibility */
#define DFNT_UCHAR       3  /* uchar=uchar8 for backward combatibility */
#define DFNT_CHAR8       4  /* 4 chosen for backward compatibility */
#define DFNT_CHAR        4  /* char=char8 for backward combatibility */
#define DFNT_CHAR16     42  /* No current plans for support */
#define DFNT_UCHAR16    43  /* No current plans for support */

/* Type info codes for Native Mode datasets */
#define DFNT_NFLOAT32   (DFNT_NATIVE | DFNT_FLOAT32)
#define DFNT_NFLOAT64   (DFNT_NATIVE | DFNT_FLOAT64)
#define DFNT_NFLOAT128  (DFNT_NATIVE | DFNT_FLOAT128)   /* Unsupported */

#define DFNT_NINT8      (DFNT_NATIVE | DFNT_INT8)
#define DFNT_NUINT8     (DFNT_NATIVE | DFNT_UINT8)
#define DFNT_NINT16     (DFNT_NATIVE | DFNT_INT16)
#define DFNT_NUINT16    (DFNT_NATIVE | DFNT_UINT16)
#define DFNT_NINT32     (DFNT_NATIVE | DFNT_INT32)
#define DFNT_NUINT32    (DFNT_NATIVE | DFNT_UINT32)
#define DFNT_NINT64     (DFNT_NATIVE | DFNT_INT64)
#define DFNT_NUINT64    (DFNT_NATIVE | DFNT_UINT64)
#define DFNT_NINT128    (DFNT_NATIVE | DFNT_INT128)     /* Unsupported */
#define DFNT_NUINT128   (DFNT_NATIVE | DFNT_UINT128)    /* Unsupported */

#define DFNT_NCHAR8     (DFNT_NATIVE | DFNT_CHAR8)
#define DFNT_NCHAR      (DFNT_NATIVE | DFNT_CHAR8)  /* backward compat */
#define DFNT_NUCHAR8    (DFNT_NATIVE | DFNT_UCHAR8)
#define DFNT_NUCHAR     (DFNT_NATIVE | DFNT_UCHAR8)     /* backward compat */
#define DFNT_NCHAR16    (DFNT_NATIVE | DFNT_CHAR16)     /* Unsupported */
#define DFNT_NUCHAR16   (DFNT_NATIVE | DFNT_UCHAR16)    /* Unsupported */

/* Type info codes for Little Endian data */
#define DFNT_LFLOAT32   (DFNT_LITEND | DFNT_FLOAT32)
#define DFNT_LFLOAT64   (DFNT_LITEND | DFNT_FLOAT64)
#define DFNT_LFLOAT128  (DFNT_LITEND | DFNT_FLOAT128)   /* Unsupported */

#define DFNT_LINT8      (DFNT_LITEND | DFNT_INT8)
#define DFNT_LUINT8     (DFNT_LITEND | DFNT_UINT8)
#define DFNT_LINT16     (DFNT_LITEND | DFNT_INT16)
#define DFNT_LUINT16    (DFNT_LITEND | DFNT_UINT16)
#define DFNT_LINT32     (DFNT_LITEND | DFNT_INT32)
#define DFNT_LUINT32    (DFNT_LITEND | DFNT_UINT32)
#define DFNT_LINT64     (DFNT_LITEND | DFNT_INT64)
#define DFNT_LUINT64    (DFNT_LITEND | DFNT_UINT64)
#define DFNT_LINT128    (DFNT_LITEND | DFNT_INT128)     /* Unsupported */
#define DFNT_LUINT128   (DFNT_LITEND | DFNT_UINT128)    /* Unsupported */

#define DFNT_LCHAR8     (DFNT_LITEND | DFNT_CHAR8)
#define DFNT_LCHAR      (DFNT_LITEND | DFNT_CHAR8)  /* backward compat */
#define DFNT_LUCHAR8    (DFNT_LITEND | DFNT_UCHAR8)
#define DFNT_LUCHAR     (DFNT_LITEND | DFNT_UCHAR8)     /* backward compat */
#define DFNT_LCHAR16    (DFNT_LITEND | DFNT_CHAR16)     /* Unsupported */
#define DFNT_LUCHAR16   (DFNT_LITEND | DFNT_UCHAR16)    /* Unsupported */

/* class info codes for int */
#define        DFNTI_MBO       1    /* Motorola byte order 2's compl */
#define        DFNTI_VBO       2    /* Vax byte order 2's compl */
#define        DFNTI_IBO       4    /* Intel byte order 2's compl */

/* class info codes for float */
#define        DFNTF_NONE      0    /* indicates subclass is not set */
#define        DFNTF_HDFDEFAULT 1   /* hdf default float format is ieee */
#define        DFNTF_IEEE      1    /* IEEE format */
#define        DFNTF_VAX       2    /* Vax format */
#define        DFNTF_CRAY      3    /* Cray format */
#define        DFNTF_PC        4    /* PC floats - flipped IEEE */
#define        DFNTF_CONVEX    5    /* CONVEX native format */
#define        DFNTF_VP        6    /* Fujitsu VP native format */

/* class info codes for char */
#define        DFNTC_BYTE      0    /* bitwise/numeric field */
#define        DFNTC_ASCII     1    /* ASCII */
#define        DFNTC_EBCDIC    5    /* EBCDIC */

/* array order */
#define        DFO_FORTRAN     1    /* column major order */
#define        DFO_C           2    /* row major order */

/*******************************************************************/
/* Sizes of number types                                            */
/*******************************************************************/

/* first the standard sizes of number types */

#    define SIZE_FLOAT32    4
#    define SIZE_FLOAT64    8
#    define SIZE_FLOAT128  16   /* No current plans for support */

#    define SIZE_INT8       1
#    define SIZE_UINT8      1
#    define SIZE_INT16      2
#    define SIZE_UINT16     2
#    define SIZE_INT32      4
#    define SIZE_UINT32     4
#    define SIZE_INT64      8
#    define SIZE_UINT64     8
#    define SIZE_INT128    16   /* No current plans for support */
#    define SIZE_UINT128   16   /* No current plans for support */

#    define SIZE_CHAR8      1
#    define SIZE_CHAR       1   /* For backward compat char8 == char */
#    define SIZE_UCHAR8     1
#    define SIZE_UCHAR      1   /* For backward compat uchar8 == uchar */
#    define SIZE_CHAR16     2   /* No current plans for support */
#    define SIZE_UCHAR16    2   /* No current plans for support */

/* then the native sizes of number types */

/* Unusual number sizes */
/* IA64 (IA64) native number sizes:
	Char = 8 bits, signed
	Short=16 int=32 long=64 float=32 double=64 bits
	Long double=64 bits
	Char pointers = 64 bits
	Int pointers = 64 bits
	Little endian, IEEE floating point
*/

#    define SIZE_NFLOAT32    4
#    define SIZE_NFLOAT64    8
#    define SIZE_NFLOAT128  16  /* No current plans for support */

#    define SIZE_NINT8       1
#    define SIZE_NUINT8      1
#    define SIZE_NINT16      2
#    define SIZE_NUINT16     2
#    define SIZE_NINT32      4
#    define SIZE_NUINT32     4
#    define SIZE_NINT64      8
#    define SIZE_NUINT64     8
#    define SIZE_NINT128    16  /* No current plans for support */
#    define SIZE_NUINT128   16  /* No current plans for support */

#    define SIZE_NCHAR8      1
#    define SIZE_NCHAR       1  /* For backward compat char8 == char */
#    define SIZE_NUCHAR8     1
#    define SIZE_NUCHAR      1  /* For backward compat uchar8 == uchar */
#    define SIZE_NCHAR16     2  /* No current plans for support */
#    define SIZE_NUCHAR16    2  /* No current plans for support */

/* then the sizes of little-endian number types */
#    define SIZE_LFLOAT32    4
#    define SIZE_LFLOAT64    8
#    define SIZE_LFLOAT128  16  /* No current plans for support */

#    define SIZE_LINT8       1
#    define SIZE_LUINT8      1
#    define SIZE_LINT16      2
#    define SIZE_LUINT16     2
#    define SIZE_LINT32      4
#    define SIZE_LUINT32     4
#    define SIZE_LINT64      8
#    define SIZE_LUINT64     8
#    define SIZE_LINT128    16  /* No current plans for support */
#    define SIZE_LUINT128   16  /* No current plans for support */

#    define SIZE_LCHAR8      1
#    define SIZE_LCHAR       1  /* For backward compat char8 == char */
#    define SIZE_LUCHAR8     1
#    define SIZE_LUCHAR      1  /* For backward compat uchar8 == uchar */
#    define SIZE_LCHAR16     2  /* No current plans for support */
#    define SIZE_LUCHAR16    2  /* No current plans for support */

    /* sizes of different number types */
#       define MACHINE_I8_SIZE     1
#       define MACHINE_I16_SIZE    2
#       define MACHINE_I32_SIZE    4
#       define MACHINE_F32_SIZE    4
#       define MACHINE_F64_SIZE    8

    /* maximum size of the atomic data types */
#       define MAX_NT_SIZE      16
#endif /* _HNT_H */

