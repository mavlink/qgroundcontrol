/*! \file

Main header file for the C API.

Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002,
2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014
University Corporation for Atmospheric Research/Unidata.

See \ref copyright file for more info.
*/

#ifndef _NETCDF_
#define _NETCDF_

#include <stddef.h> /* size_t, ptrdiff_t */
#include <errno.h>  /* netcdf functions sometimes return system errors */

/* Required for alloca on Windows */
#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#endif

/*! The nc_type type is just an int. */
typedef int nc_type;

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *  The netcdf external data types
 */
#define NC_NAT          0       /**< Not A Type */
#define NC_BYTE         1       /**< signed 1 byte integer */
#define NC_CHAR         2       /**< ISO/ASCII character */
#define NC_SHORT        3       /**< signed 2 byte integer */
#define NC_INT          4       /**< signed 4 byte integer */
#define NC_LONG         NC_INT  /**< \deprecated required for backward compatibility. */
#define NC_FLOAT        5       /**< single precision floating point number */
#define NC_DOUBLE       6       /**< double precision floating point number */
#define NC_UBYTE        7       /**< unsigned 1 byte int */
#define NC_USHORT       8       /**< unsigned 2-byte int */
#define NC_UINT         9       /**< unsigned 4-byte int */
#define NC_INT64        10      /**< signed 8-byte int */
#define NC_UINT64       11      /**< unsigned 8-byte int */
#define NC_STRING       12      /**< string */

#define NC_MAX_ATOMIC_TYPE NC_STRING /**< @internal Largest atomic type. */

/* The following are use internally in support of user-defines
 * types. They are also the class returned by nc_inq_user_type. */
#define NC_VLEN         13      /**< vlen (variable-length) types */
#define NC_OPAQUE       14      /**< opaque types */
#define NC_ENUM         15      /**< enum types */
#define NC_COMPOUND     16      /**< compound types */

/** @internal Define the first user defined type id (leave some
 * room) */
#define NC_FIRSTUSERTYPEID 32

/** Default fill value. This is used unless _FillValue attribute
 * is set.  These values are stuffed into newly allocated space as
 * appropriate.  The hope is that one might use these to notice that a
 * particular datum has not been set. */
/**@{*/
#define NC_FILL_BYTE    ((signed char)-127)
#define NC_FILL_CHAR    ((char)0)
#define NC_FILL_SHORT   ((short)-32767)
#define NC_FILL_INT     (-2147483647)
#define NC_FILL_FLOAT   (9.9692099683868690e+36f) /* near 15 * 2^119 */
#define NC_FILL_DOUBLE  (9.9692099683868690e+36)
#define NC_FILL_UBYTE   (255)
#define NC_FILL_USHORT  (65535)
#define NC_FILL_UINT    (4294967295U)
#define NC_FILL_INT64   ((long long)-9223372036854775806LL)
#define NC_FILL_UINT64  ((unsigned long long)18446744073709551614ULL)
#define NC_FILL_STRING  ((char *)"")
/**@}*/

/*! Max or min values for a type. Nothing greater/smaller can be
 * stored in a netCDF file for their associated types. Recall that a C
 * compiler may define int to be any length it wants, but a NC_INT is
 * *always* a 4 byte signed int. On a platform with 64 bit ints,
 * there will be many ints which are outside the range supported by
 * NC_INT. But since NC_INT is an external format, it has to mean the
 * same thing everywhere. */
/**@{*/
#define NC_MAX_BYTE 127
#define NC_MIN_BYTE (-NC_MAX_BYTE-1)
#define NC_MAX_CHAR 255
#define NC_MAX_SHORT 32767
#define NC_MIN_SHORT (-NC_MAX_SHORT - 1)
#define NC_MAX_INT 2147483647
#define NC_MIN_INT (-NC_MAX_INT - 1)
#define NC_MAX_FLOAT 3.402823466e+38f
#define NC_MIN_FLOAT (-NC_MAX_FLOAT)
#define NC_MAX_DOUBLE 1.7976931348623157e+308
#define NC_MIN_DOUBLE (-NC_MAX_DOUBLE)
#define NC_MAX_UBYTE NC_MAX_CHAR
#define NC_MAX_USHORT 65535U
#define NC_MAX_UINT 4294967295U
#define NC_MAX_INT64 (9223372036854775807LL)
#define NC_MIN_INT64 (-9223372036854775807LL-1)
#define NC_MAX_UINT64 (18446744073709551615ULL)
/**@}*/

/** Name of fill value attribute.  If you wish a variable to use a
 * different value than the above defaults, create an attribute with
 * the same type as the variable and this reserved name. The value you
 * give the attribute will be used as the fill value for that
 * variable. */
#define _FillValue      "_FillValue"
#define NC_FILL         0       /**< Argument to nc_set_fill() to clear NC_NOFILL */
#define NC_NOFILL       0x100   /**< Argument to nc_set_fill() to turn off filling of data. */

/* Define the ioflags bits for nc_create and nc_open.
   currently unused:
        0x0002
   and the whole upper 16 bits
*/

#define NC_NOWRITE       0x0000 /**< Set read-only access for nc_open(). */
#define NC_WRITE         0x0001 /**< Set read-write access for nc_open(). */

#define NC_CLOBBER       0x0000 /**< Destroy existing file. Mode flag for nc_create(). */
#define NC_NOCLOBBER     0x0004 /**< Don't destroy existing file. Mode flag for nc_create(). */

#define NC_DISKLESS      0x0008  /**< Use diskless file. Mode flag for nc_open() or nc_create(). */
#define NC_MMAP          0x0010  /**< \deprecated Use diskless file with mmap. Mode flag for nc_open() or nc_create()*/

#define NC_64BIT_DATA    0x0020  /**< CDF-5 format: classic model but 64 bit dimensions and sizes */
#define NC_CDF5          NC_64BIT_DATA  /**< Alias NC_CDF5 to NC_64BIT_DATA */

#define NC_UDF0          0x0040  /**< User-defined format 0. */
#define NC_UDF1          0x0080  /**< User-defined format 1. */

#define NC_CLASSIC_MODEL 0x0100 /**< Enforce classic model on netCDF-4. Mode flag for nc_create(). */
#define NC_64BIT_OFFSET  0x0200  /**< Use large (64-bit) file offsets. Mode flag for nc_create(). */

/** \deprecated The following flag currently is ignored, but use in
 * nc_open() or nc_create() may someday support use of advisory
 * locking to prevent multiple writers from clobbering a file
 */
#define NC_LOCK          0x0400

/** Share updates, limit caching.
Use this in mode flags for both nc_create() and nc_open(). */
#define NC_SHARE         0x0800

#define NC_NETCDF4       0x1000  /**< Use netCDF-4/HDF5 format. Mode flag for nc_create(). */

/** Turn on MPI I/O.
Use this in mode flags for both nc_create() and nc_open(). */
#define NC_MPIIO         0x2000 /**< \deprecated */
/** Turn on MPI POSIX I/O.
Use this in mode flags for both nc_create() and nc_open(). */
#define NC_MPIPOSIX      NC_MPIIO /**< \deprecated As of libhdf5 1.8.13. Now an alias */

#define NC_PERSIST       0x4000  /**< Save diskless contents to disk. Mode flag for nc_open() or nc_create() */
#define NC_INMEMORY      0x8000  /**< Read from memory. Mode flag for nc_open() or nc_create() */

#define NC_PNETCDF       (NC_MPIIO) /**< \deprecated Use PnetCDF library; alias for NC_MPIIO. */

#define NC_MAX_MAGIC_NUMBER_LEN 8 /**< Max len of user-defined format magic number. */

/** Format specifier for nc_set_default_format() and returned
 *  by nc_inq_format. This returns the format as provided by
 *  the API. See nc_inq_format_extended to see the true file format.
 *  Starting with version 3.6, there are different format netCDF files.
 *  4.0 introduces the third one. \see netcdf_format
 */
/**@{*/
#define NC_FORMAT_CLASSIC         (1)
/* After adding CDF5 support, this flag
   is somewhat confusing. So, it is renamed.
   Note that the name in the contributed code
   NC_FORMAT_64BIT was renamed to NC_FORMAT_CDF2
*/
#define NC_FORMAT_64BIT_OFFSET    (2)
#define NC_FORMAT_64BIT           (NC_FORMAT_64BIT_OFFSET) /**< \deprecated Saved for compatibility.  Use NC_FORMAT_64BIT_OFFSET or NC_FORMAT_64BIT_DATA, from netCDF 4.4.0 onwards. */
#define NC_FORMAT_NETCDF4         (3)
#define NC_FORMAT_NETCDF4_CLASSIC (4)
#define NC_FORMAT_64BIT_DATA      (5)

/* Alias */
#define NC_FORMAT_CDF5    NC_FORMAT_64BIT_DATA

/**@}*/

/** Extended format specifier returned by  nc_inq_format_extended()
 *  Added in version 4.3.1. This returns the true format of the
 *  underlying data.
 * The function returns two values
 * 1. a small integer indicating the underlying source type
 *    of the data. Note that this may differ from what the user
 *    sees from nc_inq_format() because this latter function
 *    returns what the user can expect to see thru the API.
 * 2. A mode value indicating what mode flags are effectively
 *    set for this dataset. This usually will be a superset
 *    of the mode flags used as the argument to nc_open
 *    or nc_create.
 * More or less, the #1 values track the set of dispatch tables.
 * The #1 values are as follows.
 * Note that CDF-5 returns NC_FORMAT_NC3, but sets the mode flag properly.
 */
/**@{*/

#define NC_FORMATX_NC3       (1)
#define NC_FORMATX_NC_HDF5   (2) /**< netCDF-4 subset of HDF5 */
#define NC_FORMATX_NC4       NC_FORMATX_NC_HDF5 /**< alias */
#define NC_FORMATX_NC_HDF4   (3) /**< netCDF-4 subset of HDF4 */
#define NC_FORMATX_PNETCDF   (4)
#define NC_FORMATX_DAP2      (5)
#define NC_FORMATX_DAP4      (6)
#define NC_FORMATX_UDF0      (8)
#define NC_FORMATX_UDF1      (9)
#define NC_FORMATX_UNDEFINED (0)

  /* To avoid breaking compatibility (such as in the python library),
   we need to retain the NC_FORMAT_xxx format as well. This may come
  out eventually, as the NC_FORMATX is more clear that it's an extended
  format specifier.*/

#define NC_FORMAT_NC3       NC_FORMATX_NC3 /**< \deprecated As of 4.4.0, use NC_FORMATX_NC3 */
#define NC_FORMAT_NC_HDF5   NC_FORMATX_NC_HDF5 /**< \deprecated As of 4.4.0, use NC_FORMATX_NC_HDF5 */
#define NC_FORMAT_NC4       NC_FORMATX_NC4 /**< \deprecated As of 4.4.0, use NC_FORMATX_NC4 */
#define NC_FORMAT_NC_HDF4   NC_FORMATX_NC_HDF4 /**< \deprecated As of 4.4.0, use NC_FORMATX_HDF4 */
#define NC_FORMAT_PNETCDF   NC_FORMATX_PNETCDF /**< \deprecated As of 4.4.0, use NC_FORMATX_PNETCDF */
#define NC_FORMAT_DAP2      NC_FORMATX_DAP2 /**< \deprecated As of 4.4.0, use NC_FORMATX_DAP2 */
#define NC_FORMAT_DAP4      NC_FORMATX_DAP4 /**< \deprecated As of 4.4.0, use NC_FORMATX_DAP4 */
#define NC_FORMAT_UNDEFINED NC_FORMATX_UNDEFINED /**< \deprecated As of 4.4.0, use NC_FORMATX_UNDEFINED */

/**@}*/

/** Let nc__create() or nc__open() figure out a suitable buffer size. */
#define NC_SIZEHINT_DEFAULT 0

/** In nc__enddef(), align to the buffer size. */
#define NC_ALIGN_CHUNK ((size_t)(-1))

/** Size argument to nc_def_dim() for an unlimited dimension. */
#define NC_UNLIMITED 0L

/** Attribute id to put/get a global attribute. */
#define NC_GLOBAL -1

/**
Maximum for classic library.

In the classic netCDF model there are maximum values for the number of
dimensions in the file (\ref NC_MAX_DIMS), the number of global or per
variable attributes (\ref NC_MAX_ATTRS), the number of variables in
the file (\ref NC_MAX_VARS), and the length of a name (\ref
NC_MAX_NAME).

These maximums are enforced by the interface, to facilitate writing
applications and utilities.  However, nothing is statically allocated
to these sizes internally.

These maximums are not used for netCDF-4/HDF5 files unless they were
created with the ::NC_CLASSIC_MODEL flag.

As a rule, NC_MAX_VAR_DIMS <= NC_MAX_DIMS.

NOTE: The NC_MAX_DIMS, NC_MAX_ATTRS, and NC_MAX_VARS limits
      are *not* enforced after version 4.5.0
*/
/**@{*/
#define NC_MAX_DIMS     1024 /* not enforced after 4.5.0 */
#define NC_MAX_ATTRS    8192 /* not enforced after 4.5.0 */
#define NC_MAX_VARS     8192 /* not enforced after 4.5.0 */
#define NC_MAX_NAME     256
#define NC_MAX_VAR_DIMS 1024 /**< max per variable dimensions */
/**@}*/

/** This is the max size of an SD dataset name in HDF4 (from HDF4 documentation).*/
#define NC_MAX_HDF4_NAME 64

/** In HDF5 files you can set the endianness of variables with
    nc_def_var_endian(). This define is used there. */
/**@{*/
#define NC_ENDIAN_NATIVE 0
#define NC_ENDIAN_LITTLE 1
#define NC_ENDIAN_BIG    2
/**@}*/

/** In HDF5 files you can set storage for each variable to be either
 * contiguous or chunked, with nc_def_var_chunking().  This define is
 * used there. */
/**@{*/
#define NC_CHUNKED    0
#define NC_CONTIGUOUS 1
/**@}*/

/** In HDF5 files you can set check-summing for each variable.
Currently the only checksum available is Fletcher-32, which can be set
with the function nc_def_var_fletcher32.  These defines are used
there. */
/**@{*/
#define NC_NOCHECKSUM 0
#define NC_FLETCHER32 1
/**@}*/

/**@{*/
/** Control the HDF5 shuffle filter. In HDF5 files you can specify
 * that a shuffle filter should be used on each chunk of a variable to
 * improve compression for that variable. This per-variable shuffle
 * property can be set with the function nc_def_var_deflate(). */
#define NC_NOSHUFFLE 0
#define NC_SHUFFLE   1
/**@}*/

#define NC_MIN_DEFLATE_LEVEL 0 /**< Minimum deflate level. */
#define NC_MAX_DEFLATE_LEVEL 9 /**< Maximum deflate level. */

/** The netcdf version 3 functions all return integer error status.
 * These are the possible values, in addition to certain values from
 * the system errno.h.
 */
#define NC_ISSYSERR(err)        ((err) > 0)

#define NC_NOERR        0          /**< No Error */
#define NC2_ERR         (-1)       /**< Returned for all errors in the v2 API. */

/** Not a netcdf id.

The specified netCDF ID does not refer to an
open netCDF dataset. */
#define	NC_EBADID	(-33)
#define	NC_ENFILE	(-34)	   /**< Too many netcdfs open */
#define	NC_EEXIST	(-35)	   /**< netcdf file exists && NC_NOCLOBBER */
#define	NC_EINVAL	(-36)	   /**< Invalid Argument */
#define	NC_EPERM	(-37)	   /**< Write to read only */

/** Operation not allowed in data mode. This is returned for netCDF
classic or 64-bit offset files, or for netCDF-4 files, when they were
been created with ::NC_CLASSIC_MODEL flag in nc_create(). */
#define NC_ENOTINDEFINE	(-38)

/** Operation not allowed in define mode.

The specified netCDF is in define mode rather than data mode.

With netCDF-4/HDF5 files, this error will not occur, unless
::NC_CLASSIC_MODEL was used in nc_create().
 */
#define	NC_EINDEFINE	(-39)

/** Index exceeds dimension bound.

The specified corner indices were out of range for the rank of the
specified variable. For example, a negative index or an index that is
larger than the corresponding dimension length will cause an error. */
#define	NC_EINVALCOORDS	(-40)

/** NC_MAX_DIMS exceeded. Max number of dimensions exceeded in a
classic or 64-bit offset file, or an netCDF-4 file with
::NC_CLASSIC_MODEL on. */
#define	NC_EMAXDIMS	(-41) /* not enforced after 4.5.0 */

#define	NC_ENAMEINUSE	(-42)	   /**< String match to name in use */
#define NC_ENOTATT	(-43)	   /**< Attribute not found */
#define	NC_EMAXATTS	(-44)	   /**< NC_MAX_ATTRS exceeded - not enforced after 4.5.0 */
#define NC_EBADTYPE	(-45)	   /**< Not a netcdf data type */
#define NC_EBADDIM	(-46)	   /**< Invalid dimension id or name */
#define NC_EUNLIMPOS	(-47)	   /**< NC_UNLIMITED in the wrong index */

/** NC_MAX_VARS exceeded. Max number of variables exceeded in a
classic or 64-bit offset file, or an netCDF-4 file with
::NC_CLASSIC_MODEL on. */
#define	NC_EMAXVARS	(-48) /* not enforced after 4.5.0 */

/** Variable not found.

The variable ID is invalid for the specified netCDF dataset. */
#define NC_ENOTVAR	(-49)
#define NC_EGLOBAL	(-50)	   /**< Action prohibited on NC_GLOBAL varid */
#define NC_ENOTNC	(-51)	   /**< Not a netcdf file */
#define NC_ESTS        	(-52)	   /**< In Fortran, string too short */
#define NC_EMAXNAME    	(-53)	   /**< NC_MAX_NAME exceeded */
#define NC_EUNLIMIT    	(-54)	   /**< NC_UNLIMITED size already in use */
#define NC_ENORECVARS  	(-55)	   /**< nc_rec op when there are no record vars */
#define NC_ECHAR	(-56)	   /**< Attempt to convert between text & numbers */

/** Start+count exceeds dimension bound.

The specified edge lengths added to the specified corner would have
referenced data out of range for the rank of the specified
variable. For example, an edge length that is larger than the
corresponding dimension length minus the corner index will cause an
error. */
#define NC_EEDGE        (-57)      /**< Start+count exceeds dimension bound. */
#define NC_ESTRIDE      (-58)      /**< Illegal stride */
#define NC_EBADNAME     (-59)      /**< Attribute or variable name contains illegal characters */
/* N.B. following must match value in ncx.h */

/** Math result not representable.

One or more of the values are out of the range of values representable
by the desired type. */
#define NC_ERANGE       (-60)
#define NC_ENOMEM       (-61)      /**< Memory allocation (malloc) failure */
#define NC_EVARSIZE     (-62)      /**< One or more variable sizes violate format constraints */
#define NC_EDIMSIZE     (-63)      /**< Invalid dimension size */
#define NC_ETRUNC       (-64)      /**< File likely truncated or possibly corrupted */
#define NC_EAXISTYPE    (-65)      /**< Unknown axis type. */

/* Following errors are added for DAP */
#define NC_EDAP         (-66)      /**< Generic DAP error */
#define NC_ECURL        (-67)      /**< Generic libcurl error */
#define NC_EIO          (-68)      /**< Generic IO error */
#define NC_ENODATA      (-69)      /**< Attempt to access variable with no data */
#define NC_EDAPSVC      (-70)      /**< DAP server error */
#define NC_EDAS         (-71)      /**< Malformed or inaccessible DAS */
#define NC_EDDS         (-72)      /**< Malformed or inaccessible DDS */
#define NC_EDMR         NC_EDDS    /**< Dap4 alias */
#define NC_EDATADDS     (-73)      /**< Malformed or inaccessible DATADDS */
#define NC_EDATADAP     NC_EDATADDS    /**< Dap4 alias */
#define NC_EDAPURL      (-74)      /**< Malformed DAP URL */
#define NC_EDAPCONSTRAINT (-75)    /**< Malformed DAP Constraint*/
#define NC_ETRANSLATION (-76)      /**< Untranslatable construct */
#define NC_EACCESS      (-77)      /**< Access Failure */
#define NC_EAUTH        (-78)      /**< Authorization Failure */

/* Misc. additional errors */
#define NC_ENOTFOUND     (-90)      /**< No such file */
#define NC_ECANTREMOVE   (-91)      /**< Can't remove file */
#define NC_EINTERNAL     (-92)      /**< NetCDF Library Internal Error */
#define NC_EPNETCDF      (-93)      /**< Error at PnetCDF layer */

/* The following was added in support of netcdf-4. Make all netcdf-4
   error codes < -100 so that errors can be added to netcdf-3 if
   needed. */
#define NC4_FIRST_ERROR  (-100)    /**< @internal All HDF5 errors < this. */
#define NC_EHDFERR       (-101)    /**< Error at HDF5 layer. */
#define NC_ECANTREAD     (-102)    /**< Can't read. */
#define NC_ECANTWRITE    (-103)    /**< Can't write. */
#define NC_ECANTCREATE   (-104)    /**< Can't create. */
#define NC_EFILEMETA     (-105)    /**< Problem with file metadata. */
#define NC_EDIMMETA      (-106)    /**< Problem with dimension metadata. */
#define NC_EATTMETA      (-107)    /**< Problem with attribute metadata. */
#define NC_EVARMETA      (-108)    /**< Problem with variable metadata. */
#define NC_ENOCOMPOUND   (-109)    /**< Not a compound type. */
#define NC_EATTEXISTS    (-110)    /**< Attribute already exists. */
#define NC_ENOTNC4       (-111)    /**< Attempting netcdf-4 operation on netcdf-3 file. */
#define NC_ESTRICTNC3    (-112)    /**< Attempting netcdf-4 operation on strict nc3 netcdf-4 file. */
#define NC_ENOTNC3       (-113)    /**< Attempting netcdf-3 operation on netcdf-4 file. */
#define NC_ENOPAR        (-114)    /**< Parallel operation on file opened for non-parallel access. */
#define NC_EPARINIT      (-115)    /**< Error initializing for parallel access. */
#define NC_EBADGRPID     (-116)    /**< Bad group ID. */
#define NC_EBADTYPID     (-117)    /**< Bad type ID. */
#define NC_ETYPDEFINED   (-118)    /**< Type has already been defined and may not be edited. */
#define NC_EBADFIELD     (-119)    /**< Bad field ID. */
#define NC_EBADCLASS     (-120)    /**< Bad class. */
#define NC_EMAPTYPE      (-121)    /**< Mapped access for atomic types only. */
#define NC_ELATEFILL     (-122)    /**< Attempt to define fill value when data already exists. */
#define NC_ELATEDEF      (-123)    /**< Attempt to define var properties, like deflate, after enddef. */
#define NC_EDIMSCALE     (-124)    /**< Problem with HDF5 dimscales. */
#define NC_ENOGRP        (-125)    /**< No group found. */
#define NC_ESTORAGE      (-126)    /**< Can't specify both contiguous and chunking. */
#define NC_EBADCHUNK     (-127)    /**< Bad chunksize. */
#define NC_ENOTBUILT     (-128)    /**< Attempt to use feature that was not turned on when netCDF was built. */
#define NC_EDISKLESS     (-129)    /**< Error in using diskless  access. */
#define NC_ECANTEXTEND   (-130)    /**< Attempt to extend dataset during ind. I/O operation. */
#define NC_EMPI          (-131)    /**< MPI operation failed. */

#define NC_EFILTER       (-132)    /**< Filter operation failed. */
#define NC_ERCFILE       (-133)    /**< RC file failure */
#define NC_ENULLPAD      (-134)    /**< Header Bytes not Null-Byte padded */
#define NC_EINMEMORY     (-135)    /**< In-memory file error */
#define NC4_LAST_ERROR   (-136)    /**< @internal All netCDF errors > this. */

/** @internal This is used in netCDF-4 files for dimensions without
 * coordinate vars. */
#define DIM_WITHOUT_VARIABLE "This is a netCDF dimension but not a netCDF variable."

/** @internal This is here at the request of the NCO team to support
 * our mistake of having chunksizes be first ints, then
 * size_t. Doh! */
#define NC_HAVE_NEW_CHUNKING_API 1

/* Errors for all remote access methods(e.g. DAP and CDMREMOTE)*/
#define NC_EURL         (NC_EDAPURL)   /**< Malformed URL */
#define NC_ECONSTRAINT  (NC_EDAPCONSTRAINT)   /**< Malformed Constraint*/

/*
 * The Interface
 */

/* Declaration modifiers for DLL support (MSC et al) */
#if defined(DLL_NETCDF) /* define when library is a DLL */
#  if defined(DLL_EXPORT) /* define when building the library */
#   define MSC_EXTRA __declspec(dllexport)
#  else
#   define MSC_EXTRA __declspec(dllimport)
#  endif
#  include <io.h>
#else
#define MSC_EXTRA  /**< Needed for DLL build. */
#endif  /* defined(DLL_NETCDF) */

#define EXTERNL MSC_EXTRA extern /**< Needed for DLL build. */

#if defined(DLL_NETCDF) /* define when library is a DLL */
EXTERNL int ncerr;
EXTERNL int ncopts;
#endif

EXTERNL const char *
nc_inq_libvers(void);

EXTERNL const char *
nc_strerror(int ncerr);

/* Set up user-defined format. */
typedef struct NC_Dispatch NC_Dispatch;
EXTERNL int
nc_def_user_format(int mode_flag, NC_Dispatch *dispatch_table, char *magic_number);

EXTERNL int
nc_inq_user_format(int mode_flag, NC_Dispatch **dispatch_table, char *magic_number);

EXTERNL int
nc__create(const char *path, int cmode, size_t initialsz,
         size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc_create(const char *path, int cmode, int *ncidp);

EXTERNL int
nc__open(const char *path, int mode,
        size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc_open(const char *path, int mode, int *ncidp);

/* Learn the path used to open/create the file. */
EXTERNL int
nc_inq_path(int ncid, size_t *pathlen, char *path);

/* Given an ncid and group name (NULL gets root group), return
 * locid. */
EXTERNL int
nc_inq_ncid(int ncid, const char *name, int *grp_ncid);

/* Given a location id, return the number of groups it contains, and
 * an array of their locids. */
EXTERNL int
nc_inq_grps(int ncid, int *numgrps, int *ncids);

/* Given locid, find name of group. (Root group is named "/".) */
EXTERNL int
nc_inq_grpname(int ncid, char *name);

/* Given ncid, find full name and len of full name. (Root group is
 * named "/", with length 1.) */
EXTERNL int
nc_inq_grpname_full(int ncid, size_t *lenp, char *full_name);

/* Given ncid, find len of full name. */
EXTERNL int
nc_inq_grpname_len(int ncid, size_t *lenp);

/* Given an ncid, find the ncid of its parent group. */
EXTERNL int
nc_inq_grp_parent(int ncid, int *parent_ncid);

/* Given a name and parent ncid, find group ncid. */
EXTERNL int
nc_inq_grp_ncid(int ncid, const char *grp_name, int *grp_ncid);

/* Given a full name and ncid, find group ncid. */
EXTERNL int
nc_inq_grp_full_ncid(int ncid, const char *full_name, int *grp_ncid);

/* Get a list of ids for all the variables in a group. */
EXTERNL int
nc_inq_varids(int ncid, int *nvars, int *varids);

/* Find all dimids for a location. This finds all dimensions in a
 * group, or any of its parents. */
EXTERNL int
nc_inq_dimids(int ncid, int *ndims, int *dimids, int include_parents);

/* Find all user-defined types for a location. This finds all
 * user-defined types in a group. */
EXTERNL int
nc_inq_typeids(int ncid, int *ntypes, int *typeids);

/* Are two types equal? */
EXTERNL int
nc_inq_type_equal(int ncid1, nc_type typeid1, int ncid2,
                  nc_type typeid2, int *equal);

/* Create a group. its ncid is returned in the new_ncid pointer. */
EXTERNL int
nc_def_grp(int parent_ncid, const char *name, int *new_ncid);

/* Rename a group */
EXTERNL int
nc_rename_grp(int grpid, const char *name);

/* Here are functions for dealing with compound types. */

/* Create a compound type. */
EXTERNL int
nc_def_compound(int ncid, size_t size, const char *name, nc_type *typeidp);

/* Insert a named field into a compound type. */
EXTERNL int
nc_insert_compound(int ncid, nc_type xtype, const char *name,
                   size_t offset, nc_type field_typeid);

/* Insert a named array into a compound type. */
EXTERNL int
nc_insert_array_compound(int ncid, nc_type xtype, const char *name,
                         size_t offset, nc_type field_typeid,
                         int ndims, const int *dim_sizes);

/* Get the name and size of a type. */
EXTERNL int
nc_inq_type(int ncid, nc_type xtype, char *name, size_t *size);

/* Get the id of a type from the name. */
EXTERNL int
nc_inq_typeid(int ncid, const char *name, nc_type *typeidp);

/* Get the name, size, and number of fields in a compound type. */
EXTERNL int
nc_inq_compound(int ncid, nc_type xtype, char *name, size_t *sizep,
                size_t *nfieldsp);

/* Get the name of a compound type. */
EXTERNL int
nc_inq_compound_name(int ncid, nc_type xtype, char *name);

/* Get the size of a compound type. */
EXTERNL int
nc_inq_compound_size(int ncid, nc_type xtype, size_t *sizep);

/* Get the number of fields in this compound type. */
EXTERNL int
nc_inq_compound_nfields(int ncid, nc_type xtype, size_t *nfieldsp);

/* Given the xtype and the fieldid, get all info about it. */
EXTERNL int
nc_inq_compound_field(int ncid, nc_type xtype, int fieldid, char *name,
                      size_t *offsetp, nc_type *field_typeidp, int *ndimsp,
                      int *dim_sizesp);

/* Given the typeid and the fieldid, get the name. */
EXTERNL int
nc_inq_compound_fieldname(int ncid, nc_type xtype, int fieldid,
                          char *name);

/* Given the xtype and the name, get the fieldid. */
EXTERNL int
nc_inq_compound_fieldindex(int ncid, nc_type xtype, const char *name,
                           int *fieldidp);

/* Given the xtype and fieldid, get the offset. */
EXTERNL int
nc_inq_compound_fieldoffset(int ncid, nc_type xtype, int fieldid,
                            size_t *offsetp);

/* Given the xtype and the fieldid, get the type of that field. */
EXTERNL int
nc_inq_compound_fieldtype(int ncid, nc_type xtype, int fieldid,
                          nc_type *field_typeidp);

/* Given the xtype and the fieldid, get the number of dimensions for
 * that field (scalars are 0). */
EXTERNL int
nc_inq_compound_fieldndims(int ncid, nc_type xtype, int fieldid,
                           int *ndimsp);

/* Given the xtype and the fieldid, get the sizes of dimensions for
 * that field. User must have allocated storage for the dim_sizes. */
EXTERNL int
nc_inq_compound_fielddim_sizes(int ncid, nc_type xtype, int fieldid,
                               int *dim_sizes);

/** This is the type of arrays of vlens. */
typedef struct {
    size_t len; /**< Length of VL data (in base type units) */
    void *p;    /**< Pointer to VL data */
} nc_vlen_t;

/** Calculate an offset for creating a compound type. This calls a
 * mysterious C macro which was found carved into one of the blocks of
 * the Newgrange passage tomb in County Meath, Ireland. This code has
 * been carbon dated to 3200 B.C.E. */
#define NC_COMPOUND_OFFSET(S,M)    (offsetof(S,M))

/* Create a variable length type. */
EXTERNL int
nc_def_vlen(int ncid, const char *name, nc_type base_typeid, nc_type *xtypep);

/* Find out about a vlen. */
EXTERNL int
nc_inq_vlen(int ncid, nc_type xtype, char *name, size_t *datum_sizep,
            nc_type *base_nc_typep);

/* When you read VLEN type the library will actually allocate the
 * storage space for the data. This storage space must be freed, so
 * pass the pointer back to this function, when you're done with the
 * data, and it will free the vlen memory. */
EXTERNL int
nc_free_vlen(nc_vlen_t *vl);

EXTERNL int
nc_free_vlens(size_t len, nc_vlen_t vlens[]);

/* Put or get one element in a vlen array. */
EXTERNL int
nc_put_vlen_element(int ncid, int typeid1, void *vlen_element,
                    size_t len, const void *data);

EXTERNL int
nc_get_vlen_element(int ncid, int typeid1, const void *vlen_element,
                    size_t *len, void *data);

/* When you read the string type the library will allocate the storage
 * space for the data. This storage space must be freed, so pass the
 * pointer back to this function, when you're done with the data, and
 * it will free the string memory. */
EXTERNL int
nc_free_string(size_t len, char **data);

/* Find out about a user defined type. */
EXTERNL int
nc_inq_user_type(int ncid, nc_type xtype, char *name, size_t *size,
                 nc_type *base_nc_typep, size_t *nfieldsp, int *classp);

/* Write an attribute of any type. */
EXTERNL int
nc_put_att(int ncid, int varid, const char *name, nc_type xtype,
           size_t len, const void *op);

/* Read an attribute of any type. */
EXTERNL int
nc_get_att(int ncid, int varid, const char *name, void *ip);

/* Enum type. */

/* Create an enum type. Provide a base type and a name. At the moment
 * only ints are accepted as base types. */
EXTERNL int
nc_def_enum(int ncid, nc_type base_typeid, const char *name,
            nc_type *typeidp);

/* Insert a named value into an enum type. The value must fit within
 * the size of the enum type, the name size must be <= NC_MAX_NAME. */
EXTERNL int
nc_insert_enum(int ncid, nc_type xtype, const char *name,
               const void *value);

/* Get information about an enum type: its name, base type and the
 * number of members defined. */
EXTERNL int
nc_inq_enum(int ncid, nc_type xtype, char *name, nc_type *base_nc_typep,
            size_t *base_sizep, size_t *num_membersp);

/* Get information about an enum member: a name and value. Name size
 * will be <= NC_MAX_NAME. */
EXTERNL int
nc_inq_enum_member(int ncid, nc_type xtype, int idx, char *name,
                   void *value);


/* Get enum name from enum value. Name size will be <= NC_MAX_NAME. */
EXTERNL int
nc_inq_enum_ident(int ncid, nc_type xtype, long long value, char *identifier);

/* Opaque type. */

/* Create an opaque type. Provide a size and a name. */
EXTERNL int
nc_def_opaque(int ncid, size_t size, const char *name, nc_type *xtypep);

/* Get information about an opaque type. */
EXTERNL int
nc_inq_opaque(int ncid, nc_type xtype, char *name, size_t *sizep);

/* Write entire var of any type. */
EXTERNL int
nc_put_var(int ncid, int varid,  const void *op);

/* Read entire var of any type. */
EXTERNL int
nc_get_var(int ncid, int varid,  void *ip);

/* Write one value. */
EXTERNL int
nc_put_var1(int ncid, int varid,  const size_t *indexp,
            const void *op);

/* Read one value. */
EXTERNL int
nc_get_var1(int ncid, int varid,  const size_t *indexp, void *ip);

/* Write an array of values. */
EXTERNL int
nc_put_vara(int ncid, int varid,  const size_t *startp,
            const size_t *countp, const void *op);

/* Read an array of values. */
EXTERNL int
nc_get_vara(int ncid, int varid,  const size_t *startp,
            const size_t *countp, void *ip);

/* Write slices of an array of values. */
EXTERNL int
nc_put_vars(int ncid, int varid,  const size_t *startp,
            const size_t *countp, const ptrdiff_t *stridep,
            const void *op);

/* Read slices of an array of values. */
EXTERNL int
nc_get_vars(int ncid, int varid,  const size_t *startp,
            const size_t *countp, const ptrdiff_t *stridep,
            void *ip);

/* Write mapped slices of an array of values. */
EXTERNL int
nc_put_varm(int ncid, int varid,  const size_t *startp,
            const size_t *countp, const ptrdiff_t *stridep,
            const ptrdiff_t *imapp, const void *op);

/* Read mapped slices of an array of values. */
EXTERNL int
nc_get_varm(int ncid, int varid,  const size_t *startp,
            const size_t *countp, const ptrdiff_t *stridep,
            const ptrdiff_t *imapp, void *ip);

/* Extra netcdf-4 stuff. */

/* Set compression settings for a variable. Lower is faster, higher is
 * better. Must be called after nc_def_var and before nc_enddef. */
EXTERNL int
nc_def_var_deflate(int ncid, int varid, int shuffle, int deflate,
                   int deflate_level);

/* Find out compression settings of a var. */
EXTERNL int
nc_inq_var_deflate(int ncid, int varid, int *shufflep,
                   int *deflatep, int *deflate_levelp);

/* Find out szip settings of a var. */
EXTERNL int
nc_inq_var_szip(int ncid, int varid, int *options_maskp, int *pixels_per_blockp);

/* Set fletcher32 checksum for a var. This must be done after nc_def_var
   and before nc_enddef. */
EXTERNL int
nc_def_var_fletcher32(int ncid, int varid, int fletcher32);

/* Inquire about fletcher32 checksum for a var. */
EXTERNL int
nc_inq_var_fletcher32(int ncid, int varid, int *fletcher32p);

/* Define chunking for a variable. This must be done after nc_def_var
   and before nc_enddef. */
EXTERNL int
nc_def_var_chunking(int ncid, int varid, int storage, const size_t *chunksizesp);

/* Inq chunking stuff for a var. */
EXTERNL int
nc_inq_var_chunking(int ncid, int varid, int *storagep, size_t *chunksizesp);

/* Define fill value behavior for a variable. This must be done after
   nc_def_var and before nc_enddef. */
EXTERNL int
nc_def_var_fill(int ncid, int varid, int no_fill, const void *fill_value);

/* Inq fill value setting for a var. */
EXTERNL int
nc_inq_var_fill(int ncid, int varid, int *no_fill, void *fill_valuep);

/* Define the endianness of a variable. */
EXTERNL int
nc_def_var_endian(int ncid, int varid, int endian);

/* Learn about the endianness of a variable. */
EXTERNL int
nc_inq_var_endian(int ncid, int varid, int *endianp);

/* Define a filter for a variable */
EXTERNL int
nc_def_var_filter(int ncid, int varid, unsigned int id, size_t nparams, const unsigned int* parms);

/* Learn about the filter on a variable */
EXTERNL int
nc_inq_var_filter(int ncid, int varid, unsigned int* idp, size_t* nparams, unsigned int* params);

/* Set the fill mode (classic or 64-bit offset files only). */
EXTERNL int
nc_set_fill(int ncid, int fillmode, int *old_modep);

/* Set the default nc_create format to NC_FORMAT_CLASSIC, NC_FORMAT_64BIT,
 * NC_FORMAT_CDF5, NC_FORMAT_NETCDF4, or NC_FORMAT_NETCDF4_CLASSIC */
EXTERNL int
nc_set_default_format(int format, int *old_formatp);

/* Set the cache size, nelems, and preemption policy. */
EXTERNL int
nc_set_chunk_cache(size_t size, size_t nelems, float preemption);

/* Get the cache size, nelems, and preemption policy. */
EXTERNL int
nc_get_chunk_cache(size_t *sizep, size_t *nelemsp, float *preemptionp);

/* Set the per-variable cache size, nelems, and preemption policy. */
EXTERNL int
nc_set_var_chunk_cache(int ncid, int varid, size_t size, size_t nelems,
                       float preemption);

/* Get the per-variable cache size, nelems, and preemption policy. */
EXTERNL int
nc_get_var_chunk_cache(int ncid, int varid, size_t *sizep, size_t *nelemsp,
                       float *preemptionp);

EXTERNL int
nc_redef(int ncid);

/* Is this ever used? Convert to parameter form */
EXTERNL int
nc__enddef(int ncid, size_t h_minfree, size_t v_align,
        size_t v_minfree, size_t r_align);

EXTERNL int
nc_enddef(int ncid);

EXTERNL int
nc_sync(int ncid);

EXTERNL int
nc_abort(int ncid);

EXTERNL int
nc_close(int ncid);

EXTERNL int
nc_inq(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimidp);

EXTERNL int
nc_inq_ndims(int ncid, int *ndimsp);

EXTERNL int
nc_inq_nvars(int ncid, int *nvarsp);

EXTERNL int
nc_inq_natts(int ncid, int *nattsp);

EXTERNL int
nc_inq_unlimdim(int ncid, int *unlimdimidp);

/* The next function is for NetCDF-4 only */
EXTERNL int
nc_inq_unlimdims(int ncid, int *nunlimdimsp, int *unlimdimidsp);

/* Added in 3.6.1 to return format of netCDF file. */
EXTERNL int
nc_inq_format(int ncid, int *formatp);

/* Added in 4.3.1 to return additional format info */
EXTERNL int
nc_inq_format_extended(int ncid, int *formatp, int* modep);

/* Begin _dim */

EXTERNL int
nc_def_dim(int ncid, const char *name, size_t len, int *idp);

EXTERNL int
nc_inq_dimid(int ncid, const char *name, int *idp);

EXTERNL int
nc_inq_dim(int ncid, int dimid, char *name, size_t *lenp);

EXTERNL int
nc_inq_dimname(int ncid, int dimid, char *name);

EXTERNL int
nc_inq_dimlen(int ncid, int dimid, size_t *lenp);

EXTERNL int
nc_rename_dim(int ncid, int dimid, const char *name);

/* End _dim */
/* Begin _att */

EXTERNL int
nc_inq_att(int ncid, int varid, const char *name,
           nc_type *xtypep, size_t *lenp);

EXTERNL int
nc_inq_attid(int ncid, int varid, const char *name, int *idp);

EXTERNL int
nc_inq_atttype(int ncid, int varid, const char *name, nc_type *xtypep);

EXTERNL int
nc_inq_attlen(int ncid, int varid, const char *name, size_t *lenp);

EXTERNL int
nc_inq_attname(int ncid, int varid, int attnum, char *name);

EXTERNL int
nc_copy_att(int ncid_in, int varid_in, const char *name, int ncid_out, int varid_out);

EXTERNL int
nc_rename_att(int ncid, int varid, const char *name, const char *newname);

EXTERNL int
nc_del_att(int ncid, int varid, const char *name);

/* End _att */
/* Begin {put,get}_att */
EXTERNL int
nc_put_att_text(int ncid, int varid, const char *name,
                size_t len, const char *op);

EXTERNL int
nc_get_att_text(int ncid, int varid, const char *name, char *ip);

EXTERNL int
nc_put_att_string(int ncid, int varid, const char *name,
                  size_t len, const char **op);

EXTERNL int
nc_get_att_string(int ncid, int varid, const char *name, char **ip);

EXTERNL int
nc_put_att_uchar(int ncid, int varid, const char *name, nc_type xtype,
                 size_t len, const unsigned char *op);

EXTERNL int
nc_get_att_uchar(int ncid, int varid, const char *name, unsigned char *ip);

EXTERNL int
nc_put_att_schar(int ncid, int varid, const char *name, nc_type xtype,
                 size_t len, const signed char *op);

EXTERNL int
nc_get_att_schar(int ncid, int varid, const char *name, signed char *ip);

EXTERNL int
nc_put_att_short(int ncid, int varid, const char *name, nc_type xtype,
                 size_t len, const short *op);

EXTERNL int
nc_get_att_short(int ncid, int varid, const char *name, short *ip);

EXTERNL int
nc_put_att_int(int ncid, int varid, const char *name, nc_type xtype,
               size_t len, const int *op);

EXTERNL int
nc_get_att_int(int ncid, int varid, const char *name, int *ip);

EXTERNL int
nc_put_att_long(int ncid, int varid, const char *name, nc_type xtype,
                size_t len, const long *op);

EXTERNL int
nc_get_att_long(int ncid, int varid, const char *name, long *ip);

EXTERNL int
nc_put_att_float(int ncid, int varid, const char *name, nc_type xtype,
                 size_t len, const float *op);

EXTERNL int
nc_get_att_float(int ncid, int varid, const char *name, float *ip);

EXTERNL int
nc_put_att_double(int ncid, int varid, const char *name, nc_type xtype,
                  size_t len, const double *op);

EXTERNL int
nc_get_att_double(int ncid, int varid, const char *name, double *ip);

EXTERNL int
nc_put_att_ushort(int ncid, int varid, const char *name, nc_type xtype,
                  size_t len, const unsigned short *op);

EXTERNL int
nc_get_att_ushort(int ncid, int varid, const char *name, unsigned short *ip);

EXTERNL int
nc_put_att_uint(int ncid, int varid, const char *name, nc_type xtype,
                size_t len, const unsigned int *op);

EXTERNL int
nc_get_att_uint(int ncid, int varid, const char *name, unsigned int *ip);

EXTERNL int
nc_put_att_longlong(int ncid, int varid, const char *name, nc_type xtype,
                 size_t len, const long long *op);

EXTERNL int
nc_get_att_longlong(int ncid, int varid, const char *name, long long *ip);

EXTERNL int
nc_put_att_ulonglong(int ncid, int varid, const char *name, nc_type xtype,
                     size_t len, const unsigned long long *op);

EXTERNL int
nc_get_att_ulonglong(int ncid, int varid, const char *name,
                     unsigned long long *ip);


/* End {put,get}_att */
/* Begin _var */

EXTERNL int
nc_def_var(int ncid, const char *name, nc_type xtype, int ndims,
           const int *dimidsp, int *varidp);

EXTERNL int
nc_inq_var(int ncid, int varid, char *name, nc_type *xtypep,
           int *ndimsp, int *dimidsp, int *nattsp);

EXTERNL int
nc_inq_varid(int ncid, const char *name, int *varidp);

EXTERNL int
nc_inq_varname(int ncid, int varid, char *name);

EXTERNL int
nc_inq_vartype(int ncid, int varid, nc_type *xtypep);

EXTERNL int
nc_inq_varndims(int ncid, int varid, int *ndimsp);

EXTERNL int
nc_inq_vardimid(int ncid, int varid, int *dimidsp);

EXTERNL int
nc_inq_varnatts(int ncid, int varid, int *nattsp);

EXTERNL int
nc_rename_var(int ncid, int varid, const char *name);

EXTERNL int
nc_copy_var(int ncid_in, int varid, int ncid_out);

#ifndef ncvarcpy
/* support the old name for now */
#define ncvarcpy(ncid_in, varid, ncid_out) ncvarcopy((ncid_in), (varid), (ncid_out))
#endif

/* End _var */
/* Begin {put,get}_var1 */

EXTERNL int
nc_put_var1_text(int ncid, int varid, const size_t *indexp, const char *op);

EXTERNL int
nc_get_var1_text(int ncid, int varid, const size_t *indexp, char *ip);

EXTERNL int
nc_put_var1_uchar(int ncid, int varid, const size_t *indexp,
                  const unsigned char *op);

EXTERNL int
nc_get_var1_uchar(int ncid, int varid, const size_t *indexp,
                  unsigned char *ip);

EXTERNL int
nc_put_var1_schar(int ncid, int varid, const size_t *indexp,
                  const signed char *op);

EXTERNL int
nc_get_var1_schar(int ncid, int varid, const size_t *indexp,
                  signed char *ip);

EXTERNL int
nc_put_var1_short(int ncid, int varid, const size_t *indexp,
                  const short *op);

EXTERNL int
nc_get_var1_short(int ncid, int varid, const size_t *indexp,
                  short *ip);

EXTERNL int
nc_put_var1_int(int ncid, int varid, const size_t *indexp, const int *op);

EXTERNL int
nc_get_var1_int(int ncid, int varid, const size_t *indexp, int *ip);

EXTERNL int
nc_put_var1_long(int ncid, int varid, const size_t *indexp, const long *op);

EXTERNL int
nc_get_var1_long(int ncid, int varid, const size_t *indexp, long *ip);

EXTERNL int
nc_put_var1_float(int ncid, int varid, const size_t *indexp, const float *op);

EXTERNL int
nc_get_var1_float(int ncid, int varid, const size_t *indexp, float *ip);

EXTERNL int
nc_put_var1_double(int ncid, int varid, const size_t *indexp, const double *op);

EXTERNL int
nc_get_var1_double(int ncid, int varid, const size_t *indexp, double *ip);

EXTERNL int
nc_put_var1_ushort(int ncid, int varid, const size_t *indexp,
                   const unsigned short *op);

EXTERNL int
nc_get_var1_ushort(int ncid, int varid, const size_t *indexp,
                   unsigned short *ip);

EXTERNL int
nc_put_var1_uint(int ncid, int varid, const size_t *indexp,
                 const unsigned int *op);

EXTERNL int
nc_get_var1_uint(int ncid, int varid, const size_t *indexp,
                 unsigned int *ip);

EXTERNL int
nc_put_var1_longlong(int ncid, int varid, const size_t *indexp,
                     const long long *op);

EXTERNL int
nc_get_var1_longlong(int ncid, int varid, const size_t *indexp,
                  long long *ip);

EXTERNL int
nc_put_var1_ulonglong(int ncid, int varid, const size_t *indexp,
                   const unsigned long long *op);

EXTERNL int
nc_get_var1_ulonglong(int ncid, int varid, const size_t *indexp,
                   unsigned long long *ip);

EXTERNL int
nc_put_var1_string(int ncid, int varid, const size_t *indexp,
                   const char **op);

EXTERNL int
nc_get_var1_string(int ncid, int varid, const size_t *indexp,
                   char **ip);

/* End {put,get}_var1 */
/* Begin {put,get}_vara */

EXTERNL int
nc_put_vara_text(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const char *op);

EXTERNL int
nc_get_vara_text(int ncid, int varid, const size_t *startp,
                 const size_t *countp, char *ip);

EXTERNL int
nc_put_vara_uchar(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const unsigned char *op);

EXTERNL int
nc_get_vara_uchar(int ncid, int varid, const size_t *startp,
                  const size_t *countp, unsigned char *ip);

EXTERNL int
nc_put_vara_schar(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const signed char *op);

EXTERNL int
nc_get_vara_schar(int ncid, int varid, const size_t *startp,
                  const size_t *countp, signed char *ip);

EXTERNL int
nc_put_vara_short(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const short *op);

EXTERNL int
nc_get_vara_short(int ncid, int varid, const size_t *startp,
                  const size_t *countp, short *ip);

EXTERNL int
nc_put_vara_int(int ncid, int varid, const size_t *startp,
                const size_t *countp, const int *op);

EXTERNL int
nc_get_vara_int(int ncid, int varid, const size_t *startp,
                const size_t *countp, int *ip);

EXTERNL int
nc_put_vara_long(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const long *op);

EXTERNL int
nc_get_vara_long(int ncid, int varid,
        const size_t *startp, const size_t *countp, long *ip);

EXTERNL int
nc_put_vara_float(int ncid, int varid,
        const size_t *startp, const size_t *countp, const float *op);

EXTERNL int
nc_get_vara_float(int ncid, int varid,
        const size_t *startp, const size_t *countp, float *ip);

EXTERNL int
nc_put_vara_double(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const double *op);

EXTERNL int
nc_get_vara_double(int ncid, int varid, const size_t *startp,
                   const size_t *countp, double *ip);

EXTERNL int
nc_put_vara_ushort(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const unsigned short *op);

EXTERNL int
nc_get_vara_ushort(int ncid, int varid, const size_t *startp,
                   const size_t *countp, unsigned short *ip);

EXTERNL int
nc_put_vara_uint(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const unsigned int *op);

EXTERNL int
nc_get_vara_uint(int ncid, int varid, const size_t *startp,
                 const size_t *countp, unsigned int *ip);

EXTERNL int
nc_put_vara_longlong(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const long long *op);

EXTERNL int
nc_get_vara_longlong(int ncid, int varid, const size_t *startp,
                  const size_t *countp, long long *ip);

EXTERNL int
nc_put_vara_ulonglong(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const unsigned long long *op);

EXTERNL int
nc_get_vara_ulonglong(int ncid, int varid, const size_t *startp,
                   const size_t *countp, unsigned long long *ip);

EXTERNL int
nc_put_vara_string(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const char **op);

EXTERNL int
nc_get_vara_string(int ncid, int varid, const size_t *startp,
                   const size_t *countp, char **ip);

/* End {put,get}_vara */
/* Begin {put,get}_vars */

EXTERNL int
nc_put_vars_text(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        const char *op);

EXTERNL int
nc_get_vars_text(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        char *ip);

EXTERNL int
nc_put_vars_uchar(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        const unsigned char *op);

EXTERNL int
nc_get_vars_uchar(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        unsigned char *ip);

EXTERNL int
nc_put_vars_schar(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        const signed char *op);

EXTERNL int
nc_get_vars_schar(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        signed char *ip);

EXTERNL int
nc_put_vars_short(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        const short *op);

EXTERNL int
nc_get_vars_short(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  short *ip);

EXTERNL int
nc_put_vars_int(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        const int *op);

EXTERNL int
nc_get_vars_int(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        int *ip);

EXTERNL int
nc_put_vars_long(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        const long *op);

EXTERNL int
nc_get_vars_long(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        long *ip);

EXTERNL int
nc_put_vars_float(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        const float *op);

EXTERNL int
nc_get_vars_float(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        float *ip);

EXTERNL int
nc_put_vars_double(int ncid, int varid,
        const size_t *startp, const size_t *countp, const ptrdiff_t *stridep,
        const double *op);

EXTERNL int
nc_get_vars_double(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   double *ip);

EXTERNL int
nc_put_vars_ushort(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const unsigned short *op);

EXTERNL int
nc_get_vars_ushort(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   unsigned short *ip);

EXTERNL int
nc_put_vars_uint(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const ptrdiff_t *stridep,
                 const unsigned int *op);

EXTERNL int
nc_get_vars_uint(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const ptrdiff_t *stridep,
                 unsigned int *ip);

EXTERNL int
nc_put_vars_longlong(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const long long *op);

EXTERNL int
nc_get_vars_longlong(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  long long *ip);

EXTERNL int
nc_put_vars_ulonglong(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const unsigned long long *op);

EXTERNL int
nc_get_vars_ulonglong(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   unsigned long long *ip);

EXTERNL int
nc_put_vars_string(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const char **op);

EXTERNL int
nc_get_vars_string(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   char **ip);

/* End {put,get}_vars */
/* Begin {put,get}_varm */

EXTERNL int
nc_put_varm_text(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const ptrdiff_t *stridep,
                 const ptrdiff_t *imapp, const char *op);

EXTERNL int
nc_get_varm_text(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const ptrdiff_t *stridep,
                 const ptrdiff_t *imapp, char *ip);

EXTERNL int
nc_put_varm_uchar(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t *imapp, const unsigned char *op);

EXTERNL int
nc_get_varm_uchar(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t *imapp, unsigned char *ip);

EXTERNL int
nc_put_varm_schar(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t *imapp, const signed char *op);

EXTERNL int
nc_get_varm_schar(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t *imapp, signed char *ip);

EXTERNL int
nc_put_varm_short(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t *imapp, const short *op);

EXTERNL int
nc_get_varm_short(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t *imapp, short *ip);

EXTERNL int
nc_put_varm_int(int ncid, int varid, const size_t *startp,
                const size_t *countp, const ptrdiff_t *stridep,
                const ptrdiff_t *imapp, const int *op);

EXTERNL int
nc_get_varm_int(int ncid, int varid, const size_t *startp,
                const size_t *countp, const ptrdiff_t *stridep,
                const ptrdiff_t *imapp, int *ip);

EXTERNL int
nc_put_varm_long(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const ptrdiff_t *stridep,
                 const ptrdiff_t *imapp, const long *op);

EXTERNL int
nc_get_varm_long(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const ptrdiff_t *stridep,
                 const ptrdiff_t *imapp, long *ip);

EXTERNL int
nc_put_varm_float(int ncid, int varid,const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t *imapp, const float *op);

EXTERNL int
nc_get_varm_float(int ncid, int varid,const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t *imapp, float *ip);

EXTERNL int
nc_put_varm_double(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const ptrdiff_t *imapp, const double *op);

EXTERNL int
nc_get_varm_double(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const ptrdiff_t * imapp, double *ip);

EXTERNL int
nc_put_varm_ushort(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const ptrdiff_t * imapp, const unsigned short *op);

EXTERNL int
nc_get_varm_ushort(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const ptrdiff_t * imapp, unsigned short *ip);

EXTERNL int
nc_put_varm_uint(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const ptrdiff_t *stridep,
                 const ptrdiff_t * imapp, const unsigned int *op);

EXTERNL int
nc_get_varm_uint(int ncid, int varid, const size_t *startp,
                 const size_t *countp, const ptrdiff_t *stridep,
                 const ptrdiff_t * imapp, unsigned int *ip);

EXTERNL int
nc_put_varm_longlong(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t * imapp, const long long *op);

EXTERNL int
nc_get_varm_longlong(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t * imapp, long long *ip);

EXTERNL int
nc_put_varm_ulonglong(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const ptrdiff_t * imapp, const unsigned long long *op);

EXTERNL int
nc_get_varm_ulonglong(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const ptrdiff_t * imapp, unsigned long long *ip);

EXTERNL int
nc_put_varm_string(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const ptrdiff_t * imapp, const char **op);

EXTERNL int
nc_get_varm_string(int ncid, int varid, const size_t *startp,
                   const size_t *countp, const ptrdiff_t *stridep,
                   const ptrdiff_t * imapp, char **ip);

/* End {put,get}_varm */
/* Begin {put,get}_var */

EXTERNL int
nc_put_var_text(int ncid, int varid, const char *op);

EXTERNL int
nc_get_var_text(int ncid, int varid, char *ip);

EXTERNL int
nc_put_var_uchar(int ncid, int varid, const unsigned char *op);

EXTERNL int
nc_get_var_uchar(int ncid, int varid, unsigned char *ip);

EXTERNL int
nc_put_var_schar(int ncid, int varid, const signed char *op);

EXTERNL int
nc_get_var_schar(int ncid, int varid, signed char *ip);

EXTERNL int
nc_put_var_short(int ncid, int varid, const short *op);

EXTERNL int
nc_get_var_short(int ncid, int varid, short *ip);

EXTERNL int
nc_put_var_int(int ncid, int varid, const int *op);

EXTERNL int
nc_get_var_int(int ncid, int varid, int *ip);

EXTERNL int
nc_put_var_long(int ncid, int varid, const long *op);

EXTERNL int
nc_get_var_long(int ncid, int varid, long *ip);

EXTERNL int
nc_put_var_float(int ncid, int varid, const float *op);

EXTERNL int
nc_get_var_float(int ncid, int varid, float *ip);

EXTERNL int
nc_put_var_double(int ncid, int varid, const double *op);

EXTERNL int
nc_get_var_double(int ncid, int varid, double *ip);

EXTERNL int
nc_put_var_ushort(int ncid, int varid, const unsigned short *op);

EXTERNL int
nc_get_var_ushort(int ncid, int varid, unsigned short *ip);

EXTERNL int
nc_put_var_uint(int ncid, int varid, const unsigned int *op);

EXTERNL int
nc_get_var_uint(int ncid, int varid, unsigned int *ip);

EXTERNL int
nc_put_var_longlong(int ncid, int varid, const long long *op);

EXTERNL int
nc_get_var_longlong(int ncid, int varid, long long *ip);

EXTERNL int
nc_put_var_ulonglong(int ncid, int varid, const unsigned long long *op);

EXTERNL int
nc_get_var_ulonglong(int ncid, int varid, unsigned long long *ip);

EXTERNL int
nc_put_var_string(int ncid, int varid, const char **op);

EXTERNL int
nc_get_var_string(int ncid, int varid, char **ip);

/* Begin Deprecated, same as functions with "_ubyte" replaced by "_uchar" */
EXTERNL int
nc_put_att_ubyte(int ncid, int varid, const char *name, nc_type xtype,
                 size_t len, const unsigned char *op);
EXTERNL int
nc_get_att_ubyte(int ncid, int varid, const char *name,
                 unsigned char *ip);
EXTERNL int
nc_put_var1_ubyte(int ncid, int varid, const size_t *indexp,
                  const unsigned char *op);
EXTERNL int
nc_get_var1_ubyte(int ncid, int varid, const size_t *indexp,
                  unsigned char *ip);
EXTERNL int
nc_put_vara_ubyte(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const unsigned char *op);
EXTERNL int
nc_get_vara_ubyte(int ncid, int varid, const size_t *startp,
                  const size_t *countp, unsigned char *ip);
EXTERNL int
nc_put_vars_ubyte(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const unsigned char *op);
EXTERNL int
nc_get_vars_ubyte(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  unsigned char *ip);
EXTERNL int
nc_put_varm_ubyte(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t * imapp, const unsigned char *op);
EXTERNL int
nc_get_varm_ubyte(int ncid, int varid, const size_t *startp,
                  const size_t *countp, const ptrdiff_t *stridep,
                  const ptrdiff_t * imapp, unsigned char *ip);
EXTERNL int
nc_put_var_ubyte(int ncid, int varid, const unsigned char *op);
EXTERNL int
nc_get_var_ubyte(int ncid, int varid, unsigned char *ip);
/* End Deprecated */

/* Set the log level. 0 shows only errors, 1 only major messages,
 * etc., to 5, which shows way too much information. */
EXTERNL int
nc_set_log_level(int new_level);

/* Use this to turn off logging by calling
   nc_log_level(NC_TURN_OFF_LOGGING) */
#define NC_TURN_OFF_LOGGING (-1)

/* Show the netCDF library's in-memory metadata for a file. */
EXTERNL int
nc_show_metadata(int ncid);

/* End {put,get}_var */

/* #ifdef _CRAYMPP */
/*
 * Public interfaces to better support
 * CRAY multi-processor systems like T3E.
 * A tip of the hat to NERSC.
 */
/*
 * It turns out we need to declare and define
 * these public interfaces on all platforms
 * or things get ugly working out the
 * FORTRAN interface. On !_CRAYMPP platforms,
 * these functions work as advertised, but you
 * can only use "processor element" 0.
 */

EXTERNL int
nc__create_mp(const char *path, int cmode, size_t initialsz, int basepe,
         size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc__open_mp(const char *path, int mode, int basepe,
        size_t *chunksizehintp, int *ncidp);

EXTERNL int
nc_delete(const char *path);

EXTERNL int
nc_delete_mp(const char *path, int basepe);

EXTERNL int
nc_set_base_pe(int ncid, int pe);

EXTERNL int
nc_inq_base_pe(int ncid, int *pe);

/* #endif _CRAYMPP */

/* This v2 function is used in the nc_test program. */
EXTERNL int
nctypelen(nc_type datatype);

/* Begin v2.4 backward compatibility */

/** Backward compatible alias. */
/**@{*/
#define FILL_BYTE       NC_FILL_BYTE
#define FILL_CHAR       NC_FILL_CHAR
#define FILL_SHORT      NC_FILL_SHORT
#define FILL_LONG       NC_FILL_INT
#define FILL_FLOAT      NC_FILL_FLOAT
#define FILL_DOUBLE     NC_FILL_DOUBLE

#define MAX_NC_DIMS     NC_MAX_DIMS
#define MAX_NC_ATTRS    NC_MAX_ATTRS
#define MAX_NC_VARS     NC_MAX_VARS
#define MAX_NC_NAME     NC_MAX_NAME
#define MAX_VAR_DIMS    NC_MAX_VAR_DIMS
/**@}*/


/*
 * Global error status
 */
EXTERNL int ncerr;

#define NC_ENTOOL       NC_EMAXNAME   /**< Backward compatibility */
#define NC_EXDR         (-32)   /**< V2 API error. */
#define NC_SYSERR       (-31)   /**< V2 API system error. */

/*
 * Global options variable.
 * Used to determine behavior of error handler.
 */
#define NC_FATAL        1  /**< For V2 API, exit on error. */
#define NC_VERBOSE      2  /**< For V2 API, be verbose on error. */

/** V2 API error handling. Default is (NC_FATAL | NC_VERBOSE). */
EXTERNL int ncopts;

EXTERNL void
nc_advise(const char *cdf_routine_name, int err, const char *fmt,...);

/**
 * C data type corresponding to a netCDF NC_LONG argument, a signed 32
 * bit object. This is the only thing in this file which architecture
 * dependent.
 */
typedef int nclong;

EXTERNL int
nccreate(const char* path, int cmode);

EXTERNL int
ncopen(const char* path, int mode);

EXTERNL int
ncsetfill(int ncid, int fillmode);

EXTERNL int
ncredef(int ncid);

EXTERNL int
ncendef(int ncid);

EXTERNL int
ncsync(int ncid);

EXTERNL int
ncabort(int ncid);

EXTERNL int
ncclose(int ncid);

EXTERNL int
ncinquire(int ncid, int *ndimsp, int *nvarsp, int *nattsp, int *unlimdimp);

EXTERNL int
ncdimdef(int ncid, const char *name, long len);

EXTERNL int
ncdimid(int ncid, const char *name);

EXTERNL int
ncdiminq(int ncid, int dimid, char *name, long *lenp);

EXTERNL int
ncdimrename(int ncid, int dimid, const char *name);

EXTERNL int
ncattput(int ncid, int varid, const char *name, nc_type xtype,
        int len, const void *op);

EXTERNL int
ncattinq(int ncid, int varid, const char *name, nc_type *xtypep, int *lenp);

EXTERNL int
ncattget(int ncid, int varid, const char *name, void *ip);

EXTERNL int
ncattcopy(int ncid_in, int varid_in, const char *name, int ncid_out,
        int varid_out);

EXTERNL int
ncattname(int ncid, int varid, int attnum, char *name);

EXTERNL int
ncattrename(int ncid, int varid, const char *name, const char *newname);

EXTERNL int
ncattdel(int ncid, int varid, const char *name);

EXTERNL int
ncvardef(int ncid, const char *name, nc_type xtype,
        int ndims, const int *dimidsp);

EXTERNL int
ncvarid(int ncid, const char *name);

EXTERNL int
ncvarinq(int ncid, int varid, char *name, nc_type *xtypep,
        int *ndimsp, int *dimidsp, int *nattsp);

EXTERNL int
ncvarput1(int ncid, int varid, const long *indexp, const void *op);

EXTERNL int
ncvarget1(int ncid, int varid, const long *indexp, void *ip);

EXTERNL int
ncvarput(int ncid, int varid, const long *startp, const long *countp,
        const void *op);

EXTERNL int
ncvarget(int ncid, int varid, const long *startp, const long *countp,
        void *ip);

EXTERNL int
ncvarputs(int ncid, int varid, const long *startp, const long *countp,
        const long *stridep, const void *op);

EXTERNL int
ncvargets(int ncid, int varid, const long *startp, const long *countp,
        const long *stridep, void *ip);

EXTERNL int
ncvarputg(int ncid, int varid, const long *startp, const long *countp,
        const long *stridep, const long *imapp, const void *op);

EXTERNL int
ncvargetg(int ncid, int varid, const long *startp, const long *countp,
        const long *stridep, const long *imapp, void *ip);

EXTERNL int
ncvarrename(int ncid, int varid, const char *name);

EXTERNL int
ncrecinq(int ncid, int *nrecvarsp, int *recvaridsp, long *recsizesp);

EXTERNL int
ncrecget(int ncid, long recnum, void **datap);

EXTERNL int
ncrecput(int ncid, long recnum, void *const *datap);

/* This function may be called to force the library to
   initialize itself. It is not required, however.
*/
EXTERNL int nc_initialize(void);

/* This function may be called to force the library to
   cleanup global memory so that memory checkers will not
   report errors. It is not required, however.
*/
EXTERNL int nc_finalize(void);

#if defined(__cplusplus)
}
#endif

/* Define two hard-coded functionality-related
   (as requested by community developers) macros.
   This is not going to be standard practice.
   Don't remove without an in-place replacement of some sort,
   the are now (for better or worse) used by downstream
   software external to Unidata. */
#ifndef NC_HAVE_RENAME_GRP
#define NC_HAVE_RENAME_GRP /*!< rename_grp() support. */
#endif

#ifndef NC_HAVE_INQ_FORMAT_EXTENDED
#define NC_HAVE_INQ_FORMAT_EXTENDED /*!< inq_format_extended() support. */
#endif

#define NC_HAVE_META_H

#endif /* _NETCDF_ */
