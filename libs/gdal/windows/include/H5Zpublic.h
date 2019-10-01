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

/* Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Thursday, April 16, 1998
 */

#ifndef _H5Zpublic_H
#define _H5Zpublic_H

/* Public headers needed by this file */
#include "H5public.h"

/*
 * Filter identifiers.  Values 0 through 255 are for filters defined by the
 * HDF5 library.  Values 256 through 511 are available for testing new
 * filters. Subsequent values should be obtained from the HDF5 development
 * team at hdf5dev@ncsa.uiuc.edu.  These values will never change because they
 * appear in the HDF5 files.
 */
typedef int H5Z_filter_t;

/* Filter IDs */
#define H5Z_FILTER_ERROR	(-1)	/*no filter			*/
#define H5Z_FILTER_NONE		0	/*reserved indefinitely		*/
#define H5Z_FILTER_DEFLATE	1 	/*deflation like gzip	     	*/
#define H5Z_FILTER_SHUFFLE      2       /*shuffle the data              */
#define H5Z_FILTER_FLETCHER32   3       /*fletcher32 checksum of EDC    */
#define H5Z_FILTER_SZIP         4       /*szip compression              */
#define H5Z_FILTER_NBIT         5       /*nbit compression              */
#define H5Z_FILTER_SCALEOFFSET  6       /*scale+offset compression      */
#define H5Z_FILTER_RESERVED     256	/*filter ids below this value are reserved for library use */

#define H5Z_FILTER_MAX		65535	/*maximum filter id		*/

/* General macros */
#define H5Z_FILTER_ALL	 	0	/* Symbol to remove all filters in H5Premove_filter */
#define H5Z_MAX_NFILTERS        32      /* Maximum number of filters allowed in a pipeline */
                                        /* (should probably be allowed to be an
                                         * unlimited amount, but currently each
                                         * filter uses a bit in a 32-bit field,
                                         * so the format would have to be
                                         * changed to accommodate that)
                                         */

/* Flags for filter definition (stored) */
#define H5Z_FLAG_DEFMASK	0x00ff	/*definition flag mask		*/
#define H5Z_FLAG_MANDATORY      0x0000  /*filter is mandatory		*/
#define H5Z_FLAG_OPTIONAL	0x0001	/*filter is optional		*/

/* Additional flags for filter invocation (not stored) */
#define H5Z_FLAG_INVMASK	0xff00	/*invocation flag mask		*/
#define H5Z_FLAG_REVERSE	0x0100	/*reverse direction; read	*/
#define H5Z_FLAG_SKIP_EDC	0x0200	/*skip EDC filters for read	*/

/* Special parameters for szip compression */
/* [These are aliases for the similar definitions in szlib.h, which we can't
 * include directly due to the duplication of various symbols with the zlib.h
 * header file] */
#define H5_SZIP_ALLOW_K13_OPTION_MASK   1
#define H5_SZIP_CHIP_OPTION_MASK        2
#define H5_SZIP_EC_OPTION_MASK          4
#define H5_SZIP_NN_OPTION_MASK          32
#define H5_SZIP_MAX_PIXELS_PER_BLOCK    32

/* Macros for the shuffle filter */
#define H5Z_SHUFFLE_USER_NPARMS    0    /* Number of parameters that users can set */
#define H5Z_SHUFFLE_TOTAL_NPARMS   1    /* Total number of parameters for filter */

/* Macros for the szip filter */
#define H5Z_SZIP_USER_NPARMS    2       /* Number of parameters that users can set */
#define H5Z_SZIP_TOTAL_NPARMS   4       /* Total number of parameters for filter */
#define H5Z_SZIP_PARM_MASK      0       /* "User" parameter for option mask */
#define H5Z_SZIP_PARM_PPB       1       /* "User" parameter for pixels-per-block */
#define H5Z_SZIP_PARM_BPP       2       /* "Local" parameter for bits-per-pixel */
#define H5Z_SZIP_PARM_PPS       3       /* "Local" parameter for pixels-per-scanline */

/* Macros for the nbit filter */
#define H5Z_NBIT_USER_NPARMS     0     /* Number of parameters that users can set */

/* Macros for the scale offset filter */
#define H5Z_SCALEOFFSET_USER_NPARMS      2    /* Number of parameters that users can set */


/* Special parameters for ScaleOffset filter*/
#define H5Z_SO_INT_MINBITS_DEFAULT 0
typedef enum H5Z_SO_scale_type_t {
    H5Z_SO_FLOAT_DSCALE = 0,
    H5Z_SO_FLOAT_ESCALE = 1,
    H5Z_SO_INT          = 2
} H5Z_SO_scale_type_t;

/* Current version of the H5Z_class_t struct */
#define H5Z_CLASS_T_VERS (1)

/* Values to decide if EDC is enabled for reading data */
typedef enum H5Z_EDC_t {
    H5Z_ERROR_EDC       = -1,   /* error value */
    H5Z_DISABLE_EDC     = 0,
    H5Z_ENABLE_EDC      = 1,
    H5Z_NO_EDC          = 2     /* must be the last */
} H5Z_EDC_t;

/* Bit flags for H5Zget_filter_info */
#define H5Z_FILTER_CONFIG_ENCODE_ENABLED (0x0001)
#define H5Z_FILTER_CONFIG_DECODE_ENABLED (0x0002)

/* Return values for filter callback function */
typedef enum H5Z_cb_return_t {
    H5Z_CB_ERROR  = -1,
    H5Z_CB_FAIL   = 0,    /* I/O should fail if filter fails. */
    H5Z_CB_CONT   = 1,    /* I/O continues if filter fails.   */
    H5Z_CB_NO     = 2
} H5Z_cb_return_t;

/* Filter callback function definition */
typedef H5Z_cb_return_t (*H5Z_filter_func_t)(H5Z_filter_t filter, void* buf,
                                size_t buf_size, void* op_data);

/* Structure for filter callback property */
typedef struct H5Z_cb_t {
    H5Z_filter_func_t   func;
    void                *op_data;
} H5Z_cb_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Before a dataset gets created, the "can_apply" callbacks for any filters used
 * in the dataset creation property list are called
 * with the dataset's dataset creation property list, the dataset's datatype and
 * a dataspace describing a chunk (for chunked dataset storage).
 *
 * The "can_apply" callback must determine if the combination of the dataset
 * creation property list setting, the datatype and the dataspace represent a
 * valid combination to apply this filter to.  For example, some cases of
 * invalid combinations may involve the filter not operating correctly on
 * certain datatypes (or certain datatype sizes), or certain sizes of the chunk
 * dataspace.
 *
 * The "can_apply" callback can be the NULL pointer, in which case, the library
 * will assume that it can apply to any combination of dataset creation
 * property list values, datatypes and dataspaces.
 *
 * The "can_apply" callback returns positive a valid combination, zero for an
 * invalid combination and negative for an error.
 */
typedef htri_t (*H5Z_can_apply_func_t)(hid_t dcpl_id, hid_t type_id, hid_t space_id);

/*
 * After the "can_apply" callbacks are checked for new datasets, the "set_local"
 * callbacks for any filters used in the dataset creation property list are
 * called.  These callbacks receive the dataset's private copy of the dataset
 * creation property list passed in to H5Dcreate (i.e. not the actual property
 * list passed in to H5Dcreate) and the datatype ID passed in to H5Dcreate
 * (which is not copied and should not be modified) and a dataspace describing
 * the chunk (for chunked dataset storage) (which should also not be modified).
 *
 * The "set_local" callback must set any parameters that are specific to this
 * dataset, based on the combination of the dataset creation property list
 * values, the datatype and the dataspace.  For example, some filters perform
 * different actions based on different datatypes (or datatype sizes) or
 * different number of dimensions or dataspace sizes.
 *
 * The "set_local" callback can be the NULL pointer, in which case, the library
 * will assume that there are no dataset-specific settings for this filter.
 *
 * The "set_local" callback must return non-negative on success and negative
 * for an error.
 */
typedef herr_t (*H5Z_set_local_func_t)(hid_t dcpl_id, hid_t type_id, hid_t space_id);

/*
 * A filter gets definition flags and invocation flags (defined above), the
 * client data array and size defined when the filter was added to the
 * pipeline, the size in bytes of the data on which to operate, and pointers
 * to a buffer and its allocated size.
 *
 * The filter should store the result in the supplied buffer if possible,
 * otherwise it can allocate a new buffer, freeing the original.  The
 * allocated size of the new buffer should be returned through the BUF_SIZE
 * pointer and the new buffer through the BUF pointer.
 *
 * The return value from the filter is the number of bytes in the output
 * buffer. If an error occurs then the function should return zero and leave
 * all pointer arguments unchanged.
 */
typedef size_t (*H5Z_func_t)(unsigned int flags, size_t cd_nelmts,
			     const unsigned int cd_values[], size_t nbytes,
			     size_t *buf_size, void **buf);

/*
 * The filter table maps filter identification numbers to structs that
 * contain a pointers to the filter function and timing statistics.
 */
typedef struct H5Z_class2_t {
    int version;                    /* Version number of the H5Z_class_t struct     */
    H5Z_filter_t id;                /* Filter ID number                             */
    unsigned encoder_present;       /* Does this filter have an encoder?            */
    unsigned decoder_present;       /* Does this filter have a decoder?             */
    const char	*name;              /* Comment for debugging                        */
    H5Z_can_apply_func_t can_apply; /* The "can apply" callback for a filter        */
    H5Z_set_local_func_t set_local; /* The "set local" callback for a filter        */
    H5Z_func_t filter;              /* The actual filter function                   */
} H5Z_class2_t;

H5_DLL herr_t H5Zregister(const void *cls);
H5_DLL herr_t H5Zunregister(H5Z_filter_t id);
H5_DLL htri_t H5Zfilter_avail(H5Z_filter_t id);
H5_DLL herr_t H5Zget_filter_info(H5Z_filter_t filter, unsigned int *filter_config_flags);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/*
 * The filter table maps filter identification numbers to structs that
 * contain a pointers to the filter function and timing statistics.
 */
typedef struct H5Z_class1_t {
    H5Z_filter_t id;		/* Filter ID number			     */
    const char	*name;		/* Comment for debugging		     */
    H5Z_can_apply_func_t can_apply; /* The "can apply" callback for a filter */
    H5Z_set_local_func_t set_local; /* The "set local" callback for a filter */
    H5Z_func_t filter;		/* The actual filter function		     */
} H5Z_class1_t;

#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif
#endif

