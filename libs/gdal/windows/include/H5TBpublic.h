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

#ifndef _H5TBpublic_H
#define _H5TBpublic_H


#ifdef __cplusplus
extern "C" {
#endif


/*-------------------------------------------------------------------------
 *
 * Create functions
 *
 *-------------------------------------------------------------------------
 */

H5_HLDLL herr_t  H5TBmake_table( const char *table_title,
                       hid_t loc_id,
                       const char *dset_name,
                       hsize_t nfields,
                       hsize_t nrecords,
                       size_t type_size,
                       const char *field_names[],
                       const size_t *field_offset,
                       const hid_t *field_types,
                       hsize_t chunk_size,
                       void *fill_data,
                       int compress,
                       const void *buf );


/*-------------------------------------------------------------------------
 *
 * Write functions
 *
 *-------------------------------------------------------------------------
 */

H5_HLDLL herr_t  H5TBappend_records( hid_t loc_id,
                           const char *dset_name,
                           hsize_t nrecords,
                           size_t type_size,
                           const size_t *field_offset,
                           const size_t *dst_sizes,
                           const void *buf );

H5_HLDLL herr_t  H5TBwrite_records( hid_t loc_id,
                          const char *dset_name,
                          hsize_t start,
                          hsize_t nrecords,
                          size_t type_size,
                          const size_t *field_offset,
                          const size_t *dst_sizes,
                          const void *buf );


H5_HLDLL herr_t  H5TBwrite_fields_name( hid_t loc_id,
                              const char *dset_name,
                              const char *field_names,
                              hsize_t start,
                              hsize_t nrecords,
                              size_t type_size,
                              const size_t *field_offset,
                              const size_t *dst_sizes,
                              const void *buf );

H5_HLDLL herr_t  H5TBwrite_fields_index( hid_t loc_id,
                               const char *dset_name,
                               hsize_t nfields,
                               const int *field_index,
                               hsize_t start,
                               hsize_t nrecords,
                               size_t type_size,
                               const size_t *field_offset,
                               const size_t *dst_sizes,
                               const void *buf );


/*-------------------------------------------------------------------------
 *
 * Read functions
 *
 *-------------------------------------------------------------------------
 */



H5_HLDLL herr_t  H5TBread_table( hid_t loc_id,
                       const char *dset_name,
                       size_t dst_size,
                       const size_t *dst_offset,
                       const size_t *dst_sizes,
                       void *dst_buf );


H5_HLDLL herr_t  H5TBread_fields_name( hid_t loc_id,
                             const char *dset_name,
                             const char *field_names,
                             hsize_t start,
                             hsize_t nrecords,
                             size_t type_size,
                             const size_t *field_offset,
                             const size_t *dst_sizes,
                             void *buf );

H5_HLDLL herr_t  H5TBread_fields_index( hid_t loc_id,
                              const char *dset_name,
                              hsize_t nfields,
                              const int *field_index,
                              hsize_t start,
                              hsize_t nrecords,
                              size_t type_size,
                              const size_t *field_offset,
                              const size_t *dst_sizes,
                              void *buf );


H5_HLDLL herr_t  H5TBread_records( hid_t loc_id,
                         const char *dset_name,
                         hsize_t start,
                         hsize_t nrecords,
                         size_t type_size,
                         const size_t *dst_offset,
                         const size_t *dst_sizes,
                         void *buf );

/*-------------------------------------------------------------------------
 *
 * Inquiry functions
 *
 *-------------------------------------------------------------------------
 */


H5_HLDLL herr_t  H5TBget_table_info ( hid_t loc_id,
                            const char *dset_name,
                            hsize_t *nfields,
                            hsize_t *nrecords );

H5_HLDLL herr_t  H5TBget_field_info( hid_t loc_id,
                           const char *dset_name,
                           char *field_names[],
                           size_t *field_sizes,
                           size_t *field_offsets,
                           size_t *type_size );


/*-------------------------------------------------------------------------
 *
 * Manipulation functions
 *
 *-------------------------------------------------------------------------
 */


H5_HLDLL herr_t  H5TBdelete_record( hid_t loc_id,
                          const char *dset_name,
                          hsize_t start,
                          hsize_t nrecords );


H5_HLDLL herr_t  H5TBinsert_record( hid_t loc_id,
                          const char *dset_name,
                          hsize_t start,
                          hsize_t nrecords,
                          size_t dst_size,
                          const size_t *dst_offset,
                          const size_t *dst_sizes,
                          void *buf );

H5_HLDLL herr_t  H5TBadd_records_from( hid_t loc_id,
                             const char *dset_name1,
                             hsize_t start1,
                             hsize_t nrecords,
                             const char *dset_name2,
                             hsize_t start2 );

H5_HLDLL herr_t  H5TBcombine_tables( hid_t loc_id1,
                           const char *dset_name1,
                           hid_t loc_id2,
                           const char *dset_name2,
                           const char *dset_name3 );

H5_HLDLL herr_t  H5TBinsert_field( hid_t loc_id,
                         const char *dset_name,
                         const char *field_name,
                         hid_t field_type,
                         hsize_t position,
                         const void *fill_data,
                         const void *buf );

H5_HLDLL herr_t  H5TBdelete_field( hid_t loc_id,
                         const char *dset_name,
                         const char *field_name );


/*-------------------------------------------------------------------------
 *
 * Table attribute functions
 *
 *-------------------------------------------------------------------------
 */

H5_HLDLL herr_t  H5TBAget_title( hid_t loc_id,
                       char *table_title );

H5_HLDLL htri_t  H5TBAget_fill(hid_t loc_id,
                      const char *dset_name,
                      hid_t dset_id,
                      unsigned char *dst_buf);

#ifdef __cplusplus
}
#endif


#endif

