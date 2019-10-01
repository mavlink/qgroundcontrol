/*
 gg_dynamic.h -- Gaia common support for geometries: DynamicLine functions
  
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
 \file gg_dynamic.h

 Geometry handling functions: DynamicLine handling
 */

#ifndef _GG_DYNAMIC
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_DYNAMIC
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* function prototypes */

/**
 Creates a new dynamicly growing line/ring object

 \return the pointer to newly created object

 \sa gaiaCreateDynamicLine, gaiaFreeDynamicLine

 \note you are responsible to destroy (before or after) any allocated 
 dynamically growing line/ring object.
 */
    GAIAGEO_DECLARE gaiaDynamicLinePtr gaiaAllocDynamicLine (void);

/**
 Destroys a dynamically growing line/ring object

 \param p pointer to object to be destroyed

 \sa gaiaAllocDynamicLine
 */
    GAIAGEO_DECLARE void gaiaFreeDynamicLine (gaiaDynamicLinePtr p);

/**
 Appends a new 2D Point [XY] at the end of a dynamically growing line/ring
 object

 \param p pointer to the dynamically growing line/ring object.
 \param x X coordinate of the Point
 \param y Y coordinate of the Point
 
 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr
	gaiaAppendPointToDynamicLine (gaiaDynamicLinePtr p, double x, double y);

/**
 Appends a new 3D Point [XYZ] at the end of a dynamically growing line/ring 
 object

 \param p pointer to the dynamically growing line/ring object.
 \param x X coordinate of the Point
 \param y Y coordinate of the Point
 \param z Z coordinate of the Point
  
 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr
	gaiaAppendPointZToDynamicLine (gaiaDynamicLinePtr p, double x, double y,
				       double z);

/**
 Appends a new 2D Point [XYM] at the end of a dynamically growing line/ring 
 object

 \param p pointer to the dynamically growing line/ring object.
 \param x X coordinate of the Point
 \param y Y coordinate of the Point
 \param m M measure of the Point
  
 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr
	gaiaAppendPointMToDynamicLine (gaiaDynamicLinePtr p, double x, double y,
				       double m);

/**
 Appends a new 3D Point [XYZM] at the end of a dynamically growing line/ring 
 object

 \param p pointer to the dynamically growing line/ring object.
 \param x X coordinate of the Point
 \param y Y coordinate of the Point
 \param z Z coordinate of the Point
 \param m M measure of the Point
  
 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr
	gaiaAppendPointZMToDynamicLine (gaiaDynamicLinePtr p, double x,
					double y, double z, double m);

/**
 Appends a new 2D Point [XY] before the first one of a dynamically growing 
 line/ring object

 \param p pointer to the dynamically growing line/ring object.
 \param x X coordinate of the Point
 \param y Y coordinate of the Point
  
 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr
	gaiaPrependPointToDynamicLine (gaiaDynamicLinePtr p, double x,
				       double y);

/**
 Appends a new 3D Point [XYZ] before the first one of a dynamically growing
 line/ring object

 \param p pointer to the dynamically growing line/ring object.
 \param x X coordinate of the Point
 \param y Y coordinate of the Point
 \param z Z coordinate of the Point
 
 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr
	gaiaPrependPointZToDynamicLine (gaiaDynamicLinePtr p, double x,
					double y, double z);

/**
 Appends a new 2D Point [XYM] before the first one of a dynamically growing
 line/ring object

 \param p pointer to the dynamically growing line/ring object.
 \param x X coordinate of the Point
 \param y Y coordinate of the Point
 \param m M measure of the Point
 
 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr
	gaiaPrependPointMToDynamicLine (gaiaDynamicLinePtr p, double x,
					double y, double m);

/**
 Appends a new 3D Point [XYZM] before the first one of a dynamically growing
 line/ring object

 \param p pointer to the dynamically growing line/ring object.
 \param x X coordinate of the Point
 \param y Y coordinate of the Point
 \param z Z coordinate of the Point
 \param m M measure of the Point
 
 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr
	gaiaPrependPointZMToDynamicLine (gaiaDynamicLinePtr p, double x,
					 double y, double z, double m);

/**
 Appends a new 2D Point [XY] immediately after the given Point into a
 dynamically growing line/ring object

 \param p pointer to the dynamically growing line/ring object.
 \param pt pointer to the given Point.
 \param x X coordinate of the Point to be appended
 \param y Y coordinate of the Point to be appended

 \sa gaiaDynamicLiceInsertBefore

 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr gaiaDynamicLineInsertAfter (gaiaDynamicLinePtr
							     p, gaiaPointPtr pt,
							     double x,
							     double y);

/**
 Appends a new 2D Point [XY] immediately before the given Point into a
 dynamically growing line/ring object

 \param p pointer to the dynamically growing line/ring object.
 \param pt pointer to the given Point.
 \param x X coordinate of the Point to be appended
 \param y Y coordinate of the Point to be appended

 \sa gaiaDynamicLiceInsertBeforeAfter

 \return the pointer to newly created Point
 */
    GAIAGEO_DECLARE gaiaPointPtr gaiaDynamicLineInsertBefore (gaiaDynamicLinePtr
							      p,
							      gaiaPointPtr pt,
							      double x,
							      double y);

/**
 Removes a given Point from a dynamically growing line/ring object

 \param p pointer to dynamically growing line/ring object.
 \param pt pointer to given Point.

 \note the given Point (referenced by its address) will be removed from
 the dynamically growin line/ring object.
 \n the given Point will be then implicitly destroyed.
 */
    GAIAGEO_DECLARE void gaiaDynamicLineDeletePoint (gaiaDynamicLinePtr p,
						     gaiaPointPtr pt);

/**
 Duplicates a dynamically growing line/ring object

 \param org pointer to dynamically growing line/ring object [origin].

 \return the pointer to newly created dynamic growing line/ring object:
 NULL on failure.

 \note the newly created object is an exact copy of the original one.
 */
    GAIAGEO_DECLARE gaiaDynamicLinePtr gaiaCloneDynamicLine (gaiaDynamicLinePtr
							     org);

/**
 Duplicates and reverts a dynamically growing line/ring object

 \param org pointer to dynamically growing line/ring object [origin].

 \return the pointer to newly created dynamic growing line/ring object:
 NULL on failure.

 \note the newly created object is an exact copy of the origina one, except
 in that direction is reverted.
 \n i.e. first inpunt point becomes last output point, and last input point
 becomes first output point.
 */
    GAIAGEO_DECLARE gaiaDynamicLinePtr
	gaiaReverseDynamicLine (gaiaDynamicLinePtr org);

/**
 Cuts a dynamically growing line/ring in two halves, using a given
 cut point

 \param org pointer to the input object [the line to be split].
 \param point pointer to given cut point.
 
 \return the pointer to newly created dynamic growing line/ring object:
 NULL on failure.

 \sa gaiaDynamicLineSplitAfter

 \note the newly created object will contain a line going from the orginal
 first point to the cut point [excluded].
 \n on completion the orginal line will be reduced, going from the cut
 point [included] to the original last point.
 */
    GAIAGEO_DECLARE gaiaDynamicLinePtr
	gaiaDynamicLineSplitBefore (gaiaDynamicLinePtr org, gaiaPointPtr point);

/**
 Cuts a dynamically growing line/ring in two halves, using a given
 cut point 

 \param org pointer to the input object [the line to be split]. 
 \param point pointer to given cut point.

 \return the pointer to newly created dynamic growing line/ring object:
 NULL on failure.

 \sa gaiaDynamicLineSplitBefore

 \note the newly created object will contain a line going from the orginal
 first point to the cut point [included].
 \n on completion the orginal line will be reduced, going from the cut
 point [excluded] to the original last point.
 */
    GAIAGEO_DECLARE gaiaDynamicLinePtr
	gaiaDynamicLineSplitAfter (gaiaDynamicLinePtr org, gaiaPointPtr point);

/**
 Merges two dynamically growing line/ring object into a single one

 \param org pointer to the first input object [first line].
 \param point pointer to the reference Point object.
 \param toJoin pointer to the second input object [second line].

 \return the pointer to newly created dynamically growing line/ring object 
 [merged line]: NULL on failure.

 \sa gaiaDynamicLineJoinBefore

 \note the reference Point must exists into the first line: the second line
 will then be inserted immediately after the reference Point.
 \n The newly created object will represent the resulting merged line:
 \n both input objects remain untouched.
 */
    GAIAGEO_DECLARE gaiaDynamicLinePtr
	gaiaDynamicLineJoinAfter (gaiaDynamicLinePtr org, gaiaPointPtr point,
				  gaiaDynamicLinePtr toJoin);

/**
 Merges two dynamically growing line/ring object into a single one

 \param org pointer to the first input object [first line].
 \param point pointer to the reference Point object.
 \param toJoin pointer to the second input object [second line].

 \return the pointer to newly created dynamically growing line/ring object 
 [merged line]: NULL on failure.

 \sa gaiaDynamicLineJoinAfter

 \note the reference Point must exists into the first line: the second line 
 will then be inserted immediately before the reference Point.
 \n The newly created object will represent the resulting merged line: 
 \n both input objects remain untouched.
 */
    GAIAGEO_DECLARE gaiaDynamicLinePtr
	gaiaDynamicLineJoinBefore (gaiaDynamicLinePtr org, gaiaPointPtr point,
				   gaiaDynamicLinePtr toJoin);

/**
 Finds a Point within a dymically growing line/ring object [by coords]

 \param p pointer to dymamically line/ring object.
 \param x Point X coordinate.
 \param y Point Y coordinate.

 \return the pointer to the corresponding Point object: NULL on failure.

 \sa gaiaDynamicLineFindByPos

 \note if the line object contains more Points sharing the same coordinates,
 a reference to the first one found will be returned.
 */
    GAIAGEO_DECLARE gaiaPointPtr gaiaDynamicLineFindByCoords (gaiaDynamicLinePtr
							      p, double x,
							      double y);

/**
 Finds a Point within a dymically growing line/ring object [by position]

 \param p pointer to dymamically line/ring object.
 \param pos relative position [first Point has index 0].

 \return the pointer to the corresponding Point object: NULL on failure.

 \sa gaiaDynamicLineFindByCoords
 */
    GAIAGEO_DECLARE gaiaPointPtr gaiaDynamicLineFindByPos (gaiaDynamicLinePtr p,
							   int pos);

/**
 Creates a new dynamicly growing line/ring object

 \param coords an array of COORDs, any dimension [XY, XYZ, XYM, XYZM]
 \param points number of points [aka vertices] into the array

 \return the pointer to newly created object 

 \sa gaiaAllocDynamicLine, gaiaFreeDynamicLine, gaiaLinestringStruct,
 gaiaRingStruct

 \note you are responsible to destroy (before or after) any allocated 
 dynamically growing line/ring object. 
 \n The COORDs array is usually expected to be one found within a
 gaiaLinestring or gaiaRing object.
 */
    GAIAGEO_DECLARE gaiaDynamicLinePtr gaiaCreateDynamicLine (double *coords,
							      int points);

#ifdef __cplusplus
}
#endif

#endif				/* _GG_DYNAMIC */
