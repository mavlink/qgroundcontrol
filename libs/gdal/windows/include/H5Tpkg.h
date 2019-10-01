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

/*
 * Programmer:	Robb Matzke <matzke@llnl.gov>
 *		Monday, December  8, 1997
 *
 * Purpose:	This file contains declarations which are visible only within
 *		the H5T package.  Source files outside the H5T package should
 *		include H5Tprivate.h instead.
 */
#if !(defined H5T_FRIEND || defined H5T_MODULE)
#error "Do not include this file outside the H5T package!"
#endif

#ifndef _H5Tpkg_H
#define _H5Tpkg_H

/*
 * Define this to enable debugging.
 */
#ifdef NDEBUG
#  undef H5T_DEBUG
#endif

/* Get package's private header */
#include "H5Tprivate.h"

/* Other private headers needed by this file */
#include "H5Fprivate.h"		/* Files				*/
#include "H5FLprivate.h"	/* Free Lists				*/
#include "H5Oprivate.h"		/* Object headers		  	*/

/* Other public headers needed by this file */
#include "H5Spublic.h"		/* Dataspace functions			*/

/* Length of debugging name buffer */
#define H5T_NAMELEN		32

/* Macro to ease detecting "complex" datatypes (i.e. those with base types or fields) */
#define H5T_IS_COMPLEX(t)       ((t) == H5T_COMPOUND || (t) == H5T_ENUM || (t) == H5T_VLEN || (t) == H5T_ARRAY)

/* Macro to ease detecting fixed "string" datatypes */
#define H5T_IS_FIXED_STRING(dt) (H5T_STRING == (dt)->type)

/* Macro to ease detecting variable-length "string" datatypes */
#define H5T_IS_VL_STRING(dt)    (H5T_VLEN == (dt)->type && H5T_VLEN_STRING == (dt)->u.vlen.type)

/* Macro to ease detecting fixed or variable-length "string" datatypes */
#define H5T_IS_STRING(dt)       (H5T_IS_FIXED_STRING(dt) || H5T_IS_VL_STRING(dt))

/* Macro to ease detecting atomic datatypes */
#define H5T_IS_ATOMIC(dt)       (!(H5T_IS_COMPLEX((dt)->type) || (dt)->type == H5T_OPAQUE))

/* Macro to ease retrieving class of shared datatype */
/* (Externally, a VL string is a string; internally, a VL string is a VL.  Lie
 *      to the user if they have a VL string and tell them it's in the string
 *      class)
 */
#define H5T_GET_CLASS(shared, internal) ((internal) ? (shared)->type : (H5T_IS_VL_STRING(shared) ?  H5T_STRING : (shared)->type))


/*
 * Datatype encoding versions
 */

/* This is the version to create all datatypes which don't contain
 * array datatypes (atomic types, compound datatypes without array fields,
 * vlen sequences of objects which aren't arrays, etc.) or VAX byte-ordered
 * objects.
 */
#define H5O_DTYPE_VERSION_1	1

/* This is the version to create all datatypes which contain H5T_ARRAY
 * class objects (array definitely, potentially compound & vlen sequences also),
 * but not VAX byte-ordered objects.
 */
#define H5O_DTYPE_VERSION_2	2

/* This is the version to create all datatypes which contain VAX byte-ordered
 * objects (floating-point types, currently).
 */
/* This version also packs compound & enum field names without padding */
/* This version also encodes the member offset of compound fields more efficiently */
/* This version also encodes array types more efficiently */
#define H5O_DTYPE_VERSION_3	3

/* The latest version of the format.  Look through the 'encode helper' routine
 *      and 'size' callback for places to change when updating this. */
#define H5O_DTYPE_VERSION_LATEST H5O_DTYPE_VERSION_3


/* Flags for visiting datatype */
#define H5T_VISIT_COMPLEX_FIRST 0x01            /* Visit complex datatype before visiting member/parent datatypes */
#define H5T_VISIT_COMPLEX_LAST  0x02            /* Visit complex datatype after visiting member/parent datatypes */
                                                /* (setting both flags will mean visiting complex type twice) */
#define H5T_VISIT_SIMPLE        0x04            /* Visit simple datatypes (at all) */
                                                /* (setting H5T_VISIT_SIMPLE and _not_ setting either H5T_VISIT_COMPLEX_FIRST or H5T_VISIT_COMPLEX_LAST will mean visiting _only_ "simple" "leafs" in the "tree" */
                                                /* (_not_ setting H5T_VISIT_SIMPLE and setting either H5T_VISIT_COMPLEX_FIRST or H5T_VISIT_COMPLEX_LAST will mean visiting all nodes _except_ "simple" "leafs" in the "tree" */


/* Define an internal macro for converting long long to long double.  Mac OS 10.4 gives some
 * incorrect conversions. */
#if (H5_WANT_DATA_ACCURACY && defined(H5_LLONG_TO_LDOUBLE_CORRECT)) || (!H5_WANT_DATA_ACCURACY)
#define H5T_CONV_INTERNAL_LLONG_LDOUBLE       1
#endif

/* Define an internal macro for converting unsigned long long to long double.  SGI compilers give
 * some incorect conversion.  64-bit Solaris does different rounding.   Windows Visual Studio 6 does
 * not support unsigned long long.  For FreeBSD(sleipnir), the last 2 bytes of mantissa are lost when
 * compiler tries to do the conversion.  For Cygwin, compiler doesn't do rounding correctly.
 * Mac OS 10.4 gives some incorrect result. */
#if (H5_WANT_DATA_ACCURACY && defined(H5_LLONG_TO_LDOUBLE_CORRECT)) || (!H5_WANT_DATA_ACCURACY)
#define H5T_CONV_INTERNAL_ULLONG_LDOUBLE         1
#endif

/* Define an internal macro for converting long double to long long.  SGI compilers give some incorrect
 * conversions. Mac OS 10.4 gives incorrect conversions. HP-UX 11.00 compiler generates floating exception.
 * The hard conversion on Windows .NET 2003 has a bug and gives wrong exception value. */
#if (H5_WANT_DATA_ACCURACY && defined(H5_LDOUBLE_TO_LLONG_ACCURATE)) || \
    (!H5_WANT_DATA_ACCURACY)
#define H5T_CONV_INTERNAL_LDOUBLE_LLONG         1
#endif

/* Define an internal macro for converting long double to unsigned long long.  SGI compilers give some
 * incorrect conversions.  Mac OS 10.4 gives incorrect conversions. HP-UX 11.00 compiler generates
 * floating exception. */
#if (H5_WANT_DATA_ACCURACY && defined(H5_LDOUBLE_TO_LLONG_ACCURATE)) || \
    (!H5_WANT_DATA_ACCURACY)
#define H5T_CONV_INTERNAL_LDOUBLE_ULLONG         1
#else
#define H5T_CONV_INTERNAL_LDOUBLE_ULLONG         0
#endif

/* Statistics about a conversion function */
struct H5T_stats_t {
    unsigned	ncalls;			/*num calls to conversion function   */
    hsize_t	nelmts;			/*total data points converted	     */
    H5_timer_t	timer;			/*total time for conversion	     */
};

/* Library internal datatype conversion functions are... */
typedef herr_t (*H5T_lib_conv_t) (hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
      size_t nelmts, size_t buf_stride, size_t bkg_stride, void *buf, void *bkg);

/* Conversion callbacks (library internal ones don't need DXPL) */
typedef struct H5T_conv_func_t {
    hbool_t is_app;                 /* Whether conversion function is registered from application */
    union {
        H5T_conv_t app_func;        /* Application data conversion function */
        H5T_lib_conv_t lib_func;    /* Library internal data conversion function */
    } u;
} H5T_conv_func_t;

/* The datatype conversion database */
struct H5T_path_t {
    char	name[H5T_NAMELEN];	/*name for debugging only	     */
    H5T_t	*src;			/*source datatype 		     */
    H5T_t	*dst;			/*destination datatype		     */
    H5T_conv_func_t conv;               /* Conversion funcion                */
    hbool_t	is_hard;		/*is it a hard function?	     */
    hbool_t	is_noop;		/*is it the noop conversion?	     */
    hbool_t	are_compounds;		/*are source and dest both compounds?*/
    H5T_stats_t	stats;			/*statistics for the conversion	     */
    H5T_cdata_t	cdata;			/*data for this function	     */
};

typedef struct H5T_atomic_t {
    H5T_order_t		order;	/*byte order				     */
    size_t		prec;	/*precision in bits			     */
    size_t		offset; /*bit position of lsb of value		     */
    H5T_pad_t	        lsb_pad;/*type of lsb padding			     */
    H5T_pad_t		msb_pad;/*type of msb padding			     */
    union {
	struct {
	    H5T_sign_t	sign;	/*type of integer sign			     */
	} i;			/*integer; integer types		     */

	struct {
	    size_t	sign;	/*bit position of sign bit		     */
	    size_t	epos;	/*position of lsb of exponent		     */
	    size_t	esize;	/*size of exponent in bits		     */
	    uint64_t	ebias;	/*exponent bias				     */
	    size_t	mpos;	/*position of lsb of mantissa		     */
	    size_t	msize;	/*size of mantissa			     */
	    H5T_norm_t	norm;	/*normalization				     */
	    H5T_pad_t	pad;	/*type of padding for internal bits	     */
	} f;			/*floating-point types			     */

	struct {
	    H5T_cset_t	cset;	/*character set				     */
	    H5T_str_t	pad;	/*space or null padding of extra bytes	     */
	} s;			/*string types				     */

	struct {
	    H5R_type_t	rtype;	/*type of reference stored		     */
            H5T_loc_t   loc;    /* Location of data in buffer		     */
	} r;			/*reference types			     */
    } u;
} H5T_atomic_t;

/* How members are sorted for compound or enum datatypes */
typedef enum H5T_sort_t {
    H5T_SORT_NONE	= 0,		/*not sorted			     */
    H5T_SORT_NAME	= 1,		/*sorted by member name		     */
    H5T_SORT_VALUE	= 2 		/*sorted by memb offset or enum value*/
} H5T_sort_t;

/* A compound datatype member */
typedef struct H5T_cmemb_t {
    char		*name;		/*name of this member		     */
    size_t		offset;		/*offset from beginning of struct    */
    size_t		size;		/*size of this member		     */
    struct H5T_t	*type;		/*type of this member		     */
} H5T_cmemb_t;

/* A compound datatype */
typedef struct H5T_compnd_t {
    unsigned	nalloc;		/*num entries allocated in MEMB array*/
    unsigned	nmembs;		/*number of members defined in struct*/
    H5T_sort_t	sorted;		/*how are members sorted?	     */
    hbool_t     packed;		/*are members packed together?       */
    H5T_cmemb_t	*memb;		/*array of struct members	     */
    size_t      memb_size;	/*total of all member sizes          */
} H5T_compnd_t;

/* An enumeration datatype */
typedef struct H5T_enum_t {
    unsigned	nalloc;		/*num entries allocated		     */
    unsigned	nmembs;		/*number of members defined in enum  */
    H5T_sort_t	sorted;		/*how are members sorted?	     */
    uint8_t	*value;		/*array of values		     */
    char	**name;		/*array of symbol names		     */
} H5T_enum_t;

/* VL function pointers */
typedef ssize_t (*H5T_vlen_getlenfunc_t)(const void *vl_addr);
typedef void * (*H5T_vlen_getptrfunc_t)(void *vl_addr);
typedef htri_t (*H5T_vlen_isnullfunc_t)(const H5F_t *f, void *vl_addr);
typedef herr_t (*H5T_vlen_readfunc_t)(H5F_t *f, void *_vl, void *buf, size_t len);
typedef herr_t (*H5T_vlen_writefunc_t)(H5F_t *f, const H5T_vlen_alloc_info_t *vl_alloc_info, void *_vl, void *buf, void *_bg, size_t seq_len, size_t base_size);
typedef herr_t (*H5T_vlen_setnullfunc_t)(H5F_t *f, void *_vl, void *_bg);

/* VL types */
typedef enum {
    H5T_VLEN_BADTYPE =  -1, /* invalid VL Type */
    H5T_VLEN_SEQUENCE = 0,  /* VL sequence */
    H5T_VLEN_STRING,        /* VL string */
    H5T_VLEN_MAXTYPE        /* highest type (Invalid as true type) */
} H5T_vlen_type_t;

/* A VL datatype */
typedef struct H5T_vlen_t {
    H5T_vlen_type_t     type;   /* Type of VL data in buffer */
    H5T_loc_t		loc;    /* Location of VL data in buffer */
    H5T_cset_t          cset;   /* For VL string. character set */
    H5T_str_t           pad;    /* For VL string.  space or null padding of
                                 * extra bytes */
    H5F_t *f;                   /* File ID (if VL data is on disk) */
    H5T_vlen_getptrfunc_t getptr;   /* Function to get VL sequence pointer */
    H5T_vlen_getlenfunc_t getlen;   /* Function to get VL sequence size (in element units, not bytes) */
    H5T_vlen_isnullfunc_t isnull;   /* Function to check if VL value is NIL */
    H5T_vlen_readfunc_t read;   /* Function to read VL sequence into buffer */
    H5T_vlen_writefunc_t write; /* Function to write VL sequence from buffer */
    H5T_vlen_setnullfunc_t setnull; /* Function to set a VL value to NIL */
} H5T_vlen_t;

/* An opaque datatype */
typedef struct H5T_opaque_t {
    char		*tag;		/*short type description string	     */
} H5T_opaque_t;

/* An array datatype */
typedef struct H5T_array_t {
    size_t	nelem;		/* total number of elements in array */
    unsigned	ndims;		/* member dimensionality        */
    size_t	dim[H5S_MAX_RANK];  /* size in each dimension       */
} H5T_array_t;

typedef enum H5T_state_t {
    H5T_STATE_TRANSIENT, 		/*type is a modifiable, closable transient */
    H5T_STATE_RDONLY,			/*transient, not modifiable, closable */
    H5T_STATE_IMMUTABLE,		/*transient, not modifiable, not closable */
    H5T_STATE_NAMED,			/*named constant, not open	     */
    H5T_STATE_OPEN			/*named constant, open object header */
} H5T_state_t;

    /* This struct is shared between all occurrences of an open named type */
typedef struct H5T_shared_t {
    hsize_t		fo_count; /* number of references to this file object */
    H5T_state_t		state;	/*current state of the type		     */
    H5T_class_t		type;	/*which class of type is this?		     */
    size_t		size;	/*total size of an instance of this type     */
    unsigned            version;        /* Version of object header message to encode this object with */
    hbool_t		force_conv;/* Set if this type always needs to be converted and H5T__conv_noop cannot be called */
    struct H5T_t	*parent;/*parent type for derived datatypes	     */
    union {
        H5T_atomic_t	atomic; /* an atomic datatype              */
        H5T_compnd_t	compnd; /* a compound datatype (struct)    */
        H5T_enum_t	enumer; /* an enumeration type (enum)       */
        H5T_vlen_t	vlen;   /* a variable-length datatype       */
        H5T_opaque_t	opaque; /* an opaque datatype              */
        H5T_array_t	array;  /* an array datatype                */
    } u;
} H5T_shared_t;

struct H5T_t {
    H5O_shared_t    sh_loc;     /* Shared message info (must be first) */

    H5T_shared_t   *shared;     /* all other information */
    H5O_loc_t       oloc;       /* Object location, if the type is a named type */
    H5G_name_t      path;       /* group hier. path if the type is a named type */
};

/* The master list of soft conversion functions */
typedef struct H5T_soft_t {
    char	name[H5T_NAMELEN];	/*name for debugging only	     */
    H5T_class_t src;			/*source datatype class	     */
    H5T_class_t dst;			/*destination datatype class	     */
    H5T_conv_func_t conv;		/*the conversion function	     */
} H5T_soft_t;

/* Bit search direction */
typedef enum H5T_sdir_t {
    H5T_BIT_LSB,			/*search lsb toward msb		     */
    H5T_BIT_MSB				/*search msb toward lsb		     */
} H5T_sdir_t;

/* Typedef for named datatype creation operation */
typedef struct {
    H5T_t *dt;                  /* Datatype to commit */
    hid_t tcpl_id;              /* Named datatype creation property list */
} H5T_obj_create_t;

/* Typedef for datatype iteration operations */
typedef herr_t (*H5T_operator_t)(H5T_t *dt, void *op_data/*in,out*/);

/*  Array of versions for Datatype */
H5_DLLVAR const unsigned H5O_dtype_ver_bounds[H5F_LIBVER_NBOUNDS];

/*
 * Alignment information for native types. A value of N indicates that the
 * data must be aligned on an address ADDR such that 0 == ADDR mod N. When
 * N=1 no alignment is required; N=0 implies that alignment constraints were
 * not calculated.  These alignment info is only for H5Tget_native_type.
 * These values are used for structure alignment.
 */
H5_DLLVAR size_t	H5T_NATIVE_SCHAR_COMP_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_SHORT_COMP_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_COMP_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_LONG_COMP_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_LLONG_COMP_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_FLOAT_COMP_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_DOUBLE_COMP_ALIGN_g;
#if H5_SIZEOF_LONG_DOUBLE !=0
H5_DLLVAR size_t	H5T_NATIVE_LDOUBLE_COMP_ALIGN_g;
#endif

H5_DLLVAR size_t H5T_POINTER_COMP_ALIGN_g;
H5_DLLVAR size_t H5T_HVL_COMP_ALIGN_g;
H5_DLLVAR size_t H5T_HOBJREF_COMP_ALIGN_g;
H5_DLLVAR size_t H5T_HDSETREGREF_COMP_ALIGN_g;

/*
 * Alignment information for native types. A value of N indicates that the
 * data must be aligned on an address ADDR such that 0 == ADDR mod N. When
 * N=1 no alignment is required; N=0 implies that alignment constraints were
 * not calculated.
 */
H5_DLLVAR size_t	H5T_NATIVE_SCHAR_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UCHAR_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_SHORT_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_USHORT_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_LONG_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_ULONG_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_LLONG_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_ULLONG_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_FLOAT_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_DOUBLE_ALIGN_g;
#if H5_SIZEOF_LONG_DOUBLE !=0
H5_DLLVAR size_t	H5T_NATIVE_LDOUBLE_ALIGN_g;
#endif

/* C9x alignment constraints */
H5_DLLVAR size_t	H5T_NATIVE_INT8_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT8_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_LEAST8_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT_LEAST8_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_FAST8_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT_FAST8_ALIGN_g;

H5_DLLVAR size_t	H5T_NATIVE_INT16_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT16_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_LEAST16_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT_LEAST16_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_FAST16_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT_FAST16_ALIGN_g;

H5_DLLVAR size_t	H5T_NATIVE_INT32_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT32_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_LEAST32_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT_LEAST32_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_FAST32_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT_FAST32_ALIGN_g;

H5_DLLVAR size_t	H5T_NATIVE_INT64_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT64_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_LEAST64_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT_LEAST64_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_INT_FAST64_ALIGN_g;
H5_DLLVAR size_t	H5T_NATIVE_UINT_FAST64_ALIGN_g;

/* Useful floating-point values for conversion routines */
/* (+/- Inf for all floating-point types) */
H5_DLLVAR float H5T_NATIVE_FLOAT_POS_INF_g;
H5_DLLVAR float H5T_NATIVE_FLOAT_NEG_INF_g;
H5_DLLVAR double H5T_NATIVE_DOUBLE_POS_INF_g;
H5_DLLVAR double H5T_NATIVE_DOUBLE_NEG_INF_g;
#if H5_SIZEOF_LONG_DOUBLE !=0
H5_DLLVAR double H5T_NATIVE_LDOUBLE_POS_INF_g;
H5_DLLVAR double H5T_NATIVE_LDOUBLE_NEG_INF_g;
#endif

/* Declare extern the free lists for H5T_t's and H5T_shared_t's */
H5FL_EXTERN(H5T_t);
H5FL_EXTERN(H5T_shared_t);

/* Common functions */
H5_DLL herr_t H5T__init_native(void);
H5_DLL H5T_t *H5T__create(H5T_class_t type, size_t size);
H5_DLL H5T_t *H5T__alloc(void);
H5_DLL herr_t H5T__free(H5T_t *dt);
H5_DLL herr_t H5T__visit(H5T_t *dt, unsigned visit_flags, H5T_operator_t op,
    void *op_value);
H5_DLL herr_t H5T__upgrade_version(H5T_t *dt, unsigned new_version);

/* Committed / named datatype routines */
H5_DLL herr_t H5T__commit(H5F_t *file, H5T_t *type, hid_t tcpl_id);
H5_DLL herr_t H5T__commit_named(const H5G_loc_t *loc, const char *name,
    H5T_t *dt, hid_t lcpl_id, hid_t tcpl_id);
H5_DLL H5T_t *H5T__open_name(const H5G_loc_t *loc, const char *name);

/* Conversion functions */
H5_DLL herr_t H5T__conv_noop(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
    size_t nelmts, size_t buf_stride, size_t bkg_stride, void *buf, void *bkg);

H5_DLL herr_t H5T__conv_order(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_order_opt(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_struct(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_struct_opt(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_enum(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_enum_numeric(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_vlen(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_array(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_i_i(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_f_f(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_f_i(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_i_f(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_s_s(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_b_b(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
			    size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);

H5_DLL herr_t H5T__conv_schar_uchar(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_schar(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_short(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_ushort(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_short(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_ushort(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_int(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_uint(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_int(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_uint(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_long(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_ulong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_long(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_ulong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_llong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_ullong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_llong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_ullong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);

H5_DLL herr_t H5T__conv_short_schar(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_uchar(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_schar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_uchar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_ushort(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_short(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_int(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_uint(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_int(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_uint(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_long(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_ulong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_long(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_ulong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_llong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_ullong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_llong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_ullong(hid_t src_id, hid_t dst_id,
				      H5T_cdata_t *cdata, size_t nelmts,
				      size_t buf_stride, size_t bkg_stride,
                                      void *buf, void *bkg);

H5_DLL herr_t H5T__conv_int_schar(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_uchar(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_schar(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_uchar(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_short(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_ushort(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_short(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_ushort(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_uint(hid_t src_id, hid_t dst_id,
				 H5T_cdata_t *cdata, size_t nelmts,
				 size_t buf_stride, size_t bkg_stride,
                                 void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_int(hid_t src_id, hid_t dst_id,
				 H5T_cdata_t *cdata, size_t nelmts,
				 size_t buf_stride, size_t bkg_stride,
                                 void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_long(hid_t src_id, hid_t dst_id,
				 H5T_cdata_t *cdata, size_t nelmts,
				 size_t buf_stride, size_t bkg_stride,
                                 void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_ulong(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_long(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_ulong(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_llong(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_ullong(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_llong(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_ullong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);

H5_DLL herr_t H5T__conv_long_schar(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_uchar(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_schar(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_uchar(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_short(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_ushort(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_short(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_ushort(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_int(hid_t src_id, hid_t dst_id,
				 H5T_cdata_t *cdata, size_t nelmts,
				 size_t buf_stride, size_t bkg_stride,
                                 void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_uint(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_int(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_uint(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_ulong(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_long(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_llong(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_ullong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_llong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_ullong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);

H5_DLL herr_t H5T__conv_llong_schar(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_uchar(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_schar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_uchar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_short(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_ushort(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_short(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_ushort(hid_t src_id, hid_t dst_id,
				      H5T_cdata_t *cdata, size_t nelmts,
				      size_t buf_stride, size_t bkg_stride,
                                      void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_int(hid_t src_id, hid_t dst_id,
				  H5T_cdata_t *cdata, size_t nelmts,
				  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_uint(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_int(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_uint(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_long(hid_t src_id, hid_t dst_id,
				   H5T_cdata_t *cdata, size_t nelmts,
				   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_ulong(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_long(hid_t src_id, hid_t dst_id,
				    H5T_cdata_t *cdata, size_t nelmts,
				    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_ulong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_ullong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_llong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_float(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_double(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_ldouble(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_schar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_uchar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_short(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_ushort(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_int(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_uint(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_long(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_ulong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_llong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_ullong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_schar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_uchar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_short(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_ushort(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_int(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_uint(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_long(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_ulong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_llong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_ullong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_schar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_uchar(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_short(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_ushort(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_int(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_uint(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_long(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_ulong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_llong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_ullong(hid_t src_id, hid_t dst_id,
				     H5T_cdata_t *cdata, size_t nelmts,
				     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg);

/* Bit twiddling functions */
H5_DLL void H5T__bit_copy(uint8_t *dst, size_t dst_offset, const uint8_t *src,
			  size_t src_offset, size_t size);
H5_DLL herr_t H5T__bit_shift(uint8_t *buf, ssize_t shift_dist, size_t offset, size_t size);
H5_DLL void H5T__bit_set(uint8_t *buf, size_t offset, size_t size,
			 hbool_t value);
H5_DLL uint64_t H5T__bit_get_d(uint8_t *buf, size_t offset, size_t size);
H5_DLL void H5T__bit_set_d(uint8_t *buf, size_t offset, size_t size,
			   uint64_t val);
H5_DLL ssize_t H5T__bit_find(uint8_t *buf, size_t offset, size_t size,
			     H5T_sdir_t direction, hbool_t value);
H5_DLL hbool_t H5T__bit_inc(uint8_t *buf, size_t start, size_t size);
H5_DLL hbool_t H5T__bit_dec(uint8_t *buf, size_t start, size_t size);
H5_DLL void H5T__bit_neg(uint8_t *buf, size_t start, size_t size);

/* VL functions */
H5_DLL H5T_t * H5T__vlen_create(const H5T_t *base);
H5_DLL htri_t H5T__vlen_set_loc(const H5T_t *dt, H5F_t *f, H5T_loc_t loc);

/* Array functions */
H5_DLL H5T_t *H5T__array_create(H5T_t *base, unsigned ndims, const hsize_t dim[/* ndims */]);
H5_DLL int    H5T__get_array_ndims(const H5T_t *dt);
H5_DLL int    H5T__get_array_dims(const H5T_t *dt, hsize_t dims[]);

/* Compound functions */
H5_DLL herr_t H5T__insert(H5T_t *parent, const char *name, size_t offset,
        const H5T_t *member);
H5_DLL size_t H5T__get_member_size(const H5T_t *dt, unsigned membno);
H5_DLL void H5T__update_packed(const H5T_t *dt);
H5_DLL H5T_subset_info_t *H5T__conv_struct_subset(const H5T_cdata_t *cdata);

/* Enumerated type functions */
H5_DLL H5T_t *H5T__enum_create(const H5T_t *parent);
H5_DLL herr_t H5T__enum_insert(const H5T_t *dt, const char *name, const void *value);
H5_DLL herr_t H5T__get_member_value(const H5T_t *dt, unsigned membno, void *value);

/* Field functions (for both compound & enumerated types) */
H5_DLL char  *H5T__get_member_name(H5T_t const *dt, unsigned membno);
H5_DLL herr_t H5T__sort_value(const H5T_t *dt, int *map);
H5_DLL herr_t H5T__sort_name(const H5T_t *dt, int *map);

/* Debugging functions */
H5_DLL herr_t H5T__print_stats(H5T_path_t *path, int *nprint/*in,out*/);

#endif /* _H5Tpkg_H */

