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

#ifndef _H5PTpublic_H
#define _H5PTpublic_H


#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------
 * Create/Open/Close functions
 *-------------------------------------------------------------------------
 */
/* NOTE: H5PTcreate is replacing H5PTcreate_fl for better name due to the
   removal of H5PTcreate_vl.  H5PTcreate_fl may be retired in 1.8.19. */
H5_HLDLL hid_t H5PTcreate(hid_t loc_id, const char *dset_name,
			hid_t dtype_id, hsize_t chunk_size, hid_t plist_id);

H5_HLDLL hid_t H5PTopen(hid_t loc_id, const char *dset_name);

H5_HLDLL herr_t H5PTclose(hid_t table_id);

/* This function may be removed from the packet table in release 1.8.19. */
H5_HLDLL hid_t H5PTcreate_fl(hid_t loc_id, const char *dset_name,
			hid_t dtype_id, hsize_t chunk_size, int compression);


/*-------------------------------------------------------------------------
 * Write functions
 *-------------------------------------------------------------------------
 */
H5_HLDLL herr_t H5PTappend(hid_t table_id, size_t nrecords, const void *data);

/*-------------------------------------------------------------------------
 * Read functions
 *-------------------------------------------------------------------------
 */
H5_HLDLL herr_t H5PTget_next(hid_t table_id, size_t nrecords, void * data);

H5_HLDLL herr_t H5PTread_packets(hid_t table_id, hsize_t start,
			size_t nrecords, void *data);

/*-------------------------------------------------------------------------
 * Inquiry functions
 *-------------------------------------------------------------------------
 */
H5_HLDLL herr_t H5PTget_num_packets(hid_t table_id, hsize_t *nrecords);

H5_HLDLL herr_t H5PTis_valid(hid_t table_id);

H5_HLDLL herr_t H5PTis_varlen(hid_t table_id);

/*-------------------------------------------------------------------------
 *
 * Accessor functions
 *
 *-------------------------------------------------------------------------
 */

H5_HLDLL hid_t H5PTget_dataset(hid_t table_id);

H5_HLDLL hid_t H5PTget_type(hid_t table_id);

/*-------------------------------------------------------------------------
 *
 * Packet Table "current index" functions
 *
 *-------------------------------------------------------------------------
 */

H5_HLDLL herr_t  H5PTcreate_index( hid_t table_id );

H5_HLDLL herr_t  H5PTset_index( hid_t table_id,
                             hsize_t pt_index );

H5_HLDLL herr_t  H5PTget_index( hid_t table_id,
                             hsize_t *pt_index );

/*-------------------------------------------------------------------------
 *
 * Memory Management functions
 *
 *-------------------------------------------------------------------------
 */

H5_HLDLL herr_t  H5PTfree_vlen_buff( hid_t table_id,
                               size_t bufflen,
                               void * buff );

#ifdef __cplusplus
}
#endif

#endif

