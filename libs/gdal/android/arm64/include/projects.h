/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Primary (private) include file for PROJ.4 library.
 * Author:   Gerald Evenden
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

/* General projections header file */
#ifndef PROJECTS_H
#define PROJECTS_H

#ifdef _MSC_VER
#  ifndef _CRT_SECURE_NO_DEPRECATE
#    define _CRT_SECURE_NO_DEPRECATE
#  endif
#  ifndef _CRT_NONSTDC_NO_DEPRECATE
#    define _CRT_NONSTDC_NO_DEPRECATE
#  endif
/* enable predefined math constants M_* for MS Visual Studio workaround */
#  ifndef _USE_MATH_DEFINES
#     define _USE_MATH_DEFINES
#  endif
#endif

/* standard inclusions */
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#define C_NAMESPACE extern "C"
#define C_NAMESPACE_VAR extern "C"
extern "C" {
#else
#define C_NAMESPACE extern
#define C_NAMESPACE_VAR
#endif

#ifndef NULL
#  define NULL 0
#endif

#ifndef FALSE
#  define FALSE 0
#endif

#ifndef TRUE
#  define TRUE  1
#endif

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

#ifndef ABS
#  define ABS(x)        ((x<0) ? (-1*(x)) : x)
#endif

#if INT_MAX == 2147483647
typedef int pj_int32;
#elif LONG_MAX == 2147483647
typedef long pj_int32;
#else
#warning It seems no 32-bit integer type is available
#endif

/* maximum path/filename */
#ifndef MAX_PATH_FILENAME
#define MAX_PATH_FILENAME 1024
#endif

/* If we still haven't got M_PI*, we rely on our own defines.
 * For example, this is necessary when compiling with gcc and
 * the -ansi flag.
 */
#ifndef M_PI
#define M_PI            3.14159265358979323846
#define M_PI_2          1.57079632679489661923
#define M_PI_4          0.78539816339744830962
#define M_2_PI          0.63661977236758134308
#endif

/* M_SQRT2 might be missing */
#ifndef M_SQRT2
#define M_SQRT2         1.41421356237309504880
#endif

/* some more useful math constants and aliases */
#define M_FORTPI        M_PI_4                   /* pi/4 */
#define M_HALFPI        M_PI_2                   /* pi/2 */
#define M_PI_HALFPI     4.71238898038468985769   /* 1.5*pi */
#define M_TWOPI         6.28318530717958647693   /* 2*pi */
#define M_TWO_D_PI      M_2_PI                   /* 2/pi */
#define M_TWOPI_HALFPI  7.85398163397448309616   /* 2.5*pi */


/* maximum tag id length for +init and default files */
#ifndef ID_TAG_MAX
#define ID_TAG_MAX 50
#endif

/* Use WIN32 as a standard windows 32 bit declaration */
#if defined(_WIN32) && !defined(WIN32)
#  define WIN32
#endif

#if defined(_WINDOWS) && !defined(WIN32)
#  define WIN32
#endif

/* directory delimiter for DOS support */
#ifdef WIN32
#define DIR_CHAR '\\'
#else
#define DIR_CHAR '/'
#endif

#define USE_PROJUV

typedef struct { double u, v; } projUV;
typedef struct { double r, i; } COMPLEX;
typedef struct { double u, v, w; } projUVW;

/* If user explicitly includes proj.h, before projects.h, then avoid implicit type-punning */
#ifndef PROJ_H
#ifndef PJ_LIB__
#define XY projUV
#define LP projUV
#define XYZ projUVW
#define LPZ projUVW

#else
typedef struct { double x, y; }        XY;
typedef struct { double x, y, z; }     XYZ;
typedef struct { double lam, phi; }    LP;
typedef struct { double lam, phi, z; } LPZ;
typedef struct { double u, v; }        UV;
typedef struct { double u, v, w; }     UVW;
#endif  /* ndef PJ_LIB__ */

#else
typedef PJ_XY XY;
typedef PJ_LP LP;
typedef PJ_UV UV;
typedef PJ_XYZ XYZ;
typedef PJ_LPZ LPZ;
typedef PJ_UVW UVW;

#endif  /* ndef PROJ_H   */


/* Forward declarations and typedefs for stuff needed inside the PJ object */
struct PJconsts;

union  PJ_COORD;
struct geod_geodesic;
struct pj_opaque;
struct ARG_list;
struct PJ_REGION_S;
typedef struct PJ_REGION_S  PJ_Region;
typedef struct ARG_list paralist;   /* parameter list */
#ifndef PROJ_INTERNAL_H
enum pj_io_units {
    PJ_IO_UNITS_WHATEVER  = 0,  /* Doesn't matter (or depends on pipeline neighbours) */
    PJ_IO_UNITS_CLASSIC   = 1,  /* Scaled meters (right), projected system */
    PJ_IO_UNITS_PROJECTED = 2,  /* Meters, projected system */
    PJ_IO_UNITS_CARTESIAN = 3,  /* Meters, 3D cartesian system */
    PJ_IO_UNITS_ANGULAR   = 4   /* Radians */
};
#endif
#ifndef PROJ_H
typedef struct PJconsts PJ;         /* the PJ object herself */
typedef union  PJ_COORD PJ_COORD;
#endif

struct PJ_REGION_S {
    double ll_long;        /* lower left corner coordinates (radians) */
    double ll_lat;
    double ur_long;        /* upper right corner coordinates (radians) */
    double ur_lat;
};

struct PJ_AREA {
    int     id;         /* Area ID in the EPSG database */
    LP      ll;         /* Lower left corner of bounding box */
    LP      ur;         /* Upper right corner of bounding box */
    char    descr[64];  /* text representation of area */
};

struct projCtx_t;
typedef struct projCtx_t projCtx_t;

/*****************************************************************************

    Some function types that are especially useful when working with PJs

******************************************************************************

PJ_CONSTRUCTOR:

    A function taking a pointer-to-PJ as arg, and returning a pointer-to-PJ.
    Historically called twice: First with a 0 argument, to allocate memory,
    second with the first return value as argument, for actual setup.

PJ_DESTRUCTOR:

    A function taking a pointer-to-PJ and an integer as args, then first
    handling the deallocation of the PJ, afterwards handing the integer over
    to the error reporting subsystem, and finally returning a null pointer in
    support of the "return free (P)" (aka "get the hell out of here") idiom.

PJ_OPERATOR:

    A function taking a PJ_COORD and a pointer-to-PJ as args, applying the
    PJ to the PJ_COORD, and returning the resulting PJ_COORD.

*****************************************************************************/
typedef    PJ       *(* PJ_CONSTRUCTOR) (PJ *);
typedef    void     *(* PJ_DESTRUCTOR)  (PJ *, int);
typedef    PJ_COORD  (* PJ_OPERATOR)    (PJ_COORD, PJ *);
/****************************************************************************/



/* base projection data structure */
struct PJconsts {

    /*************************************************************************************

                         G E N E R A L   P A R A M E T E R   S T R U C T

    **************************************************************************************

        TODO: Need some description here - especially about the thread context...
        This is the struct behind the PJ typedef

    **************************************************************************************/

    projCtx_t *ctx;
    const char *descr;             /* From pj_list.h or individual PJ_*.c file */
    paralist *params;              /* Parameter list */
    char *def_full;                /* Full textual definition (usually 0 - set by proj_pj_info) */
    char *def_size;                /* Shape and size parameters extracted from params */
    char *def_shape;
    char *def_spherification;
    char *def_ellps;

    struct geod_geodesic *geod;    /* For geodesic computations */
    struct pj_opaque *opaque;      /* Projection specific parameters, Defined in PJ_*.c */
    int inverted;                  /* Tell high level API functions to swap inv/fwd */


    /*************************************************************************************

                          F U N C T I O N    P O I N T E R S

    **************************************************************************************

        For projection xxx, these are pointers to functions in the corresponding
        PJ_xxx.c file.

        pj_init() delegates the setup of these to pj_projection_specific_setup_xxx(),
        a name which is currently hidden behind the magic curtain of the PROJECTION
        macro.

    **************************************************************************************/


    XY  (*fwd)(LP,    PJ *);
    LP  (*inv)(XY,    PJ *);
    XYZ (*fwd3d)(LPZ, PJ *);
    LPZ (*inv3d)(XYZ, PJ *);
    PJ_OPERATOR fwd4d;
    PJ_OPERATOR inv4d;

    PJ_DESTRUCTOR destructor;


    /*************************************************************************************

                          E L L I P S O I D     P A R A M E T E R S

    **************************************************************************************

        Despite YAGNI, we add a large number of ellipsoidal shape parameters, which
        are not yet set up in pj_init. They are, however, inexpensive to compute,
        compared to the overall time taken for setting up the complex PJ object
        (cf. e.g. https://en.wikipedia.org/wiki/Angular_eccentricity).

        But during single point projections it will often be a useful thing to have
        these readily available without having to recompute at every pj_fwd / pj_inv
        call.

        With this wide selection, we should be ready for quite a number of geodetic
        algorithms, without having to incur further ABI breakage.

    **************************************************************************************/

    /* The linear parameters */

    double  a;                         /* semimajor axis (radius if eccentricity==0) */
    double  b;                         /* semiminor axis */
    double  ra;                        /* 1/a */
    double  rb;                        /* 1/b */

    /* The eccentricities */

    double  alpha;                     /* angular eccentricity */
    double  e;                         /* first  eccentricity */
    double  es;                        /* first  eccentricity squared */
    double  e2;                        /* second eccentricity */
    double  e2s;                       /* second eccentricity squared */
    double  e3;                        /* third  eccentricity */
    double  e3s;                       /* third  eccentricity squared */
    double  one_es;                    /* 1 - e^2 */
    double  rone_es;                   /* 1/one_es */


    /* The flattenings */
    double  f;                         /* first  flattening */
    double  f2;                        /* second flattening */
    double  n;                         /* third  flattening */
    double  rf;                        /* 1/f  */
    double  rf2;                       /* 1/f2 */
    double  rn;                        /* 1/n  */

    /* This one's for GRS80 */
    double  J;                         /* "Dynamic form factor" */

    double  es_orig, a_orig;           /* es and a before any +proj related adjustment */


    /*************************************************************************************

                          C O O R D I N A T E   H A N D L I N G

    **************************************************************************************/

    int  over;                      /* Over-range flag */
    int  geoc;                      /* Geocentric latitude flag */
    int  is_latlong;                /* proj=latlong ... not really a projection at all */
    int  is_geocent;                /* proj=geocent ... not really a projection at all */
    int  is_pipeline;               /* 1 if PJ represents a pipeline */
    int  need_ellps;                /* 0 for operations that are purely cartesian */
    int  skip_fwd_prepare;
    int  skip_fwd_finalize;
    int  skip_inv_prepare;
    int  skip_inv_finalize;

    enum pj_io_units left;          /* Flags for input/output coordinate types */
    enum pj_io_units right;

    /* These PJs are used for implementing cs2cs style coordinate handling in the 4D API */
    PJ *axisswap;
    PJ *cart;
    PJ *cart_wgs84;
    PJ *helmert;
    PJ *hgridshift;
    PJ *vgridshift;


    /*************************************************************************************

                       C A R T O G R A P H I C       O F F S E T S

    **************************************************************************************/

    double  lam0, phi0;                /* central meridian, parallel */
    double  x0, y0, z0, t0;            /* false easting and northing (and height and time) */


    /*************************************************************************************

                                    S C A L I N G

    **************************************************************************************/

    double  k0;                        /* General scaling factor - e.g. the 0.9996 of UTM */
    double  to_meter, fr_meter;        /* Plane coordinate scaling. Internal unit [m] */
    double  vto_meter, vfr_meter;      /* Vertical scaling. Internal unit [m] */


    /*************************************************************************************

                  D A T U M S   A N D   H E I G H T   S Y S T E M S

    **************************************************************************************

        It may be possible, and meaningful, to move the list parts of this up to the
        PJ_CONTEXT level.

    **************************************************************************************/

    int     datum_type;                /* PJD_UNKNOWN/3PARAM/7PARAM/GRIDSHIFT/WGS84 */
    double  datum_params[7];           /* Parameters for 3PARAM and 7PARAM */
    struct _pj_gi **gridlist;          /* TODO: Description needed */
    int     gridlist_count;

    int     has_geoid_vgrids;          /* TODO: Description needed */
    struct _pj_gi **vgridlist_geoid;   /* TODO: Description needed */
    int     vgridlist_geoid_count;

    double  from_greenwich;            /* prime meridian offset (in radians) */
    double  long_wrap_center;          /* 0.0 for -180 to 180, actually in radians*/
    int     is_long_wrap_set;
    char    axis[4];                   /* Axis order, pj_transform/pj_adjust_axis */

    /* New Datum Shift Grid Catalogs */
    char   *catalog_name;
    struct _PJ_GridCatalog *catalog;

    double  datum_date;                 /* TODO: Description needed */

    struct _pj_gi *last_before_grid;    /* TODO: Description needed */
    PJ_Region     last_before_region;   /* TODO: Description needed */
    double        last_before_date;     /* TODO: Description needed */

    struct _pj_gi *last_after_grid;     /* TODO: Description needed */
    PJ_Region     last_after_region;    /* TODO: Description needed */
    double        last_after_date;      /* TODO: Description needed */


    /*************************************************************************************

                 E N D   O F    G E N E R A L   P A R A M E T E R   S T R U C T

    **************************************************************************************/

};




/* Parameter list (a copy of the +proj=... etc. parameters) */
struct ARG_list {
    paralist *next;
    char used;
#if defined(__GNUC__) && __GNUC__ >= 8
    char param[]; /* variable-length member */
    /* Safer to use [] for gcc 8. See https://github.com/OSGeo/proj.4/pull/1087 */
    /* and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86914 */
#else
    char param[1]; /* variable-length member */
#endif
};


typedef union { double  f; int  i; char *s; } PROJVALUE;


struct PJ_ELLPS {
    char    *id;           /* ellipse keyword name */
    char    *major;        /* a= value */
    char    *ell;          /* elliptical parameter */
    char    *name;         /* comments */
};

struct PJ_UNITS {
    char    *id;           /* units keyword */
    char    *to_meter;     /* multiply by value to get meters */
    char    *name;         /* comments */
    double   factor;       /* to_meter factor in actual numbers */
};

struct PJ_DATUMS {
    char    *id;           /* datum keyword */
    char    *defn;         /* ie. "to_wgs84=..." */
    char    *ellipse_id;   /* ie from ellipse table */
    char    *comments;     /* EPSG code, etc */
};

struct PJ_PRIME_MERIDIANS {
    char    *id;           /* prime meridian keyword */
    char    *defn;         /* offset from greenwich in DMS format. */
};


struct DERIVS {
    double x_l, x_p;       /* derivatives of x for lambda-phi */
    double y_l, y_p;       /* derivatives of y for lambda-phi */
};

struct FACTORS {
    struct DERIVS der;
    double h, k;           /* meridional, parallel scales */
    double omega, thetap;  /* angular distortion, theta prime */
    double conv;           /* convergence */
    double s;              /* areal scale factor */
    double a, b;           /* max-min scale error */
    int    code;           /* always 0 */
};

enum deprecated_constants_for_now_dropped_analytical_factors {
    IS_ANAL_XL_YL =  01,   /* derivatives of lon analytic */
    IS_ANAL_XP_YP =  02,   /* derivatives of lat analytic */
    IS_ANAL_HK    =  04,   /* h and k analytic */
    IS_ANAL_CONV  = 010    /* convergence analytic */
};



/* datum_type values */
#define PJD_UNKNOWN   0
#define PJD_3PARAM    1
#define PJD_7PARAM    2
#define PJD_GRIDSHIFT 3
#define PJD_WGS84     4   /* WGS84 (or anything considered equivalent) */

/* library errors */
#define PJD_ERR_NO_ARGS                  -1
#define PJD_ERR_NO_OPTION_IN_INIT_FILE   -2
#define PJD_ERR_NO_COLON_IN_INIT_STRING  -3
#define PJD_ERR_PROJ_NOT_NAMED           -4
#define PJD_ERR_UNKNOWN_PROJECTION_ID    -5
#define PJD_ERR_ECCENTRICITY_IS_ONE      -6
#define PJD_ERR_UNKNOW_UNIT_ID           -7  /* deprecated: typo */
#define PJD_ERR_UNKNOWN_UNIT_ID          -7
#define PJD_ERR_INVALID_BOOLEAN_PARAM    -8
#define PJD_ERR_UNKNOWN_ELLP_PARAM       -9
#define PJD_ERR_REV_FLATTENING_IS_ZERO  -10
#define PJD_ERR_REF_RAD_LARGER_THAN_90  -11
#define PJD_ERR_ES_LESS_THAN_ZERO       -12
#define PJD_ERR_MAJOR_AXIS_NOT_GIVEN    -13
#define PJD_ERR_LAT_OR_LON_EXCEED_LIMIT -14
#define PJD_ERR_INVALID_X_OR_Y          -15
#define PJD_ERR_WRONG_FORMAT_DMS_VALUE  -16
#define PJD_ERR_NON_CONV_INV_MERI_DIST  -17
#define PJD_ERR_NON_CON_INV_PHI2        -18
#define PJD_ERR_ACOS_ASIN_ARG_TOO_LARGE -19
#define PJD_ERR_TOLERANCE_CONDITION     -20
#define PJD_ERR_CONIC_LAT_EQUAL         -21
#define PJD_ERR_LAT_LARGER_THAN_90      -22
#define PJD_ERR_LAT1_IS_ZERO            -23
#define PJD_ERR_LAT_TS_LARGER_THAN_90   -24
#define PJD_ERR_CONTROL_POINT_NO_DIST   -25
#define PJD_ERR_NO_ROTATION_PROJ        -26
#define PJD_ERR_W_OR_M_ZERO_OR_LESS     -27
#define PJD_ERR_LSAT_NOT_IN_RANGE       -28
#define PJD_ERR_PATH_NOT_IN_RANGE       -29
#define PJD_ERR_H_LESS_THAN_ZERO        -30
#define PJD_ERR_K_LESS_THAN_ZERO        -31
#define PJD_ERR_LAT_1_OR_2_ZERO_OR_90   -32
#define PJD_ERR_LAT_0_OR_ALPHA_EQ_90    -33
#define PJD_ERR_ELLIPSOID_USE_REQUIRED  -34
#define PJD_ERR_INVALID_UTM_ZONE        -35
#define PJD_ERR_TCHEBY_VAL_OUT_OF_RANGE -36
#define PJD_ERR_FAILED_TO_FIND_PROJ     -37
#define PJD_ERR_FAILED_TO_LOAD_GRID     -38
#define PJD_ERR_INVALID_M_OR_N          -39
#define PJD_ERR_N_OUT_OF_RANGE          -40
#define PJD_ERR_LAT_1_2_UNSPECIFIED     -41
#define PJD_ERR_ABS_LAT1_EQ_ABS_LAT2    -42
#define PJD_ERR_LAT_0_HALF_PI_FROM_MEAN -43
#define PJD_ERR_UNPARSEABLE_CS_DEF      -44
#define PJD_ERR_GEOCENTRIC              -45
#define PJD_ERR_UNKNOWN_PRIME_MERIDIAN  -46
#define PJD_ERR_AXIS                    -47
#define PJD_ERR_GRID_AREA               -48
#define PJD_ERR_INVALID_SWEEP_AXIS      -49
#define PJD_ERR_MALFORMED_PIPELINE      -50
#define PJD_ERR_UNIT_FACTOR_LESS_THAN_0 -51
#define PJD_ERR_INVALID_SCALE           -52
#define PJD_ERR_NON_CONVERGENT          -53
#define PJD_ERR_MISSING_ARGS            -54
#define PJD_ERR_LAT_0_IS_ZERO           -55
#define PJD_ERR_ELLIPSOIDAL_UNSUPPORTED -56
#define PJD_ERR_TOO_MANY_INITS          -57
#define PJD_ERR_INVALID_ARG             -58
#define PJD_ERR_INCONSISTENT_UNIT       -59
/* NOTE: Remember to update pj_strerrno.c and transient_error in */
/* pj_transform.c when adding new value */

struct projFileAPI_t;

/* proj thread context */
struct projCtx_t {
    int     last_errno;
    int     debug_level;
    void    (*logger)(void *, int, const char *);
    void    *app_data;
    struct projFileAPI_t *fileapi;
};

/* classic public API */
#include "proj_api.h"


/* Generate pj_list external or make list from include file */

struct PJ_LIST {
    char    *id;                 /* projection keyword */
    PJ *(*proj)(PJ *);           /* projection entry point */
    char    * const *descr;      /* description text */
};


#ifndef USE_PJ_LIST_H
extern struct PJ_LIST pj_list[];
#endif

#ifndef PJ_ELLPS__
extern struct PJ_ELLPS pj_ellps[];
#endif

#ifndef PJ_UNITS__
extern struct PJ_UNITS pj_units[];
#endif

#ifndef PJ_DATUMS__
extern struct PJ_DATUMS pj_datums[];
extern struct PJ_PRIME_MERIDIANS pj_prime_meridians[];
#endif





#ifdef PJ_LIB__
#define PROJ_HEAD(name, desc) static const char des_##name [] = desc

#define OPERATION(name, NEED_ELLPS)                          \
                                                             \
pj_projection_specific_setup_##name (PJ *P);                 \
C_NAMESPACE PJ *pj_##name (PJ *P);                           \
                                                             \
C_NAMESPACE_VAR const char * const pj_s_##name = des_##name; \
                                                             \
C_NAMESPACE PJ *pj_##name (PJ *P) {                          \
    if (P)                                                   \
        return pj_projection_specific_setup_##name (P);      \
    P = (PJ*) pj_calloc (1, sizeof(PJ));                     \
    if (0==P)                                                \
        return 0;                                            \
    P->destructor = pj_default_destructor;                   \
    P->descr = des_##name;                                   \
    P->need_ellps = NEED_ELLPS;                              \
    P->left  = PJ_IO_UNITS_ANGULAR;                          \
    P->right = PJ_IO_UNITS_CLASSIC;                          \
    return P;                                                \
}                                                            \
                                                             \
PJ *pj_projection_specific_setup_##name (PJ *P)

/* In ISO19000 lingo, an operation is either a conversion or a transformation */
#define CONVERSION(name, need_ellps)      OPERATION (name, need_ellps)
#define TRANSFORMATION(name, need_ellps)  OPERATION (name, need_ellps)

/* In PROJ.4 a projection is a conversion taking angular input and giving scaled linear output */
#define PROJECTION(name) CONVERSION (name, 1)

#endif /* def PJ_LIB__ */


#define MAX_TAB_ID 80
typedef struct { float lam, phi; } FLP;
typedef struct { pj_int32 lam, phi; } ILP;

struct CTABLE {
    char id[MAX_TAB_ID];    /* ascii info */
    LP ll;                  /* lower left corner coordinates */
    LP del;                 /* size of cells */
    ILP lim;                /* limits of conversion matrix */
    FLP *cvs;               /* conversion matrix */
};

typedef struct _pj_gi {
    char *gridname;     /* identifying name of grid, eg "conus" or ntv2_0.gsb */
    char *filename;     /* full path to filename */

    const char *format; /* format of this grid, ie "ctable", "ntv1",
                           "ntv2" or "missing". */

    long   grid_offset;  /* offset in file, for delayed loading */
    int   must_swap;    /* only for NTv2 */

    struct CTABLE *ct;

    struct _pj_gi *next;
    struct _pj_gi *child;
} PJ_GRIDINFO;

typedef struct {
    PJ_Region region;
    int  priority;      /* higher used before lower */
    double date;        /* year.fraction */
    char *definition;   /* usually the gridname */

    PJ_GRIDINFO  *gridinfo;
    int available;      /* 0=unknown, 1=true, -1=false */
} PJ_GridCatalogEntry;

typedef struct _PJ_GridCatalog {
    char *catalog_name;

    PJ_Region region;   /* maximum extent of catalog data */

    int entry_count;
    PJ_GridCatalogEntry *entries;

    struct _PJ_GridCatalog *next;
} PJ_GridCatalog;

/* procedure prototypes */
double dmstor(const char *, char **);
double dmstor_ctx(projCtx ctx, const char *, char **);
void   set_rtodms(int, int);
char  *rtodms(char *, double, int, int);
double adjlon(double);
double aacos(projCtx,double), aasin(projCtx,double), asqrt(double), aatan2(double, double);

PROJVALUE pj_param(projCtx ctx, paralist *, const char *);
paralist *pj_param_exists (paralist *list, const char *parameter);
paralist *pj_mkparam(char *);
paralist *pj_mkparam_ws (char *str);


int pj_ell_set(projCtx ctx, paralist *, double *, double *);
int pj_datum_set(projCtx,paralist *, PJ *);
int pj_prime_meridian_set(paralist *, PJ *);
int pj_angular_units_set(paralist *, PJ *);

paralist *pj_clone_paralist( const paralist* );
paralist *pj_search_initcache( const char *filekey );
void      pj_insert_initcache( const char *filekey, const paralist *list);
paralist *pj_expand_init(projCtx ctx, paralist *init);

void     *pj_dealloc_params (projCtx ctx, paralist *start, int errlev);


double *pj_enfn(double);
double  pj_mlfn(double, double, double, double *);
double  pj_inv_mlfn(projCtx, double, double, double *);
double  pj_qsfn(double, double, double);
double  pj_tsfn(double, double, double);
double  pj_msfn(double, double, double);
double  pj_phi2(projCtx, double, double);
double  pj_qsfn_(double, PJ *);
double *pj_authset(double);
double  pj_authlat(double, double *);

COMPLEX pj_zpoly1(COMPLEX, const COMPLEX *, int);
COMPLEX pj_zpolyd1(COMPLEX, const COMPLEX *, int, COMPLEX *);

int pj_deriv(LP, double, const PJ *, struct DERIVS *);
int pj_factors(LP, const PJ *, double, struct FACTORS *);

struct PW_COEF {    /* row coefficient structure */
    int m;          /* number of c coefficients (=0 for none) */
    double *c;      /* power coefficients */
};

/* Approximation structures and procedures */
typedef struct {    /* Chebyshev or Power series structure */
    projUV a, b;    /* power series range for evaluation */
                    /* or Chebyshev argument shift/scaling */
    struct PW_COEF *cu, *cv;
    int mu, mv;     /* maximum cu and cv index (+1 for count) */
    int power;      /* != 0 if power series, else Chebyshev */
} Tseries;

Tseries *mk_cheby(projUV, projUV, double, projUV *, projUV (*)(projUV), int, int, int);
projUV   bpseval(projUV, Tseries *);
projUV   bcheval(projUV, Tseries *);
projUV   biveval(projUV, Tseries *);
void    *vector1(int, int);
void   **vector2(int, int, int);
void     freev2(void **v, int nrows);
int      bchgen(projUV, projUV, int, int, projUV **, projUV(*)(projUV));
int      bch2bps(projUV, projUV, projUV **, int, int);

/* nadcon related protos */
LP             nad_intr(LP, struct CTABLE *);
LP             nad_cvt(LP, int, struct CTABLE *);
struct CTABLE *nad_init(projCtx ctx, char *);
struct CTABLE *nad_ctable_init( projCtx ctx, PAFile fid );
int            nad_ctable_load( projCtx ctx, struct CTABLE *, PAFile fid );
struct CTABLE *nad_ctable2_init( projCtx ctx, PAFile fid );
int            nad_ctable2_load( projCtx ctx, struct CTABLE *, PAFile fid );
void           nad_free(struct CTABLE *);

/* higher level handling of datum grid shift files */

int pj_apply_vgridshift( PJ *defn, const char *listname,
                         PJ_GRIDINFO ***gridlist_p,
                         int *gridlist_count_p,
                         int inverse,
                         long point_count, int point_offset,
                         double *x, double *y, double *z );
int pj_apply_gridshift_2( PJ *defn, int inverse,
                          long point_count, int point_offset,
                          double *x, double *y, double *z );
int pj_apply_gridshift_3( projCtx ctx,
                          PJ_GRIDINFO **gridlist, int gridlist_count,
                          int inverse, long point_count, int point_offset,
                          double *x, double *y, double *z );

PJ_GRIDINFO **pj_gridlist_from_nadgrids( projCtx, const char *, int * );
void pj_deallocate_grids();

PJ_GRIDINFO *pj_gridinfo_init( projCtx, const char * );
int          pj_gridinfo_load( projCtx, PJ_GRIDINFO * );
void         pj_gridinfo_free( projCtx, PJ_GRIDINFO * );

PJ_GridCatalog *pj_gc_findcatalog( projCtx, const char * );
PJ_GridCatalog *pj_gc_readcatalog( projCtx, const char * );
void pj_gc_unloadall( projCtx );
int pj_gc_apply_gridshift( PJ *defn, int inverse,
                           long point_count, int point_offset,
                           double *x, double *y, double *z );
int pj_gc_apply_gridshift( PJ *defn, int inverse,
                           long point_count, int point_offset,
                           double *x, double *y, double *z );

PJ_GRIDINFO *pj_gc_findgrid( projCtx ctx,
                             PJ_GridCatalog *catalog, int after,
                             LP location, double date,
                             PJ_Region *optional_region,
                             double *grid_date );

double pj_gc_parsedate( projCtx, const char * );

void  *proj_mdist_ini(double);
double proj_mdist(double, double, double, const void *);
double proj_inv_mdist(projCtx ctx, double, const void *);
void  *pj_gauss_ini(double, double, double *,double *);
LP     pj_gauss(projCtx, LP, const void *);
LP     pj_inv_gauss(projCtx, LP, const void *);

extern char const pj_release[];

struct PJ_ELLPS            *pj_get_ellps_ref( void );
struct PJ_DATUMS           *pj_get_datums_ref( void );
struct PJ_UNITS            *pj_get_units_ref( void );
struct PJ_LIST             *pj_get_list_ref( void );
struct PJ_PRIME_MERIDIANS  *pj_get_prime_meridians_ref( void );

void *pj_default_destructor (PJ *P, int errlev);

double pj_atof( const char* nptr );
double pj_strtod( const char *nptr, char **endptr );
void   pj_freeup_plain (PJ *P);

#ifdef __cplusplus
}
#endif

#ifndef PROJECTS_H_ATEND
#define PROJECTS_H_ATEND
#endif
#endif /* end of basic projections header */
