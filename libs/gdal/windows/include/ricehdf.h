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
/********************************************************/
/* defines and declarations to interface to the szip    */
/* functions in file rice.c.                            */
/* USE EXTREME CARE WHEN MODIFYING THIS FILE            */
/********************************************************/
extern int szip_allow_encoding;

#define SZ_ALLOW_K13_OPTION_MASK         1
#define SZ_CHIP_OPTION_MASK              2 
#define SZ_EC_OPTION_MASK                4
#define SZ_LSB_OPTION_MASK               8
#define SZ_MSB_OPTION_MASK              16
#define SZ_NN_OPTION_MASK               32
#define SZ_RAW_OPTION_MASK             128

#define SZ_STREAM_ERROR 	(-1)
#define SZ_MEM_ERROR    	(-2)
#define SZ_INIT_ERROR   	(-3)
#define SZ_PARAM_ERROR  	(-4)
#define SZ_NO_ENCODER_ERROR (-5)

#define SZ_MAX_BLOCKS_PER_SCANLINE            128
#define SZ_MAX_PIXELS_PER_BLOCK                32
#define SZ_MAX_PIXELS_PER_SCANLINE     (SZ_MAX_BLOCKS_PER_SCANLINE)*(SZ_MAX_PIXELS_PER_BLOCK)

long szip_compress_memory(
	int options_mask,
	int bits_per_pixel,
	int pixels_per_block,
	int pixels_per_scanline,
	const void *in,
	long pixels,
	char *out);

long szip_uncompress_memory(
	int new_options_mask,
	int new_bits_per_pixel,
	int new_pixels_per_block,
	int new_pixels_per_scanline, 
	const char *in,
	long in_bytes,
	void *out,
	long out_pixels);

int szip_check_params(
	int bits_per_pixel,
	int pixels_per_block,
	int pixels_per_scanline,
	long image_pixels,
	char **msg);
