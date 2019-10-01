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

/* $Id: hproto.h 5400 2010-04-22 03:45:32Z bmribler $ */

#ifndef _HDATAINFO_H
#define _HDATAINFO_H

#include "H4api_adpt.h"

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

/* Structure that holds a data descriptor.  First added for GRgetpalinfo. */
typedef struct hdf_ddinfo_t
{
    uint16 tag;
    uint16 ref;
    int32 offset;
    int32 length;
} hdf_ddinfo_t;

/* Public functions for getting raw data information */

    HDFLIBAPI intn ANgetdatainfo
		(int32 ann_id, int32 *offset, int32 *length);

    HDFLIBAPI intn HDgetdatainfo
		(int32 file_id, uint16 data_tag, uint16 data_ref,
		 int32 *chk_coord, uintn start_block, uintn info_count,
		 int32 *offsetarray, int32 *lengtharray);

    HDFLIBAPI intn VSgetdatainfo
		(int32 vsid, uintn start_block, uintn info_count,
		 int32 *offsetarray, int32 *lengtharray);

    HDFLIBAPI intn VSgetattdatainfo
		(int32 vsid, int32 findex, intn attrindex, int32 *offset, int32 *length);

    HDFLIBAPI intn Vgetattdatainfo
		(int32 vgid, intn attrindex, int32 *offset, int32 *length);

    HDFLIBAPI intn GRgetdatainfo
		(int32 riid, uintn start_block, uintn info_count,
		 int32 *offsetarray, int32 *lengtharray);

    HDFLIBAPI intn GRgetattdatainfo
		(int32 id, int32 attrindex, int32 *offset, int32 *length);

    HDFLIBAPI intn GRgetpalinfo(int32 gr_id, uintn pal_count, hdf_ddinfo_t *palinfo_array);

#if defined c_plusplus || defined __cplusplus
}
#endif				/* c_plusplus || __cplusplus */
#endif                          /* _HDATAINFO */

