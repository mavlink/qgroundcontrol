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
 * File:    hconv.h
 * Purpose: header file for data conversion information & structures
 * Invokes:
 * Contents:
 * Structure definitions:
 * Constant definitions: lots...
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef _HCONV_H
#define _HCONV_H

/* This looks redundant because previously, VMS platform would undefine DUFF. */
/* Don't know why. -AKC 2014/1/18 */
#define DUFF
#ifdef DUFF
#define DUFF_sb4b

#define DUFF_ui2i
#define DUFF_ui2s
#define DUFF_uo2i
#define DUFF_ui4i
#define DUFF_ui4s
#define DUFF_uo4i
#define DUFF_ui4f
#define DUFF_uo4f
#define DUFF_ui8f
#define DUFF_uo8f

#define DUFF_lui2i
#define DUFF_lui2s
#define DUFF_luo2i
#define DUFF_lui4i
#define DUFF_lui4s
#define DUFF_luo4i
#define DUFF_lui4f
#define DUFF_luo4f
#define DUFF_lui8f
#define DUFF_luo8f
#endif

/*****************************************************************************/
/* CONSTANT DEFINITIONS                                                      */
/*****************************************************************************/
/* Generally Big-Endian machines */
#if !defined(INTEL86) && !defined(MIPSEL) && !defined(DEC_ALPHA) && !defined(I860) && !defined(SUN386) && !(defined(__ia64) && !(defined(hpux) || defined(__hpux))) && !defined(__x86_64__)
#       define UI8_IN     DFKnb1b   /* Unsigned Integer, 8 bits */
#       define UI8_OUT    DFKnb1b
#       define SI16_IN    DFKnb2b   /* S = Signed */
#       define SI16_OUT   DFKnb2b
#       define UI16_IN    DFKnb2b
#       define UI16_OUT   DFKnb2b
#       define SI32_IN    DFKnb4b
#       define SI32_OUT   DFKnb4b
#       define UI32_IN    DFKnb4b
#       define UI32_OUT   DFKnb4b
#       define F32_IN     DFKnb4b   /* Float, 32 bits */
#       define F32_OUT    DFKnb4b
#       define F64_IN     DFKnb8b
#       define F64_OUT    DFKnb8b

#       define LUI8_IN    DFKnb1b   /* Little Endian Unsigned Integer, 8 bits */
#       define LUI8_OUT   DFKnb1b
#       define LSI16_IN   DFKsb2b
#       define LSI16_OUT  DFKsb2b
#       define LUI16_IN   DFKsb2b
#       define LUI16_OUT  DFKsb2b
#       define LSI32_IN   DFKsb4b
#       define LSI32_OUT  DFKsb4b
#       define LUI32_IN   DFKsb4b
#       define LUI32_OUT  DFKsb4b
#       define LF32_IN    DFKsb4b
#       define LF32_OUT   DFKsb4b
#       define LF64_IN    DFKsb8b
#       define LF64_OUT   DFKsb8b

#else  /* must be INTEL86 || MIPSEL || DEC_ALPHA || I860 || SUN386 || IA64 || Linux64 (Generally, little-endian machines */
#   define UI8_IN     DFKnb1b   /* Big-Endian IEEE support */
#   define UI8_OUT    DFKnb1b   /* The s in DFKsb2b is for swap */
#   define SI16_IN    DFKsb2b
#   define SI16_OUT   DFKsb2b
#   define UI16_IN    DFKsb2b
#   define UI16_OUT   DFKsb2b
#   define SI32_IN    DFKsb4b
#   define SI32_OUT   DFKsb4b
#   define UI32_IN    DFKsb4b
#   define UI32_OUT   DFKsb4b
#   define F32_IN     DFKsb4b
#   define F32_OUT    DFKsb4b
#   define F64_IN     DFKsb8b
#   define F64_OUT    DFKsb8b

#   define LUI8_IN    DFKnb1b   /* Little-Endian IEEE support */
#   define LUI8_OUT   DFKnb1b
#   define LSI16_IN   DFKnb2b
#   define LSI16_OUT  DFKnb2b
#   define LUI16_IN   DFKnb2b
#   define LUI16_OUT  DFKnb2b
#   define LSI32_IN   DFKnb4b
#   define LSI32_OUT  DFKnb4b
#   define LUI32_IN   DFKnb4b
#   define LUI32_OUT  DFKnb4b
#   define LF32_IN    DFKnb4b
#   define LF32_OUT   DFKnb4b
#   define LF64_IN    DFKnb8b
#   define LF64_OUT   DFKnb8b

#endif /* !INTEL86 && !MIPS && !DEC_ALPHA && !I860 && !SUN386 && !IA64 && !Linux64*/

/* All Machines currently use the same routines */
/* for Native mode "conversions" */
#     define NUI8_IN    DFKnb1b
#     define NUI8_OUT   DFKnb1b
#     define NSI16_IN   DFKnb2b
#     define NSI16_OUT  DFKnb2b
#     define NUI16_IN   DFKnb2b
#     define NUI16_OUT  DFKnb2b
#     define NSI32_IN   DFKnb4b
#     define NSI32_OUT  DFKnb4b
#     define NUI32_IN   DFKnb4b
#     define NUI32_OUT  DFKnb4b
#     define NF32_IN    DFKnb4b
#     define NF32_OUT   DFKnb4b
#     define NF64_IN    DFKnb8b
#     define NF64_OUT   DFKnb8b

/*****************************************************************************/
/* STRUCTURE DEFINTIONS                                                      */
/*****************************************************************************/
union fpx
  {
      float       f;
      long        l;
  };

union float_uint_uchar
  {
      float32     f;
      int32       i;
      unsigned char c[4];
  };

#endif /* _HCONV_H */
