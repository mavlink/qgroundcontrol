/*
 gg_const.h -- Gaia common support for geometries: constants
  
 version 4.3, 2015 June 29

 Author: Sandro Furieri a.furieri@lqt.it

 ------------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008-2015
the Initial Developer. All Rights Reserved.

Contributor(s):
Klaus Foerster klaus.foerster@svg.cc

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/


/**
 \file gg_const.h

 Geometry constants and macros
 */

#ifndef _GG_CONST_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_CONST_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constant values for getVectorLayersList modes */

/** mode: FAST (QGIS data-provider) */
#define GAIA_VECTORS_LIST_FAST	0

/** mode: OPTIMISTIC */
#define GAIA_VECTORS_LIST_OPTIMISTIC	1

/** mode: PESSIMISTIC */
#define GAIA_VECTORS_LIST_PESSIMISTIC	2

/* constant values for Vector Layer Types */

/** Vector Layer: unknown type */
#define GAIA_VECTOR_UNKNOWN	-1
/** Vector Layer: Spatial Table */
#define GAIA_VECTOR_TABLE	1
/** Vector Layer: Spatial View */
#define GAIA_VECTOR_VIEW	2
/** Vector Layer: Virtual Shape */
#define GAIA_VECTOR_VIRTUAL	3

/* constant values for Vector Layer Geometry Types */

/** Vector Layer Geometry: Geometry */
#define GAIA_VECTOR_GEOMETRY		0
/** Vector Layer Geometry: Point */
#define GAIA_VECTOR_POINT		1
/** Vector Layer Geometry: Linestring */
#define GAIA_VECTOR_LINESTRING		2
/** Vector Layer Geometry: Polygon */
#define GAIA_VECTOR_POLYGON		3
/** Vector Layer Geometry: MultiPoint */
#define GAIA_VECTOR_MULTIPOINT		4
/** Vector Layer Geometry: MultiLinestring */
#define GAIA_VECTOR_MULTILINESTRING	5
/** Vector Layer Geometry: MultiPolygon */
#define GAIA_VECTOR_MULTIPOLYGON	6
/** Vector Layer Geometry: GeometryCollection */
#define GAIA_VECTOR_GEOMETRYCOLLECTION	7

/* constant values for Spatial Index */

/** Vector Layer: no Spatial Index */
#define GAIA_SPATIAL_INDEX_NONE		0
/** Vector Layer: Spatial Index RTree */
#define GAIA_SPATIAL_INDEX_RTREE	1
/** Vector Layer: Spatial Index MbrCache */
#define GAIA_SPATIAL_INDEX_MBRCACHE	2

/* constant values for generic geometry classes */

/** WKT parser: unknown Geometry type */
#define GAIA_TYPE_NONE		0
/** WKT parser: Point Geometry type */
#define GAIA_TYPE_POINT		1
/** WKT parser: Linestring Geometry type */
#define GAIA_TYPE_LINESTRING	2
/** WKT parser: Polygon Geometry type */
#define GAIA_TYPE_POLYGON	3

/* constants that defines byte storage order  */
/** Big-Endian marker */
#define GAIA_BIG_ENDIAN		0
/** Little-Endian marker */
#define GAIA_LITTLE_ENDIAN	1

/* constants that defines special markers used for encoding of SpatiaLite internal BLOB geometries  */
/** BLOB-Geometry internal marker: START */
#define GAIA_MARK_START		0x00
/** BLOB-Geometry internal marker: END */
#define GAIA_MARK_END		0xFE
/** BLOB-Geometry internal marker: MBR */
#define GAIA_MARK_MBR		0x7C
/** BLOB-Geometry internal marker: ENTITY */
#define GAIA_MARK_ENTITY	0x69

/* constants that defines GEOMETRY CLASSes */
/** BLOB-Geometry CLASS: unknown */
#define GAIA_UNKNOWN			0
/** BLOB-Geometry CLASS: POINT */
#define GAIA_POINT			1
/** BLOB-Geometry CLASS: LINESTRING */
#define GAIA_LINESTRING			2
/** BLOB-Geometry CLASS: POLYGON */
#define GAIA_POLYGON			3
/** BLOB-Geometry CLASS: MULTIPOINT */
#define GAIA_MULTIPOINT			4
/** BLOB-Geometry CLASS: MULTILINESTRING */
#define GAIA_MULTILINESTRING		5
/** BLOB-Geometry CLASS: MULTIPOLYGON */
#define GAIA_MULTIPOLYGON		6
/** BLOB-Geometry CLASS: GEOMETRYCOLLECTION */
#define GAIA_GEOMETRYCOLLECTION		7
/** BLOB-Geometry CLASS: POINT Z */
#define GAIA_POINTZ			1001
/** BLOB-Geometry CLASS: LINESTRING Z */
#define GAIA_LINESTRINGZ		1002
/** BLOB-Geometry CLASS: POLYGON Z */
#define GAIA_POLYGONZ			1003
/** BLOB-Geometry CLASS: MULTIPOINT Z */
#define GAIA_MULTIPOINTZ		1004
/** BLOB-Geometry CLASS: MULTILINESTRING Z */
#define GAIA_MULTILINESTRINGZ		1005
/** BLOB-Geometry CLASS: MULTIPOLYGON Z */
#define GAIA_MULTIPOLYGONZ		1006
/** BLOB-Geometry CLASS: GEOMETRYCOLLECTION Z */
#define GAIA_GEOMETRYCOLLECTIONZ	1007
/** BLOB-Geometry CLASS: POINT M */
#define GAIA_POINTM			2001
/** BLOB-Geometry CLASS: LINESTRING M */
#define GAIA_LINESTRINGM		2002
/** BLOB-Geometry CLASS: POLYGON M */
#define GAIA_POLYGONM			2003
/** BLOB-Geometry CLASS: MULTIPOINT M */
#define GAIA_MULTIPOINTM		2004
/** BLOB-Geometry CLASS: MULTILINESTRING M */
#define GAIA_MULTILINESTRINGM		2005
/** BLOB-Geometry CLASS: MULTIPOLYGON M */
#define GAIA_MULTIPOLYGONM		2006
/** BLOB-Geometry CLASS: GEOMETRYCOLLECTION M */
#define GAIA_GEOMETRYCOLLECTIONM	2007
/** BLOB-Geometry CLASS: POINT ZM */
#define GAIA_POINTZM			3001
/** BLOB-Geometry CLASS: LINESTRING ZM */
#define GAIA_LINESTRINGZM		3002
/** BLOB-Geometry CLASS: POLYGON ZM */
#define GAIA_POLYGONZM			3003
/** BLOB-Geometry CLASS: MULTIPOINT ZM */
#define GAIA_MULTIPOINTZM		3004
/** BLOB-Geometry CLASS: MULTILINESTRING ZM */
#define GAIA_MULTILINESTRINGZM		3005
/** BLOB-Geometry CLASS: MULTIPOLYGON ZM */
#define GAIA_MULTIPOLYGONZM		3006
/** BLOB-Geometry CLASS: GEOMETRYCOLLECTION ZM */
#define GAIA_GEOMETRYCOLLECTIONZM	3007

/* constants that defines Compressed GEOMETRY CLASSes */
/** BLOB-Geometry CLASS: compressed LINESTRING */
#define GAIA_COMPRESSED_LINESTRING		1000002
/** BLOB-Geometry CLASS: compressed POLYGON */
#define GAIA_COMPRESSED_POLYGON			1000003
/** BLOB-Geometry CLASS: compressed LINESTRING Z */
#define GAIA_COMPRESSED_LINESTRINGZ		1001002
/** BLOB-Geometry CLASS: compressed POLYGON Z */
#define GAIA_COMPRESSED_POLYGONZ		1001003
/** BLOB-Geometry CLASS: compressed LINESTRING M */
#define GAIA_COMPRESSED_LINESTRINGM		1002002
/** BLOB-Geometry CLASS: compressed POLYGON M */
#define GAIA_COMPRESSED_POLYGONM		1002003
/** BLOB-Geometry CLASS: compressed LINESTRING ZM */
#define GAIA_COMPRESSED_LINESTRINGZM		1003002
/** BLOB-Geometry CLASS: compressed POLYGON ZM */
#define GAIA_COMPRESSED_POLYGONZM		1003003

/* constants that defines GEOS-WKB 3D CLASSes */
/** GEOS-WKB 3D CLASS: POINT Z */
#define GAIA_GEOSWKB_POINTZ			-2147483647
/** GEOS-WKB 3D CLASS: LINESTRING Z */
#define GAIA_GEOSWKB_LINESTRINGZ		-2147483646
/** GEOS-WKB 3D CLASS: POLYGON Z */
#define GAIA_GEOSWKB_POLYGONZ			-2147483645
/** GEOS-WKB 3D CLASS: MULTIPOINT Z */
#define GAIA_GEOSWKB_MULTIPOINTZ		-2147483644
/** GEOS-WKB 3D CLASS: MULTILINESTRING Z */
#define GAIA_GEOSWKB_MULTILINESTRINGZ		-2147483643
/** GEOS-WKB 3D CLASS: MULTIPOLYGON Z */
#define GAIA_GEOSWKB_MULTIPOLYGONZ		-2147483642
/** GEOS-WKB 3D CLASS: POINT Z */
#define GAIA_GEOSWKB_GEOMETRYCOLLECTIONZ	-2147483641

/* constants that defines multitype values */
/** DBF data type: NULL */
#define GAIA_NULL_VALUE		0
/** DBF data type: TEXT */
#define GAIA_TEXT_VALUE		1
/** DBF data type: INT */
#define GAIA_INT_VALUE		2
/** DBF data type: DOUBLE */
#define GAIA_DOUBLE_VALUE	3

/* constants that defines POINT index for LINESTRING */
/** Linestring/Ring functions: START POINT */
#define GAIA_START_POINT	1
/** Linestring/Ring functions: END POINT */
#define GAIA_END_POINT		2
/** Linestring/Ring functions: POINTN */
#define GAIA_POINTN		3

/* constants that defines MBRs spatial relationships */
/** MBR relationships: CONTAINS */
#define GAIA_MBR_CONTAINS	1
/** MBR relationships: DISJOINT */
#define GAIA_MBR_DISJOINT	2
/** MBR relationships: EQUAL */
#define GAIA_MBR_EQUAL		3
/** MBR relationships: INTERSECTS */
#define GAIA_MBR_INTERSECTS	4
/** MBR relationships: OVERLAP */
#define GAIA_MBR_OVERLAPS	5
/** MBR relationships: TOUCHES */
#define GAIA_MBR_TOUCHES	6
/** MBR relationships: WITHIN */
#define GAIA_MBR_WITHIN		7

/* constants used for FilterMBR */
/** FilerMBR relationships: WITHIN */
#define GAIA_FILTER_MBR_WITHIN		74
/** FilerMBR relationships: CONTAINS */
#define GAIA_FILTER_MBR_CONTAINS	77
/** FilerMBR relationships: INTERSECTS */
#define GAIA_FILTER_MBR_INTERSECTS	79
/** FilerMBR relationships: DECLARE */
#define GAIA_FILTER_MBR_DECLARE		89

/* constants defining SVG default values */
/** SVG precision: RELATIVE */
#define GAIA_SVG_DEFAULT_RELATIVE 	0
/** SVG precision: DEFAULT */
#define GAIA_SVG_DEFAULT_PRECISION	6
/** SVG precision: MAX */
#define GAIA_SVG_DEFAULT_MAX_PRECISION 15

/* constants used for VirtualNetwork */
/** VirtualNetwork internal markers: START */
#define GAIA_NET_START		0x67
/** VirtualNetwork internal markers: 64 bit START */
#define GAIA_NET64_START	0x68
/** VirtualNetwork internal markers: A-Stat START */
#define GAIA_NET64_A_STAR_START	0x69
/** VirtualNetwork internal markers: END */
#define GAIA_NET_END		0x87
/** VirtualNetwork internal markers: HEADER */
#define GAIA_NET_HEADER		0xc0
/** VirtualNetwork internal markers: CODE */
#define GAIA_NET_CODE		0xa6
/** VirtualNetwork internal markers: ID */
#define GAIA_NET_ID		0xb5
/** VirtualNetwork internal markers: NODE */
#define GAIA_NET_NODE		0xde
/** VirtualNetwork internal markers: ARC */
#define GAIA_NET_ARC		0x54
/** VirtualNetwork internal markers: TABLE */
#define GAIA_NET_TABLE		0xa0
/** VirtualNetwork internal markers: FROM */
#define GAIA_NET_FROM		0xa1
/** VirtualNetwork internal markers: TO */
#define GAIA_NET_TO		0xa2
/** VirtualNetwork internal markers: GEOM */
#define GAIA_NET_GEOM		0xa3
/** VirtualNetwork internal markers: NAME */
#define GAIA_NET_NAME		0xa4
/** VirtualNetwork internal markers: COEFF */
#define GAIA_NET_A_STAR_COEFF	0xa5
/** VirtualNetwork internal markers: BLOCK */
#define GAIA_NET_BLOCK		0xed

/* constants used for Coordinate Dimensions */
/** Coordinate Dimensions: XY */
#define GAIA_XY		0x00
/** Coordinate Dimensions: XYZ */
#define GAIA_XY_Z	0x01
/** Coordinate Dimensions: XYM */
#define GAIA_XY_M	0x02
/** Coordinate Dimensions: XYZM */
#define GAIA_XY_Z_M	0x03

/* constants used for length unit conversion */
/** Length unit conversion: Kilometer */
#define GAIA_KM		0
/** Length unit conversion: Meter */
#define GAIA_M		1
/** Length unit conversion: Decimeter */
#define GAIA_DM		2
/** Length unit conversion: Centimeter */
#define GAIA_CM		3
/** Length unit conversion: Millimeter */
#define GAIA_MM		4
/** Length unit conversion: International Nautical Mile */
#define GAIA_KMI	5
/** Length unit conversion: Inch */
#define GAIA_IN		6
/** Length unit conversion: Feet */
#define GAIA_FT		7
/** Length unit conversion: Yard */
#define GAIA_YD		8
/** Length unit conversion: Mile */
#define GAIA_MI		9
/** Length unit conversion: Fathom */
#define GAIA_FATH	10
/** Length unit conversion: Chain */
#define GAIA_CH		11
/** Length unit conversion: Link */
#define GAIA_LINK	12
/** Length unit conversion: US Inch */
#define GAIA_US_IN	13
/** Length unit conversion: US Feet */
#define GAIA_US_FT	14
/** Length unit conversion: US Yard */
#define GAIA_US_YD	15
/** Length unit conversion: US Chain */
#define GAIA_US_CH	16
/** Length unit conversion: US Mile */
#define GAIA_US_MI	17
/** Length unit conversion: Indian Yard */
#define GAIA_IND_YD	18
/** Length unit conversion: Indian Feet */
#define GAIA_IND_FT	19
/** Length unit conversion: Indian Chain */
#define GAIA_IND_CH	20
/** Length unit conversion: MIN */
#define GAIA_MIN_UNIT	GAIA_KM
/** Length unit conversion: MAX */
#define GAIA_MAX_UNIT	GAIA_IND_CH

/* constants used for SHAPES */
/** SHP shape: unknown */
#define GAIA_SHP_NULL		0
/** SHP shape: POINT */
#define GAIA_SHP_POINT		1
/** SHP shape: POLYLINE */
#define GAIA_SHP_POLYLINE	3
/** SHP shape: POLYGON */
#define GAIA_SHP_POLYGON	5
/** SHP shape: MULTIPOINT */
#define GAIA_SHP_MULTIPOINT	8
/** SHP shape: POINT Z */
#define GAIA_SHP_POINTZ		11
/** SHP shape: POLYLINE Z */
#define GAIA_SHP_POLYLINEZ	13
/** SHP shape: POLYGON Z */
#define GAIA_SHP_POLYGONZ	15
/** SHP shape: MULTIPOINT Z */
#define GAIA_SHP_MULTIPOINTZ	18
/** SHP shape: POINT M */
#define GAIA_SHP_POINTM		21
/** SHP shape: POLYLINE M */
#define GAIA_SHP_POLYLINEM	23
/** SHP shape: POLYGON M */
#define GAIA_SHP_POLYGONM	25
/** SHP shape: MULTIPOINT M */
#define GAIA_SHP_MULTIPOINTM	28

/* constants used for Clone Special modes */
/** Clone Special Mode: Same Order as input */
#define GAIA_SAME_ORDER		0
/** Clone Special Mode: Reversed Order */
#define GAIA_REVERSE_ORDER	-1
/** Clone Special Mode: apply Left Handle Rule to Polygon Rings */
#define GAIA_LHR_ORDER		-2

/* macros */
/**
 macro extracting XY coordinates

 \param xy pointer [const void *] to COORD mem-array
 \param v [int] point index [first point has index 0]
 \param x [double *] X coordinate
 \param y [double *] Y coordinate 

 \sa gaiaLineGetPoint, gaiaRingGetPoint

 \note using this macro on behalf of COORDs not of [XY] dims may cause serious 
 problems
 */
#define gaiaGetPoint(xy,v,x,y)	\
				{*x = xy[(v) * 2]; \
				 *y = xy[(v) * 2 + 1];}

/**
 macro setting XY coordinates

 \param xy pointer [const void *] to COORD mem-array
 \param v [int] point index [first point has index 0]
 \param x [double] X coordinate
 \param y [double] Y coordinate

 \sa gaiaLineSetPoint, gaiaRingSetPoint

 \note using this macro on behalf on COORDs not of [XY] dims may cause
 serious problems
 */
#define gaiaSetPoint(xy,v,x,y)	\
				{xy[(v) * 2] = x; \
				 xy[(v) * 2 + 1] = y;}

/**
 macro extracting XYZ coordinates

 \param xyz pointer [const void *] to COORD mem-array
 \param v [int] point index [first point has index 0]
 \param x [double *] X coordinate
 \param y [double *] Y coordinate 
 \param z [double *] Z coordinate 

 \sa gaiaLineGetPoint, gaiaRingGetPoint

 \note using this macro on behalf of COORDs not of [XYZ] dims may cause serious 
 problems
 */
#define gaiaGetPointXYZ(xyz,v,x,y,z)	\
				{*x = xyz[(v) * 3]; \
				 *y = xyz[(v) * 3 + 1]; \
				 *z = xyz[(v) * 3 + 2];}

/**
 macro setting XYZ coordinates

 \param xyz pointer [const void *] to COORD mem-array
 \param v [int] point index [first point has index 0]
 \param x [double] X coordinate
 \param y [double] Y coordinate
 \param z [double] Z coordinate

 \sa gaiaLineSetPoint, gaiaRingSetPoint

 \note using this macro on behalf on COORDs not of [XYZ] dims may cause
 serious problems
 */
#define gaiaSetPointXYZ(xyz,v,x,y,z)	\
				{xyz[(v) * 3] = x; \
				 xyz[(v) * 3 + 1] = y; \
				 xyz[(v) * 3 + 2] = z;}

/**
 macro extracting XYM coordinates

 \param xym pointer [const void *] to COORD mem-array
 \param v [int] point index [first point has index 0]
 \param x [double *] X coordinate
 \param y [double *] Y coordinate 
 \param m [double *] M measure

 \sa gaiaLineGetPoint, gaiaRingGetPoint

 \note using this macro on behalf of COORDs not of [XYM] dims may cause serious 
 problems
 */
#define gaiaGetPointXYM(xym,v,x,y,m)	\
				{*x = xym[(v) * 3]; \
				 *y = xym[(v) * 3 + 1]; \
				 *m = xym[(v) * 3 + 2];}

/**
 macro setting XYM coordinates

 \param xym pointer [const void *] to COORD mem-array
 \param v [int] point index [first point has index 0]
 \param x [double] X coordinate
 \param y [double] Y coordinate
 \param m [double] M measure

 \sa gaiaLineSetPoint, gaiaRingSetPoint

 \note using this macro on behalf on COORDs not of [XYM] dims may cause
 serious problems
 */
#define gaiaSetPointXYM(xym,v,x,y,m)	\
				{xym[(v) * 3] = x; \
				 xym[(v) * 3 + 1] = y; \
				 xym[(v) * 3 + 2] = m;}

/**
 macro extracting XYZM coordinates

 \param xyzm pointer [const void *] to COORD mem-array
 \param v [int] point index [first point has index 0]
 \param x [double *] X coordinate
 \param y [double *] Y coordinate 
 \param z [double *] Z coordinate 
 \param m [double *] M measure

 \sa gaiaLineGetPoint, gaiaRingGetPoint

 \note using this macro on behalf of COORDs not of [XYZM] dims may cause serious 
 problems
 */
#define gaiaGetPointXYZM(xyzm,v,x,y,z,m)	\
				{*x = xyzm[(v) * 4]; \
				 *y = xyzm[(v) * 4 + 1]; \
				 *z = xyzm[(v) * 4 + 2]; \
				 *m = xyzm[(v) * 4 + 3];}

/**
 macro setting XYZM coordinates

 \param xyzm pointer [const void *] to COORD mem-array
 \param v [int] point index [first point has index 0]
 \param x [double] X coordinate
 \param y [double] Y coordinate
 \param z [double] Z coordinate
 \param m [double] M measure

 \sa gaiaLineSetPoint, gaiaRingSetPoint

 \note using this macro on behalf on COORDs not of [XYZM] dims may cause
 serious problems
 */
#define gaiaSetPointXYZM(xyzm,v,x,y,z,m)	\
				{xyzm[(v) * 4] = x; \
				 xyzm[(v) * 4 + 1] = y; \
				 xyzm[(v) * 4 + 2] = z; \
				 xyzm[(v) * 4 + 3] = m;}


#ifdef __cplusplus
}
#endif

#endif				/* _GG_CONST_H */
