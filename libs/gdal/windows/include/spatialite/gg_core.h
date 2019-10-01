/*
 gg_core.h -- Gaia common support for geometries: core functions
  
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
 \file gg_core.h

 Geometry handling functions: core
 */

#ifndef _GG_CORE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_CORE_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* function prototypes */

/**
 Safely frees any dynamic memory block allocated by the library itself

 \param ptr pointer to dynamically allocated memory

 \note on some platforms (most notably, Microsoft Windows) many different
 runtime libraries may actually support the same process.
 \n attempting to free() a memory block allocated by a different runtime 
 module may easily cause fatal memory corruption.
 */
    GAIAGEO_DECLARE void gaiaFree (void *ptr);

/**
 Allocates a 2D POINT [XY]

 \param x the X coordinate.
 \param y the Y coordinate.
 
 \return the pointer to the newly created POINT object: NULL on failure

 \sa gaiaFreePoint

 \note you are responsible to destroy (before or after) any allocated
 POINT, unless you've passed ownership of the POINT object to some 
 further object: in this case destroying the higher order object will
 implicitly destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaPointPtr gaiaAllocPoint (double x, double y);

/**
 Allocates a 3D POINT [XYZ]

 \param x the X coordinate.
 \param y the Y coordinate.
 \param z the Z coordinate.
 
 \return the pointer to the newly created POINT object: NULL on failure

 \sa gaiaFreePoint

 \note you are responsible to destroy (before or after) any allocated
 POINT, unless you've passed ownership of the POINT object to some 
 further object: in this case destroying the higher order object will
 implicitly destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaPointPtr gaiaAllocPointXYZ (double x, double y,
						    double z);

/**
 Allocates a 2D POINT [XYM]

 \param x the X coordinate.
 \param y the Y coordinate.
 \param m the M measure.
 
 \return the pointer to the newly created POINT object: NULL on failure

 \sa gaiaFreePoint

 \note you are responsible to destroy (before or after) any allocated
 POINT, unless you've passed ownership of the POINT object to some 
 further object: in this case destroying the higher order object will
 implicitly destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaPointPtr gaiaAllocPointXYM (double x, double y,
						    double m);

/**
 Allocates a 3D POINT [XYZM]

 \param x the X coordinate.
 \param y the Y coordinate.
 \param z the Z coordinate.
 \param m the M measure.
 
 \return the pointer to the newly created POINT object: NULL on failure

 \sa gaiaFreePoint

 \note you are responsible to destroy (before or after) any allocated
 POINT, unless you've passed ownership of the POINT object to some 
 further object: in this case destroying the higher order object will
 implicitly destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaPointPtr gaiaAllocPointXYZM (double x, double y,
						     double z, double m);

/**
 Destroys a POINT object

 \param ptr pointer to the POINT object to be destroyed

 \sa gaiaAllocPoint, gaiaAllocPointXYZ, gaiaAllocPointXYM, gaiaAllocPointXYZM

 \note attempting to destroy any POINT object whose ownership has already
 been transferred to some other (higher order) object is a serious
 error, and will easily cause severe memory corruption.
 */
    GAIAGEO_DECLARE void gaiaFreePoint (gaiaPointPtr ptr);

/**
 Allocates a 2D LINESTRING [XY]

 \param vert number of points [aka vertices] into the Linestring

 \return the pointer to newly created LINESTRING object: NULL on failure

 \sa gaiaFreeLinestring, gaiaLineSetPoint, gaiaLineGetPoint, gaiaSetPoint,
 gaiaGetPoint

 \note you are responsible to destroy (before or after) any allocated LINESTRING,
 unless you've passed ownership of the LINESTRING object to some further 
 object: in this case destroying the higher order object will implicitly 
 destroy any contained child object. 
 */
    GAIAGEO_DECLARE gaiaLinestringPtr gaiaAllocLinestring (int vert);

/**
 Allocates a 3D LINESTRING [XYZ]

 \param vert number of points [aka vertices] into the Linestring

 \return the pointer to newly created LINESTRING object: NULL on failure

 \sa gaiaFreeLinestring, gaiaLineSetPoint, gaiaLineGetPoint, gaiaSetPointXYZ,
 gaiaGetPointXYZ

 \note you are responsible to destroy (before or after) any allocated LINESTRING, 
 unless you've passed ownership of the LINESTRING object to some further  
 object: in this case destroying the higher order object will implicitly 
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaLinestringPtr gaiaAllocLinestringXYZ (int vert);

/**
 Allocates a 2D LINESTRING [XYM]

 \param vert number of points [aka vertices] into the Linestring

 \return the pointer to newly created LINESTRING object: NULL on failure

 \sa gaiaFreeLinestring, gaiaLineSetPoint, gaiaLineGetPoint, gaiaSetPointXYM,
 gaiaGetPointXYM

 \note you are responsible to destroy (before or after) any allocated LINESTRING, 
 unless you've passed ownership of the LINESTRING object to some further  
 object: in this case destroying the higher order object will implicitly 
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaLinestringPtr gaiaAllocLinestringXYM (int vert);

/**
 Allocates a 3D LINESTRING [XYZM]

 \param vert number of points [aka vertices] into the Linestring

 \return the pointer to newly created LINESTRING object: NULL on failure

 \sa gaiaFreeLinestring, gaiaLineSetPoint, gaiaLineGetPoint, gaiaSetPointXYZM,
 gaiaGetPointXYZM

 \note you are responsible to destroy (before or after) any allocated LINESTRING, 
 unless you've passed ownership of the LINESTRING object to some further  
 object: in this case destroying the higher order object will implicitly 
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaLinestringPtr gaiaAllocLinestringXYZM (int vert);

/**
 Destroys a LINESTRING object

 \param ptr pointer to the LINESTRING object to be destroyed

 \sa gaiaAllocLinestring, gaiaAllocLinestringXYZ, gaiaAllocLinestringXYM,
  gaiaAllocLinestringXYZM

 \note attempting to destroy any LINESTRING object whose ownnership has already
 been transferred to some other (higher order) object is a serious
 error, and will easily cause severe memory corruption.
 */
    GAIAGEO_DECLARE void gaiaFreeLinestring (gaiaLinestringPtr ptr);

/**
 Copies coordinates between two LINESTRING objects

 \param dst destination LINESTRING [output]
 \param src origin LINESTRING [input]

 \sa gaiaCopyLinestringCoordsReverse

 \note both LINESTRING objects must have exactly the same number of points:
 if dimensions aren't the same for both objects, then the appropriate 
 conversion will be silently applied.
 */
    GAIAGEO_DECLARE void gaiaCopyLinestringCoords (gaiaLinestringPtr dst,
						   gaiaLinestringPtr src);

/**
 Copies coordinates between two LINESTRING objects in reverse order

 \param dst destination LINESTRING [output]
 \param src origin LINESTRING [input]

 \sa gaiaCopyLinestringCoords

 \note both LINESTRING objects must have exactly the same number of points:
 if dimensions aren't the same for both objects, then the appropriate 
 conversion will be silently applied.
 */
    GAIAGEO_DECLARE void gaiaCopyLinestringCoordsReverse (gaiaLinestringPtr dst,
							  gaiaLinestringPtr
							  src);

/**
 Allocates a 2D RING [XY]

 \param vert number of points [aka vertices] into the Ring

 \return the pointer to newly created RING object: NULL on failure

 \sa gaiaFreeRing, gaiaRingSetPoint, gaiaRingGetPoint, gaiaSetPoint,
 gaiaGetPoint

 \note you are responsible to destroy (before or after) any allocated RING,
 unless you've passed ownership of the RING object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaRingPtr gaiaAllocRing (int vert);

/**
 Allocates a 3D RING [XYZ]

 \param vert number of points [aka vertices] into the Ring

 \return the pointer to newly created RING object: NULL on failure

 \sa gaiaFreeRing, gaiaRingSetPoint, gaiaRingGetPoint, gaiaSetPointXYZ,
 gaiaGetPointXYZ

 \note you are responsible to destroy (before or after) any allocated RING,
 unless you've passed ownership of the RING object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaRingPtr gaiaAllocRingXYZ (int vert);

/**
 Allocates 2D RING [XYM]

 \param vert number of points [aka vertices] into the Ring

 \return the pointer to newly created RING object: NULL on failure

 \sa gaiaFreeRing, gaiaRingSetPoint, gaiaRingGetPoint, gaiaSetPointXYM,
 gaiaGetPointXYM

 \note you are responsible to destroy (before or after) any allocated RING,
 unless you've passed ownership of the RING object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaRingPtr gaiaAllocRingXYM (int vert);

/**
 Allocates a 3D RING [XYZM]

 \param vert number of points [aka vertices] into the Ring

 \return the pointer to newly created RING object: NULL on failure

 \sa gaiaFreeRing, gaiaRingSetPoint, gaiaRingGetPoint, gaiaSetPointXYZM,
 gaiaSetPointXYZM

 \note you are responsible to destroy (before or after) any allocated RING,
 unless you've passed ownership of the RING object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaRingPtr gaiaAllocRingXYZM (int vert);

/**
 Destroys a RING object

 \param ptr pointer to the RING object to be destroyed

 \sa gaiaAllocRing, gaiaAllocRingXYZ, gaiaAllocRingXYM,
  gaiaAllocRingXYZM

 \note attempting to destroy any RING object whose ownership has already
 been transferred to some other (higher order) object is a serious
 error, and will easily cause severe memory corruption.
 */
    GAIAGEO_DECLARE void gaiaFreeRing (gaiaRingPtr ptr);

/**
 Copies coordinates between two RING objects

 \param dst destination RING [output]
 \param src origin RING [input]

 \sa gaiaCopyRingCoordsReverse

 \note both RING objects must have exactly the same number of points:
 if dimensions aren't the same for both objects, then the appropriate
 conversion will be silently applied.
 */
    GAIAGEO_DECLARE void gaiaCopyRingCoords (gaiaRingPtr dst, gaiaRingPtr src);

/**
 Copies coordinates between two RING objects in reverse order

 \param dst destination RING [output]
 \param src origin RING [input]

 \sa gaiaCopyRingCoords

 \note both RING objects must have exactly the same number of points:
 if dimensions aren't the same for both objects, then the appropriate
 conversion will be silently applied.
 */
    GAIAGEO_DECLARE void gaiaCopyRingCoordsReverse (gaiaRingPtr dst,
						    gaiaRingPtr src);

/**
 Allocates a 2D POLYGON [XY]

 \param vert number of points [aka vertices] into the Exterior Ring.
 \param holes number of Interior Rings [0, if no Interior Ring is required].

 \return the pointer to newly created POLYGON object: NULL on failure

 \sa gaiaFreePolygon

 \note you are responsible to destroy (before or after) any allocated POLYGON,
 unless you've passed ownership of the POLYGON object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaPolygonPtr gaiaAllocPolygon (int vert, int holes);

/**
 Allocates a 3D POLYGON [XYZ]
    
 \param vert number of points [aka vertices] into the Exterior Ring.
 \param holes number of Interior Rings [0, if no Interior Ring is required].
    
 \return the pointer to newly created POLYGON object: NULL on failure
    
 \sa gaiaFreePolygon
    
 \note you are responsible to destroy (before or after) any allocated POLYGON,
 unless you've passed ownership of the POLYGON object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaPolygonPtr gaiaAllocPolygonXYZ (int vert, int holes);

/**
 Allocates a 2D POLYGON [XYM]
    
 \param vert number of points [aka vertices] into the Exterior Ring.
 \param holes number of Interior Rings [0, if no Interior Ring is required].
    
 \return the pointer to newly created POLYGON object: NULL on failure
    
 \sa gaiaFreePolygon
    
 \note you are responsible to destroy (before or after) any allocated POLYGON,
 unless you've passed ownership of the POLYGON object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaPolygonPtr gaiaAllocPolygonXYM (int vert, int holes);

/**
 Allocates a 3D POLYGON [XYZM]
    
 \param vert number of points [aka vertices] into the Exterior Ring.
 \param holes number of Interior Rings [may by 0, if no Interior Ring is required].
    
 \return the pointer to newly created POLYGON object: NULL on failure
    
 \sa gaiaFreePolygon
    
 \note you are responsible to destroy (before or after) any allocated POLYGON,
 unless you've passed ownership of the POLYGON object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaPolygonPtr gaiaAllocPolygonXYZM (int vert, int holes);

/**
 Allocates a POLYGON

 \param ring pointer to a valid RING object: assumed to be the Polygon's
  Exterior Ring.

 \return the pointer to newly created POLYGON object: NULL on failure

 \sa gaiaAllocRing, gaiaAllocRingXYZ, gaiaAllocRingXYM, gaiaAllocRingXYZM,
 gaiaFreePolygon

 \note you are responsible to destroy (before or after) any allocated POLYGON,
 unless you've passed ownership of the POLYGON object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object. 
 \n Ownership of passed Ring object will be transferred to the 
 Polygon object being created. 
 */
    GAIAGEO_DECLARE gaiaPolygonPtr gaiaCreatePolygon (gaiaRingPtr ring);

/**
 Destroys a POLYGON object

 \param polyg pointer to the POLYGON object to be destroyed

 \sa gaiaAllocPolygon, gaiaAllocPolygonXYZ, gaiaAllocPolygonXYM,
  gaiaAllocPolygonXYZM, gaiaCreatePolygon

 \note attempting to destroy any POLYGON object whose ownership has already
 been transferred to some other (higher order) object is a serious
 error, and will easily cause severe memory corruption.
 \n Ownership of each RING object referenced by a POLYGON object always belongs
 to the POLYGON itself, so destroying the POLYGON will surely destroy
 any related RING as well.
 */
    GAIAGEO_DECLARE void gaiaFreePolygon (gaiaPolygonPtr polyg);

/**
 Allocates a 2D Geometry [XY]

 \return the pointer to newly created Geometry object: NULL on failure

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaAllocGeomColl (void);

/**
 Allocates a 3D Geometry [XYZ]

 \return the pointer to newly created Geometry object: NULL on failure

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaAllocGeomCollXYZ (void);

/**
 Allocates a 2D Geometry [XYM]

 \return the pointer to newly created Geometry object: NULL on failure

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaAllocGeomCollXYM (void);

/**
 Allocates a 3D Geometry [XYZM]

 \return the pointer to newly created Geometry object: NULL on failure

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further
 object: in this case destroying the higher order object will implicitly
 destroy any contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaAllocGeomCollXYZM (void);

/**
 Destroys a Geometry object

 \param geom pointer to the Geometry object to be destroyed

 \sa gaiaAllocGeomColl, gaiaAllocGeomCollXYZ, gaiaAllocGeomCollXYM,
  gaiaAllocGeomCollXYZM

 \note attempting to destroy any Geometry object whose ownership has already
 been transferred to some other (higher order) object is a serious
 error, and will easily cause severe memory corruption.
 \n Ownership of each POINT, LINESTRING or POLYGON object referenced by a
 Geometry object always belongs to the Geometry itself, so destroying the 
 Geometry will surely destroy any related elementary geometry item as well.
 */
    GAIAGEO_DECLARE void gaiaFreeGeomColl (gaiaGeomCollPtr geom);

/**
 Creates a new 2D Point [XY] object into a Geometry object

 \param p pointer to the Geometry object
 \param x X coordinate of the Point to be created
 \param y X coordinate of the Point to be created

 \note ownership of the newly created POINT object belongs to the Geometry
 object.
 */
    GAIAGEO_DECLARE void gaiaAddPointToGeomColl (gaiaGeomCollPtr p, double x,
						 double y);

/**
 Creates a new 3D Point [XYZ] object into a Geometry object

 \param p pointer to the Geometry object
 \param x X coordinate of the Point to be created
 \param y X coordinate of the Point to be created
 \param z Z coordinate of the Point to be created

 \note ownership of the newly created POINT object belongs to the Geometry
 object.
 */
    GAIAGEO_DECLARE void gaiaAddPointToGeomCollXYZ (gaiaGeomCollPtr p, double x,
						    double y, double z);

/**
 Creates a new 2D Point [XYM] object into a Geometry object

 \param p pointer to the Geometry object
 \param x X coordinate of the Point to be created
 \param y X coordinate of the Point to be created
 \param m M measure of the Point to be created

 \note ownership of the newly created POINT object belongs to the Geometry
 object.
 */
    GAIAGEO_DECLARE void gaiaAddPointToGeomCollXYM (gaiaGeomCollPtr p, double x,
						    double y, double m);

/**
 Creates a new 3D Point [XYZM] object into a Geometry object

 \param p pointer to the Geometry object
 \param x X coordinate of the Point to be created
 \param y X coordinate of the Point to be created
 \param z Z coordinate of the Point to be created
 \param m M measure of the Point to be created

 \note ownership of the newly created POINT object belongs to the Geometry
 object.
 */
    GAIAGEO_DECLARE void gaiaAddPointToGeomCollXYZM (gaiaGeomCollPtr p,
						     double x, double y,
						     double z, double m);

/**
 Creates a new Linestring object into a Geometry object

 \param p pointer to the Geometry object.
 \param vert number of points [aka vertices] into the Linestring.

 \return the pointer to newly created Linestring: NULL on failure.

 \note ownership of the newly created Linestring object belongs to the Geometry object. 
 \n the newly created Linestring will have the same dimensions as the Geometry has.
 */
    GAIAGEO_DECLARE gaiaLinestringPtr
	gaiaAddLinestringToGeomColl (gaiaGeomCollPtr p, int vert);

/**
 Inserts an already existing Linestring object into a Geometry object

 \param p pointer to the Geometry object.
 \param line pointer to the Linestring object.

 \note ownership of the Linestring object will be transferred to the
 Geometry object.
 */
    GAIAGEO_DECLARE void gaiaInsertLinestringInGeomColl (gaiaGeomCollPtr p,
							 gaiaLinestringPtr
							 line);

/**
 Creates a new Polygon object into a Geometry object

 \param p pointer to the Geometry object.
 \param vert number of points [aka vertices] into the Polygon's Exterior Ring.
 \param interiors number of Interiors Rings [0, if no Interior Ring is required]

 \return the pointer to newly created Polygon: NULL on failure.

 \note ownership of the newly created Polygon object belongs to the Geometry object.
 \n the newly created Polygon will have the same dimensions as the Geometry has.
 */
    GAIAGEO_DECLARE gaiaPolygonPtr gaiaAddPolygonToGeomColl (gaiaGeomCollPtr p,
							     int vert,
							     int interiors);

/**
 Creates a new Polygon object into a Geometry object starting from an
 already existing Ring object

 \param p pointer to the Geometry object.
 \param ring pointer to the Ring object [assumed to represent to Polygon's
 Exterior Ring].

 \return the pointer to the newly created Polygon object: NULL on failure.

 \note ownership of the Ring object will be transferred to the
 Polygon object, and the Polygon object ownerships belongs to the Geometry object.
 \n the Polygon object will have the same dimensions as the Ring object has.
 */
    GAIAGEO_DECLARE gaiaPolygonPtr gaiaInsertPolygonInGeomColl (gaiaGeomCollPtr
								p,
								gaiaRingPtr
								ring);

/**
 Creates a new Interior Ring object into a Polygon object

 \param p pointer to the Polygon object.
 \param pos relative position index [first Interior Ring has index 0].
 \param vert number of points (aka vertices) into the Ring.

 \return the pointer to the newly created Ring object: NULL on failure.

 \sa gaiaAllocPolygon, gaiaAllocPolygonXYZ, gaiaAllocPolygonXYM,
 gaiaAllocPolygonXYZM

 \note ownership of the Ring object belongs to the Polygon object.
 \n the newly created Ring will have the same dimensions the Polygon has.
 */
    GAIAGEO_DECLARE gaiaRingPtr gaiaAddInteriorRing (gaiaPolygonPtr p, int pos,
						     int vert);

/**
 Inserts an already existing Ring object into a Polygon object

 \param p pointer to the Polygon object
 \param ring pointer to the Ring object

 \sa gaiaAddRingToPolygon

 \note ownership of the Ring object still remains to the calling procedure
 (a duplicated copy of the original Ring will be inserted into the Polygon).
 \n the newly created Polygon will have the same dimensions as the Ring has.
 \n if required the Polygon's Interior Rings count could be increased.
 */
    GAIAGEO_DECLARE void gaiaInsertInteriorRing (gaiaPolygonPtr p,
						 gaiaRingPtr ring);

/**
 Inserts an already existing Ring object into a Polygon object

 \param polyg pointer to the Polygon object
 \param ring pointer to the Ring object

 \sa gaiaInsertInteriorRing

 \note ownership of the Ring object will be transferred to the Polygon object.
 \n the newly created Polygon will have the same dimensions as the Ring has.
 \n if required the Polygon's Interior Rings count could be increased.
 */
    GAIAGEO_DECLARE void gaiaAddRingToPolyg (gaiaPolygonPtr polyg,
					     gaiaRingPtr ring);

/**
 Duplicates a Linestring object

 \param line pointer to Linestring object [origin].

 \return the pointer to newly created Linestring object: NULL on failure.

 \sa gaiaCloneRing, gaiaClonePolygon, gaiaCloneGeomColl,
 gaiaCloneGeomCollPoints, gaiaCloneGeomCollLinestrings, 
 gaiaCloneGeomCollPolygons, gaiaCloneLinestringSpecial

 \note the newly created object is an exact copy of the original one. 
 */
    GAIAGEO_DECLARE gaiaLinestringPtr gaiaCloneLinestring (gaiaLinestringPtr
							   line);
/**
 Duplicates a Linestring object (special)

 \param line pointer to Linestring object [origin].
 \param mode one of GAIA_SAME_ORDER or GAIA_REVERSE_ORDER.

 \return the pointer to newly created Linestring object: NULL on failure.

 \sa gaiaCloneLinestring, gaiaCloneGeomCollSpecial

 \note if GAIA_REVERSE_ORDER is specified, then any vertex into the newly created
  object will be in reverse order [first vertex will be last one, and last vertex
  will be the first one]. In any other case this function will simply default to 
  gaiaCloneLinestring. 
 */
    GAIAGEO_DECLARE gaiaLinestringPtr
	gaiaCloneLinestringSpecial (gaiaLinestringPtr line, int mode);

/**
 Duplicates a Ring object

 \param ring pointer to Ring object [origin].

 \return the pointer to newly created Ring object: NULL on failure.

 \sa gaiaCloneLinestring, gaiaClonePolygon, gaiaCloneGeomColl,
 gaiaCloneGeomCollPoints, gaiaCloneGeomCollLinestrings, 
 gaiaCloneGeomCollPolygons, gaiaCloneRingSpecial

 \note the newly created object is an exact copy of the original one. 
 */
    GAIAGEO_DECLARE gaiaRingPtr gaiaCloneRing (gaiaRingPtr ring);

/**
 Duplicates a Ring object (special)

 \param ring pointer to Ring object [origin].
 \param mode one of GAIA_SAME_ORDER or GAIA_REVERSE_ORDER.

 \return the pointer to newly created Ring object: NULL on failure.

 \sa gaiaCloneRing, gaiaClonePolygonSpecial

 \note if GAIA_REVERSE_ORDER is specified, then any vertex into the newly created
  object will be in reverse order [first vertex will be last one, and last vertex
  will be the first one]. In any other case this function will simply default to 
  gaiaCloneRing. 
 */
    GAIAGEO_DECLARE gaiaRingPtr gaiaCloneRingSpecial (gaiaRingPtr ring,
						      int mode);

/**
 Duplicates a Polygon object

 \param polyg pointer to Polygon object [origin].

 \return the pointer to newly created Polygon object: NULL on failure.

 \sa gaiaCloneLinestring, gaiaCloneRing, gaiaCloneGeomColl,
 gaiaCloneGeomCollPoints, gaiaCloneGeomCollLinestrings, 
 gaiaCloneGeomCollPolygons, gaiaClonePolygonSpecial

 \note the newly created object is an exact copy of the original one. 
 */
    GAIAGEO_DECLARE gaiaPolygonPtr gaiaClonePolygon (gaiaPolygonPtr polyg);

/**
 Duplicates a Polygon object (special)

 \param polyg pointer to Polygon object [origin].
 \param mode one of GAIA_SAME_ORDER, GAIA_REVERSE_ORDER or GAIA_LHR_ORDER.

 \return the pointer to newly created Polygon object: NULL on failure.

 \sa gaiaClonePolygon, gaiaCloneGeomCollSpecial

 \note if GAIA_REVERSE_ORDER is specified, then any Ring into the newly created
  object will be in reverse order. If GAIA_LHR_ORDER is specified instead, any
  Exterior Ring will have clockwise orientation, and any Interior Ring will have
  counter-clockwise orientation. In any other case this function will simply 
  default to gaiaClonePolygon. 
 */
    GAIAGEO_DECLARE gaiaPolygonPtr gaiaClonePolygonSpecial (gaiaPolygonPtr
							    polyg, int mode);

/**
 Duplicates a Geometry object

 \param geom pointer to Geometry object [origin].

 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaCloneLinestring, gaiaCloneRing, gaiaClonePolygon, 
 gaiaCloneGeomCollPoints, gaiaCloneGeomCollLinestrings,
 gaiaCloneGeomCollPolygons, gaiaCastGeomCollToXY, gaiaCastGeomCollToXYZ,
 gaiaCastGeomCollToXYM, gaiaCastGeomCollToXYZM, gaiaExtractPointsFromGeomColl,
 gaiaExtractLinestringsFromGeomColl, gaiaExtractPolygonsFromGeomColl,
 gaiaMergeGeometries, gaiaCloneGeomCollSpecial

 \note the newly created object is an exact copy of the original one. 
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaCloneGeomColl (gaiaGeomCollPtr geom);

/**
 Duplicates a Geometry object (special)

 \param geom pointer to Geometry object [origin].
 \param mode one of GAIA_SAME_ORDER, GAIA_REVERSE_ORDER or GAIA_LHR_ORDER.

 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaCloneLinestringSpecial, gaiaCloneRingSpecial, gaiaClonePolygonSpecial, 
 gaiaCloneGeomColl

 \note if GAIA_REVERSE_ORDER is specified, then any Linestring and/or Ring into
 the newly created object will be in reverse order. If GAIA_LHR_ORDER is specified 
  instead, any Polygong will have the Exterior Ring in clockwise orientation, and any 
  Interior Ring int counter-clockwise orientation. In any other case this function will 
  simply default to gaiaCloneGeomColl. 
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaCloneGeomCollSpecial (gaiaGeomCollPtr
							      geom, int mode);

/**
 Duplicates a Geometry object [Points only]

 \param geom pointer to Geometry object [origin].

 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaCloneLinestring, gaiaCloneRing, gaiaClonePolygon, gaiaCloneGeomColl,
 gaiaCloneGeomCollLinestrings, 
 gaiaCloneGeomCollPolygons

 \note the newly created object is an exact copy of the original one; except
 in that only Point objects will be copied.
 \n Caveat: an empty Geometry could be returned.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaCloneGeomCollPoints (gaiaGeomCollPtr
							     geom);

/**
 Duplicates a Geometry object [Linestrings only]
                                                             
 \param geom pointer to Geometry object [origin].
        
 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaCloneLinestring, gaiaCloneRing, gaiaClonePolygon, gaiaCloneGeomColl,
 gaiaCloneGeomCollPoints, gaiaCloneGeomCollPolygons

 \note the newly created object is an exact copy of the original one; except
 in that only Linestrings objects will be copied.
 \n Caveat: an empty Geometry could be returned.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr
	gaiaCloneGeomCollLinestrings (gaiaGeomCollPtr geom);

/**
 Duplicates a Geometry object [Polygons only]
 
 \param geom pointer to Geometry object [origin].
 
 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaCloneLinestring, gaiaCloneRing, gaiaClonePolygon, gaiaCloneGeomColl,
 gaiaCloneGeomCollPoints, gaiaCloneGeomCollLinestrings

 \note the newly created object is an exact copy of the original one; except
 in that only Polygons objects will be copied.
 \n Caveat: an empty Geometry could be returned.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaCloneGeomCollPolygons (gaiaGeomCollPtr
							       geom);

/**
 Duplicates a Geometry object [casting dimensions to 2D XY]

 \param geom pointer to Geometry object [origin].

 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaCloneGeomColl, gaiaCastGeomCollToXYZ,
 gaiaCastGeomCollToXYM, gaiaCastGeomCollToXYZM

 \note the newly created object is an exact copy of the original one; except
 in that any elementary item  will be casted to 2D [XY] dimensions.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaCastGeomCollToXY (gaiaGeomCollPtr geom);

/**
 Duplicates a Geometry object [casting dimensions to 3D XYZ]

 \param geom pointer to Geometry object [origin].

 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaCloneGeomColl, gaiaCastGeomCollToXY, 
 gaiaCastGeomCollToXYM, gaiaCastGeomCollToXYZM

 \note the newly created object is an exact copy of the original one; except
 in that any elementary item  will be cast to 3D [XYZ] dimensions.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaCastGeomCollToXYZ (gaiaGeomCollPtr
							   geom);

/**
 Duplicates a Geometry object [casting dimensions to 2D XYM]

 \param geom pointer to Geometry object [origin].

 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaCloneGeomColl, gaiaCastGeomCollToXY, gaiaCastGeomCollToXYZ,
 gaiaCastGeomCollToXYZM

 \note the newly created object is an exact copy of the original one; except
 in that any elementary item  will be cast to 2D [XYM] dimensions.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaCastGeomCollToXYM (gaiaGeomCollPtr
							   geom);

/**
 Duplicates a Geometry object [casting dimensions to 3D XYZM]

 \param geom pointer to Geometry object [origin].

 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaCloneGeomColl, gaiaCastGeomCollToXY, gaiaCastGeomCollToXYZ,
 gaiaCastGeomCollToXYM

 \note the newly created object is an exact copy of the original one; except
 in that any elementary item  will be cast to 3D [XYZM] dimensions.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaCastGeomCollToXYZM (gaiaGeomCollPtr
							    geom);

/**
 Gets coodinates from a Linestring's Point

 \param ln pointer to Linestring object.
 \param v relative position of Point: first Point has index 0
 \param x on completion this variable will contain the Point X coordinate.
 \param y on completion this variable will contain the Point Y coordinate.
 \param z on completion this variable will contain the Point Z coordinate.
 \param m on completion this variable will contain the Point M measure.

 \return 0 on failure: any other different value on success.

 \sa gaiaLineSetPoint, gaiaGetPoint, gaiaGetPointXYZ, gaiaGetPointXYM,
 gaiaGetPointXYZM

 \note this function perform the same identical task performed by
 gaiaGetPoint(), gaiaGetPointXYZ(), gaiaGetPointXYM() and gaiaGetPointXYZM()
 macros.
 \n using the gaiaLineGetPoint() function is a little bit slower but is
 intrinsically safest, because misused macros can easily cause severe
 memory corruption.
 \n gaiaLineGetPoint() instead will always ensure that the appropriate
 dimensions (as declared by the Linestring object) will be correctly used.
 */
    GAIAGEO_DECLARE int gaiaLineGetPoint (gaiaLinestringPtr ln, int v,
					  double *x, double *y, double *z,
					  double *m);

/**
 Sets coordinates for a Linestring's Point

 \param ln pointer to Linestring object.
 \param v relative position of Point: first Point has index 0
 \param x the Point's X coordinate.
 \param y the Point's Y coordinate.
 \param z the Point's Z coordinate.
 \param m the Point's M measure.

 \return 0 on failure: any other different value on success.

 \sa gaiaLineGetPoint, gaiaSetPoint, gaiaSetPointXYZ, gaiaSetPointXYM,
 gaiaSetPointXYZM

 \note this function perform the same identical task performed by 
 gaiaSetPoint(), gaiaSetPointXYZ(), gaiaSetPointXYM() and gaiaSetPointXYZM()
 macros.
 \n using the gaiaLineSetPoint() function is a little bit slower but is
 intrinsically safest, because misused macros can easily cause severe
 memory corruption.
 \n gaiaLineSetPoint() instead will always ensure that the appropriate 
 dimensions (as declared by the Linestring object) will be correctly used.
 */
    GAIAGEO_DECLARE int gaiaLineSetPoint (gaiaLinestringPtr ln, int v, double x,
					  double y, double z, double m);

/**
 Gets coordinates from a Ring's Point

 \param rng pointer to Ring object.
 \param v relative position of Point: first Point has index 0
 \param x on completion this variable will contain the Point X coordinate.
 \param y on completion this variable will contain the Point Y coordinate.
 \param z on completion this variable will contain the Point Z coordinate.
 \param m on completion this variable will contain the Point M measure.

 \return 0 on failure: any other different value on success.

 \sa gaiaRingSetPoint, gaiaGetPoint, gaiaGetPointXYZ, gaiaGetPointXYM,
 gaiaGetPointXYZM

 \note this function perform the same identical task performed by
 gaiaGetPoint(), gaiaGetPointXYZ(), gaiaGetPointXYM() and gaiaGetPointXYZM()
 macros.
 \n using the gaiaRingGetPoint() function is a little bit slower but is
 intrinsically safest, because misused macros can easily cause severe
 memory corruption.
 \n gaiaRingGetPoint() instead will always ensure that the appropriate
 dimensions (as declared by the Ring object) will be correctly used.
 */
    GAIAGEO_DECLARE int gaiaRingGetPoint (gaiaRingPtr rng, int v, double *x,
					  double *y, double *z, double *m);

/**
 Sets coodinates for a Ring's Point

 \param rng pointer to Ring object.
 \param v relative position of Point: first Point has index 0
 \param x the Point's X coordinate.
 \param y the Point's Y coordinate.
 \param z the Point's Z coordinate.
 \param m the Point's M measure.

 \return 0 on failure: any other different value on success.

 \sa gaiaRingGetPoint, gaiaGetPoint, gaiaGetPointXYZ, gaiaSetPointXYM,
 gaiaSetPointXYZM

 \note this function perform the same identical task performed by
 gaiaSetPoint(), gaiaSetPointXYZ(), gaiaSetPointXYM() and gaiaSetPointXYZM()
 macros.
 \n using the gaiaRingSetPoint() function is a little bit slower but is
 intrinsically safest, because misused macros can easily cause severe
 memory corruption.
 \n gaiaRingSetPoint() instead will always ensure that the appropriate
 dimensions (as declared by the Ring object) will be correctly used.
 */
    GAIAGEO_DECLARE int gaiaRingSetPoint (gaiaRingPtr rng, int v, double x,
					  double y, double z, double m);

/**
 Determines OGC dimensions for a Geometry object

 \param geom pointer to Geometry object
 
 \return OGC dimensions

 \note OGC dimensions are defined as follows:
 \li if the Geometry doesn't contain any elementary item: \b -1
 \li if the Geometry only contains Point items: \b 0
 \li if the Geometry only contains Point / Linestring items: \b 1
 \li if the Geometry contains some Polygon item: \b 2
 */
    GAIAGEO_DECLARE int gaiaDimension (gaiaGeomCollPtr geom);

/**
 Determines the corresponding Type for a Geometry object
 
 \param geom pointer to Geometry object

 \return the corresponding Geometry Type

 \note Type is one of: GAIA_POINT, GAIA_LINESTRING, GAIA_POLYGON,
 GAIA_MULTIPOINT, GAIA_MULTILINESTRING, GAIA_MULTIPOLYGON,
 GAIA_GEOMETRYCOLLECTION, GAIA_POINTZ, GAIA_LINESTRINGZ, GAIA_POLYGONZ,
 GAIA_MULTIPOINTZ, GAIA_MULTILINESTRINGZ, GAIA_MULTIPOLYGONZ,
 GAIA_GEOMETRYCOLLECTIONZ, GAIA_POINTM, GAIA_LINESTRINGM, GAIA_POLYGONM,
 GAIA_MULTIPOINTM, GAIA_MULTILINESTRINGM, GAIA_MULTIPOLYGONM,
 GAIA_GEOMETRYCOLLECTIONM, GAIA_POINTZM, GAIA_LINESTRINGZM, GAIA_POLYGONZM,
 GAIA_MULTIPOINTZM, GAIA_MULTILINESTRINGZM, GAIA_MULTIPOLYGONZM,
 GAIA_GEOMETRYCOLLECTIONZM
 \n on failure GAIA_NONE will be returned.
 */
    GAIAGEO_DECLARE int gaiaGeometryType (gaiaGeomCollPtr geom);

/**
 Determines the corresponding Type for a Geometry object

 \param geom pointer to Geometry object

 \return the corresponding Geometry Type

 \sa gaiaGeometryType

 \note Type is one of: GAIA_POINT, GAIA_LINESTRING, GAIA_POLYGON,
 GAIA_MULTIPOINT, GAIA_MULTILINESTRING, GAIA_MULTIPOLYGON,
 GAIA_GEOMETRYCOLLECTION
 \n on failure GAIA_NONE will be returned.

 \remark deprecated function (used in earlier SpatiaLite versions).
 */
    GAIAGEO_DECLARE int gaiaGeometryAliasType (gaiaGeomCollPtr geom);

/**
 Checks for empty Geometry object

 \param geom pointer to Geometry object

 \return 0 if the Geometry is empty: otherwise any other different value.

 \note an empty Geometry is a Geometry not containing any elementary
 item: i.e. no Points, no Linestrings and no Polygons at all.
 */
    GAIAGEO_DECLARE int gaiaIsEmpty (gaiaGeomCollPtr geom);

/**
 Checks for toxic Geometry object

 \param geom pointer to Geometry object

 \return 0 if the Geometry is not toxic: otherwise any other different value.

 \sa gaiaIsToxic_r, gaiaSanitize

 \note a \b toxic Geometry is a Geometry containing severely malformed
 Polygons: i.e. containing less than 4 Points.
 \n Or containing severely malformed Linestrings: i.e. containing less 
 than 2 Points.
 \n Attempting to pass any toxic Geometry to GEOS supported functions
 will easily cause a crash.\n
 not reentrant and thread unsafe.
 */
    GAIAGEO_DECLARE int gaiaIsToxic (gaiaGeomCollPtr geom);

/**
 Checks for toxic Geometry object

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param geom pointer to Geometry object

 \return 0 if the Geometry is not toxic: otherwise any other different value.

 \sa gaiaIsToxic, gaiaSanitize

 \note a \b toxic Geometry is a Geometry containing severely malformed
 Polygons: i.e. containing less than 4 Points.
 \n Or containing severely malformed Linestrings: i.e. containing less 
 than 2 Points.
 \n Attempting to pass any toxic Geometry to GEOS supported functions
 will easily cause a crash.\n
 reentrant and thread-safe.
 */
    GAIAGEO_DECLARE int gaiaIsToxic_r (const void *p_cache,
				       gaiaGeomCollPtr geom);

/**
 Checks for not-closed Rings 

 \param ring pointer to Ring object

 \return 0 if the Ring in unclosed: otherwise any other different value.

 \sa gaiaIsNotClosedRing_r, gaiaIsToxic, gaiaIsNotClosedGeomColl

 \note unclosed Rings cause GEOS supported functions to crash.
 \n SpatiaLite will always carefully check any Ring before passing it
 to GEOS, eventually silently inserting a further point required so 
 to properly close the figure.
 \n This function allows to explicitly identify any unclosed Ring.\n
 not reentrant and thread unsafe.
 */
    GAIAGEO_DECLARE int gaiaIsNotClosedRing (gaiaRingPtr ring);

/**
 Checks for not-closed Rings 

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param ring pointer to Ring object

 \return 0 if the Ring in unclosed: otherwise any other different value.

 \sa gaiaIsNotClosedRing, gaiaIsToxic, gaiaIsNotClosedGeomColl

 \note unclosed Rings cause GEOS supported functions to crash.
 \n SpatiaLite will always carefully check any Ring before passing it
 to GEOS, eventually silently inserting a further point required so 
 to properly close the figure.
 \n This function allows to explicitly identify any unclosed Ring.\n
 reentrant and thread-safe.
 */
    GAIAGEO_DECLARE int gaiaIsNotClosedRing_r (const void *p_data,
					       gaiaRingPtr ring);

/**
 Checks for not-closed Rings in a Geometry object

 \param geom pointer to Geometry object

 \return 0 if the Geometry has no unclosed Rings: otherwise any other different value.

 \sa gaiaIsNotClosedGeomColl_r, gaiaIsToxic, gaiaIsNotClosedRing

 \note This function allows to explicitly identify any Geometry containing
 at least one unclosed Ring.\n
 not reentrant and thread unsafe.
 */
    GAIAGEO_DECLARE int gaiaIsNotClosedGeomColl (gaiaGeomCollPtr geom);

/**
 Checks for not-closed Rings in a Geometry object

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param geom pointer to Geometry object

 \return 0 if the Geometry has no unclosed Rings: otherwise any other different value.

 \sa gaiaIsNotClosedGeomColl, gaiaIsToxic, gaiaIsNotClosedRing

 \note This function allows to explicitly identify any Geometry containing
 at least one unclosed Ring.\n
 reentrant and thread-safe.
 */
    GAIAGEO_DECLARE int gaiaIsNotClosedGeomColl_r (const void *p_data,
						   gaiaGeomCollPtr geom);

/**
 Attempts to sanitize a possibly malformed Geometry object

 \param org pointer to Geometry object.

 \return the pointer to newly created Geometry: NULL on failure.

 \sa gaiaIsToxic

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaSanitize()
 \n the output Geometry will surely have:
 \li no repeated Points on Linestrings or Rings (i.e. consecutive Points 
 sharing exactly the same coordinates): any repeated Point will be suppressed,
 simply leaving only the first occurrence.
 \li proper Ring closure: for sure any Ring will have exactly coinciding
 first and last Points.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaSanitize (gaiaGeomCollPtr org);


/**
 Attempts to resolve a (Multi)Linestring from a Geometry object

 \param geom pointer to Geometry object.
 \param force_multi: 0 if the returned Geometry could represent a Linestring:
 any other value if casting to MultiLinestring is required unconditionally.

 \return the pointer to newly created Geometry: NULL on failure.

 \sa gaiaDissolveSegments, gaiaDissolvePoints

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaLinearize()
 \n the input Geometry is expected to contain Polygons only: then any Ring 
 will be transformed into the corresponding Linestring.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaLinearize (gaiaGeomCollPtr geom,
						   int force_multi);

/**
 Attempts to resolve a collection of Segments from a Geometry object

 \param geom pointer to Geometry object.

 \return the pointer to newly created Geometry: NULL on failure.

 \sa gaiaLinearize, gaiaDissolvePoints

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaDissolveSegments()
 \n the input Geometry can be of any arbitrary type:
 \li any Point will be copied untouched.
 \li any Linestring will be dissolved into Segments.
 \li any Ring will be dissolved into Segments.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaDissolveSegments (gaiaGeomCollPtr geom);

/** 
 Attempts to resolve a collection of Points from a Geometry object

 \param geom pointer to Geometry object.
 
 \return the pointer to newly created Geometry: NULL on failure.

 \sa gaiaLinearize, gaiaDissolveSegments

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaDissolvePoints()
 \n the input Geometry can be of any arbitrary type:
 \li any Point will be copied untouched.
 \li any Linestring will be dissolved into sparse Points.
 \li any Ring will be dissolved into sparse Points.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaDissolvePoints (gaiaGeomCollPtr geom);

/**
 Extracts any Point from a Geometry object

 \param geom pointer to Geometry object

 \return the pointer to newly created Geometry: NULL on failure.

 \sa gaiaExtractLinestringsFromGeomColl,
 gaiaExtractPolygonsFromGeomColl, gaiaCloneGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaExtractPointsFromGeomColl()
 \n the newly created Geometry will contain any Point contained into the
 input Geometry.
 \n if the input Geometry doesn't contains any Point, then NULL will be returned.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr
	gaiaExtractPointsFromGeomColl (gaiaGeomCollPtr geom);

/**
 Extracts any Linestring from a Geometry object

 \param geom pointer to Geometry object

 \return the pointer to newly created Geometry: NULL on failure.

 \sa gaiaExtractPointsFromGeomColl, gaiaExtractPolygonsFromGeomColl, 
 gaiaCloneGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaExtractLinestringsFromGeomColl()
 \n the newly created Geometry will contain any Linestring contained into the
 input Geometry.
 \n if the input Geometry doesn't contains any Linestring, then NULL will be returned.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr
	gaiaExtractLinestringsFromGeomColl (gaiaGeomCollPtr geom);

/**
 Extracts any Polygon from a Geometry object
        
 \param geom pointer to Geometry object

 \return the pointer to newly created Geometry: NULL on failure.

 \sa gaiaExtractPointsFromGeomColl, gaiaExtractLinestringsFromGeomColl,
 gaiaCloneGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaExtractPolygonsFromGeomColl()
 \n the newly created Geometry will contain any Polygon contained into the
 input Geometry.
 \n if the input Geometry doesn't contains any Polygon, then NULL will be returned.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr
	gaiaExtractPolygonsFromGeomColl (gaiaGeomCollPtr geom);

/**
 Merges two Geometry objects into a single one

 \param geom1 pointer to first Geometry object.
 \param geom2 pointer to second Geometry object.

 \return the pointer to newly created Geometry: NULL on failure.

 \sa gaiaMergeGeometries_r, gaiaCloneGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaMergeGeometries()
 \n the newly created Geometry will contain any Point, Linestring and/or
 Polygon contained in both input Geometries.\n
 not reentrant and thread unsafe.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaMergeGeometries (gaiaGeomCollPtr geom1,
							 gaiaGeomCollPtr geom2);

/**
 Merges two Geometry objects into a single one

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param geom1 pointer to first Geometry object.
 \param geom2 pointer to second Geometry object.

 \return the pointer to newly created Geometry: NULL on failure.

 \sa gaiaMergeGeometries, gaiaCloneGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaMergeGeometries()
 \n the newly created Geometry will contain any Point, Linestring and/or
 Polygon contained in both input Geometries.\n
 reentrant and thread-safe.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaMergeGeometries_r (const void *p_cache,
							   gaiaGeomCollPtr
							   geom1,
							   gaiaGeomCollPtr
							   geom2);

/**
 Return a GeometryCollection containing elements matching the specified range of measures

 \param geom pointer to Geometry object
 \param m_start range of measures: start value
 \param m_end range of measures: end value

 \return the pointer to newly created Geometry: NULL on failure.

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry created by gaiaLocateBetweenMeasures()
 \n the newly created Geometry will contain Points and/or Linestrings.
 \n if the input Geometry has no M dimension then NULL will be returned.
 \n if the input Geometry doesn't contains any point/vertex corresponding to the
 required range of measures then NULL will be returned.
 \n if the input Geometry contains any Polygon (or is a GeometryCollection) then
 NULL will be returned.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr
	gaiaLocateBetweenMeasures (gaiaGeomCollPtr geom, double m_start,
				   double m_end);

/**
 Measures the geometric length for a Linestring or Ring

 \param dims dimensions: one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M or GAIA_XY_ZM
 \param coords pointed to COORD mem-array
 \param vert number of Points (aka Vertices) within the COORD mem-array

 \return the calculated geometric length

 \sa gaiaGeomCollLength

 \note \b dims, \b coords and \b vert are usually expected to correspond to
 \b DimensionModel, \b Coords and \b Points members from a gaiaLinestringStruct
 or gaiaRingStruct

 \remark internal method: doesn't require any GEOS support.
 */
    GAIAGEO_DECLARE double gaiaMeasureLength (int dims, double *coords,
					      int vert);

/**
 Measures the geometric area for a Ring object

 \param ring pointer to Ring object

 \return the calculated geometric area

 \sa gaiaGeomCollArea

 \remark internal method: doesn't require any GEOS support.
 */
    GAIAGEO_DECLARE double gaiaMeasureArea (gaiaRingPtr ring);

/**
 Determines the Centroid for a Ring object

 \param ring pointer to Ring object.
 \param rx on completion this variable will contain the centroid X coordinate.
 \param ry on completion this variable will contain the centroid Y coordinate.

 \sa gaiaGeomCollCentroid

 \remark internal method: doesn't require any GEOS support.
 */
    GAIAGEO_DECLARE void gaiaRingCentroid (gaiaRingPtr ring, double *rx,
					   double *ry);

/**
 Determines the direction for a Ring object

 \param p pointer to Ring object

 \return 0 if the ring has counter-clockwise direction; any other different
 value for clockwise direction.
 */
    GAIAGEO_DECLARE void gaiaClockwise (gaiaRingPtr p);

/**
 Check if a Point lays on a Ring surface

 \param ring pointer to Ring object
 \param pt_x Point X coordinate
 \param pt_y Point Y coordinate

 \return 0 if false: any other value if true
 */
    GAIAGEO_DECLARE int gaiaIsPointOnRingSurface (gaiaRingPtr ring, double pt_x,
						  double pt_y);

/**
 Checks if a Point lays on a Polygon surface

 \param polyg pointer to Polygon object
 \param x Point X coordinate
 \param y Point Y coordinate

 \return 0 if false: any other value if true
 */
    GAIAGEO_DECLARE int gaiaIsPointOnPolygonSurface (gaiaPolygonPtr polyg,
						     double x, double y);

/**
 Computes the minimum distance between a Point and a Linestring or Ring

 \param x0 Point X coordinate
 \param y0 Point Y coordinate
 \param dims dimensions: one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M or GAIA_XY_ZM
 \param coords pointed to COORD mem-array
 \param vert number of Points (aka Vertices) within the COORD mem-array

 \return the calculated minumum distance.

 \note \b dims, \b coords and \b vert are usually expected to correspond to
 \b DimensionModel, \b Coords and \b Points members from a gaiaLinestringStruct
 or gaiaRingStruct
 */
    GAIAGEO_DECLARE double gaiaMinDistance (double x0, double y0,
					    int dims, double *coords, int vert);

/**
 Determines the intesection Point between two Segments

 \param x0 on completion this variable will contain the Intersection X coord
 \param y0 on completion this variable will contain the Intersection Y coord
 \param x1 start Point X of first Segment
 \param y1 start Point Y of first Segment
 \param x2 end Point X of first Segment
 \param y2 end Point Y of first Segment
 \param x3 start Point X of second Segment
 \param y3 start Point Y of second Segment
 \param x4 end Point X of second Segment
 \param y4 end Point Y of second Segment

 \return 0 if the Segments doesn't intersect at all: any other value on
 success.
 */
    GAIAGEO_DECLARE int gaiaIntersect (double *x0, double *y0, double x1,
				       double y1, double x2, double y2,
				       double x3, double y3, double x4,
				       double y4);

/**
 Shifts any coordinate within a Geometry object

 \param geom pointer to Geometry object.
 \param shift_x X axis shift factor.
 \param shift_y Y axis shift factor.

 \sa gaiaScaleCoords, gaiaRotateCoords, gaiaReflectCoords, gaiaSwapCoords,
     gaiaShiftCoords3D, gaiaShiftLongitude
 */
    GAIAGEO_DECLARE void gaiaShiftCoords (gaiaGeomCollPtr geom, double shift_x,
					  double shift_y);

/**
 Shifts any coordinate within a 3D Geometry object

 \param geom pointer to Geometry object.
 \param shift_x X axis shift factor.
 \param shift_y Y axis shift factor.
 \param shift_z Z axis shift factor.

 \sa gaiaScaleCoords, gaiaRotateCoords, gaiaReflectCoords, gaiaSwapCoords,
     gaiaShiftCoords, gaiaShiftLongitude, gaiaNormalizeLonLat
 */
    GAIAGEO_DECLARE void gaiaShiftCoords3D (gaiaGeomCollPtr geom,
					    double shift_x, double shift_y,
					    double shift_z);

/**
 Shifts negative longitudes

 \param geom pointer to Geometry object.

 \sa gaiaShiftCoords, gaiaShiftCoords3D, gaiaNormalizeLonLat

 \note only intended for geographic (longitude/latitude) coordinates.
 Negative longitudes (-180/0) will be shifted by 360, thus allowing
 to represent longitudes in the 0/360 range and effectively crossing 
 the International Date Line.
 
 */
    GAIAGEO_DECLARE void gaiaShiftLongitude (gaiaGeomCollPtr geom);

/**
 Shifts any coordinate to within the "normal range" of longitude and
 latitude values (-180.0 to 180.0 longitude and -90.0 to 90.0 latitude).

 \param geom pointer to Geometry object.

 \sa gaiaScaleCoords, gaiaRotateCoords, gaiaReflectCoords, gaiaSwapCoords,
     gaiaShiftCoords3D, gaiaShiftLongitude
 */
    GAIAGEO_DECLARE void gaiaNormalizeLonLat (gaiaGeomCollPtr geom);


/**
 Scales any coordinate within a Geometry object

 \param geom pointer to Geometry object.
 \param scale_x X axis scale factor.
 \param scale_y Y axis scale factor.

 \sa gaiaShiftCoords, gaiaRotateCoords, gaiaReflectCoords, gaiaSwapCoords
 */
    GAIAGEO_DECLARE void gaiaScaleCoords (gaiaGeomCollPtr geom, double scale_x,
					  double scale_y);

/**
 Rotates any coordinate within a Geometry object

 \param geom pointer to Geometry object.
 \param angle rotation angle [expressed in Degrees].

 \sa gaiaShiftCoords, gaiaScaleCoords, gaiaReflectCoords, gaiaSwapCoords
 */
    GAIAGEO_DECLARE void gaiaRotateCoords (gaiaGeomCollPtr geom, double angle);

/**
 Reflects any coordinate within a Geometry object

 \param geom pointer to Geometry object.
 \param x_axis if set to 0, no X axis reflection will be applied:
 otherwise the X axis will be reflected.
 \param y_axis if set to 0, no Y axis reflection will be applied:
 otherwise the Y axis will be reflected.

 \sa gaiaShiftCoords, gaiaScaleCoords, gaiaRotateCoords, gaiaSwapCoords
 */
    GAIAGEO_DECLARE void gaiaReflectCoords (gaiaGeomCollPtr geom, int x_axis,
					    int y_axis);

/**
 Swaps any coordinate within a Geometry object

 \param geom pointer to Geometry object.

 \sa gaiaShiftCoords, gaiaScaleCoords, gaiaRotateCoords, gaiaReflectCoords

 \note the X and Y axes will be swapped.
 */
    GAIAGEO_DECLARE void gaiaSwapCoords (gaiaGeomCollPtr geom);

/**
 Checks if two Linestring objects are equivalent

 \param line1 pointer to first Linestring object.
 \param line2 pointer to second Linestring object.

 \return 0 if false: any other different value if true

 \sa gaiaPolygonEquals

 \note two Linestrings objects are assumed to be equivalent if exactly
 \remark deprecated function (used in earlier SpatiaLite versions).
 the same Points are found in both them.
 */
    GAIAGEO_DECLARE int gaiaLinestringEquals (gaiaLinestringPtr line1,
					      gaiaLinestringPtr line2);

/**
 Checks if two Polygons objects are equivalent

 \param polyg1 pointer to first Polygon object.
 \param polyg2 pointer to second Polygon object.

 \return 0 if false: any other different value if true

 \sa gaiaLinestringEquals

 \note two Polygon objects are assumed to be equivalent if exactly 
 the same Points are found in both them.

 \remark deprecated function (used in earlier SpatiaLite versions).
 */
    GAIAGEO_DECLARE int gaiaPolygonEquals (gaiaPolygonPtr polyg1,
					   gaiaPolygonPtr polyg2);

/**
 Retrieves Geodesic params for an Ellipsoid definition

 \param name text string identifying an Ellipsoid definition.
 \param a on completion this variable will contain the first geodesic param.
 \param b on completion this variable will contain the second geodesic param.
 \param rf on completion this variable will contain the third geodesic param.

 \return 0 on failure: any other value on success.

 \sa gaiaGreatCircleDistance, gaiaGeodesicDistance,
 gaiaGreatCircleTotalLength, gaiaGeodesicTotalLength

 \note supported Ellipsoid definitions are: \b MERIT, \b SGS85, \b GRS80,
 \b IAU76, \b airy, \b APL4.9, \b NWL9D, \b mod_airy, \b andrae, \b aust_SA,
 \b GRS67, \b bessel, \b bess_nam, \b clrk66, \b clrk80, \b CPM, \b delmbr, 
 \b engelis, \b evrst30, \b evrst48, \b evrst56, \b evrst69, \b evrstSS, 
 \b fschr60
 */
    GAIAGEO_DECLARE int gaiaEllipseParams (const char *name, double *a,
					   double *b, double *rf);

/**
 Calculates the Great Circle Distance between between two Points

 \param a first geodesic parameter.
 \param b second geodesic parameter.
 \param lat1 Latitude of first Point.
 \param lon1 Longitude of first Point.
 \param lat2 Latitude of second Point.
 \param lon2 Longitude of second Point.

 \return the calculated Great Circle Distance.
 
 \sa gaiaEllipseParams, gaiaGeodesicDistance,
 gaiaGreatCircleTotalLength, gaiaGeodesicTotalLength

 \note the returned distance is expressed in Kilometers.
 \n the Great Circle method is less accurate but fastest to be calculated.
 */
    GAIAGEO_DECLARE double gaiaGreatCircleDistance (double a, double b,
						    double lat1, double lon1,
						    double lat2, double lon2);

/**
 Calculates the Geodesic Distance between between two Points

 \param a first geodesic parameter.
 \param b second geodesic parameter.
 \param rf third geodesic parameter.
 \param lat1 Latitude of first Point.
 \param lon1 Longitude of first Point.
 \param lat2 Latitude of second Point.
 \param lon2 Longitude of second Point.

 \return the calculated Geodesic Distance.

 \sa gaiaEllipseParams, gaiaGreatCircleDistance, gaiaGreatCircleTotalLength,
 gaiaGeodesicTotalLength

 \note the returned distance is expressed in Kilometers.
 \n the Geodesic method is much more accurate but slowest to be calculated.
 */
    GAIAGEO_DECLARE double gaiaGeodesicDistance (double a, double b, double rf,
						 double lat1, double lon1,
						 double lat2, double lon2);

/**
 Calculates the Great Circle Total Length for a Linestring / Ring

 \param a first geodesic parameter.
 \param b second geodesic parameter.
 \param dims dimensions: one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M or GAIA_XY_ZM
 \param coords pointed to COORD mem-array
 \param vert number of Points (aka Vertices) within the COORD mem-array

 \return the calculated Great Circle Total Length.

 \sa gaiaEllipseParams, gaiaGreatCircleDistance, gaiaGeodesicDistance,
 gaiaGeodesicTotalLength

 \note the returned length is expressed in Kilometers.
 \n the Great Circle method is less accurate but fastest to be calculated.
 \n \b dims, \b coords and \b vert are usually expected to correspond to
 \b DimensionModel, \b Coords and \b Points members from a gaiaLinestringStruct
 or gaiaRingStruct
 */
    GAIAGEO_DECLARE double gaiaGreatCircleTotalLength (double a, double b,
						       int dims, double *coords,
						       int vert);

/**
 Calculates the Geodesic Total Length for a Linestring / Ring

 \param a first geodesic parameter.
 \param b second geodesic parameter.
 \param rf third geodesic parameter.
 \param dims dimensions: one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M or GAIA_XY_ZM
 \param coords pointed to COORD mem-array
 \param vert number of Points (aka Vertices) within the COORD mem-array

 \return the calculated Geodesic Total Length.

 \sa gaiaEllipseParams, gaiaGreatCircleDistance, gaiaGeodesicDistance,
 gaiaGreatCircleTotalLength

 \note the returned length is expressed in Kilometers.
 \n the Geodesic method is much more accurate but slowest to be calculated.
 \n \b dims, \b coords and \b vert are usually expected to correspond to
 \b DimensionModel, \b Coords and \b Points members from a gaiaLinestringStruct
 or gaiaRingStruct
 */
    GAIAGEO_DECLARE double gaiaGeodesicTotalLength (double a, double b,
						    double rf, int dims,
						    double *coords, int vert);

/**
 Convert a Length from a Measure Unit to another

 \param value the length measure to be converted.
 \param unit_from original Measure Unit.
 \param unit_to converted Measure Unit.
 \param cvt on completion this variable will contain the converted length 
 measure. 

 \note supported Measu Units are: GAIA_KM, GAIA_M, GAIA_DM, GAIA_CM, GAIA_MM,
 GAIA_KMI, GAIA_IN, GAIA_FT, GAIA_YD, GAIA_MI, GAIA_FATH, GAIC_CH, GAIA_LINK,
 GAIA_US_IN, GAIA_US_FT, GAIA_US_YD, GAIA_US_CH, GAIA_US_MI, GAIA_IND_YD,
 GAIA_IND_FT, GAIA_IND_CH
 */
    GAIAGEO_DECLARE int gaiaConvertLength (double value, int unit_from,
					   int unit_to, double *cvt);

/**
 Creates a Circle (Linestring) Geometry

 \param center_x center point X coordinate.
 \param center_y center point Y coordinate.
 \param radius the circle's radius.
 \param step angular distance (in degrees) between points on the circumference.

 \sa gaiaMakeArc, gaiaMakeEllipse, gaiaMakeEllipticArc

 \note simply a convenience method defaulting to gaiaMakeEllipse
 with both axes set to radius value
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaMakeCircle (double center_x,
						    double center_y,
						    double radius, double step);

/**
 Creates an Ellipse (Linestring) Geometry

 \param center_x center point X coordinate.
 \param center_y center point Y coordinate.
 \param x_axis the ellipses's X axis.
 \param y_axis the ellipses's Y axis.
 \param step angular distance (in degrees) between points on the ellipse.

 \sa gaiaMakeEllipticArc, gaiaMakeCircle, gaiaMakeArc
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaMakeEllipse (double center_x,
						     double center_y,
						     double x_axis,
						     double y_axis,
						     double step);

/**
 Creates a Circular Arc (Linestring) Geometry

 \param center_x center point X coordinate.
 \param center_y center point Y coordinate.
 \param radius the circle's radius.
 \param start the start angle (in degrees).
 \param start the stop angle (in degrees).
 \param step angular distance (in degrees) between points on the circumference.

 \sa gaiaMakeCircle, gaiaMakeEllipse, gaiaMakeEllipticArc

 \note simply a convenience method defaulting to gaiaMakeEllipticArc
 with both axes set to radius value
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaMakeArc (double center_x,
						 double center_y, double radius,
						 double start, double stop,
						 double step);

/**
 Creates an Elliptic Arc (Linestring) Geometry

 \param center_x center point X coordinate.
 \param center_y center point Y coordinate.
 \param x_axis the ellipses's X axis.
 \param y_axis the ellipses's Y axis.
 \param start the start angle (in degrees).
 \param start the stop angle (in degrees).
 \param step angular distance (in degrees) between points on the ellipse.

 \sa gaiaMakeCircle, gaiaMakeEllipse, gaiaMakeEllipticArc
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaMakeEllipticArc (double center_x,
							 double center_y,
							 double x_axis,
							 double y_axis,
							 double start,
							 double stop,
							 double step);

/**
 Creates a Polygon from closed Linestrings

 \param exterior a closed Linestring assumed to represent the Exterior Ring.
 \param interiors one (or more than one) clsed Linestrings assumed to represent 
 all Interior Rings (could be a Linstring or a MultiLinestring).\n
 NULL if there are no Interior Rings at all.

 \sa gaiaPolygonize

 \note this method will simply check if all the received Linestrings are 
 closed, but it could possibly return an invalid Polygon if there is any 
 topology inconsistency between the exterior and interior rings.
 You are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaPolygonize()\n
 not reentrant and thread unsafe.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaMakePolygon (gaiaGeomCollPtr exterior,
						     gaiaGeomCollPtr interiors);

#ifdef __cplusplus
}
#endif

#endif				/* _GG_CORE_H */
