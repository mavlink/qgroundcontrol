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

#ifndef _H5DSpublic_H
#define _H5DSpublic_H



#define DIMENSION_SCALE_CLASS "DIMENSION_SCALE"
#define DIMENSION_LIST        "DIMENSION_LIST"
#define REFERENCE_LIST        "REFERENCE_LIST"
#define DIMENSION_LABELS      "DIMENSION_LABELS"


typedef herr_t  (*H5DS_iterate_t)(hid_t dset, unsigned dim, hid_t scale, void *visitor_data);


#ifdef __cplusplus
extern "C" {
#endif

H5_HLDLL herr_t  H5DSattach_scale( hid_t did,
                        hid_t dsid,
                        unsigned int idx);

H5_HLDLL herr_t  H5DSdetach_scale( hid_t did,
                        hid_t dsid,
                        unsigned int idx);

H5_HLDLL herr_t  H5DSset_scale( hid_t dsid,
                     const char *dimname);

H5_HLDLL int H5DSget_num_scales( hid_t did,
                       unsigned int dim);

H5_HLDLL herr_t  H5DSset_label( hid_t did,
                     unsigned int idx,
                     const char *label);

H5_HLDLL ssize_t H5DSget_label( hid_t did,
                      unsigned int idx,
                      char *label,
                      size_t size);

H5_HLDLL ssize_t H5DSget_scale_name( hid_t did,
                           char *name,
                           size_t size);

H5_HLDLL htri_t H5DSis_scale( hid_t did);

H5_HLDLL herr_t  H5DSiterate_scales( hid_t did,
                          unsigned int dim,
                          int *idx,
                          H5DS_iterate_t visitor,
                          void *visitor_data);

H5_HLDLL htri_t H5DSis_attached( hid_t did,
                       hid_t dsid,
                       unsigned int idx);



#ifdef __cplusplus
}
#endif

#endif
