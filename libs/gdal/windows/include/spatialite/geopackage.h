/*

    GeoPackage extensions for SpatiaLite / SQLite
     
    version 4.3, 2015 June 29
 
Version: MPL 1.1/GPL 2.0/LGPL 2.1

The contents of this file are subject to the Mozilla Public License Version
1.1 (the "License"); you may not use this file except in compliance with
the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is GeoPackage Extensions

The Initial Developer of the Original Code is Brad Hards (bradh@frogmouth.net)
 
Portions created by the Initial Developer are Copyright (C) 2012-2015
the Initial Developer. All Rights Reserved.

Contributor(s):
Sandro Furieri (a.furieri@lqt.it) 

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
 \file geopackage.h

 GeoPackage: supporting functions and constants
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef _WIN32
#ifdef DLL_EXPORT
#define GEOPACKAGE_DECLARE __declspec(dllexport)
#define GEOPACKAGE_PRIVATE
#else
#define GEOPACKAGE_DECLARE extern
#define GEOPACKAGE_PRIVATE
#endif
#else
#define GEOPACKAGE_DECLARE __attribute__ ((visibility("default")))
#define GEOPACKAGE_PRIVATE __attribute__ ((visibility("hidden")))
#endif
#endif

#ifndef _GEOPACKAGE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GEOPACKAGE_H
#endif

#include "sqlite.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <spatialite/gaiageo.h>

/* Internal geopackage SQL function implementation */
    GEOPACKAGE_PRIVATE void fnct_gpkgCreateBaseTables (sqlite3_context *
						       context, int argc,
						       sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgCreateTilesTable (sqlite3_context *
						       context, int argc,
						       sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgCreateTilesZoomLevel (sqlite3_context *
							   context, int argc,
							   sqlite3_value **
							   argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgInsertEpsgSRID (sqlite3_context *
						     context, int argc,
						     sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgAddTileTriggers (sqlite3_context * context,
						      int argc,
						      sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgGetNormalRow (sqlite3_context * context,
						   int argc,
						   sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgGetNormalZoom (sqlite3_context * context,
						    int argc,
						    sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgGetImageType (sqlite3_context * context,
						   int argc,
						   sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgAddGeometryColumn (sqlite3_context *
							context, int argc,
							sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgMakePoint (sqlite3_context *
						context, int argc,
						sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgMakePointWithSRID (sqlite3_context *
							context, int argc,
							sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgMakePointZ (sqlite3_context *
						 context, int argc,
						 sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgMakePointZWithSRID (sqlite3_context *
							 context, int argc,
							 sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgMakePointM (sqlite3_context *
						 context, int argc,
						 sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgMakePointMWithSRID (sqlite3_context *
							 context, int argc,
							 sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgMakePointZM (sqlite3_context *
						  context, int argc,
						  sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgMakePointZMWithSRID (sqlite3_context *
							  context, int argc,
							  sqlite3_value **
							  argv);
    GEOPACKAGE_PRIVATE void fnct_ToGPB (sqlite3_context * context, int argc,
					sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_GeomFromGPB (sqlite3_context * context,
					      int argc, sqlite3_value ** argv);

    GEOPACKAGE_DECLARE gaiaGeomCollPtr gaiaFromGeoPackageGeometryBlob (const
								       unsigned
								       char
								       *gpb,
								       unsigned
								       int
								       gpb_len);

/* Sandro Furieri - 2014-05-19 */
    GEOPACKAGE_DECLARE int gaiaIsValidGPB (const unsigned char *gpb,
					   int gpb_len);
    GEOPACKAGE_DECLARE int gaiaGetSridFromGPB (const unsigned char *gpb,
					       int gpb_len);
    GEOPACKAGE_DECLARE int gaiaIsEmptyGPB (const unsigned char *gpb,
					   int gpb_len);
    GEOPACKAGE_DECLARE int gaiaGetEnvelopeFromGPB (const unsigned char *gpb,
						   int gpb_len, double *min_x,
						   double *max_x, double *min_y,
						   double *max_y, int *has_z,
						   double *min_z, double *max_z,
						   int *has_m, double *min_m,
						   double *max_m);
    GEOPACKAGE_DECLARE char *gaiaGetGeometryTypeFromGPB (const unsigned char
							 *gpb, int gpb_len);
    GEOPACKAGE_PRIVATE void fnct_IsValidGPB (sqlite3_context * context,
					     int argc, sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_GPKG_IsAssignable (sqlite3_context * context,
						    int argc,
						    sqlite3_value ** argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgAddGeometryTriggers (sqlite3_context *
							  context, int argc,
							  sqlite3_value **
							  argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgAddGeometryTriggers (sqlite3_context *
							  context, int argc,
							  sqlite3_value **
							  argv);
    GEOPACKAGE_PRIVATE void fnct_gpkgAddSpatialIndex (sqlite3_context * context,
						      int argc,
						      sqlite3_value ** argv);
/* end Sandro Furieri - 2014-05-19 */

/* Sandro Furieri - 2015-06-14 */
    GEOPACKAGE_DECLARE void
	gaiaToGPB (gaiaGeomCollPtr geom, unsigned char **result, int *size);
/* end Sandro Furieri - 2015-06-14 */



/* Markers for unused arguments / variable */
#if __GNUC__
#define UNUSED __attribute__ ((__unused__))
#else
#define UNUSED
#endif

#ifdef __cplusplus
}
#endif

#endif
