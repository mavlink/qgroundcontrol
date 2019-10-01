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

#ifndef _H5IMpublic_H
#define _H5IMpublic_H


#ifdef __cplusplus
extern "C" {
#endif


H5_HLDLL herr_t  H5IMmake_image_8bit( hid_t loc_id,
                            const char *dset_name,
                            hsize_t width,
                            hsize_t height,
                            const unsigned char *buffer );

H5_HLDLL herr_t  H5IMmake_image_24bit( hid_t loc_id,
                             const char *dset_name,
                             hsize_t width,
                             hsize_t height,
                             const char *interlace,
                             const unsigned char *buffer );

H5_HLDLL herr_t  H5IMget_image_info( hid_t loc_id,
                     const char *dset_name,
                     hsize_t *width,
                     hsize_t *height,
                     hsize_t *planes,
                     char    *interlace,
                     hssize_t *npals );

H5_HLDLL herr_t  H5IMread_image( hid_t loc_id,
                       const char *dset_name,
                       unsigned char *buffer );

H5_HLDLL herr_t  H5IMmake_palette( hid_t loc_id,
                         const char *pal_name,
                         const hsize_t *pal_dims,
                         const unsigned char *pal_data );

H5_HLDLL herr_t  H5IMlink_palette( hid_t loc_id,
                        const char *image_name,
                        const char *pal_name );

H5_HLDLL herr_t  H5IMunlink_palette( hid_t loc_id,
                           const char *image_name,
                           const char *pal_name );

H5_HLDLL herr_t  H5IMget_npalettes( hid_t loc_id,
                          const char *image_name,
                          hssize_t *npals );

H5_HLDLL herr_t  H5IMget_palette_info( hid_t loc_id,
                        const char *image_name,
                        int pal_number,
                        hsize_t *pal_dims );

H5_HLDLL herr_t  H5IMget_palette( hid_t loc_id,
                        const char *image_name,
                        int pal_number,
                        unsigned char *pal_data );

H5_HLDLL herr_t  H5IMis_image( hid_t loc_id,
                     const char *dset_name );

H5_HLDLL herr_t  H5IMis_palette( hid_t loc_id,
                     const char *dset_name );

#ifdef __cplusplus
}
#endif

#endif
