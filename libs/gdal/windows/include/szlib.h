/* szlib.h -- defines, typedefs, and data structures for szlib API functions. */

/*==============================================================================
The SZIP Science Data Lossless Compression Program is Copyright (C) 2001 Science
& Technology Corporation @ UNM.  All rights released.  Copyright (C) 2003 Lowell
H. Miles and Jack A. Venbrux.  Licensed to ICs Corp. for distribution by the
University of Illinois' National Center for Supercomputing Applications as a
part of the HDF data storage and retrieval file format and software library
products package.  All rights reserved.  Do not modify or use for other
purposes.

SZIP implements an extended Rice adaptive lossless compression algorithm
for sample data.  The primary algorithm was developed by R. F. Rice at
Jet Propulsion Laboratory.

SZIP embodies certain inventions patented by the National Aeronautics &
Space Administration.  United States Patent Nos. 5,448,642, 5,687,255,
and 5,822,457 have been licensed to ICs Corp. for distribution with the
HDF data storage and retrieval file format and software library products.
All rights reserved.

Revocable (in the event of breach by the user or if required by law),
royalty-free, nonexclusive sublicense to use SZIP decompression software
routines and underlying patents is hereby granted by ICs Corp. to all users
of and in conjunction with HDF data storage and retrieval file format and
software library products.

Revocable (in the event of breach by the user or if required by law),
royalty-free, nonexclusive sublicense to use SZIP compression software
routines and underlying patents for non-commercial, scientific use only
is hereby granted by ICs Corp. to users of and in conjunction with HDF
data storage and retrieval file format and software library products.

For commercial use license to SZIP compression software routines and underlying
patents please contact ICs Corp. at ICs Corp., 721 Lochsa Street, Suite 8,
Post Falls, ID 83854.  (208) 262-2008.

==============================================================================*/
#ifndef _SZLIB_H
#define _SZLIB_H

#include "ricehdf.h"
#include "szip_adpt.h"

#define SZLIB_VERSION "2.1.1"

/*
   The application must update next_in and avail_in when avail_in has
   dropped to zero. It must update next_out and avail_out when avail_out
   has dropped to zero. The application must initialize zalloc, zfree and
   opaque before calling the init function. All other fields are set by the
   compression library and must not be updated by the application.

   The fields total_in and total_out can be used for statistics or
   progress reports. After compression, total_in holds the total size of
   the uncompressed data and may be saved for use in the decompressor
   (particularly if the decompressor wants to decompress everything in
   a single step).
*/

#define SZ_NULL  0

/*** API flush values ***/
#define SZ_NO_FLUSH      0
#define SZ_FINISH        4

/*** API state values ***/
#define SZ_INPUT_IMAGE   5
#define SZ_OUTPUT_IMAGE  6

/*** API return values ***/
#define SZ_OK            0
#define SZ_STREAM_END    1
#define SZ_OUTBUFF_FULL 2

/*** API error values defined in ricehdf.h ***/
/* SZ_STREAM_ERROR */
/* SZ_MEM_ERROR    */
/* SZ_INIT_ERROR   */
/* SZ_PARAM_ERROR  */
/* SZ_NO_ENCODER_ERROR  */

/*** API options_mask values defined in ricehdf.h ***/
/* SZ_ALLOW_K13_OPTION_MASK */
/* SZ_CHIP_OPTION_MASK      */
/* SZ_EC_OPTION_MASK        */
/* SZ_LSB_OPTION_MASK       */
/* SZ_MSB_OPTION_MASK       */
/* SZ_NN_OPTION_MASK        */
/* SZ_RAW_OPTION_MASK       */

/*** API MAX limits defined in ricehdf.h ***/
/* SZ_MAX_BLOCKS_PER_SCANLINE */
/* SZ_MAX_PIXELS_PER_BLOCK    */
/* SZ_MAX_PIXELS_PER_SCANLINE */

typedef struct sz_hidden_data_s
    {
    char *image_in;
    long avail_in;
    char *next_in;

    char *image_out;
    long avail_out;
    char *next_out;
    } sz_hidden_data;

typedef struct sz_stream_s
    {
    char            *next_in;  /* next input byte */
    unsigned int    avail_in;  /* number of bytes available at next_in */
    unsigned long    total_in;  /* total nb of input bytes read so far */

    char            *next_out; /* next output byte should be put there */
    unsigned int    avail_out; /* remaining free space at next_out */
    unsigned long    total_out; /* total nb of bytes output so far */

    char            *msg;
    int                state;

    void            *hidden;    /* this data hidden from user */

    int        options_mask;
    int        bits_per_pixel;
    int        pixels_per_block;
    int        pixels_per_scanline;
    long    image_pixels;
    } sz_stream;

typedef sz_stream *sz_streamp;

typedef struct SZ_com_t_s
    {
    int options_mask;
    int bits_per_pixel;
    int pixels_per_block;
    int pixels_per_scanline;
    } SZ_com_t;

__SZ_DLL__ int SZ_BufftoBuffCompress(void *dest, size_t *destLen, const void *source, size_t sourceLen, SZ_com_t *param);
__SZ_DLL__ int SZ_BufftoBuffDecompress(void *dest, size_t *destLen, const void *source, size_t sourceLen, SZ_com_t *param);

__SZ_DLL__ int SZ_DecompressInit(sz_stream *strm);
__SZ_DLL__ int SZ_Decompress(sz_stream *strm, int flush);
__SZ_DLL__ int SZ_DecompressEnd(sz_stream *strm);

__SZ_DLL__ int SZ_CompressInit(sz_stream *strm);
__SZ_DLL__ int SZ_Compress(sz_stream *strm, int flush);
__SZ_DLL__ int SZ_CompressEnd(sz_stream *strm);
__SZ_DLL__ int SZ_encoder_enabled(void);

#endif /* _SZLIB_H */
