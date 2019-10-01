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
 * File:    hcompi.h
 * Purpose: Internal library header file for compression information
 * Dependencies: should be included after hdf.h
 * Invokes:
 * Contents:
 * Structure definitions:
 * Constant definitions:
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef __HCOMPI_H
#define __HCOMPI_H

#include "hfile.h"

/* Modeling information */

/* structure for storing modeling information */
/* only allow modeling and master compression routines access */

#include "mstdio.h"     /* stdio modeling header */

typedef struct comp_model_info_tag
  {
      comp_model_t model_type;  /* model this stream is using */
      union
        {                       /* union of all the different types of model information */
            comp_model_stdio_info_t stdio_info;     /* stdio model info */
        }
      model_info;
      funclist_t  model_funcs;  /* functions to perform modeling */
  }
comp_model_info_t;

/* Coding information */

/* structure for storing modeling information */
/* only allow encoding and master compression routines access */

#include "cnone.h"  /* no encoding header */
#include "crle.h"   /* run-length encoding header */
#include "cnbit.h"  /* N-bit encoding header */
#include "cskphuff.h"   /* Skipping huffman encoding header */
#include "cdeflate.h"   /* gzip 'deflate' encoding header */
#include "cszip.h"   /* szip encoding header */

typedef struct comp_coder_info_tag
  {
      comp_coder_t coder_type;  /* coding scheme this stream is using */
      union
        {                       /* union of all the different types of coding information */
            comp_coder_none_info_t none_info;   /* "None" coding info */
            comp_coder_rle_info_t rle_info;     /* RLE coding info */
            comp_coder_nbit_info_t nbit_info;   /* N-bit coding info */
            comp_coder_skphuff_info_t skphuff_info;     /* Skipping huffman coding info */
            comp_coder_deflate_info_t deflate_info;   /* gzip 'deflate' coding info */
	    comp_coder_szip_info_t szip_info;   /* szip coding info */

        }
      coder_info;
      funclist_t  coder_funcs;  /* functions to perform encoding */
  }
comp_coder_info_t;

/* structure for storing a state */
typedef struct comp_stateinfo_tag
  {
      uint32      d_offset;     /* the offset of the state in the dataset */
      uint32      c_offset;     /* offset of the state in the compressed data */
      comp_model_info_t minfo;  /* modeling information */
      comp_coder_info_t cinfo;  /* coding information */
  }
comp_stateinfo_t;

/* structure for storing state caching information */
typedef struct comp_state_cache_tag
  {
      intn        num_states;   /* the number of states cached */
      comp_stateinfo_t **comp_state;    /* pointer to an array of pointers to
                                           compression states */
  }
comp_state_cache_t;

/* compinfo_t -- compressed element information structure */
typedef struct compinfo_tag
  {
      intn        attached;     /* number of access records attached
                                   to this information structure */
      int32       length;       /* the actual length of the data elt */
      uint16      comp_ref;     /* compressed info ref. number */
      int32       aid;          /* AID of the compressed info */
      comp_model_info_t minfo;  /* modeling information */
      comp_coder_info_t cinfo;  /* coding information */
      intn        caching;      /* whether caching is turned on */
      comp_state_cache_t sinfo; /* state information for caching */
  }
compinfo_t;

#endif /* __HCOMPI_H */
