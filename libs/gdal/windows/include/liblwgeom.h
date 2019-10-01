/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2011 Sandro Santilli <strk@kbt.io>
 * Copyright 2011 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2007-2008 Mark Cave-Ayland
 * Copyright 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


#ifndef _LIBLWGEOM_H
#define _LIBLWGEOM_H 1

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include "proj_api.h"

#ifdef _MSC_VER
#ifdef BUILD_DLL
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif
#else
#define DLLEXPORT
#endif

#if defined(PJ_VERSION) && PJ_VERSION >= 490
/* Enable new geodesic functions */
#define PROJ_GEODESIC 1
#else
/* Use the old (pre-2.2) geodesic functions */
#define PROJ_GEODESIC 0
#endif

/**
* @file liblwgeom.h
*
* This library is the generic geometry handling section of PostGIS. The geometry
* objects, constructors, destructors, and a set of spatial processing functions,
* are implemented here.
*
* The library is designed for use in non-PostGIS applications if necessary. The
* units tests at cunit/cu_tester.c and the loader/dumper programs at
* ../loader/shp2pgsql.c are examples of non-PostGIS applications using liblwgeom.
*
* Programs using this library can install their custom memory managers and error
* handlers by calling the lwgeom_set_handlers() function, otherwise the default
* ones will be used.
*/

/**
 * liblwgeom versions
 */
#define LIBLWGEOM_VERSION "3"
#define LIBLWGEOM_VERSION_MAJOR "7"
#define LIBLWGEOM_VERSION_MINOR "0"
#define LIBLWGEOM_GEOS_VERSION "3.7.0"

/** Return lwgeom version string (not to be freed) */
DLLEXPORT const char* lwgeom_version(void);

/**
* Return types for functions with status returns.
*/
#define LW_TRUE 1
#define LW_FALSE 0
#define LW_UNKNOWN 2
#define LW_FAILURE 0
#define LW_SUCCESS 1

/**
* LWTYPE numbers, used internally by PostGIS
*/
#define	POINTTYPE                1
#define	LINETYPE                 2
#define	POLYGONTYPE              3
#define	MULTIPOINTTYPE           4
#define	MULTILINETYPE            5
#define	MULTIPOLYGONTYPE         6
#define	COLLECTIONTYPE           7
#define CIRCSTRINGTYPE           8
#define COMPOUNDTYPE             9
#define CURVEPOLYTYPE           10
#define MULTICURVETYPE          11
#define MULTISURFACETYPE        12
#define POLYHEDRALSURFACETYPE   13
#define TRIANGLETYPE            14
#define TINTYPE                 15

#define NUMTYPES                16

/**
* Flags applied in EWKB to indicate Z/M dimensions and
* presence/absence of SRID and bounding boxes
*/
#define WKBZOFFSET  0x80000000
#define WKBMOFFSET  0x40000000
#define WKBSRIDFLAG 0x20000000
#define WKBBBOXFLAG 0x10000000

/** Ordinate names */
typedef enum LWORD_T {
  LWORD_X = 0,
  LWORD_Y = 1,
  LWORD_Z = 2,
  LWORD_M = 3
} LWORD;

/**********************************************************************
** Spherical radius.
** Moritz, H. (1980). Geodetic Reference System 1980, by resolution of
** the XVII General Assembly of the IUGG in Canberra.
** http://en.wikipedia.org/wiki/Earth_radius
** http://en.wikipedia.org/wiki/World_Geodetic_System
*/

#define WGS84_MAJOR_AXIS 6378137.0
#define WGS84_INVERSE_FLATTENING 298.257223563
#define WGS84_MINOR_AXIS (WGS84_MAJOR_AXIS - WGS84_MAJOR_AXIS / WGS84_INVERSE_FLATTENING)
#define WGS84_RADIUS ((2.0 * WGS84_MAJOR_AXIS + WGS84_MINOR_AXIS ) / 3.0)


/**
* Macros for manipulating the 'flags' byte. A uint8_t used as follows:
* VVSRGBMZ
* Version bit, followed by
* Validty, Solid, ReadOnly, Geodetic, HasBBox, HasM and HasZ flags.
*/
#define FLAGS_GET_Z(flags) ((flags) & 0x01)
#define FLAGS_GET_M(flags) (((flags) & 0x02)>>1)
#define FLAGS_GET_BBOX(flags) (((flags) & 0x04)>>2)
#define FLAGS_GET_GEODETIC(flags) (((flags) & 0x08)>>3)
#define FLAGS_GET_READONLY(flags) (((flags) & 0x10)>>4)
#define FLAGS_GET_SOLID(flags) (((flags) & 0x20)>>5)
#define FLAGS_SET_Z(flags, value) ((flags) = (value) ? ((flags) | 0x01) : ((flags) & 0xFE))
#define FLAGS_SET_M(flags, value) ((flags) = (value) ? ((flags) | 0x02) : ((flags) & 0xFD))
#define FLAGS_SET_BBOX(flags, value) ((flags) = (value) ? ((flags) | 0x04) : ((flags) & 0xFB))
#define FLAGS_SET_GEODETIC(flags, value) ((flags) = (value) ? ((flags) | 0x08) : ((flags) & 0xF7))
#define FLAGS_SET_READONLY(flags, value) ((flags) = (value) ? ((flags) | 0x10) : ((flags) & 0xEF))
#define FLAGS_SET_SOLID(flags, value) ((flags) = (value) ? ((flags) | 0x20) : ((flags) & 0xDF))
#define FLAGS_NDIMS(flags) (2 + FLAGS_GET_Z(flags) + FLAGS_GET_M(flags))
#define FLAGS_GET_ZM(flags) (FLAGS_GET_M(flags) + FLAGS_GET_Z(flags) * 2)
#define FLAGS_NDIMS_BOX(flags) (FLAGS_GET_GEODETIC(flags) ? 3 : FLAGS_NDIMS(flags))

/**
* Macros for manipulating the 'typemod' int. An int32_t used as follows:
* Plus/minus = Top bit.
* Spare bits = Next 2 bits.
* SRID = Next 21 bits.
* TYPE = Next 6 bits.
* ZM Flags = Bottom 2 bits.
*/

#define TYPMOD_GET_SRID(typmod) ((((typmod) & 0x0FFFFF00) - ((typmod) & 0x10000000)) >> 8)
#define TYPMOD_SET_SRID(typmod, srid) ((typmod) = (((typmod) & 0xE00000FF) | ((srid & 0x001FFFFF)<<8)))
#define TYPMOD_GET_TYPE(typmod) ((typmod & 0x000000FC)>>2)
#define TYPMOD_SET_TYPE(typmod, type) ((typmod) = (typmod & 0xFFFFFF03) | ((type & 0x0000003F)<<2))
#define TYPMOD_GET_Z(typmod) ((typmod & 0x00000002)>>1)
#define TYPMOD_SET_Z(typmod) ((typmod) = typmod | 0x00000002)
#define TYPMOD_GET_M(typmod) (typmod & 0x00000001)
#define TYPMOD_SET_M(typmod) ((typmod) = typmod | 0x00000001)
#define TYPMOD_GET_NDIMS(typmod) (2+TYPMOD_GET_Z(typmod)+TYPMOD_GET_M(typmod))

/**
* Maximum allowed SRID value in serialized geometry.
* Currently we are using 21 bits (2097152) of storage for SRID.
*/
#define SRID_MAXIMUM 999999

/**
 * Maximum valid SRID value for the user
 * We reserve 1000 values for internal use
 */
#define SRID_USER_MAXIMUM 999999

/** Unknown SRID value */
#define SRID_UNKNOWN 0
#define SRID_IS_UNKNOWN(x) ((int)x<=0)

/* Invalid SRID value, for internal use */
#define SRID_INVALID (SRID_MAXIMUM + 2)

/*
** EPSG WGS84 geographics, OGC standard default SRS, better be in
** the SPATIAL_REF_SYS table!
*/
#define SRID_DEFAULT 4326

#ifndef __GNUC__
# define __attribute__(x)
#endif

/**
* Return a valid SRID from an arbitrary integer
* Raises a notice if what comes out is different from
* what went in.
* Raises an error if SRID value is out of bounds.
*/
extern DLLEXPORT int clamp_srid(int srid);

/* Raise an lwerror if srids do not match */
void error_if_srid_mismatch(int srid1, int srid2);

/**
 * Global functions for memory/logging handlers.
 */
typedef void* (*lwallocator)(size_t size);
typedef void* (*lwreallocator)(void *mem, size_t size);
typedef void (*lwfreeor)(void* mem);
typedef void (*lwreporter)(const char* fmt, va_list ap)
  __attribute__ (( format(printf, 1, 0) ));
typedef void (*lwdebuglogger)(int level, const char* fmt, va_list ap)
  __attribute__ (( format(printf, 2,0) ));

/**
* Install custom memory management and error handling functions you want your
* application to use.
* @ingroup system
* @todo take a structure ?
*/
extern DLLEXPORT void lwgeom_set_handlers(lwallocator allocator,
        lwreallocator reallocator, lwfreeor freeor, lwreporter errorreporter,
        lwreporter noticereporter);

extern DLLEXPORT void lwgeom_set_debuglogger(lwdebuglogger debuglogger);

/**
 * Request interruption of any running code
 *
 * Safe for use from signal handlers
 *
 * Interrupted code will (as soon as it finds out
 * to be interrupted) cleanup and return as soon as possible.
 *
 * The return value from interrupted code is undefined,
 * it is the caller responsibility to not take it in consideration.
 *
 */
extern void DLLEXPORT lwgeom_request_interrupt(void);

/**
 * Cancel any interruption request
 */
extern void lwgeom_cancel_interrupt(void);

/**
 * Install a callback to be called periodically during
 * algorithm execution. Mostly only needed on WIN32 to
 * dispatch queued signals.
 *
 * The callback is invoked before checking for interrupt
 * being requested, so you can request interruption from
 * the callback, if you want (see lwgeom_request_interrupt).
 *
 */
typedef void (lwinterrupt_callback)();
extern DLLEXPORT lwinterrupt_callback *lwgeom_register_interrupt_callback(lwinterrupt_callback *);

/******************************************************************/

typedef struct {
	double afac, bfac, cfac, dfac, efac, ffac, gfac, hfac, ifac, xoff, yoff, zoff;
} AFFINE;

/******************************************************************/

typedef struct
{
	double xmin, ymin, zmin;
	double xmax, ymax, zmax;
	int32_t srid;
}
BOX3D;

/******************************************************************
* GBOX structure.
* We include the flags (information about dimensionality),
* so we don't have to constantly pass them
* into functions that use the GBOX.
*/
typedef struct
{
	uint8_t flags;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	double zmin;
	double zmax;
	double mmin;
	double mmax;
} GBOX;


/******************************************************************
* SPHEROID
*
*  Standard definition of an ellipsoid (what wkt calls a spheroid)
*    f = (a-b)/a
*    e_sq = (a*a - b*b)/(a*a)
*    b = a - fa
*/
typedef struct
{
	double	a;	/* semimajor axis */
	double	b; 	/* semiminor axis b = (a - fa) */
	double	f;	/* flattening f = (a-b)/a */
	double	e;	/* eccentricity (first) */
	double	e_sq;	/* eccentricity squared (first) e_sq = (a*a-b*b)/(a*a) */
	double  radius;  /* spherical average radius = (2*a+b)/3 */
	char	name[20];  /* name of ellipse */
}
SPHEROID;

/******************************************************************
* POINT2D, POINT3D, POINT3DM, POINT4D
*/
typedef struct
{
	double x, y;
}
POINT2D;

typedef struct
{
	double x, y, z;
}
POINT3DZ;

typedef struct
{
	double x, y, z;
}
POINT3D;

typedef struct
{
	double x, y, m;
}
POINT3DM;

typedef struct
{
	double x, y, z, m;
}
POINT4D;

/******************************************************************
*  POINTARRAY
*  Point array abstracts a lot of the complexity of points and point lists.
*  It handles 2d/3d translation
*    (2d points converted to 3d will have z=0 or NaN)
*  DO NOT MIX 2D and 3D POINTS! EVERYTHING* is either one or the other
*/
typedef struct
{
	/* Array of POINT 2D, 3D or 4D, possibly misaligned. */
	uint8_t *serialized_pointlist;

	/* Use FLAGS_* macros to handle */
	uint8_t  flags;

	uint32_t npoints;   /* how many points we are currently storing */
	uint32_t maxpoints; /* how many points we have space for in serialized_pointlist */
}
POINTARRAY;

/******************************************************************
* GSERIALIZED
*/
typedef struct
{
	uint32_t size; /* For PgSQL use only, use VAR* macros to manipulate. */
	uint8_t srid[3]; /* 24 bits of SRID */
	uint8_t flags; /* HasZ, HasM, HasBBox, IsGeodetic, IsReadOnly */
	uint8_t data[1]; /* See gserialized.txt */
} GSERIALIZED;


/******************************************************************
* LWGEOM (any geometry type)
*
* Abstract type, note that 'type', 'bbox' and 'srid' are available in
* all geometry variants.
*/
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	void *data;
}
LWGEOM;

/* POINTYPE */
typedef struct
{
	uint8_t type; /* POINTTYPE */
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	POINTARRAY *point;  /* hide 2d/3d (this will be an array of 1 point) */
}
LWPOINT; /* "light-weight point" */

/* LINETYPE */
typedef struct
{
	uint8_t type; /* LINETYPE */
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	POINTARRAY *points; /* array of POINT3D */
}
LWLINE; /* "light-weight line" */

/* TRIANGLE */
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	POINTARRAY *points;
}
LWTRIANGLE;

/* CIRCSTRINGTYPE */
typedef struct
{
	uint8_t type; /* CIRCSTRINGTYPE */
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	POINTARRAY *points; /* array of POINT(3D/3DM) */
}
LWCIRCSTRING; /* "light-weight circularstring" */

/* POLYGONTYPE */
typedef struct
{
	uint8_t type; /* POLYGONTYPE */
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t nrings;   /* how many rings we are currently storing */
	uint32_t maxrings; /* how many rings we have space for in **rings */
	POINTARRAY **rings; /* list of rings (list of points) */
}
LWPOLY; /* "light-weight polygon" */

/* MULTIPOINTTYPE */
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t ngeoms;   /* how many geometries we are currently storing */
	uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
	LWPOINT **geoms;
}
LWMPOINT;

/* MULTILINETYPE */
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t ngeoms;   /* how many geometries we are currently storing */
	uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
	LWLINE **geoms;
}
LWMLINE;

/* MULTIPOLYGONTYPE */
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t ngeoms;   /* how many geometries we are currently storing */
	uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
	LWPOLY **geoms;
}
LWMPOLY;

/* COLLECTIONTYPE */
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t ngeoms;   /* how many geometries we are currently storing */
	uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
	LWGEOM **geoms;
}
LWCOLLECTION;

/* COMPOUNDTYPE */
typedef struct
{
	uint8_t type; /* COMPOUNDTYPE */
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t ngeoms;   /* how many geometries we are currently storing */
	uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
	LWGEOM **geoms;
}
LWCOMPOUND; /* "light-weight compound line" */

/* CURVEPOLYTYPE */
typedef struct
{
	uint8_t type; /* CURVEPOLYTYPE */
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t nrings;    /* how many rings we are currently storing */
	uint32_t maxrings;  /* how many rings we have space for in **rings */
	LWGEOM **rings; /* list of rings (list of points) */
}
LWCURVEPOLY; /* "light-weight polygon" */

/* MULTICURVE */
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t ngeoms;   /* how many geometries we are currently storing */
	uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
	LWGEOM **geoms;
}
LWMCURVE;

/* MULTISURFACETYPE */
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t ngeoms;   /* how many geometries we are currently storing */
	uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
	LWGEOM **geoms;
}
LWMSURFACE;

/* POLYHEDRALSURFACETYPE */
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t ngeoms;   /* how many geometries we are currently storing */
	uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
	LWPOLY **geoms;
}
LWPSURFACE;

/* TINTYPE */
typedef struct
{
	uint8_t type;
	uint8_t flags;
	GBOX *bbox;
	int32_t srid;
	uint32_t ngeoms;   /* how many geometries we are currently storing */
	uint32_t maxgeoms; /* how many geometries we have space for in **geoms */
	LWTRIANGLE **geoms;
}
LWTIN;

/* Casts LWGEOM->LW* (return NULL if cast is illegal) */
extern DLLEXPORT LWMPOLY *lwgeom_as_lwmpoly(const LWGEOM *lwgeom);
extern DLLEXPORT LWMLINE *lwgeom_as_lwmline(const LWGEOM *lwgeom);
extern DLLEXPORT LWMPOINT *lwgeom_as_lwmpoint(const LWGEOM *lwgeom);
extern DLLEXPORT LWCOLLECTION *lwgeom_as_lwcollection(const LWGEOM *lwgeom);
extern DLLEXPORT LWPOLY *lwgeom_as_lwpoly(const LWGEOM *lwgeom);
extern DLLEXPORT LWLINE *lwgeom_as_lwline(const LWGEOM *lwgeom);
extern DLLEXPORT LWPOINT *lwgeom_as_lwpoint(const LWGEOM *lwgeom);
extern DLLEXPORT LWCIRCSTRING *lwgeom_as_lwcircstring(const LWGEOM *lwgeom);
extern DLLEXPORT LWCURVEPOLY *lwgeom_as_lwcurvepoly(const LWGEOM *lwgeom);
extern DLLEXPORT LWCOMPOUND *lwgeom_as_lwcompound(const LWGEOM *lwgeom);
extern DLLEXPORT LWPSURFACE *lwgeom_as_lwpsurface(const LWGEOM *lwgeom);
extern DLLEXPORT LWTRIANGLE *lwgeom_as_lwtriangle(const LWGEOM *lwgeom);
extern DLLEXPORT LWTIN *lwgeom_as_lwtin(const LWGEOM *lwgeom);
extern DLLEXPORT LWGEOM *lwgeom_as_multi(const LWGEOM *lwgeom);
extern DLLEXPORT LWGEOM *lwgeom_as_curve(const LWGEOM *lwgeom);

/* Casts LW*->LWGEOM (always cast) */
extern DLLEXPORT LWGEOM *lwtin_as_lwgeom(const LWTIN *obj);
extern DLLEXPORT LWGEOM *lwtriangle_as_lwgeom(const LWTRIANGLE *obj);
extern DLLEXPORT LWGEOM *lwpsurface_as_lwgeom(const LWPSURFACE *obj);
extern DLLEXPORT LWGEOM *lwmpoly_as_lwgeom(const LWMPOLY *obj);
extern DLLEXPORT LWGEOM *lwmline_as_lwgeom(const LWMLINE *obj);
extern DLLEXPORT LWGEOM *lwmpoint_as_lwgeom(const LWMPOINT *obj);
extern DLLEXPORT LWGEOM *lwcollection_as_lwgeom(const LWCOLLECTION *obj);
extern DLLEXPORT LWGEOM *lwcircstring_as_lwgeom(const LWCIRCSTRING *obj);
extern DLLEXPORT LWGEOM *lwcompound_as_lwgeom(const LWCOMPOUND *obj);
extern DLLEXPORT LWGEOM *lwcurvepoly_as_lwgeom(const LWCURVEPOLY *obj);
extern DLLEXPORT LWGEOM *lwpoly_as_lwgeom(const LWPOLY *obj);
extern DLLEXPORT LWGEOM *lwline_as_lwgeom(const LWLINE *obj);
extern DLLEXPORT LWGEOM *lwpoint_as_lwgeom(const LWPOINT *obj);


extern DLLEXPORT LWCOLLECTION* lwcollection_add_lwgeom(LWCOLLECTION *col, const LWGEOM *geom);
extern DLLEXPORT LWMPOINT* lwmpoint_add_lwpoint(LWMPOINT *mobj, const LWPOINT *obj);
extern DLLEXPORT LWMLINE* lwmline_add_lwline(LWMLINE *mobj, const LWLINE *obj);
extern DLLEXPORT LWMPOLY* lwmpoly_add_lwpoly(LWMPOLY *mobj, const LWPOLY *obj);
extern DLLEXPORT LWPSURFACE* lwpsurface_add_lwpoly(LWPSURFACE *mobj, const LWPOLY *obj);
extern DLLEXPORT LWTIN* lwtin_add_lwtriangle(LWTIN *mobj, const LWTRIANGLE *obj);



/***********************************************************************
** Utility functions for flag byte and srid_flag integer.
*/

/**
* Construct a new flags char.
*/
extern DLLEXPORT uint8_t gflags(int hasz, int hasm, int geodetic);

/**
* Extract the geometry type from the serialized form (it hides in
* the anonymous data area, so this is a handy function).
*/
extern DLLEXPORT uint32_t gserialized_get_type(const GSERIALIZED *g);

/**
* Returns the size in bytes to read from toast to get the basic
* information from a geometry: GSERIALIZED struct, bbox and type
*/
extern DLLEXPORT uint32_t gserialized_max_header_size(void);

/**
* Returns the size in bytes of the header, from the start of the
* object up to the type number.
*/
extern DLLEXPORT uint32_t gserialized_header_size(const GSERIALIZED *gser);

/**
* Extract the SRID from the serialized form (it is packed into
* three bytes so this is a handy function).
*/
extern DLLEXPORT int32_t gserialized_get_srid(const GSERIALIZED *g);

/**
* Write the SRID into the serialized form (it is packed into
* three bytes so this is a handy function).
*/
extern DLLEXPORT void gserialized_set_srid(GSERIALIZED *g, int32_t srid);

/**
* Check if a #GSERIALIZED is empty without deserializing first.
* Only checks if the number of elements of the parent geometry
* is zero, will not catch collections of empty, eg:
* GEOMETRYCOLLECTION(POINT EMPTY)
*/
extern DLLEXPORT int gserialized_is_empty(const GSERIALIZED *g);

/**
* Check if a #GSERIALIZED has a bounding box without deserializing first.
*/
extern DLLEXPORT int gserialized_has_bbox(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED has a Z ordinate.
*/
extern DLLEXPORT int gserialized_has_z(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED has an M ordinate.
*/
extern DLLEXPORT int gserialized_has_m(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED is a geography.
*/
extern DLLEXPORT int gserialized_is_geodetic(const GSERIALIZED *gser);

/**
* Return a number indicating presence of Z and M coordinates.
* 0 = None, 1 = M, 2 = Z, 3 = ZM
*/
extern DLLEXPORT int gserialized_get_zm(const GSERIALIZED *gser);

/**
* Return the number of dimensions (2, 3, 4) in a geometry
*/
extern DLLEXPORT int gserialized_ndims(const GSERIALIZED *gser);

/**
* Return -1 if g1 is "less than" g2, 1 if g1 is "greater than"
* g2 and 0 if g1 and g2 are the "same". Equality is evaluated
* with a memcmp and size check. So it is possible that two
* identical objects where one lacks a bounding box could be
* evaluated as non-equal initially. Greater and less than
* are evaluated by calculating a sortable key from the center
* point of the object bounds.
*/
extern DLLEXPORT int gserialized_cmp(const GSERIALIZED *g1, const GSERIALIZED *g2);

/**
* Call this function to drop BBOX and SRID
* from LWGEOM. If LWGEOM type is *not* flagged
* with the HASBBOX flag and has a bbox, it
* will be released.
*/
extern DLLEXPORT void lwgeom_drop_bbox(LWGEOM *lwgeom);
extern DLLEXPORT void lwgeom_drop_srid(LWGEOM *lwgeom);

/**
 * Compute a bbox if not already computed
 *
 * After calling this function lwgeom->bbox is only
 * NULL if the geometry is empty.
 */
extern DLLEXPORT void lwgeom_add_bbox(LWGEOM *lwgeom);
/**
* Drop current bbox and calculate a fresh one.
*/
extern DLLEXPORT void lwgeom_refresh_bbox(LWGEOM *lwgeom);
/**
* Compute a box for geom and all sub-geometries, if not already computed
*/
extern DLLEXPORT void lwgeom_add_bbox_deep(LWGEOM *lwgeom, GBOX *gbox);

/**
 * Get a non-empty geometry bounding box, computing and
 * caching it if not already there
 *
 * NOTE: empty geometries don't have a bounding box so
 *       you'd still get a NULL for them.
 */
extern DLLEXPORT const GBOX *lwgeom_get_bbox(const LWGEOM *lwgeom);

/**
* Determine whether a LWGEOM can contain sub-geometries or not
*/
extern DLLEXPORT int lwgeom_is_collection(const LWGEOM *lwgeom);

/******************************************************************/
/* Functions that work on type numbers */

/**
* Determine whether a type number is a collection or not
*/
extern DLLEXPORT int lwtype_is_collection(uint8_t type);

/**
* Given an lwtype number, what homogeneous collection can hold it?
*/
extern DLLEXPORT uint32_t lwtype_get_collectiontype(uint8_t type);

/**
* Return the type name string associated with a type number
* (e.g. Point, LineString, Polygon)
*/
extern DLLEXPORT const char *lwtype_name(uint8_t type);
extern DLLEXPORT uint8_t lwtype_multitype(uint8_t type);

/******************************************************************/

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * will set point's m=0 (or NaN) if pa is 3d or 2d
 * NOTE: point is a real POINT3D *not* a pointer
 */
extern DLLEXPORT POINT4D getPoint4d(const POINTARRAY *pa, uint32_t n);

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * will set point's m=0 (or NaN) if pa is 3d or 2d
 * NOTE: this will modify the point4d pointed to by 'point'.
 */
extern DLLEXPORT int getPoint4d_p(const POINTARRAY *pa, uint32_t n, POINT4D *point);

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * NOTE: point is a real POINT3D *not* a pointer
 */
extern DLLEXPORT POINT3DZ getPoint3dz(const POINTARRAY *pa, uint32_t n);
extern DLLEXPORT POINT3DM getPoint3dm(const POINTARRAY *pa, uint32_t n);

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * NOTE: this will modify the point3d pointed to by 'point'.
 */
extern DLLEXPORT int getPoint3dz_p(const POINTARRAY *pa, uint32_t n, POINT3DZ *point);
extern DLLEXPORT int getPoint3dm_p(const POINTARRAY *pa, uint32_t n, POINT3DM *point);


/*
 * copies a point from the point array into the parameter point
 * z value (if present is not returned)
 * NOTE: point is a real POINT3D *not* a pointer
 */
extern DLLEXPORT POINT2D getPoint2d(const POINTARRAY *pa, uint32_t n);

/*
 * copies a point from the point array into the parameter point
 * z value (if present is not returned)
 * NOTE: this will modify the point2d pointed to by 'point'.
 */
extern DLLEXPORT int getPoint2d_p(const POINTARRAY *pa, uint32_t n, POINT2D *point);

/**
* Returns a POINT2D pointer into the POINTARRAY serialized_ptlist,
* suitable for reading from. This is very high performance
* and declared const because you aren't allowed to muck with the
* values, only read them.
*/
extern DLLEXPORT const POINT2D* getPoint2d_cp(const POINTARRAY *pa, uint32_t n);

/**
* Returns a POINT3DZ pointer into the POINTARRAY serialized_ptlist,
* suitable for reading from. This is very high performance
* and declared const because you aren't allowed to muck with the
* values, only read them.
*/
extern DLLEXPORT const POINT3DZ* getPoint3dz_cp(const POINTARRAY *pa, uint32_t n);

/**
* Returns a POINT4D pointer into the POINTARRAY serialized_ptlist,
* suitable for reading from. This is very high performance
* and declared const because you aren't allowed to muck with the
* values, only read them.
*/
extern DLLEXPORT const POINT4D* getPoint4d_cp(const POINTARRAY *pa, uint32_t n);

/*
 * set point N to the given value
 * NOTE that the pointarray can be of any
 * dimension, the appropriate ordinate values
 * will be extracted from it
 *
 * N must be a valid point index
 */
extern DLLEXPORT void ptarray_set_point4d(POINTARRAY *pa, uint32_t n, const POINT4D *p4d);

/*
 * get a pointer to nth point of a POINTARRAY
 * You'll need to cast it to appropriate dimensioned point.
 * Note that if you cast to a higher dimensional point you'll
 * possibly corrupt the POINTARRAY.
 *
 * WARNING: Don't cast this to a POINT !
 * it would not be reliable due to memory alignment constraints
 */
extern DLLEXPORT uint8_t *getPoint_internal(const POINTARRAY *pa, uint32_t n);

/*
 * size of point represeneted in the POINTARRAY
 * 16 for 2d, 24 for 3d, 32 for 4d
 */
extern DLLEXPORT size_t ptarray_point_size(const POINTARRAY *pa);


/**
* Construct an empty pointarray, allocating storage and setting
* the npoints, but not filling in any information. Should be used in conjunction
* with ptarray_set_point4d to fill in the information in the array.
*/
extern DLLEXPORT POINTARRAY* ptarray_construct(char hasz, char hasm, uint32_t npoints);

/**
* Construct a new #POINTARRAY, <em>copying</em> in the data from ptlist
*/
extern DLLEXPORT POINTARRAY* ptarray_construct_copy_data(char hasz, char hasm, uint32_t npoints, const uint8_t *ptlist);

/**
* Construct a new #POINTARRAY, <em>referencing</em> to the data from ptlist
*/
extern DLLEXPORT POINTARRAY* ptarray_construct_reference_data(char hasz, char hasm, uint32_t npoints, uint8_t *ptlist);

/**
* Create a new #POINTARRAY with no points. Allocate enough storage
* to hold maxpoints vertices before having to reallocate the storage
* area.
*/
extern DLLEXPORT POINTARRAY* ptarray_construct_empty(char hasz, char hasm, uint32_t maxpoints);

/**
* Append a point to the end of an existing #POINTARRAY
* If allow_duplicate is LW_FALSE, then a duplicate point will
* not be added.
*/
extern DLLEXPORT int ptarray_append_point(POINTARRAY *pa, const POINT4D *pt, int allow_duplicates);

/**
 * Append a #POINTARRAY, pa2 to the end of an existing #POINTARRAY, pa1.
 *
 * If gap_tolerance is >= 0 then the end point of pa1 will be checked for
 * being within gap_tolerance 2d distance from start point of pa2 or an
 * error will be raised and LW_FAILURE returned.
 * A gap_tolerance < 0 disables the check.
 *
 * If end point of pa1 and start point of pa2 are 2d-equal, then pa2 first
 * point will not be appended.
 */
extern DLLEXPORT int ptarray_append_ptarray(POINTARRAY *pa1, POINTARRAY *pa2, double gap_tolerance);

/**
* Insert a point into an existing #POINTARRAY. Zero
* is the index of the start of the array.
*/
extern DLLEXPORT int ptarray_insert_point(POINTARRAY *pa, const POINT4D *p, uint32_t where);

/**
* Remove a point from an existing #POINTARRAY. Zero
* is the index of the start of the array.
*/
extern DLLEXPORT int ptarray_remove_point(POINTARRAY *pa, uint32_t where);

/**
 * @brief Add a point in a pointarray.
 *
 * @param pa the source POINTARRAY
 * @param p the point to add
 * @param pdims number of ordinates in p (2..4)
 * @param where to insert the point. 0 prepends, pa->npoints appends
 *
 * @returns a newly constructed POINTARRAY using a newly allocated buffer
 *          for the actual points, or NULL on error.
 */
extern DLLEXPORT POINTARRAY *ptarray_addPoint(const POINTARRAY *pa, uint8_t *p, size_t pdims, uint32_t where);

/**
 * @brief Remove a point from a pointarray.
 * 	@param which -  is the offset (starting at 0)
 * @return #POINTARRAY is newly allocated
 */
extern DLLEXPORT POINTARRAY *ptarray_removePoint(POINTARRAY *pa, uint32_t where);

/**
 * @brief Merge two given POINTARRAY and returns a pointer
 * on the new aggregate one.
 * Warning: this function free the two inputs POINTARRAY
 * @return #POINTARRAY is newly allocated
 */
extern DLLEXPORT POINTARRAY *ptarray_merge(POINTARRAY *pa1, POINTARRAY *pa2);

extern DLLEXPORT int ptarray_is_closed(const POINTARRAY *pa);
extern DLLEXPORT int ptarray_is_closed_2d(const POINTARRAY *pa);
extern DLLEXPORT int ptarray_is_closed_3d(const POINTARRAY *pa);
extern DLLEXPORT int ptarray_is_closed_z(const POINTARRAY *pa);
extern DLLEXPORT POINTARRAY* ptarray_flip_coordinates(POINTARRAY *pa);

/**
 * @d1 start location (distance from start / total distance)
 * @d2   end location (distance from start / total distance)
 * @param tolerance snap to vertices at locations < tolerance away from given ones
 */
extern DLLEXPORT POINTARRAY *ptarray_substring(POINTARRAY *pa, double d1, double d2,
                                                          double tolerance);


/**
* Strip out the Z/M components of an #LWGEOM
*/
extern DLLEXPORT LWGEOM* lwgeom_force_2d(const LWGEOM *geom);
extern DLLEXPORT LWGEOM* lwgeom_force_3dz(const LWGEOM *geom);
extern DLLEXPORT LWGEOM* lwgeom_force_3dm(const LWGEOM *geom);
extern DLLEXPORT LWGEOM* lwgeom_force_4d(const LWGEOM *geom);

extern DLLEXPORT LWGEOM* lwgeom_set_effective_area(const LWGEOM *igeom, int set_area, double area);
extern DLLEXPORT LWGEOM* lwgeom_chaikin(const LWGEOM *igeom, int n_iterations, int preserve_endpoint);
extern DLLEXPORT LWGEOM* lwgeom_filter_m(LWGEOM *geom, double min, double max, int returnm);

/*
 * Force to use SFS 1.1 geometry type
 * (rather than SFS 1.2 and/or SQL/MM)
 */
extern DLLEXPORT LWGEOM* lwgeom_force_sfs(LWGEOM *geom, int version);


/*--------------------------------------------------------
 * all the base types (point/line/polygon) will have a
 * basic constructor, basic de-serializer, basic serializer,
 * bounding box finder and (TODO) serialized form size finder.
 *--------------------------------------------------------*/

/*
 * convenience functions to hide the POINTARRAY
 */
extern DLLEXPORT int lwpoint_getPoint2d_p(const LWPOINT *point, POINT2D *out);
extern DLLEXPORT int lwpoint_getPoint3dz_p(const LWPOINT *point, POINT3DZ *out);
extern DLLEXPORT int lwpoint_getPoint3dm_p(const LWPOINT *point, POINT3DM *out);
extern DLLEXPORT int lwpoint_getPoint4d_p(const LWPOINT *point, POINT4D *out);

/******************************************************************
 * LWLINE functions
 ******************************************************************/

/**
 * Add a LWPOINT to an LWLINE
 */
extern DLLEXPORT int lwline_add_lwpoint(LWLINE *line, LWPOINT *point, uint32_t where);

/**
 * Interpolate one or more points along a line
 */
extern DLLEXPORT POINTARRAY* lwline_interpolate_points(const LWLINE *line, double length_fraction, char repeat);

/******************************************************************
 * LWPOLY functions
 ******************************************************************/

/**
* Add a ring, allocating extra space if necessary. The polygon takes
* ownership of the passed point array.
*/
extern DLLEXPORT int lwpoly_add_ring(LWPOLY *poly, POINTARRAY *pa);

/**
* Add a ring, allocating extra space if necessary. The curvepolygon takes
* ownership of the passed point array.
*/
extern DLLEXPORT int lwcurvepoly_add_ring(LWCURVEPOLY *poly, LWGEOM *ring);

/**
* Add a component, allocating extra space if necessary. The compoundcurve
* takes owership of the passed geometry.
*/
extern DLLEXPORT int lwcompound_add_lwgeom(LWCOMPOUND *comp, LWGEOM *geom);

/**
* Construct an equivalent compound curve from a linestring.
* Compound curves can have linear components, so this works fine
*/
extern DLLEXPORT LWCOMPOUND* lwcompound_construct_from_lwline(const LWLINE *lwpoly);

/**
* Construct an equivalent curve polygon from a polygon. Curve polygons
* can have linear rings as their rings, so this works fine (in theory?)
*/
DLLEXPORT extern LWCURVEPOLY* lwcurvepoly_construct_from_lwpoly(LWPOLY *lwpoly);


/******************************************************************
 * LWGEOM functions
 ******************************************************************/

extern DLLEXPORT int lwcollection_ngeoms(const LWCOLLECTION *col);

/* Given a generic geometry/collection, return the "simplest" form.
 * The elements of the homogenized collection are references to the
 * input geometry; a deep clone is not performed.
 * TODO: consider returning a geometry that does not reference the
 * input
 * */
extern DLLEXPORT LWGEOM *lwgeom_homogenize(const LWGEOM *geom);


/******************************************************************
 * LWMULTIx and LWCOLLECTION functions
 ******************************************************************/

DLLEXPORT LWGEOM *lwcollection_getsubgeom(LWCOLLECTION *col, int gnum);

/* WARNING: the output will contain references to geometries in the input, */
/* so the result must be carefully released, not freed. */
DLLEXPORT LWCOLLECTION* lwcollection_extract(LWCOLLECTION *col, int type);


/******************************************************************
 * SERIALIZED FORM functions
 ******************************************************************/

/**
* Set the SRID on an LWGEOM
* For collections, only the parent gets an SRID, all
* the children get SRID_UNKNOWN.
*/
extern DLLEXPORT void lwgeom_set_srid(LWGEOM *geom, int srid);

/*------------------------------------------------------
 * other stuff
 *
 * handle the double-to-float conversion.  The results of this
 * will usually be a slightly bigger box because of the difference
 * between float8 and float4 representations.
 */

extern DLLEXPORT BOX3D* box3d_from_gbox(const GBOX *gbox);
extern DLLEXPORT GBOX* box3d_to_gbox(const BOX3D *b3d);

DLLEXPORT void expand_box3d(BOX3D *box, double d);


/****************************************************************
 * MEMORY MANAGEMENT
 ****************************************************************/

/*
* The *_free family of functions frees *all* memory associated
* with the pointer. When the recursion gets to the level of the
* POINTARRAY, the POINTARRAY is only freed if it is not flagged
* as "read only". LWGEOMs constructed on top of GSERIALIZED
* from PgSQL use read only point arrays.
*/

extern DLLEXPORT void ptarray_free(POINTARRAY *pa);
extern DLLEXPORT void lwpoint_free(LWPOINT *pt);
extern DLLEXPORT void lwline_free(LWLINE *line);
extern DLLEXPORT void lwpoly_free(LWPOLY *poly);
extern DLLEXPORT void lwtriangle_free(LWTRIANGLE *triangle);
extern DLLEXPORT void lwmpoint_free(LWMPOINT *mpt);
extern DLLEXPORT void lwmline_free(LWMLINE *mline);
extern DLLEXPORT void lwmpoly_free(LWMPOLY *mpoly);
extern DLLEXPORT void lwpsurface_free(LWPSURFACE *psurf);
extern DLLEXPORT void lwtin_free(LWTIN *tin);
extern DLLEXPORT void lwcollection_free(LWCOLLECTION *col);
extern DLLEXPORT void lwcircstring_free(LWCIRCSTRING *curve);
extern DLLEXPORT void lwgeom_free(LWGEOM *geom);

/*
* The *_release family of functions frees the LWGEOM structures
* surrounding the POINTARRAYs but leaves the POINTARRAYs
* intact. Useful when re-shaping geometries between types,
* or splicing geometries together.
*/

extern DLLEXPORT void lwpoint_release(LWPOINT *lwpoint);
extern DLLEXPORT void lwline_release(LWLINE *lwline);
extern DLLEXPORT void lwpoly_release(LWPOLY *lwpoly);
extern DLLEXPORT void lwtriangle_release(LWTRIANGLE *lwtriangle);
extern DLLEXPORT void lwcircstring_release(LWCIRCSTRING *lwcirc);
extern DLLEXPORT void lwmpoint_release(LWMPOINT *lwpoint);
extern DLLEXPORT void lwmline_release(LWMLINE *lwline);
extern DLLEXPORT void lwmpoly_release(LWMPOLY *lwpoly);
extern DLLEXPORT void lwpsurface_release(LWPSURFACE *lwpsurface);
extern DLLEXPORT void lwtin_release(LWTIN *lwtin);
extern DLLEXPORT void lwcollection_release(LWCOLLECTION *lwcollection);
extern DLLEXPORT void lwgeom_release(LWGEOM *lwgeom);


/****************************************************************
* Utility
****************************************************************/

extern DLLEXPORT void printBOX3D(BOX3D *b);
extern DLLEXPORT void printPA(POINTARRAY *pa);
extern DLLEXPORT void printLWPOINT(LWPOINT *point);
extern DLLEXPORT void printLWLINE(LWLINE *line);
extern DLLEXPORT void printLWPOLY(LWPOLY *poly);
extern DLLEXPORT void printLWTRIANGLE(LWTRIANGLE *triangle);
extern DLLEXPORT void printLWPSURFACE(LWPSURFACE *psurf);
extern DLLEXPORT void printLWTIN(LWTIN *tin);

extern DLLEXPORT float  next_float_down(double d);
extern DLLEXPORT float  next_float_up(double d);

/* general utilities 2D */
extern DLLEXPORT double  distance2d_pt_pt(const POINT2D *p1, const POINT2D *p2);
extern DLLEXPORT double  distance2d_sqr_pt_pt(const POINT2D *p1, const POINT2D *p2);
extern DLLEXPORT double  distance2d_pt_seg(const POINT2D *p, const POINT2D *A, const POINT2D *B);
extern DLLEXPORT double  distance2d_sqr_pt_seg(const POINT2D *p, const POINT2D *A, const POINT2D *B);
extern DLLEXPORT LWGEOM* lwgeom_closest_line(const LWGEOM *lw1, const LWGEOM *lw2);
extern DLLEXPORT LWGEOM* lwgeom_furthest_line(const LWGEOM *lw1, const LWGEOM *lw2);
extern DLLEXPORT LWGEOM* lwgeom_closest_point(const LWGEOM *lw1, const LWGEOM *lw2);
extern DLLEXPORT LWGEOM* lwgeom_furthest_point(const LWGEOM *lw1, const LWGEOM *lw2);
extern DLLEXPORT double  lwgeom_mindistance2d(const LWGEOM *lw1, const LWGEOM *lw2);
extern DLLEXPORT double  lwgeom_mindistance2d_tolerance(const LWGEOM *lw1, const LWGEOM *lw2, double tolerance);
extern DLLEXPORT double  lwgeom_maxdistance2d(const LWGEOM *lw1, const LWGEOM *lw2);
extern DLLEXPORT double  lwgeom_maxdistance2d_tolerance(const LWGEOM *lw1, const LWGEOM *lw2, double tolerance);

/* 3D */
extern DLLEXPORT double distance3d_pt_pt(const POINT3D *p1, const POINT3D *p2);
extern DLLEXPORT double distance3d_pt_seg(const POINT3D *p, const POINT3D *A, const POINT3D *B);

extern DLLEXPORT LWGEOM* lwgeom_furthest_line_3d(LWGEOM *lw1, LWGEOM *lw2);
extern DLLEXPORT LWGEOM* lwgeom_closest_line_3d(const LWGEOM *lw1, const LWGEOM *lw2);
extern DLLEXPORT LWGEOM* lwgeom_closest_point_3d(const LWGEOM *lw1, const LWGEOM *lw2);


extern DLLEXPORT double lwgeom_mindistance3d(const LWGEOM *lw1, const LWGEOM *lw2);
extern DLLEXPORT double lwgeom_mindistance3d_tolerance(const LWGEOM *lw1, const LWGEOM *lw2, double tolerance);
extern DLLEXPORT double lwgeom_maxdistance3d(const LWGEOM *lw1, const LWGEOM *lw2);
extern DLLEXPORT double lwgeom_maxdistance3d_tolerance(const LWGEOM *lw1, const LWGEOM *lw2, double tolerance);

extern DLLEXPORT double lwgeom_area(const LWGEOM *geom);
extern DLLEXPORT double lwgeom_length(const LWGEOM *geom);
extern DLLEXPORT double lwgeom_length_2d(const LWGEOM *geom);
extern DLLEXPORT double lwgeom_perimeter(const LWGEOM *geom);
extern DLLEXPORT double lwgeom_perimeter_2d(const LWGEOM *geom);
extern DLLEXPORT int lwgeom_dimension(const LWGEOM *geom);

extern DLLEXPORT LWPOINT* lwline_get_lwpoint(const LWLINE *line, uint32_t where);
extern DLLEXPORT LWPOINT* lwcircstring_get_lwpoint(const LWCIRCSTRING *circ, uint32_t where);

extern DLLEXPORT LWPOINT* lwcompound_get_startpoint(const LWCOMPOUND *lwcmp);
extern DLLEXPORT LWPOINT* lwcompound_get_endpoint(const LWCOMPOUND *lwcmp);
extern DLLEXPORT LWPOINT* lwcompound_get_lwpoint(const LWCOMPOUND *lwcmp, uint32_t where);

extern DLLEXPORT double ptarray_length_2d(const POINTARRAY *pts);
extern DLLEXPORT int pt_in_ring_2d(const POINT2D *p, const POINTARRAY *ring);
extern DLLEXPORT int azimuth_pt_pt(const POINT2D *p1, const POINT2D *p2, double *ret);
extern DLLEXPORT int lwpoint_inside_circle(const LWPOINT *p, double cx, double cy, double rad);

extern DLLEXPORT LWGEOM* lwgeom_reverse(const LWGEOM *lwgeom);
extern DLLEXPORT char* lwgeom_summary(const LWGEOM *lwgeom, int offset);
extern DLLEXPORT char* lwpoint_to_latlon(const LWPOINT *p, const char *format);
extern DLLEXPORT int lwgeom_startpoint(const LWGEOM* lwgeom, POINT4D* pt);

extern DLLEXPORT void interpolate_point4d(const POINT4D *A, const POINT4D *B, POINT4D *I, double F);

/**
* Ensure the outer ring is clockwise oriented and all inner rings
* are counter-clockwise.
*/
extern DLLEXPORT int lwgeom_is_clockwise(LWGEOM *lwgeom);


extern DLLEXPORT LWGEOM* lwgeom_simplify(const LWGEOM *igeom, double dist, int preserve_collapsed);
extern DLLEXPORT LWGEOM* lwgeom_remove_repeated_points(const LWGEOM *in, double tolerance);

/****************************************************************
* READ/WRITE FUNCTIONS
*
* Coordinate writing functions, which will alter the coordinates
* and potentially the structure of the input geometry. When
* called from within PostGIS, the LWGEOM argument should be built
* on top of a gserialized copy, created using
* PG_GETARG_GSERIALIZED_P_COPY()
****************************************************************/

extern DLLEXPORT void lwgeom_reverse_in_place(LWGEOM *lwgeom);
extern DLLEXPORT void lwgeom_force_clockwise(LWGEOM *lwgeom);
extern DLLEXPORT void lwgeom_longitude_shift(LWGEOM *lwgeom);
extern DLLEXPORT void lwgeom_simplify_in_place(LWGEOM *igeom, double dist, int preserve_collapsed);
extern DLLEXPORT void lwgeom_affine(LWGEOM *geom, const AFFINE *affine);
extern DLLEXPORT void lwgeom_scale(LWGEOM *geom, const POINT4D *factors);
extern DLLEXPORT void lwgeom_remove_repeated_points_in_place(LWGEOM *in, double tolerance);


/**
 * @brief wrap geometry on given cut x value
 *
 * For a positive amount, shifts anything that is on the left
 * of "cutx" to the right by the given amount.
 *
 * For a negative amount, shifts anything that is on the right
 * of "cutx" to the left by the given absolute amount.
 *
 * @param cutx the X value to perform wrapping on
 * @param amount shift amount and wrapping direction
 */
LWGEOM DLLEXPORT *lwgeom_wrapx(const LWGEOM *lwgeom, double cutx, double amount);


/**
* @brief Check whether or not a lwgeom is big enough to warrant a bounding box.
*
* Check whether or not a lwgeom is big enough to warrant a bounding box
* when stored in the serialized form on disk. Currently only points are
* considered small enough to not require a bounding box, because the
* index operations can generate a large number of box-retrieval operations
* when scanning keys.
*/
extern DLLEXPORT int lwgeom_needs_bbox(const LWGEOM *geom);

/**
* Count the total number of vertices in any #LWGEOM.
*/
extern DLLEXPORT uint32_t lwgeom_count_vertices(const LWGEOM *geom);

/**
* Count the total number of rings in any #LWGEOM. Multipolygons
* and other collections get counted, not the same as OGC st_numrings.
*/
extern DLLEXPORT uint32_t lwgeom_count_rings(const LWGEOM *geom);

/**
* Return true or false depending on whether a geometry has
* a valid SRID set.
*/
extern DLLEXPORT int lwgeom_has_srid(const LWGEOM *geom);

/**
* Return true or false depending on whether a geometry is an "empty"
* geometry (no vertices members)
*/
extern DLLEXPORT int lwgeom_is_empty(const LWGEOM *geom);

/**
* Return true or false depending on whether a geometry is a linear
* feature that closes on itself.
*/
extern DLLEXPORT int lwgeom_is_closed(const LWGEOM *geom);

/**
* Return the dimensionality (relating to point/line/poly) of an lwgeom
*/
extern DLLEXPORT int lwgeom_dimensionality(const LWGEOM *geom);

/* Is lwgeom1 geometrically equal to lwgeom2 ? */
extern DLLEXPORT char lwgeom_same(const LWGEOM *lwgeom1, const LWGEOM *lwgeom2);


/**
 * @brief Clone LWGEOM object. Serialized point lists are not copied.
 *
 * #GBOX are copied
 *
 * @see ptarray_clone
 */
extern DLLEXPORT LWGEOM *lwgeom_clone(const LWGEOM *lwgeom);

/**
* Deep clone an LWGEOM, everything is copied
*/
extern DLLEXPORT LWGEOM *lwgeom_clone_deep(const LWGEOM *lwgeom);
extern DLLEXPORT POINTARRAY *ptarray_clone_deep(const POINTARRAY *ptarray);


/*
* Geometry constructors. These constructors to not copy the point arrays
* passed to them, they just take references, so do not free them out
* from underneath the geometries.
*/
extern DLLEXPORT LWPOINT* lwpoint_construct(int srid, GBOX *bbox, POINTARRAY *point);
extern DLLEXPORT LWMPOINT *lwmpoint_construct(int srid, const POINTARRAY *pa);
extern DLLEXPORT LWLINE* lwline_construct(int srid, GBOX *bbox, POINTARRAY *points);
extern DLLEXPORT LWCIRCSTRING* lwcircstring_construct(int srid, GBOX *bbox, POINTARRAY *points);
extern DLLEXPORT LWPOLY* lwpoly_construct(int srid, GBOX *bbox, uint32_t nrings, POINTARRAY **points);
extern DLLEXPORT LWCURVEPOLY* lwcurvepoly_construct(int srid, GBOX *bbox, uint32_t nrings, LWGEOM **geoms);
extern DLLEXPORT LWTRIANGLE* lwtriangle_construct(int srid, GBOX *bbox, POINTARRAY *points);
extern DLLEXPORT LWCOLLECTION* lwcollection_construct(uint8_t type, int srid, GBOX *bbox, uint32_t ngeoms, LWGEOM **geoms);
/*
* Empty geometry constructors.
*/
extern DLLEXPORT LWGEOM* lwgeom_construct_empty(uint8_t type, int srid, char hasz, char hasm);
extern DLLEXPORT LWPOINT* lwpoint_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWLINE* lwline_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWPOLY* lwpoly_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWCURVEPOLY* lwcurvepoly_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWCIRCSTRING* lwcircstring_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWCOMPOUND* lwcompound_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWTRIANGLE* lwtriangle_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWMPOINT* lwmpoint_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWMLINE* lwmline_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWMPOLY* lwmpoly_construct_empty(int srid, char hasz, char hasm);
extern DLLEXPORT LWCOLLECTION* lwcollection_construct_empty(uint8_t type, int srid, char hasz, char hasm);


/* Other constructors */
extern DLLEXPORT LWPOINT *lwpoint_make2d(int srid, double x, double y);
extern DLLEXPORT LWPOINT *lwpoint_make3dz(int srid, double x, double y, double z);
extern DLLEXPORT LWPOINT *lwpoint_make3dm(int srid, double x, double y, double m);
extern DLLEXPORT LWPOINT *lwpoint_make4d(int srid, double x, double y, double z, double m);
extern DLLEXPORT LWPOINT *lwpoint_make(int srid, int hasz, int hasm, const POINT4D *p);
extern DLLEXPORT LWLINE *lwline_from_lwgeom_array(int srid, uint32_t ngeoms, LWGEOM **geoms);
extern DLLEXPORT LWLINE *lwline_from_ptarray(int srid, uint32_t npoints, LWPOINT **points); /* TODO: deprecate */
extern DLLEXPORT LWLINE *lwline_from_lwmpoint(int srid, const LWMPOINT *mpoint);
extern DLLEXPORT LWLINE *lwline_addpoint(LWLINE *line, LWPOINT *point, uint32_t where);
extern DLLEXPORT LWLINE *lwline_removepoint(LWLINE *line, uint32_t which);
extern DLLEXPORT void lwline_setPoint4d(LWLINE *line, uint32_t which, POINT4D *newpoint);
extern DLLEXPORT LWPOLY *lwpoly_from_lwlines(const LWLINE *shell, uint32_t nholes, const LWLINE **holes);
extern DLLEXPORT LWPOLY *lwpoly_construct_rectangle(char hasz, char hasm, POINT4D *p1, POINT4D *p2, POINT4D *p3, POINT4D *p4);
extern DLLEXPORT LWPOLY *lwpoly_construct_envelope(int srid, double x1, double y1, double x2, double y2);
extern DLLEXPORT LWPOLY *lwpoly_construct_circle(int srid, double x, double y, double radius, uint32_t segments_per_quarter, char exterior);
extern DLLEXPORT LWTRIANGLE *lwtriangle_from_lwline(const LWLINE *shell);
extern DLLEXPORT LWMPOINT *lwmpoint_from_lwgeom(const LWGEOM *g); /* Extract the coordinates of an LWGEOM into an LWMPOINT */

/* Some point accessors */
extern DLLEXPORT double lwpoint_get_x(const LWPOINT *point);
extern DLLEXPORT double lwpoint_get_y(const LWPOINT *point);
extern DLLEXPORT double lwpoint_get_z(const LWPOINT *point);
extern DLLEXPORT double lwpoint_get_m(const LWPOINT *point);

/**
* Return SRID number
*/
extern DLLEXPORT int32_t lwgeom_get_srid(const LWGEOM *geom);

/**
* Return LWTYPE number
*/
extern DLLEXPORT uint32_t lwgeom_get_type(const LWGEOM *geom);

/**
* Return #LW_TRUE if geometry has Z ordinates
*/
extern DLLEXPORT int lwgeom_has_z(const LWGEOM *geom);

/**
* Return #LW_TRUE if geometry has M ordinates.
*/
extern DLLEXPORT int lwgeom_has_m(const LWGEOM *geom);

/**
* Return the number of dimensions (2, 3, 4) in a geometry
*/
extern DLLEXPORT int lwgeom_ndims(const LWGEOM *geom);

/*
 * Given a point, returns the location of closest point on pointarray
 * as a fraction of total length (0: first point -- 1: last point).
 *
 * If not-null, the third argument will be set to the actual distance
 * of the point from the pointarray.
 */
extern DLLEXPORT double ptarray_locate_point(const POINTARRAY *pa, const POINT4D *pt, double *dist, POINT4D *p_located);

/**
* Add a measure dimension to a line, interpolating linearly from the start
* to the end value.
*/
extern DLLEXPORT LWLINE *lwline_measured_from_lwline(const LWLINE *lwline, double m_start, double m_end);
extern DLLEXPORT LWMLINE* lwmline_measured_from_lwmline(const LWMLINE *lwmline, double m_start, double m_end);

/**
* Determine the location(s) along a measured line where m occurs and
* return as a multipoint. Offset to left (positive) or right (negative).
*/
extern DLLEXPORT LWGEOM* lwgeom_locate_along(const LWGEOM *lwin, double m, double offset);

/**
* Determine the segments along a measured line that fall within the m-range
* given. Return as a multiline or geometrycollection.
* Offset to left (positive) or right (negative).
*/
extern DLLEXPORT LWCOLLECTION* lwgeom_locate_between(const LWGEOM *lwin, double from, double to, double offset);

/**
* Find the measure value at the location on the line closest to the point.
*/
extern DLLEXPORT double lwgeom_interpolate_point(const LWGEOM *lwin, const LWPOINT *lwpt);

/**
* Find the time of closest point of approach
*
* @param mindist if not null will be set to the minimum distance between
*                the trajectories at the closest point of approach.
*
* @return the time value in which the minimum distance was reached, -1
*         if inputs are invalid (lwerror is called in that case),
*         -2 if the trajectories do not share any point in time.
*/
extern DLLEXPORT double lwgeom_tcpa(const LWGEOM *g1, const LWGEOM *g2, double *mindist);

/**
* Is the closest point of approach within a distance ?
*
* @return LW_TRUE or LW_FALSE
*/
extern DLLEXPORT int lwgeom_cpa_within(const LWGEOM *g1, const LWGEOM *g2, double maxdist);

/**
* Return LW_TRUE or LW_FALSE depending on whether or not a geometry is
* a linestring with measure value growing from start to end vertex
*/
extern DLLEXPORT int lwgeom_is_trajectory(const LWGEOM *geom);
extern DLLEXPORT int lwline_is_trajectory(const LWLINE *geom);

/*
 * Ensure every segment is at most 'dist' long.
 * Returned LWGEOM might is unchanged if a POINT.
 */
extern DLLEXPORT LWGEOM *lwgeom_segmentize2d(const LWGEOM *line, double dist);
extern DLLEXPORT POINTARRAY *ptarray_segmentize2d(const POINTARRAY *ipa, double dist);
extern DLLEXPORT LWLINE *lwline_segmentize2d(const LWLINE *line, double dist);
extern DLLEXPORT LWPOLY *lwpoly_segmentize2d(const LWPOLY *line, double dist);
extern DLLEXPORT LWCOLLECTION *lwcollection_segmentize2d(const LWCOLLECTION *coll, double dist);

/*
 * Point density functions
 */
extern DLLEXPORT LWMPOINT *lwpoly_to_points(const LWPOLY *poly, uint32_t npoints);
extern DLLEXPORT LWMPOINT *lwmpoly_to_points(const LWMPOLY *mpoly, uint32_t npoints);
extern DLLEXPORT LWMPOINT *lwgeom_to_points(const LWGEOM *lwgeom, uint32_t npoints);

/*
 * Geometric median
 */
extern DLLEXPORT LWPOINT* lwgeom_median(const LWGEOM *g, double tol, uint32_t maxiter, char fail_if_not_converged);
extern DLLEXPORT LWPOINT* lwmpoint_median(const LWMPOINT *g, double tol, uint32_t maxiter, char fail_if_not_converged);

/**
* Calculate the GeoHash (http://geohash.org) string for a geometry. Caller must free.
*/
DLLEXPORT char *lwgeom_geohash(const LWGEOM *lwgeom, int precision);
DLLEXPORT unsigned int geohash_point_as_int(POINT2D *pt);


/**
* The return values of lwline_crossing_direction()
*/
enum CG_LINE_CROSS_TYPE {
    LINE_NO_CROSS = 0,
    LINE_CROSS_LEFT = -1,
    LINE_CROSS_RIGHT = 1,
    LINE_MULTICROSS_END_LEFT = -2,
    LINE_MULTICROSS_END_RIGHT = 2,
    LINE_MULTICROSS_END_SAME_FIRST_LEFT = -3,
    LINE_MULTICROSS_END_SAME_FIRST_RIGHT = 3
};

/**
* Given two lines, characterize how (and if) they cross each other
*/
DLLEXPORT int lwline_crossing_direction(const LWLINE *l1, const LWLINE *l2);

/**
* Given a geometry clip  based on the from/to range of one of its ordinates (x, y, z, m). Use for m- and z- clipping.
*/
DLLEXPORT LWCOLLECTION* lwgeom_clip_to_ordinate_range(const LWGEOM *lwin, char ordinate, double from, double to, double offset);

/**
 * Macros for specifying GML options.
 * @{
 */
/** For GML3 only, include srsDimension attribute in output */
#define LW_GML_IS_DIMS     (1<<0)
/** For GML3 only, declare that datas are lat/lon. Swaps axis order */
#define LW_GML_IS_DEGREE   (1<<1)
/** For GML3, use <LineString> rather than <Curve> for lines */
#define LW_GML_SHORTLINE   (1<<2)
/** For GML2 and GML3, output only extent of geometry */
#define LW_GML_EXTENT      (1<<4)


#define IS_DIMS(x) ((x) & LW_GML_IS_DIMS)
#define IS_DEGREE(x) ((x) & LW_GML_IS_DEGREE)
/** @} */

/**
 * Macros for specifying X3D options.
 * @{
 */
/** For flip X/Y coordinates to Y/X */
#define LW_X3D_FLIP_XY     (1<<0)
#define LW_X3D_USE_GEOCOORDS     (1<<1)
#define X3D_USE_GEOCOORDS(x) ((x) & LW_X3D_USE_GEOCOORDS)



extern DLLEXPORT char* lwgeom_to_gml2(const LWGEOM *geom, const char *srs, int precision, const char *prefix);
extern DLLEXPORT char* lwgeom_extent_to_gml2(const LWGEOM *geom, const char *srs, int precision, const char *prefix);
/**
 * @param opts output options bitfield, see LW_GML macros for meaning
 */
extern DLLEXPORT char* lwgeom_extent_to_gml3(const LWGEOM *geom, const char *srs, int precision, int opts, const char *prefix);
extern DLLEXPORT char* lwgeom_to_gml3(const LWGEOM *geom, const char *srs, int precision, int opts, const char *prefix, const char *id);
extern DLLEXPORT char* lwgeom_to_kml2(const LWGEOM *geom, int precision, const char *prefix);
extern DLLEXPORT char* lwgeom_to_geojson(const LWGEOM *geo, char *srs, int precision, int has_bbox);
extern DLLEXPORT char* lwgeom_to_svg(const LWGEOM *geom, int precision, int relative);
extern DLLEXPORT char* lwgeom_to_x3d3(const LWGEOM *geom, char *srs, int precision, int opts, const char *defid);
extern DLLEXPORT char* lwgeom_to_encoded_polyline(const LWGEOM *geom, int precision);

/**
 * Create an LWGEOM object from a GeoJSON representation
 *
 * @param geojson the GeoJSON input
 * @param srs output parameter. Will be set to a newly allocated
 *            string holding the spatial reference string, or NULL
 *            if no such parameter is found in input.
 *            If not null, the pointer must be freed with lwfree.
 */
extern DLLEXPORT LWGEOM* lwgeom_from_geojson(const char *geojson, char **srs);

/**
 * Create an LWGEOM object from an Encoded Polyline representation
 *
 * @param encodedpolyline the Encoded Polyline input
 */
extern DLLEXPORT LWGEOM* lwgeom_from_encoded_polyline(const char *encodedpolyline, int precision);

/**
* Initialize a spheroid object for use in geodetic functions.
*/
extern DLLEXPORT void spheroid_init(SPHEROID *s, double a, double b);

/**
* Calculate the geodetic distance from lwgeom1 to lwgeom2 on the spheroid.
* A spheroid with major axis == minor axis will be treated as a sphere.
* Pass in a tolerance in spheroid units.
*/
extern DLLEXPORT double lwgeom_distance_spheroid(const LWGEOM *lwgeom1, const LWGEOM *lwgeom2, const SPHEROID *spheroid, double tolerance);

/**
* Calculate the location of a point on a spheroid, give a start point, bearing and distance.
*/
extern DLLEXPORT LWPOINT* lwgeom_project_spheroid(const LWPOINT *r, const SPHEROID *spheroid, double distance, double azimuth);

/**
* Derive a new geometry with vertices added to ensure no vertex is more
* than max_seg_length (in radians) from any other vertex.
*/
extern DLLEXPORT LWGEOM* lwgeom_segmentize_sphere(const LWGEOM *lwg_in, double max_seg_length);

/**
* Calculate the bearing between two points on a spheroid.
*/
extern DLLEXPORT double lwgeom_azumith_spheroid(const LWPOINT *r, const LWPOINT *s, const SPHEROID *spheroid);

/**
* Calculate the geodetic area of a lwgeom on the sphere. The result
* will be multiplied by the average radius of the supplied spheroid.
*/
extern DLLEXPORT double lwgeom_area_sphere(const LWGEOM *lwgeom, const SPHEROID *spheroid);

/**
* Calculate the geodetic area of a lwgeom on the spheroid. The result
* will have the squared units of the spheroid axes.
*/
extern DLLEXPORT double lwgeom_area_spheroid(const LWGEOM *lwgeom, const SPHEROID *spheroid);

/**
* Calculate the geodetic length of a lwgeom on the unit sphere. The result
* will have to by multiplied by the real radius to get the real length.
*/
extern DLLEXPORT double lwgeom_length_spheroid(const LWGEOM *geom, const SPHEROID *s);

/**
* Calculate covers predicate for two lwgeoms on the sphere. Currently
* only handles point-in-polygon.
*/
extern DLLEXPORT int lwgeom_covers_lwgeom_sphere(const LWGEOM *lwgeom1, const LWGEOM *lwgeom2);

typedef struct {
	POINT2D* center;
	double radius;
} LWBOUNDINGCIRCLE;

extern void DLLEXPORT lwboundingcircle_destroy(LWBOUNDINGCIRCLE* c);

/* Calculates the minimum circle that encloses all of the points in g, using a
 * two-dimensional implementation of the algorithm proposed in:
 *
 * Welzl, Emo (1991), "Smallest enclosing disks (balls and elipsoids)."
 * New Results and Trends in Computer Science (H. Maurer, Ed.), Lecture Notes
 * in Computer Science, 555 (1991) 359-370.
 *
 * Available online at the time of this writing at
 * https://www.inf.ethz.ch/personal/emo/PublFiles/SmallEnclDisk_LNCS555_91.pdf
 *
 * Returns NULL if the circle could not be calculated.
 */
extern DLLEXPORT LWBOUNDINGCIRCLE* lwgeom_calculate_mbc(const LWGEOM* g);

/**
 * Swap ordinate values in every vertex of the geometry.
 *
 * Ordinates to swap are specified using an index with meaning:
 * 0=x, 1=y, 2=z, 3=m
 *
 * Swapping an existing ordinate with an unexisting one results
 * in undefined value being written in the existing ordinate.
 * Caller should verify and prevent such calls.
 *
 * Availability: 2.2.0
 */
extern void DLLEXPORT lwgeom_swap_ordinates(LWGEOM *in, LWORD o1, LWORD o2);


struct LWPOINTITERATOR;
typedef struct LWPOINTITERATOR LWPOINTITERATOR;

/**
 * Create a new LWPOINTITERATOR over supplied LWGEOM*
 */
extern DLLEXPORT LWPOINTITERATOR* lwpointiterator_create(const LWGEOM* g);

/**
 * Create a new LWPOINTITERATOR over supplied LWGEOM*
 * Supports modification of coordinates during iteration.
 */
extern DLLEXPORT LWPOINTITERATOR* lwpointiterator_create_rw(LWGEOM* g);

/**
 * Free all memory associated with the iterator
 */
extern DLLEXPORT void lwpointiterator_destroy(LWPOINTITERATOR* s);

/**
 * Returns LW_TRUE if there is another point available in the iterator.
 */
extern DLLEXPORT int lwpointiterator_has_next(LWPOINTITERATOR* s);

/**
 * Attempts to replace the next point int the iterator with p, and advances
 * the iterator to the next point.
 * Returns LW_SUCCESS if the assignment was successful, LW_FAILURE otherwise.
 * */
extern DLLEXPORT int lwpointiterator_modify_next(LWPOINTITERATOR* s, const POINT4D* p);

/**
 * Attempts to assign the next point in the iterator to p, and advances
 * the iterator to the next point.  If p is NULL, the iterator will be
 * advanced without reading a point.
 * Returns LW_SUCCESS if the assignment was successful, LW_FAILURE otherwise.
 * */
extern DLLEXPORT int lwpointiterator_next(LWPOINTITERATOR* s, POINT4D* p);

/**
 * Attempts to assigns the next point in the iterator to p.  Does not advance.
 * Returns LW_SUCCESS if the assignment was successful, LW_FAILURE otherwise.
 */
extern DLLEXPORT int lwpointiterator_peek(LWPOINTITERATOR* s, POINT4D* p);


/**
* Convert a single hex digit into the corresponding char
*/
extern DLLEXPORT uint8_t parse_hex(char *str);

/**
* Convert a char into a human readable hex digit
*/
extern DLLEXPORT void deparse_hex(uint8_t str, char *result);



/***********************************************************************
** Functions for managing serialized forms and bounding boxes.
*/

/**
* Calculate the geocentric bounding box directly from the serialized
* form of the geodetic coordinates. Only accepts serialized geographies
* flagged as geodetic. Caller is responsible for disposing of the GBOX.
*/
extern DLLEXPORT GBOX* gserialized_calculate_gbox_geocentric(const GSERIALIZED *g);

/**
* Calculate the geocentric bounding box directly from the serialized
* form of the geodetic coordinates. Only accepts serialized geographies
* flagged as geodetic.
*/
int DLLEXPORT gserialized_calculate_gbox_geocentric_p(const GSERIALIZED *g, GBOX *g_box);

/**
* Return a WKT representation of the gserialized geometry.
* Caller is responsible for disposing of the char*.
*/
extern DLLEXPORT char* gserialized_to_string(const GSERIALIZED *g);

/**
* Return a copy of the input serialized geometry.
*/
extern DLLEXPORT GSERIALIZED* gserialized_copy(const GSERIALIZED *g);

/**
* Check that coordinates of LWGEOM are all within the geodetic range (-180, -90, 180, 90)
*/
extern DLLEXPORT int lwgeom_check_geodetic(const LWGEOM *geom);

/**
* Gently move coordinates of LWGEOM if they are close enough into geodetic range.
*/
extern DLLEXPORT int lwgeom_nudge_geodetic(LWGEOM *geom);

/**
* Force coordinates of LWGEOM into geodetic range (-180, -90, 180, 90)
*/
extern DLLEXPORT int lwgeom_force_geodetic(LWGEOM *geom);

/**
* Set the FLAGS geodetic bit on geometry an all sub-geometries and pointlists
*/
extern DLLEXPORT void lwgeom_set_geodetic(LWGEOM *geom, int value);

/**
* Calculate the geodetic bounding box for an LWGEOM. Z/M coordinates are
* ignored for this calculation. Pass in non-null, geodetic bounding box for function
* to fill out. LWGEOM must have been built from a GSERIALIZED to provide
* double aligned point arrays.
*/
extern DLLEXPORT int lwgeom_calculate_gbox_geodetic(const LWGEOM *geom, GBOX *gbox);

/**
* Calculate the 2-4D bounding box of a geometry. Z/M coordinates are honored
* for this calculation, though for curves they are not included in calculations
* of curvature.
*/
extern DLLEXPORT int lwgeom_calculate_gbox_cartesian(const LWGEOM *lwgeom, GBOX *gbox);

/**
* Calculate bounding box of a geometry, automatically taking into account
* whether it is cartesian or geodetic.
*/
extern DLLEXPORT int lwgeom_calculate_gbox(const LWGEOM *lwgeom, GBOX *gbox);

/**
* New function to read doubles directly from the double* coordinate array
* of an aligned lwgeom #POINTARRAY (built by de-serializing a #GSERIALIZED).
*/
extern DLLEXPORT int getPoint2d_p_ro(const POINTARRAY *pa, uint32_t n, POINT2D **point);

/**
* Calculate geodetic (x/y/z) box and add values to gbox. Return #LW_SUCCESS on success.
*/
extern DLLEXPORT int ptarray_calculate_gbox_geodetic(const POINTARRAY *pa, GBOX *gbox);

/**
* Calculate box (x/y) and add values to gbox. Return #LW_SUCCESS on success.
*/
extern DLLEXPORT int ptarray_calculate_gbox_cartesian(const POINTARRAY *pa, GBOX *gbox );

/**
* Calculate a spherical point that falls outside the geocentric gbox
*/
DLLEXPORT void gbox_pt_outside(const GBOX *gbox, POINT2D *pt_outside);

/**
* Create a new gbox with the dimensionality indicated by the flags. Caller
* is responsible for freeing.
*/
extern DLLEXPORT GBOX* gbox_new(uint8_t flags);

/**
* Zero out all the entries in the #GBOX. Useful for cleaning
* statically allocated gboxes.
*/
extern DLLEXPORT void gbox_init(GBOX *gbox);

/**
* Update the merged #GBOX to be large enough to include itself and the new box.
*/
extern DLLEXPORT int gbox_merge(const GBOX *new_box, GBOX *merged_box);

/**
* Update the output #GBOX to be large enough to include both inputs.
*/
extern DLLEXPORT int gbox_union(const GBOX *g1, const GBOX *g2, GBOX *gout);

/**
* Move the box minimums down and the maximums up by the distance provided.
*/
extern DLLEXPORT void gbox_expand(GBOX *g, double d);

/**
* Move the box minimums down and the maximums up by the distances provided.
*/
extern DLLEXPORT void gbox_expand_xyzm(GBOX *g, double dx, double dy, double dz, double dm);

/**
* Initialize a #GBOX using the values of the point.
*/
extern DLLEXPORT int gbox_init_point3d(const POINT3D *p, GBOX *gbox);

/**
* Update the #GBOX to be large enough to include itself and the new point.
*/
extern DLLEXPORT int gbox_merge_point3d(const POINT3D *p, GBOX *gbox);

/**
* Return true if the point is inside the gbox
*/
extern DLLEXPORT int gbox_contains_point3d(const GBOX *gbox, const POINT3D *pt);

/**
* Allocate a string representation of the #GBOX, based on dimensionality of flags.
*/
extern DLLEXPORT char* gbox_to_string(const GBOX *gbox);

/**
* Return a copy of the #GBOX, based on dimensionality of flags.
*/
extern DLLEXPORT GBOX* gbox_copy(const GBOX *gbox);

/**
* Warning, do not use this function, it is very particular about inputs.
*/
extern DLLEXPORT GBOX* gbox_from_string(const char *str);

/**
* Return #LW_TRUE if the #GBOX overlaps, #LW_FALSE otherwise.
*/
extern DLLEXPORT int gbox_overlaps(const GBOX *g1, const GBOX *g2);

/**
* Return #LW_TRUE if the #GBOX overlaps on the 2d plane, #LW_FALSE otherwise.
*/
extern DLLEXPORT int gbox_overlaps_2d(const GBOX *g1, const GBOX *g2);

/**
* Return #LW_TRUE if the first #GBOX contains the second on the 2d plane, #LW_FALSE otherwise.
*/
extern DLLEXPORT int  gbox_contains_2d(const GBOX *g1, const GBOX *g2);

/**
* Copy the values of original #GBOX into duplicate.
*/
extern DLLEXPORT void gbox_duplicate(const GBOX *original, GBOX *duplicate);

/**
* Return the number of bytes necessary to hold a #GBOX of this dimension in
* serialized form.
*/
extern DLLEXPORT size_t gbox_serialized_size(uint8_t flags);

/**
* Check if 2 given Gbox are the same
*/
extern DLLEXPORT int gbox_same(const GBOX *g1, const GBOX *g2);

/**
* Check if 2 given GBOX are the same in x and y
*/
extern DLLEXPORT int gbox_same_2d(const GBOX *g1, const GBOX *g2);

/**
* Check if two given GBOX are the same in x and y, or would round to the same
* GBOX in x and if serialized in GSERIALIZED
*/
extern DLLEXPORT int gbox_same_2d_float(const GBOX *g1, const GBOX *g2);

/**
 * Round given GBOX to float boundaries
 *
 * This turns a GBOX into the version it would become
 * after a serialize/deserialize round trip.
 */
extern DLLEXPORT void gbox_float_round(GBOX *gbox);

/**
* Return false if any of the dimensions is NaN or infinite
*/
extern DLLEXPORT int gbox_is_valid(const GBOX *gbox);

/**
* Utility function to get type number from string. For example, a string 'POINTZ'
* would return type of 1 and z of 1 and m of 0. Valid
*/
extern DLLEXPORT int geometry_type_from_string(const char *str, uint8_t *type, int *z, int *m);

/**
* Calculate required memory segment to contain a serialized form of the LWGEOM.
* Primarily used internally by the serialization code. Exposed to allow the cunit
* tests to exercise it.
*/
extern DLLEXPORT size_t gserialized_from_lwgeom_size(const LWGEOM *geom);

/**
* Allocate a new #GSERIALIZED from an #LWGEOM. For all non-point types, a bounding
* box will be calculated and embedded in the serialization. The geodetic flag is used
* to control the box calculation (cartesian or geocentric). If set, the size pointer
* will contain the size of the final output, which is useful for setting the PgSQL
* VARSIZE information.
*/
extern DLLEXPORT GSERIALIZED* gserialized_from_lwgeom(LWGEOM *geom, size_t *size);

/**
* Allocate a new #LWGEOM from a #GSERIALIZED. The resulting #LWGEOM will have coordinates
* that are double aligned and suitable for direct reading using getPoint2d_p_ro
*/
extern DLLEXPORT LWGEOM* lwgeom_from_gserialized(const GSERIALIZED *g);

/**
* Pull a #GBOX from the header of a #GSERIALIZED, if one is available. If
* it is not, calculate it from the geometry. If that doesn't work (null
* or empty) return LW_FAILURE.
*/
extern DLLEXPORT int gserialized_get_gbox_p(const GSERIALIZED *g, GBOX *gbox);


/**
 * Parser check flags
 *
 *  @see lwgeom_from_wkb
 *  @see lwgeom_from_hexwkb
 *  @see lwgeom_parse_wkt
 */
#define LW_PARSER_CHECK_MINPOINTS  1
#define LW_PARSER_CHECK_ODD        2
#define LW_PARSER_CHECK_CLOSURE    4
#define LW_PARSER_CHECK_ZCLOSURE   8

#define LW_PARSER_CHECK_NONE   0
#define LW_PARSER_CHECK_ALL	(LW_PARSER_CHECK_MINPOINTS | LW_PARSER_CHECK_ODD | LW_PARSER_CHECK_CLOSURE)

/**
 * Parser result structure: returns the result of attempting to convert
 * (E)WKT/(E)WKB to LWGEOM
 */
typedef struct struct_lwgeom_parser_result
{
	const char *wkinput;        /* Copy of pointer to input WKT/WKB */
	uint8_t *serialized_lwgeom; /* Pointer to serialized LWGEOM */
	size_t size;                /* Size of serialized LWGEOM in bytes */
	LWGEOM *geom;               /* Pointer to LWGEOM struct */
	const char *message;        /* Error/warning message */
	int errcode;                /* Error/warning number */
	int errlocation;            /* Location of error */
	int parser_check_flags;     /* Bitmask of validity checks run during this parse */
}
LWGEOM_PARSER_RESULT;

/*
 * Parser error messages (these must match the message array in lwgparse.c)
 */
#define PARSER_ERROR_MOREPOINTS     1
#define PARSER_ERROR_ODDPOINTS      2
#define PARSER_ERROR_UNCLOSED       3
#define PARSER_ERROR_MIXDIMS        4
#define PARSER_ERROR_INVALIDGEOM    5
#define PARSER_ERROR_INVALIDWKBTYPE 6
#define PARSER_ERROR_INCONTINUOUS   7
#define PARSER_ERROR_TRIANGLEPOINTS 8
#define PARSER_ERROR_LESSPOINTS     9
#define PARSER_ERROR_OTHER          10



/*
 * Unparser result structure: returns the result of attempting to convert LWGEOM to (E)WKT/(E)WKB
 */
typedef struct struct_lwgeom_unparser_result
{
	uint8_t *serialized_lwgeom;	/* Copy of pointer to input serialized LWGEOM */
	char *wkoutput;			/* Pointer to WKT or WKB output */
	size_t size;			/* Size of serialized LWGEOM in bytes */
	const char *message;		/* Error/warning message */
	int errlocation;		/* Location of error */
}
LWGEOM_UNPARSER_RESULT;

/*
 * Unparser error messages (these must match the message array in lwgunparse.c)
 */
#define UNPARSER_ERROR_MOREPOINTS 	1
#define UNPARSER_ERROR_ODDPOINTS	2
#define UNPARSER_ERROR_UNCLOSED		3


/*
** Variants available for WKB and WKT output types
*/

#define WKB_ISO 0x01
#define WKB_SFSQL 0x02
#define WKB_EXTENDED 0x04
#define WKB_NDR 0x08
#define WKB_XDR 0x10
#define WKB_HEX 0x20
#define WKB_NO_NPOINTS 0x40 /* Internal use only */
#define WKB_NO_SRID 0x80 /* Internal use only */

#define WKT_ISO 0x01
#define WKT_SFSQL 0x02
#define WKT_EXTENDED 0x04


/*
** Variants available for TWKB
*/
#define TWKB_BBOX 0x01 /* User wants bboxes */
#define TWKB_SIZE 0x02 /* User wants sizes */
#define TWKB_ID 0x04 /* User wants id */
#define TWKB_NO_TYPE 0x10 /* No type because it is a sub geoemtry */
#define TWKB_NO_ID 0x20 /* No ID because it is a subgeoemtry */
#define TWKB_DEFAULT_PRECISION 0 /* Aim for 1m (or ft) rounding by default */

/*
** New parsing and unparsing functions.
*/

/**
* @param lwgeom geometry to convert to WKT
* @param variant output format to use (WKT_ISO, WKT_SFSQL, WKT_EXTENDED)
*/
extern DLLEXPORT char*   lwgeom_to_wkt(const LWGEOM *geom, uint8_t variant, int precision, size_t *size_out);

/**
* @param lwgeom geometry to convert to WKT
* @param variant output format to use
*                (WKB_ISO, WKB_SFSQL, WKB_EXTENDED, WKB_NDR, WKB_XDR)
*/
extern DLLEXPORT uint8_t*  lwgeom_to_wkb(const LWGEOM *geom, uint8_t variant, size_t *size_out);

/**
* @param lwgeom geometry to convert to HEXWKB
* @param variant output format to use
*                (WKB_ISO, WKB_SFSQL, WKB_EXTENDED, WKB_NDR, WKB_XDR)
*/
extern DLLEXPORT char*   lwgeom_to_hexwkb(const LWGEOM *geom, uint8_t variant, size_t *size_out);

/**
* @param lwgeom geometry to convert to EWKT
*/
extern DLLEXPORT char *lwgeom_to_ewkt(const LWGEOM *lwgeom);

/**
 * @param check parser check flags, see LW_PARSER_CHECK_* macros
 * @param size length of WKB byte buffer
 * @param wkb WKB byte buffer
 */
extern DLLEXPORT LWGEOM* lwgeom_from_wkb(const uint8_t *wkb, const size_t wkb_size, const char check);

/**
 * @param wkt WKT string
 * @param check parser check flags, see LW_PARSER_CHECK_* macros
 */
extern DLLEXPORT LWGEOM* lwgeom_from_wkt(const char *wkt, const char check);

/**
 * @param check parser check flags, see LW_PARSER_CHECK_* macros
 */
extern DLLEXPORT LWGEOM* lwgeom_from_hexwkb(const char *hexwkb, const char check);

extern DLLEXPORT uint8_t*  bytes_from_hexbytes(const char *hexbuf, size_t hexsize);

extern DLLEXPORT char*   hexbytes_from_bytes(const uint8_t *bytes, size_t size);

/*
* WKT detailed parsing support
*/
extern DLLEXPORT int lwgeom_parse_wkt(LWGEOM_PARSER_RESULT *parser_result, char *wktstr, int parse_flags);
DLLEXPORT void lwgeom_parser_result_init(LWGEOM_PARSER_RESULT *parser_result);
DLLEXPORT void lwgeom_parser_result_free(LWGEOM_PARSER_RESULT *parser_result);


/* Memory management */
extern DLLEXPORT void *lwalloc(size_t size);
extern DLLEXPORT void *lwrealloc(void *mem, size_t size);
extern DLLEXPORT void lwfree(void *mem);

/* Utilities */
extern DLLEXPORT char *lwmessage_truncate(char *str, int startpos, int endpos, int maxlength, int truncdirection);

/*
* TWKB functions
*/

/**
 * @param check parser check flags, see LW_PARSER_CHECK_* macros
 * @param size parser check flags, see LW_PARSER_CHECK_* macros
 */
extern DLLEXPORT LWGEOM* lwgeom_from_twkb(const uint8_t *twkb, size_t twkb_size, char check);

/**
 * @param geom input geometry
 * @param variant what variations on TWKB are requested?
 * @param twkb_size returns the length of the output TWKB in bytes if set
 */
extern DLLEXPORT uint8_t* lwgeom_to_twkb(const LWGEOM *geom, uint8_t variant, int8_t precision_xy, int8_t precision_z, int8_t precision_m, size_t *twkb_size);

extern DLLEXPORT uint8_t* lwgeom_to_twkb_with_idlist(const LWGEOM *geom, int64_t *idlist, uint8_t variant, int8_t precision_xy, int8_t precision_z, int8_t precision_m, size_t *twkb_size);

/**
 * Trim the bits of an LWGEOM in place, to optimize it for compression.
 * Sets all bits to zero that are not required to maintain a specified
 * number of digits after the decimal point. Negative precision values
 * indicate digits before the decimal point do not need to be preserved.
 *
 * @param geom input geometry
 * @param prec_x precision (digits after decimal point) in x dimension
 * @param prec_y precision (digits after decimal point) in y dimension
 * @param prec_z precision (digits after decimal point) in z dimension
 * @param prec_m precision (digits after decimal point) in m dimension
 */
extern DLLEXPORT void lwgeom_trim_bits_in_place(LWGEOM *geom, int32_t prec_x, int32_t prec_y, int32_t prec_z, int32_t prec_m);

/*******************************************************************************
 * SQLMM internal functions
 ******************************************************************************/

int lwgeom_has_arc(const LWGEOM *geom);
LWGEOM *lwgeom_stroke(const LWGEOM *geom, uint32_t perQuad);
LWGEOM *lwgeom_unstroke(const LWGEOM *geom);

/**
 * Semantic of the `tolerance` argument passed to
 * lwcurve_linearize
 */
typedef enum {
	/**
	 * Tolerance expresses the number of segments to use
	 * for each quarter of circle (quadrant). Must be
	 * an integer.
	 */
	LW_LINEARIZE_TOLERANCE_TYPE_SEGS_PER_QUAD = 0,
	/**
	 * Tolerance expresses the maximum distance between
	 * an arbitrary point on the curve and the closest
	 * point to it on the resulting approximation, in
	 * cartesian units.
	 */
	LW_LINEARIZE_TOLERANCE_TYPE_MAX_DEVIATION = 1,
	/**
	 * Tolerance expresses the maximum angle between
	 * the radii generating approximation line vertices,
	 * given in radiuses. A value of 1 would result
	 * in an approximation of a semicircle composed by
	 * 180 segments
	 */
	LW_LINEARIZE_TOLERANCE_TYPE_MAX_ANGLE = 2
} LW_LINEARIZE_TOLERANCE_TYPE;

typedef enum {
  /**
   * Symmetric linearization means that the output
   * vertices would be the same no matter the order
   * of the points defining the input curve.
   */
	LW_LINEARIZE_FLAG_SYMMETRIC = 1 << 0,

  /**
   * Retain angle instructs the engine to try its best
   * to retain the requested angle between generating
   * radii (where angle can be given explicitly with
   * LW_LINEARIZE_TOLERANCE_TYPE_MAX_ANGLE or implicitly
   * with LW_LINEARIZE_TOLERANCE_TYPE_SEGS_PER_QUAD or
   * LW_LINEARIZE_TOLERANCE_TYPE_MAX_DEVIATION).
   *
   * It only makes sense with LW_LINEARIZE_FLAG_SYMMETRIC
   * which would otherwise reduce the angle as needed to
   * keep it constant among all radiis so that all
   * segments are of the same length.
   *
   * When this flag is set, the first and last generating
   * angles (and thus the first and last segments) may
   * instead be smaller (shorter) than the others.
   *
   */
	LW_LINEARIZE_FLAG_RETAIN_ANGLE = 1 << 1
} LW_LINEARIZE_FLAGS;

/**
 * @param geom input geometry
 * @param tol tolerance, semantic driven by tolerance_type
 * @param tolerance_type see LW_LINEARIZE_TOLERANCE_TYPE
 * @param flags bitwise OR of operational flags, see LW_LINEARIZE_FLAGS
 *
 * @return a newly allocated LWGEOM
 */
extern DLLEXPORT LWGEOM* lwcurve_linearize(const LWGEOM *geom, double tol, LW_LINEARIZE_TOLERANCE_TYPE type, int flags);

/*******************************************************************************
 * GEOS proxy functions on LWGEOM
 ******************************************************************************/

/** Return GEOS version string (not to be freed) */
DLLEXPORT const char* lwgeom_geos_version(void);

/** Convert an LWGEOM to a GEOS Geometry and convert back -- for debug only */
DLLEXPORT LWGEOM* lwgeom_geos_noop(const LWGEOM *geom) ;

DLLEXPORT LWGEOM *lwgeom_normalize(const LWGEOM *geom);
DLLEXPORT LWGEOM *lwgeom_intersection(const LWGEOM *geom1, const LWGEOM *geom2);
DLLEXPORT LWGEOM *lwgeom_difference(const LWGEOM *geom1, const LWGEOM *geom2);
DLLEXPORT LWGEOM *lwgeom_symdifference(const LWGEOM* geom1, const LWGEOM* geom2);
DLLEXPORT LWGEOM *lwgeom_pointonsurface(const LWGEOM* geom);
DLLEXPORT LWGEOM *lwgeom_centroid(const LWGEOM* geom);
DLLEXPORT LWGEOM *lwgeom_union(const LWGEOM *geom1, const LWGEOM *geom2);
DLLEXPORT LWGEOM *lwgeom_linemerge(const LWGEOM *geom1);
DLLEXPORT LWGEOM *lwgeom_unaryunion(const LWGEOM *geom1);
DLLEXPORT LWGEOM *lwgeom_clip_by_rect(const LWGEOM *geom1, double x0, double y0, double x1, double y1);
DLLEXPORT LWCOLLECTION *lwgeom_subdivide(const LWGEOM *geom, uint32_t maxvertices);

/**
 * Snap vertices and segments of a geometry to another using a given tolerance.
 *
 * @param geom1 the geometry to snap
 * @param geom2 the geometry to snap to
 * @param tolerance the distance under which vertices and segments are snapped
 *
 * Requires GEOS-3.3.0+
 */
DLLEXPORT LWGEOM* lwgeom_snap(const LWGEOM* geom1, const LWGEOM* geom2, double tolerance);

/*
 * Return the set of paths shared between two linear geometries,
 * and their direction (same or opposite).
 *
 * @param geom1 a lineal geometry
 * @param geom2 another lineal geometry
 *
 * Requires GEOS-3.3.0+
 */
DLLEXPORT LWGEOM* lwgeom_sharedpaths(const LWGEOM* geom1, const LWGEOM* geom2);

/*
 * An offset curve against the input line.
 *
 * @param geom a lineal geometry or collection of them
 * @param size offset distance. Offset left if negative and right if positive
 * @param quadsegs number of quadrature segments in curves (try 8)
 * @param joinStyle (1 = round, 2 = mitre, 3 = bevel)
 * @param mitreLimit (try 5.0)
 * @return derived geometry (linestring or multilinestring)
 *
 */
DLLEXPORT LWGEOM* lwgeom_offsetcurve(const LWGEOM *geom, double size, int quadsegs, int joinStyle, double mitreLimit);

/*
 * Return true if the input geometry is "simple" as per OGC defn.
 *
 * @return 1 if simple, 0 if non-simple, -1 on exception (lwerror is called
 *         in that case)
 */
DLLEXPORT int lwgeom_is_simple(const LWGEOM *lwgeom);


/*******************************************************************************
 * PROJ4-dependent extra functions on LWGEOM
 ******************************************************************************/

/**
 * Get a projection from a string representation
 *
 * Eg: "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"
 */
DLLEXPORT projPJ lwproj_from_string(const char* txt);

/**
 * Transform (reproject) a geometry in-place.
 * @param geom the geometry to transform
 * @param inpj the input (or current, or source) projection
 * @param outpj the output (or destination) projection
 */
DLLEXPORT int lwgeom_transform(LWGEOM *geom, projPJ inpj, projPJ outpj);
DLLEXPORT int ptarray_transform(POINTARRAY *pa, projPJ inpj, projPJ outpj);


/*******************************************************************************
 * GEOS-dependent extra functions on LWGEOM
 ******************************************************************************/

/**
 * Take a geometry and return an areal geometry
 * (Polygon or MultiPolygon).
 * Actually a wrapper around GEOSpolygonize,
 * transforming the resulting collection into
 * a valid polygon Geometry.
 */
DLLEXPORT LWGEOM* lwgeom_buildarea(const LWGEOM *geom) ;

/**
 * Attempts to make an invalid geometries valid w/out losing points.
 */
DLLEXPORT LWGEOM* lwgeom_make_valid(LWGEOM* geom);

/*
 * Split (multi)polygon by line; (multi)line by (multi)line,
 * (multi)point or (multi)polygon boundary.
 *
 * Collections are accepted as first argument.
 * Returns all obtained pieces as a collection.
 */
DLLEXPORT LWGEOM* lwgeom_split(const LWGEOM* lwgeom_in, const LWGEOM* blade_in);

/*
 * Fully node a set of linestrings, using the least nodes preserving
 * all the input ones.
 */
DLLEXPORT LWGEOM* lwgeom_node(const LWGEOM* lwgeom_in);

/**
 * Take vertices of a geometry and build a delaunay
 * triangulation on them.
 *
 * @param geom the input geometry
 * @param tolerance an optional snapping tolerance for improved robustness
 * @param edgeOnly if non-zero the result will be a MULTILINESTRING,
 *                 otherwise it'll be a COLLECTION of polygons.
 */
DLLEXPORT LWGEOM* lwgeom_delaunay_triangulation(const LWGEOM *geom, double tolerance, int32_t edgeOnly);

/**
 * Take vertices of a geometry and build the Voronoi diagram
 *
 * @param g the input geometry
 * @param env an optional envelope for clipping the results
 * @param tolerance an optional snapping tolerance for improved robustness
 * @param output_edges if non-zero the result will be a MULTILINESTRING,
 *                 otherwise it'll be a COLLECTION of polygons.
 */
DLLEXPORT LWGEOM* lwgeom_voronoi_diagram(const LWGEOM* g, const GBOX* env, double tolerance, int output_edges);

/**
* Take a list of LWGEOMs and a number of clusters and return an integer
* array indicating which cluster each geometry is in.
*
* @param geoms the input array of LWGEOM pointers
* @param ngeoms the number of elements in the array
* @param k the number of clusters to calculate
*/
DLLEXPORT int * lwgeom_cluster_2d_kmeans(const LWGEOM **geoms, uint32_t ngeoms, uint32_t k);

#endif /* !defined _LIBLWGEOM_H  */

