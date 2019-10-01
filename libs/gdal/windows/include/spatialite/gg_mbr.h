/*
 gg_mbr.h -- Gaia common support for geometries: MBR functions
  
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
 \file gg_mbr.h

 Geometry handling functions: MBR
 */

#ifndef _GG_MBR_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_MBR_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* function prototypes */

/**
 Updates the actual MBR for a Linestring object

 \param line pointer to the Linestring object 
 */
    GAIAGEO_DECLARE void gaiaMbrLinestring (gaiaLinestringPtr line);

/**
 Updates the actual MBR for a Ring object

 \param rng pointer to the Ring object
 */
    GAIAGEO_DECLARE void gaiaMbrRing (gaiaRingPtr rng);

/**
 Updates the actual MBR for a Polygon object

 \param polyg pointer to the Polygon object
 */
    GAIAGEO_DECLARE void gaiaMbrPolygon (gaiaPolygonPtr polyg);

/**
 Updates the actual MBR for a Geometry object

 \param geom pointer to the Geometry object
 */
    GAIAGEO_DECLARE void gaiaMbrGeometry (gaiaGeomCollPtr geom);

/**
 Retrieves the MBR (MinX) from a BLOB-Geometry object

 \param blob pointer to BLOB-Geometry.
 \param size the BLOB's size (in bytes).
 \param minx on completion this variable will contain the MBR MinX coordinate.

 \return 0 on failure: any other value on success.

 \sa gaiaGetMbrMaxX, gaiaGetMbrMinY, gaiaGetMbrMaxY
 */
    GAIAGEO_DECLARE int gaiaGetMbrMinX (const unsigned char *blob,
					unsigned int size, double *minx);

/**
 Retrieves the MBR (MaxX) from a BLOB-Geometry object

 \param blob pointer to BLOB-Geometry.
 \param size the BLOB's size (in bytes).
 \param maxx on completion this variable will contain the MBR MaxX coordinate.

 \return 0 on failure: any other value on success.

 \sa gaiaGetMbrMinX, gaiaGetMbrMinY, gaiaGetMbrMaxY
 */
    GAIAGEO_DECLARE int gaiaGetMbrMaxX (const unsigned char *blob,
					unsigned int size, double *maxx);

/**
 Retrieves the MBR (MinY) from a BLOB-Geometry object

 \param blob pointer to BLOB-Geometry.
 \param size the BLOB's size (in bytes).
 \param miny on completion this variable will contain the MBR MinY coordinate.

 \return 0 on failure: any other value on success.

 \sa gaiaGetMbrMinX, gaiaGetMbrMaxX, gaiaGetMbrMaxY
 */
    GAIAGEO_DECLARE int gaiaGetMbrMinY (const unsigned char *blob,
					unsigned int size, double *miny);

/**
 Retrieves the MBR (MaxY) from a BLOB-Geometry object

 \param blob pointer to BLOB-Geometry.
 \param size the BLOB's size (in bytes).
 \param maxy on completion this variable will contain the MBR MaxY coordinate.

 \return 0 on failure: any other value on success.

 \sa gaiaGetMbrMinX, gaiaGetMbrMaxX, gaiaGetMbrMinY
 */
    GAIAGEO_DECLARE int gaiaGetMbrMaxY (const unsigned char *blob,
					unsigned int size, double *maxy);

/**
 Creates a Geometry object corresponding to the Envelope [MBR] for a
 BLOB-Geometry

 \param blob pointer to BLOB-Geometry
 \param size the BLOB's size (in bytes)

 \return the pointer to the newly created Geometry object: NULL on failure

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any 
 contained child object. 
 */

    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromSpatiaLiteBlobMbr (const unsigned
							       char *blob,
							       unsigned int
							       size);

/**
 MBRs comparison: Contains

 \param mbr1 pointer to first Geometry object.
 \param mbr2 pointer to second Geometry object.

 \return 0 if false; any other value if mbr1 spatially \e contains mbr2

 \sa gaiaMbrsDisjoint, gaiaMbrsEqual, gaiaMbrsIntersects,
 gaiaMbrsOverlaps, gaiaMbrsTouches, gaiaMbrsWithin
 */
    GAIAGEO_DECLARE int gaiaMbrsContains (gaiaGeomCollPtr mbr1,
					  gaiaGeomCollPtr mbr2);

/**
 MBRs comparison: Disjoint

 \param mbr1 pointer to first Geometry object.
 \param mbr2 pointer to second Geometry object.

 \return 0 if false; any other value if mbr1 and mbr2 are spatially \e disjoint

 \sa gaiaMbrsContains, gaiaMbrsEqual, gaiaMbrsIntersects,
 gaiaMbrsOverlaps, gaiaMbrsTouches, gaiaMbrsWithin
 */
    GAIAGEO_DECLARE int gaiaMbrsDisjoint (gaiaGeomCollPtr mbr1,
					  gaiaGeomCollPtr mbr2);

/**
 MBRs comparison: Equal

 \param mbr1 pointer to first Geometry object.
 \param mbr2 pointer to second Geometry object.

 \return 0 if false; any other value if mbr1 and mbr2 are spatially \e equal

 \sa gaiaMbrsContains, gaiaMbrsDisjoint, gaiaMbrsIntersects,
 gaiaMbrsOverlaps, gaiaMbrsTouches, gaiaMbrsWithin
 */
    GAIAGEO_DECLARE int gaiaMbrsEqual (gaiaGeomCollPtr mbr1,
				       gaiaGeomCollPtr mbr2);

/**
 MBRs comparison: Intersects

 \param mbr1 pointer to first Geometry object.
 \param mbr2 pointer to second Geometry object.

 \return 0 if false; any other value if mbr1 and mbr2 spatially \e intersect

 \sa gaiaMbrsContains, gaiaMbrsDisjoint, gaiaMbrsEqual, 
 gaiaMbrsOverlaps, gaiaMbrsTouches, gaiaMbrsWithin
 */
    GAIAGEO_DECLARE int gaiaMbrsIntersects (gaiaGeomCollPtr mbr1,
					    gaiaGeomCollPtr mbr2);

/**
 MBRs comparison: Overlaps

 \param mbr1 pointer to first Geometry object.
 \param mbr2 pointer to second Geometry object.

 \return 0 if false; any other value if mbr1 and mbr2 spatially \e overlap

 \sa gaiaMbrsContains, gaiaMbrsDisjoint, gaiaMbrsEqual, gaiaMbrsIntersects,
 gaiaMbrsTouches, gaiaMbrsWithin
 */
    GAIAGEO_DECLARE int gaiaMbrsOverlaps (gaiaGeomCollPtr mbr1,
					  gaiaGeomCollPtr mbr2);

/**
 MBRs comparison: Touches

 \param mbr1 pointer to first Geometry object.
 \param mbr2 pointer to second Geometry object.

 \return 0 if false; any other value if mbr1 and mbr2 spatially \e touche

 \sa gaiaMbrsContains, gaiaMbrsDisjoint, gaiaMbrsEqual, gaiaMbrsIntersects,
 gaiaMbrsOverlaps, gaiaMbrsWithin
 */
    GAIAGEO_DECLARE int gaiaMbrsTouches (gaiaGeomCollPtr mbr1,
					 gaiaGeomCollPtr mbr2);

/**
 MBRs comparison: Within

 \param mbr1 pointer to first Geometry object.
 \param mbr2 pointer to second Geometry object.

 \return 0 if false; any other value if mbr1 is spatially \e within mbr2

 \sa gaiaMbrsContains, gaiaMbrsDisjoint, gaiaMbrsEqual, gaiaMbrsIntersects,
 gaiaMbrsOverlaps, gaiaMbrsTouches
 */
    GAIAGEO_DECLARE int gaiaMbrsWithin (gaiaGeomCollPtr mbr1,
					gaiaGeomCollPtr mbr2);

/**
 Creates a BLOB-Geometry representing an Envelope [MBR]

 \param x1 first X coordinate.
 \param y1 first Y coordinate.
 \param x2 second X coordinate.
 \param y2 second Y coordinate.
 \param srid the SRID associated to the Envelope
 \param result on completion will contain a pointer to newly created
 BLOB-Geometry
 \param size on completion this variabile will contain the BLOB's size (in 
 bytes)

 \sa gaiaBuildCircleMbr

 \note [XY] coords must define two extreme Points identifying a diagonal
 of the MBR [Envelope]
 \n no special order is required for coords: MAX / MIN values will be 
 internally arranged as appropriate.
 */
    GAIAGEO_DECLARE void gaiaBuildMbr (double x1, double y1, double x2,
				       double y2, int srid,
				       unsigned char **result, int *size);

/**
 Creates a BLOB-Geometry representing an Envelope [MBR]

 \param x centre X coordinate.
 \param y centre Y coordinate.
 \param radius the radius of the circle
 \param srid the SRID associated to the Envelope
 \param result on completion will contain a pointer to newly created
 BLOB-Geometry
 \param size on completion this variabile will contain the BLOB's size (in
 bytes)

 \sa gaiaBuildMbr

 \note the \e circle of givern \e radius and \e centre will be used so to
 determine the corresponding \e square Envelope
 */
    GAIAGEO_DECLARE void gaiaBuildCircleMbr (double x, double y, double radius,
					     int srid, unsigned char **result,
					     int *size);

/**
 Creates a BLOB-FilterMBR

 \param x1 first X coordinate.
 \param y1 first Y coordinate.
 \param x2 second X coordinate.
 \param y2 second Y coordinate.
 \param mode one of: GAIA_FILTER_MBR_WITHIN, GAIA_FILTER_MBR_CONTAINS,
 GAIA_FILTER_MBR_INTERSECTS, GAIA_FILTER_MBR_DECLARE
 \param result on completion will contain a pointer to newly created
 BLOB-FilterMBR
 \param size on completion this variabile will contain the BLOB's size (in
 bytes)

 \sa gaiaParseFilterMbr

 \note [XY] coords must define two extreme Points identifying a diagonal
 of the MBR [Envelope]
 \n no special order is required for coords: MAX / MIN values will be
 internally arranged as appropriate.

 \remark internally used to implement Geometry Callback R*Tree filtering.
 */
    GAIAGEO_DECLARE void gaiaBuildFilterMbr (double x1, double y1, double x2,
					     double y2, int mode,
					     unsigned char **result, int *size);

/**
 Creates a BLOB-FilterMBR

 \param result pointer to BLOB-FilterMBR [previously created by 
 gaiaBuildFilterMbr]
 BLOB-Geometry
 \param size BLOB's size (in bytes)
 \param minx on completion this variable will contain the MBR MinX coord.
 \param miny on completion this variable will contain the MBR MinY coord.
 \param maxx on completion this variable will contain the MBR MinY coord.
 \param maxy on completion this variable will contain the MBR MaxY coord.
 \param mode on completion this variable will contain the FilterMBR mode.

 \sa gaiaBuildFilterMbr

 \remark internally used to implement Geometry Callback R*Tree filtering.
 */
    GAIAGEO_DECLARE int gaiaParseFilterMbr (unsigned char *result, int size,
					    double *minx, double *miny,
					    double *maxx, double *maxy,
					    int *mode);

/**
 Computes the Z-Range for a Linestring object

 \param line pointer to the Linestring object
 \param min on completion this variable will contain the min Z value found 
 \param max on completion this variable will contain the max Z value found 

 \note if the Linestring has XY or XYM dims, the Z-Range is meaningless
 */
    GAIAGEO_DECLARE void gaiaZRangeLinestring (gaiaLinestringPtr line,
					       double *min, double *max);

/**
 Computes the Z-Range for a Ring object

 \param rng pointer to the Ring object
 \param min on completion this variable will contain the min Z value found
 \param max on completion this variable will contain the max Z value found

 \note if the Ring has XY or XYM dims, the Z-Range is meaningless
 */
    GAIAGEO_DECLARE void gaiaZRangeRing (gaiaRingPtr rng, double *min,
					 double *max);

/**
 Computes the Z-Range for a Polygon object

 \param polyg pointer to the Polygon object
 \param min on completion this variable will contain the min Z value found
 \param max on completion this variable will contain the max Z value found

 \note if the Polygon has XY or XYM dims, the Z-Range is meaningless
 */
    GAIAGEO_DECLARE void gaiaZRangePolygon (gaiaPolygonPtr polyg, double *min,
					    double *max);

/**
 Computes the Z-Range for a Geometry object

 \param geom pointer to the Geometry object
 \param min on completion this variable will contain the min Z value found
 \param max on completion this variable will contain the max Z value found

 \note if the Geometry has XY or XYM dims, the Z-Range is meaningless
 */
    GAIAGEO_DECLARE void gaiaZRangeGeometry (gaiaGeomCollPtr geom, double *min,
					     double *max);

/**
 Computes the M-Range for a Linestring object

 \param line pointer to the Linestring object
 \param min on completion this variable will contain the min M value found
 \param max on completion this variable will contain the max M value found

 \note if the Linestring has XY or XYZ dims, the M-Range is meaningless
 */
    GAIAGEO_DECLARE void gaiaMRangeLinestring (gaiaLinestringPtr line,
					       double *min, double *max);

/**
 Computes the M-Range for a Ring object

 \param rng pointer to the Ring object
 \param min on completion this variable will contain the min M value found
 \param max on completion this variable will contain the max M value found

 \note if the Ring has XY or XYZ dims, the M-Range is meaningless
 */
    GAIAGEO_DECLARE void gaiaMRangeRing (gaiaRingPtr rng, double *min,
					 double *max);

/**
 Computes the M-Range for a Polygon object

 \param polyg pointer to the Polygon object
 \param min on completion this variable will contain the min M value found
 \param max on completion this variable will contain the max M value found

 \note if the Polygon has XY or XYZ dims, the M-Range is meaningless
 */
    GAIAGEO_DECLARE void gaiaMRangePolygon (gaiaPolygonPtr polyg, double *min,
					    double *max);

/**
 Computes the Z-Range for a Geometry object

 \param geom pointer to the Geometry object
 \param min on completion this variable will contain the min M value found
 \param max on completion this variable will contain the max M value found

 \note if the Geometry has XY or XYZ dims, the M-Range is meaningless
 */
    GAIAGEO_DECLARE void gaiaMRangeGeometry (gaiaGeomCollPtr geom, double *min,
					     double *max);


#ifdef __cplusplus
}
#endif

#endif				/* _GG_MBR_H */
