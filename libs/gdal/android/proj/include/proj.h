/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Revised, experimental API for PROJ.4, intended as the foundation
 *           for added geodetic functionality.
 *
 *           The original proj API (defined in projects.h) has grown organically
 *           over the years, but it has also grown somewhat messy.
 *
 *           The same has happened with the newer high level API (defined in
 *           proj_api.h): To support various historical objectives, proj_api.h
 *           contains a rather complex combination of conditional defines and
 *           typedefs. Probably for good (historical) reasons, which are not
 *           always evident from today's perspective.
 *
 *           This is an evolving attempt at creating a re-rationalized API
 *           with primary design goals focused on sanitizing the namespaces.
 *           Hence, all symbols exposed are being moved to the proj_ namespace,
 *           while all data types are being moved to the PJ_ namespace.
 *
 *           Please note that this API is *orthogonal* to  the previous APIs:
 *           Apart from some inclusion guards, projects.h and proj_api.h are not
 *           touched - if you do not include proj.h, the projects and proj_api
 *           APIs should work as they always have.
 *
 *           A few implementation details:
 *
 *           Apart from the namespacing efforts, I'm trying to eliminate three
 *           proj_api elements, which I have found especially confusing.
 *
 *           FIRST and foremost, I try to avoid typedef'ing away pointer
 *           semantics. I agree that it can be occasionally useful, but I
 *           prefer having the pointer nature of function arguments being
 *           explicitly visible.
 *
 *           Hence, projCtx has been replaced by PJ_CONTEXT *.
 *           and    projPJ  has been replaced by PJ *
 *
 *           SECOND, I try to eliminate cases of information hiding implemented
 *           by redefining data types to void pointers.
 *
 *           I prefer using a combination of forward declarations and typedefs.
 *           Hence:
 *               typedef void *projCtx;
 *           Has been replaced by:
 *               struct projCtx_t;
 *               typedef struct projCtx_t PJ_CONTEXT;
 *           This makes it possible for the calling program to know that the
 *           PJ_CONTEXT data type exists, and handle pointers to that data type
 *           without having any idea about its internals.
 *
 *           (obviously, in this example, struct projCtx_t should also be
 *           renamed struct pj_ctx some day, but for backwards compatibility
 *           it remains as-is for now).
 *
 *           THIRD, I try to eliminate implicit type punning. Hence this API
 *           introduces the PJ_COORD union data type, for generic 4D coordinate
 *           handling.
 *
 *           PJ_COORD makes it possible to make explicit the previously used
 *           "implicit type punning", where a XY is turned into a LP by
 *           re#defining both as UV, behind the back of the user.
 *
 *           The PJ_COORD union is used for storing 1D, 2D, 3D and 4D coordinates.
 *
 *           The bare essentials API presented here follows the PROJ.4
 *           convention of sailing the coordinate to be reprojected, up on
 *           the stack ("call by value"), and symmetrically returning the
 *           result on the stack. Although the PJ_COORD object is twice as large
 *           as the traditional XY and LP objects, timing results have shown the
 *           overhead to be very reasonable.
 *
 *           Contexts and thread safety
 *           --------------------------
 *
 *           After a year of experiments (and previous experience from the
 *           trlib transformation library) it has become clear that the
 *           context subsystem is unavoidable in a multi-threaded world.
 *           Hence, instead of hiding it away, we move it into the limelight,
 *           highly recommending (but not formally requiring) the bracketing
 *           of any code block calling PROJ.4 functions with calls to
 *           proj_context_create(...)/proj_context_destroy()
 *
 *           Legacy single threaded code need not do anything, but *may*
 *           implement a bit of future compatibility by using the backward
 *           compatible call proj_context_create(0), which will not create
 *           a new context, but simply provide a pointer to the default one.
 *
 *           See proj_4D_api_test.c for examples of how to use the API.
 *
 * Author:   Thomas Knudsen, <thokn@sdfe.dk>
 *           Benefitting from a large number of comments and suggestions
 *           by (primarily) Kristian Evers and Even Rouault.
 *
 ******************************************************************************
 * Copyright (c) 2016, 2017, Thomas Knudsen / SDFE
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO COORD SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include <stddef.h>  /* For size_t */


#ifdef PROJECTS_H
#error proj.h must be included before projects.h
#endif
#ifdef PROJ_API_H
#error proj.h must be included before proj_api.h
#endif

#ifndef PROJ_H
#define PROJ_H
#ifdef __cplusplus
extern "C" {
#endif

/* The version numbers should be updated with every release! **/
#define PROJ_VERSION_MAJOR 5
#define PROJ_VERSION_MINOR 2
#define PROJ_VERSION_PATCH 0

extern char const pj_release[]; /* global release id string */

/* first forward declare everything needed */

/* Data type for generic geodetic 3D data plus epoch information */
union PJ_COORD;
typedef union PJ_COORD PJ_COORD;

struct PJ_AREA;
typedef struct PJ_AREA PJ_AREA;

struct P5_FACTORS {                  /* Common designation */
    double meridional_scale;               /* h */
    double parallel_scale;                 /* k */
    double areal_scale;                    /* s */

    double angular_distortion;             /* omega */
    double meridian_parallel_angle;        /* theta-prime */
    double meridian_convergence;           /* alpha */

    double tissot_semimajor;               /* a */
    double tissot_semiminor;               /* b */

    double dx_dlam, dx_dphi;
    double dy_dlam, dy_dphi;
};
typedef struct P5_FACTORS PJ_FACTORS;

/* Data type for projection/transformation information */
struct PJconsts;
typedef struct PJconsts PJ;         /* the PJ object herself */

/* Data type for library level information */
struct PJ_INFO;
typedef struct PJ_INFO PJ_INFO;

struct PJ_PROJ_INFO;
typedef struct PJ_PROJ_INFO PJ_PROJ_INFO;

struct PJ_GRID_INFO;
typedef struct PJ_GRID_INFO PJ_GRID_INFO;

struct PJ_INIT_INFO;
typedef struct PJ_INIT_INFO PJ_INIT_INFO;

/* Data types for list of operations, ellipsoids, datums and units used in PROJ.4 */
struct PJ_LIST;
typedef struct PJ_LIST PJ_OPERATIONS;

struct PJ_ELLPS;
typedef struct PJ_ELLPS PJ_ELLPS;

struct PJ_UNITS;
typedef struct PJ_UNITS PJ_UNITS;

struct PJ_PRIME_MERIDIANS;
typedef struct PJ_PRIME_MERIDIANS PJ_PRIME_MERIDIANS;


/* Geodetic, mostly spatiotemporal coordinate types */
typedef struct { double   x,   y,  z, t; }  PJ_XYZT;
typedef struct { double   u,   v,  w, t; }  PJ_UVWT;
typedef struct { double lam, phi,  z, t; }  PJ_LPZT;
typedef struct { double o, p, k; }          PJ_OPK;  /* Rotations: omega, phi, kappa */
typedef struct { double e, n, u; }          PJ_ENU;  /* East, North, Up */
typedef struct { double s, a1, a2; }        PJ_GEOD; /* Geodesic length, fwd azi, rev azi */

/* Classic proj.4 pair/triplet types - moved into the PJ_ name space */
typedef struct { double   u,   v; }  PJ_UV;
typedef struct { double   x,   y; }  PJ_XY;
typedef struct { double lam, phi; }  PJ_LP;

typedef struct { double   x,   y,  z; }  PJ_XYZ;
typedef struct { double   u,   v,  w; }  PJ_UVW;
typedef struct { double lam, phi,  z; }  PJ_LPZ;


/* Avoid preprocessor renaming and implicit type-punning: Use a union to make it explicit */
union PJ_COORD {
     double v[4];   /* First and foremost, it really is "just 4 numbers in a vector" */
    PJ_XYZT xyzt;
    PJ_UVWT uvwt;
    PJ_LPZT lpzt;
    PJ_GEOD geod;
     PJ_OPK opk;
     PJ_ENU enu;
     PJ_XYZ xyz;
     PJ_UVW uvw;
     PJ_LPZ lpz;
      PJ_XY xy;
      PJ_UV uv;
      PJ_LP lp;
};


struct PJ_INFO {
    int         major;              /* Major release number                 */
    int         minor;              /* Minor release number                 */
    int         patch;              /* Patch level                          */
    const char  *release;           /* Release info. Version + date         */
    const char  *version;           /* Full version number                  */
    const char  *searchpath;        /* Paths where init and grid files are  */
                                    /* looked for. Paths are separated by   */
                                    /* semi-colons.                         */
    const char * const *paths;
    size_t path_count;
};

struct PJ_PROJ_INFO {
    const char  *id;                /* Name of the projection in question                       */
    const char  *description;       /* Description of the projection                            */
    const char  *definition;        /* Projection definition                                    */
    int         has_inverse;        /* 1 if an inverse mapping exists, 0 otherwise              */
    double      accuracy;           /* Expected accuracy of the transformation. -1 if unknown.  */
};

struct PJ_GRID_INFO {
    char        gridname[32];       /* name of grid                         */
    char        filename[260];      /* full path to grid                    */
    char        format[8];          /* file format of grid                  */
    PJ_LP       lowerleft;          /* Coordinates of lower left corner     */
    PJ_LP       upperright;         /* Coordinates of upper right corner    */
    int         n_lon, n_lat;       /* Grid size                            */
    double      cs_lon, cs_lat;     /* Cell size of grid                    */
};

struct PJ_INIT_INFO {
    char        name[32];           /* name of init file                        */
    char        filename[260];      /* full path to the init file.              */
    char        version[32];        /* version of the init file                 */
    char        origin[32];         /* origin of the file, e.g. EPSG            */
    char        lastupdate[16];     /* Date of last update in YYYY-MM-DD format */
};

typedef enum PJ_LOG_LEVEL {
    PJ_LOG_NONE  = 0,
    PJ_LOG_ERROR = 1,
    PJ_LOG_DEBUG = 2,
    PJ_LOG_TRACE = 3,
    PJ_LOG_TELL  = 4,
    PJ_LOG_DEBUG_MAJOR = 2, /* for proj_api.h compatibility */
    PJ_LOG_DEBUG_MINOR = 3  /* for proj_api.h compatibility */
} PJ_LOG_LEVEL;

typedef void (*PJ_LOG_FUNCTION)(void *, int, const char *);


/* The context type - properly namespaced synonym for projCtx */
struct projCtx_t;
typedef struct projCtx_t PJ_CONTEXT;

/* A P I */


/* Functionality for handling thread contexts */
#define PJ_DEFAULT_CTX 0
PJ_CONTEXT *proj_context_create (void);
PJ_CONTEXT *proj_context_destroy (PJ_CONTEXT *ctx);


/* Manage the transformation definition object PJ */
PJ  *proj_create (PJ_CONTEXT *ctx, const char *definition);
PJ  *proj_create_argv (PJ_CONTEXT *ctx, int argc, char **argv);
PJ  *proj_create_crs_to_crs(PJ_CONTEXT *ctx, const char *srid_from, const char *srid_to, PJ_AREA *area);
PJ  *proj_destroy (PJ *P);

/* Setter-functions for the opaque PJ_AREA struct */
/* Uncomment these when implementing support for area-based transformations.
void proj_area_bbox(PJ_AREA *area, LP ll, LP ur);
void proj_area_description(PJ_AREA *area, const char *descr);
*/

/* Apply transformation to observation - in forward or inverse direction */
enum PJ_DIRECTION {
    PJ_FWD   =  1,   /* Forward    */
    PJ_IDENT =  0,   /* Do nothing */
    PJ_INV   = -1    /* Inverse    */
};
typedef enum PJ_DIRECTION PJ_DIRECTION;


int proj_angular_input (PJ *P, enum PJ_DIRECTION dir);
int proj_angular_output (PJ *P, enum PJ_DIRECTION dir);


PJ_COORD proj_trans (PJ *P, PJ_DIRECTION direction, PJ_COORD coord);
int proj_trans_array (PJ *P, PJ_DIRECTION direction, size_t n, PJ_COORD *coord);
size_t proj_trans_generic (
    PJ *P,
    PJ_DIRECTION direction,
    double *x, size_t sx, size_t nx,
    double *y, size_t sy, size_t ny,
    double *z, size_t sz, size_t nz,
    double *t, size_t st, size_t nt
);


/* Initializers */
PJ_COORD proj_coord (double x, double y, double z, double t);

/* Measure internal consistency - in forward or inverse direction */
double proj_roundtrip (PJ *P, PJ_DIRECTION direction, int n, PJ_COORD *coord);

/* Geodesic distance between two points with angular 2D coordinates */
double proj_lp_dist (const PJ *P, PJ_COORD a, PJ_COORD b);

/* The geodesic distance AND the vertical offset */
double proj_lpz_dist (const PJ *P, PJ_COORD a, PJ_COORD b);

/* Euclidean distance between two points with linear 2D coordinates */
double proj_xy_dist (PJ_COORD a, PJ_COORD b);

/* Euclidean distance between two points with linear 3D coordinates */
double proj_xyz_dist (PJ_COORD a, PJ_COORD b);

/* Geodesic distance (in meter) + fwd and rev azimuth between two points on the ellipsoid */
PJ_COORD proj_geod (const PJ *P, PJ_COORD a, PJ_COORD b);


/* Set or read error level */
int  proj_context_errno (PJ_CONTEXT *ctx);
int  proj_errno (const PJ *P);
int  proj_errno_set (const PJ *P, int err);
int  proj_errno_reset (const PJ *P);
int  proj_errno_restore (const PJ *P, int err);
const char* proj_errno_string (int err);

PJ_LOG_LEVEL proj_log_level (PJ_CONTEXT *ctx, PJ_LOG_LEVEL log_level);
void proj_log_func (PJ_CONTEXT *ctx, void *app_data, PJ_LOG_FUNCTION logf);

/* Scaling and angular distortion factors */
PJ_FACTORS proj_factors(PJ *P, PJ_COORD lp);

/* Info functions - get information about various PROJ.4 entities */
PJ_INFO proj_info(void);
PJ_PROJ_INFO proj_pj_info(PJ *P);
PJ_GRID_INFO proj_grid_info(const char *gridname);
PJ_INIT_INFO proj_init_info(const char *initname);

/* List functions: */
/* Get lists of operations, ellipsoids, units and prime meridians. */
const PJ_OPERATIONS       *proj_list_operations(void);
const PJ_ELLPS            *proj_list_ellps(void);
const PJ_UNITS            *proj_list_units(void);
const PJ_PRIME_MERIDIANS  *proj_list_prime_meridians(void);

/* These are trivial, and while occasionally useful in real code, primarily here to      */
/* simplify demo code, and in acknowledgement of the proj-internal discrepancy between   */
/* angular units expected by classical proj, and by Charles Karney's geodesics subsystem */
double proj_torad (double angle_in_degrees);
double proj_todeg (double angle_in_radians);

/* Geographical to geocentric latitude - another of the "simple, but useful" */
PJ_COORD proj_geocentric_latitude (const PJ *P, PJ_DIRECTION direction, PJ_COORD coord);

double proj_dmstor(const char *is, char **rs);
char*  proj_rtodms(char *s, double r, int pos, int neg);

#ifdef __cplusplus
}
#endif

#endif /* ndef PROJ_H */
