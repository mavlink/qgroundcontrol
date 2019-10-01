/*
 gg_formats.h -- Gaia common support for geometries: formats
  
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
 \file gg_formats.h

 Geometry handling functions: formats
 */

#ifndef _GG_FORMATS_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_FORMATS_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* function prototypes */

/**
 Test CPU endianness

 \return 0 if big-endian: any other value if little-endian
 */
    GAIAGEO_DECLARE int gaiaEndianArch (void);

/**
 Import an INT-16 value in endian-aware fashion
 
 \param p endian-dependent representation (input buffer).
 \param little_endian 0 if the input buffer is big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \return the internal SHORT value 

 \sa gaiaEndianArch, gaiaExport16

 \note you are expected to pass an input buffer corresponding to an
 allocation size of (at least) 2 bytes.
 */
    GAIAGEO_DECLARE short gaiaImport16 (const unsigned char *p,
					int little_endian,
					int little_endian_arch);

/**
 Import an INT-32 value in endian-aware fashion
 
 \param p endian-dependent representation (input buffer).
 \param little_endian 0 if the input buffer is big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \return the internal INT value 

 \sa gaiaEndianArch, gaiaExport32

 \note you are expected to pass an input buffer corresponding to an
 allocation size of (at least) 4 bytes.
 */
    GAIAGEO_DECLARE int gaiaImport32 (const unsigned char *p, int little_endian,
				      int little_endian_arch);

/**
 Import an UINT-32 value in endian-aware fashion
 
 \param p endian-dependent representation (input buffer).
 \param little_endian 0 if the input buffer is big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \return the internal UINT value 

 \sa gaiaEndianArch, gaiaExportU32

 \note you are expected to pass an input buffer corresponding to an
 allocation size of (at least) 4 bytes.
 */
    GAIAGEO_DECLARE unsigned int gaiaImportU32 (const unsigned char *p,
						int little_endian,
						int little_endian_arch);

/**
 Import a FLOAT-32 value in endian-aware fashion
 
 \param p endian-dependent representation (input buffer).
 \param little_endian 0 if the input buffer is big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \return the internal FLOAT value 

 \sa gaiaEndianArch, gaiaExportF32

 \note you are expected to pass an input buffer corresponding to an
 allocation size of (at least) 4 bytes.
 */
    GAIAGEO_DECLARE float gaiaImportF32 (const unsigned char *p,
					 int little_endian,
					 int little_endian_arch);

/**
 Import an DOUBLE-64 in endian-aware fashion
 
 \param p endian-dependent representation (input buffer).
 \param little_endian 0 if the input buffer is big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \return the internal DOUBLE value 

 \sa gaiaEndianArch, gaiaExport64

 \note you are expected to pass an input buffer corresponding to an
 allocation size of (at least) 8 bytes.
 */
    GAIAGEO_DECLARE double gaiaImport64 (const unsigned char *p,
					 int little_endian,
					 int little_endian_arch);

/**
 Import an INT-64 in endian-aware fashion
 
 \param p endian-dependent representation (input buffer).
 \param little_endian 0 if the input buffer is big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \return the internal INT-64 value 

 \sa gaiaEndianArch, gaiaExportI64

 \note you are expected to pass an input buffer corresponding to an
 allocation size of (at least) 8 bytes.
 */
    GAIAGEO_DECLARE sqlite3_int64 gaiaImportI64 (const unsigned char *p,
						 int little_endian,
						 int little_endian_arch);

/**
 Export an INT-16 value in endian-aware fashion
 
 \param p endian-dependent representation (output buffer).
 \param value the internal value to be exported.
 \param little_endian 0 if the output buffer has to be big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \sa gaiaEndianArch, gaiaImport16

 \note you are expected to pass an output buffer corresponding to an
 allocation size of (at least) 2 bytes.
 */
    GAIAGEO_DECLARE void gaiaExport16 (unsigned char *p, short value,
				       int little_endian,
				       int little_endian_arch);

/**
 Export an INT-32 value in endian-aware fashion
 
 \param p endian-dependent representation (output buffer).
 \param value the internal value to be exported.
 \param little_endian 0 if the output buffer has to be big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \sa gaiaEndianArch, gaiaImport32

 \note you are expected to pass an output buffer corresponding to an
 allocation size of (at least) 4 bytes.
 */
    GAIAGEO_DECLARE void gaiaExport32 (unsigned char *p, int value,
				       int little_endian,
				       int little_endian_arch);

/**
 Export an UINT-32 value in endian-aware fashion
 
 \param p endian-dependent representation (output buffer).
 \param value the internal value to be exported.
 \param little_endian 0 if the output buffer has to be big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \sa gaiaEndianArch, gaiaImportU32

 \note you are expected to pass an output buffer corresponding to an
 allocation size of (at least) 4 bytes.
 */
    GAIAGEO_DECLARE void gaiaExportU32 (unsigned char *p, unsigned int value,
					int little_endian,
					int little_endian_arch);

/**
 Export a FLOAT-32 value in endian-aware fashion
 
 \param p endian-dependent representation (output buffer).
 \param value the internal value to be exported.
 \param little_endian 0 if the output buffer has to be big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \sa gaiaEndianArch, gaiaImportF32

 \note you are expected to pass an output buffer corresponding to an
 allocation size of (at least) 4 bytes.
 */
    GAIAGEO_DECLARE void gaiaExportF32 (unsigned char *p, float value,
					int little_endian,
					int little_endian_arch);

/**
 Export a DOUBLE value in endian-aware fashion
 
 \param p endian-dependent representation (output buffer).
 \param value the internal value to be exported.
 \param little_endian 0 if the output buffer has to be big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \sa gaiaEndianArch, gaiaImport64

 \note you are expected to pass an output buffer corresponding to an
 allocation size of (at least) 8 bytes.
 */
    GAIAGEO_DECLARE void gaiaExport64 (unsigned char *p, double value,
				       int little_endian,
				       int little_endian_arch);

/**
 Export an INT-64 value in endian-aware fashion
 
 \param p endian-dependent representation (output buffer).
 \param value the internal value to be exported.
 \param little_endian 0 if the output buffer has to be big-endian: any other value
 for little-endian.
 \param little_endian_arch the value returned by gaiaEndianArch()

 \sa gaiaEndianArch, gaiaImportI64

 \note you are expected to pass an output buffer corresponding to an
 allocation size of (at least) 8 bytes.
 */
    GAIAGEO_DECLARE void gaiaExportI64 (unsigned char *p, sqlite3_int64 value,
					int little_endian,
					int little_endian_arch);

/**
 Initializes a dynamically growing Text output buffer

 \param buf pointer to gaiaOutBufferStruct structure
 
 \sa gaiaOutBufferReset, gaiaAppendToOutBuffer

 \note Text notations representing Geometry objects may easily require
 a huge storage amount: the gaiaOutBufferStruct automatically supports
 a dynamically growing output buffer.
 \n You are required to initialize this structure before attempting
 any further operation;
 and you are responsible to cleanup any related memory allocation
 when it's any longer required.
 */
    GAIAGEO_DECLARE void gaiaOutBufferInitialize (gaiaOutBufferPtr buf);

/**
 Resets a dynamically growing Text output buffer to its initial (empty) state

 \param buf pointer to gaiaOutBufferStruct structure
 
 \sa gaiaOutBufferInitialize, gaiaAppendToOutBuffer

 \note You are required to initialize this structure before attempting
 any further operation:
 this function will release any related memory allocation.
 */
    GAIAGEO_DECLARE void gaiaOutBufferReset (gaiaOutBufferPtr buf);

/**
 Appends a text string at the end of Text output buffer

 \param buf pointer to gaiaOutBufferStruct structure.
 \param text the text string to be appended.

 \sa gaiaOutBufferInitialize, gaiaOutBufferReset

 \note You are required to initialize this structure before attempting
 any further operation:
 the dynamically growing Text buffer will be automatically allocated
 and/or extended as required.
 */
    GAIAGEO_DECLARE void gaiaAppendToOutBuffer (gaiaOutBufferPtr buf,
						const char *text);

/**
 Creates a BLOB-Geometry representing a Point

 \param x Point X coordinate.
 \param y Point Y coordinate.
 \param srid the SRID to be set for the Point.
 \param result on completion will containt a pointer to BLOB-Geometry:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)

 \sa gaiaFromSpatiaLiteBlobWkb

 \note the BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaMakePoint (double x, double y, int srid,
					unsigned char **result, int *size);

/**
 Creates a BLOB-Geometry representing a PointZ

 \param x Point X coordinate.
 \param y Point Y coordinate.
 \param z Point Z coordinate.
 \param srid the SRID to be set for the Point.
 \param result on completion will containt a pointer to BLOB-Geometry:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)

 \sa gaiaFromSpatiaLiteBlobWkb

 \note the BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaMakePointZ (double x, double y, double z, int srid,
					 unsigned char **result, int *size);

/**
 Creates a BLOB-Geometry representing a PointM

 \param x Point X coordinate.
 \param y Point Y coordinate.
 \param m Point M coordinate.
 \param srid the SRID to be set for the Point.
 \param result on completion will containt a pointer to BLOB-Geometry:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)

 \sa gaiaFromSpatiaLiteBlobWkb

 \note the BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaMakePointM (double x, double y, double m, int srid,
					 unsigned char **result, int *size);

/**
 Creates a BLOB-Geometry representing a PointZM

 \param x Point X coordinate.
 \param y Point Y coordinate.
 \param z Point Z coordinate.
 \param m Point M coordinate.
 \param srid the SRID to be set for the Point.
 \param result on completion will containt a pointer to BLOB-Geometry:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)

 \sa gaiaFromSpatiaLiteBlobWkb

 \note the BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaMakePointZM (double x, double y, double z,
					  double m, int srid,
					  unsigned char **result, int *size);

/**
 Creates a BLOB-Geometry representing a Segment (2-Points Linestring)

 \param geom1 pointer to first Geometry object (expected to represent a Point).
 \param geom2 pointer to second Geometry object (expected to represent a Point).
 \param result on completion will containt a pointer to BLOB-Geometry:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)

 \sa gaiaFromSpatiaLiteBlobWkb

 \note the BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaMakeLine (gaiaGeomCollPtr geom1,
				       gaiaGeomCollPtr geom2,
				       unsigned char **result, int *size);

/**
 Creates a Geometry object from the corresponding BLOB-Geometry 

 \param blob pointer to BLOB-Geometry
 \param size the BLOB's size

 \return the pointer to the newly created Geometry object: NULL on failure

 \sa gaiaFreeGeomColl, gaiaToSpatiaLiteBlobWkb, gaiaToCompressedBlobWkb,
 gaiaFromSpatiaLiteBlobWkbEx

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any 
 contained child object. 
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromSpatiaLiteBlobWkb (const unsigned
							       char *blob,
							       unsigned int
							       size);

/**
 Creates a Geometry object from the corresponding BLOB-Geometry 

 \param blob pointer to BLOB-Geometry
 \param size the BLOB's size
 \param gpkg_mode is set to TRUE will accept only GPKG Geometry-BLOBs
 \param gpkg_amphibious is set to TRUE will indifferenctly accept
  either SpatiaLite Geometry-BLOBs or GPKG Geometry-BLOBs

 \return the pointer to the newly created Geometry object: NULL on failure

 \sa gaiaFreeGeomColl, gaiaToSpatiaLiteBlobWkb, gaiaToCompressedBlobWkb

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any 
 contained child object. 
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromSpatiaLiteBlobWkbEx (const unsigned
								 char *blob,
								 unsigned int
								 size,
								 int gpkg_mode,
								 int
								 gpkg_amphibious);

/**
 Creates a BLOB-Geometry corresponding to a Geometry object

 \param geom pointer to the Geometry object.
 \param result on completion will containt a pointer to BLOB-Geometry:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)

 \sa gaiaFromSpatiaLiteBlobWkb, gaiaToCompressedBlobWkb

 \note the BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaToSpatiaLiteBlobWkb (gaiaGeomCollPtr geom,
						  unsigned char **result,
						  int *size);

/**
 Creates a BLOB-Geometry corresponding to a Geometry object

 \param geom pointer to the Geometry object.
 \param result on completion will containt a pointer to BLOB-Geometry:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)
 \param gpkg_mode is set to TRUE will always return GPKG Geometry-BLOBs

 \sa gaiaFromSpatiaLiteBlobWkb, gaiaToCompressedBlobWkb

 \note the BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaToSpatiaLiteBlobWkbEx (gaiaGeomCollPtr geom,
						    unsigned char **result,
						    int *size, int gpkg_mode);

/**
 Creates a Compressed BLOB-Geometry corresponding to a Geometry object

 \param geom pointer to the Geometry object.
 \param result on completion will containt a pointer to Compressed BLOB-Geometry:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)

 \sa gaiaFromSpatiaLiteBlobWkb, gaiaToSpatiaLiteBlobWkb

 \note this function will apply compression to any Linestring / Ring found
 within the Geometry to be encoded.
 \n the returned BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaToCompressedBlobWkb (gaiaGeomCollPtr geom,
						  unsigned char **result,
						  int *size);

/**
 Creates a Geometry object from WKB notation

 \param blob pointer to WKB buffer
 \param size the BLOB's size (in bytes)

 \return the pointer to the newly created Geometry object: NULL on failure.

 \sa gaiaToWkb, gaiaToHexWkb, gaiaFromEWKB, gaiaToEWKB

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any
 contained child object. 
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromWkb (const unsigned char *blob,
						 unsigned int size);

/**
 Encodes a Geometry object into WKB notation

 \param geom pointer to Geometry object
 \param result on completion will containt a pointer to the WKB buffer [BLOB]:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)

 \sa gaiaFromWkb, gaiaToHexWkb, gaiaFromEWKB, gaiaToEWKB

 \note this function will apply 3D WKB encoding as internally intended by
 SpatiaLite: not necessarily intended by other OGC-like implementations.
 \n Anyway, 2D WKB is surely standard and safely interoperable.
 \n the returned BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaToWkb (gaiaGeomCollPtr geom,
				    unsigned char **result, int *size);

/**
 Encodes a Geometry object into (hex) WKB notation

 \param geom pointer to Geometry object

 \return the pointer to a text buffer containing WKB translated into plain
 hexadecimal: NULL on failure.

 \sa gaiaFromWkb, gaiaToWkb, gaiaFromEWKB, gaiaToEWKB

 \note the returned buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE char *gaiaToHexWkb (gaiaGeomCollPtr geom);

/**
 Encodes a Geometry object into EWKB notation

 \param out_buf pointer to dynamically growing Text buffer
 \param geom pointer to Geometry object

 \sa gaiaFromWkb, gaiaToWkb, gaiaToHexWkb, gaiaFromEWKB, gaiaToEWKB

 \note this function will produce strictly conformat EWKB; you can
 safely use this for PostGIS data exchange.
 */
    GAIAGEO_DECLARE void gaiaToEWKB (gaiaOutBufferPtr out_buf,
				     gaiaGeomCollPtr geom);

/**
 Creates a Geometry object from EWKB notation

 \param in_buffer pointer to EWKB buffer

 \return the pointer to the newly created Geometry object: NULL on failure.

 \sa gaiaToWkb, gaiaToHexWkb, gaiaParseHexEWKB, gaiaToEWKB, gaiaEwkbGetPoint,
 gaiaEwkbGetLinestring, gaiaEwkbGetPolygon, gaiaEwkbGetMultiGeometry

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any
 contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromEWKB (const unsigned char
						  *in_buffer);

/**
 Translates an EWKB notation from hexadecimal into binary

 \param blob_hex pointer to EWKB input buffer (hexadecimal text string)
 \param blob_size lenght (in bytes) of the input buffer; if succesfull will 
  contain the lenght of the returned output buffer.

 \return the pointer to the newly created EWKB binary buffer: NULL on failure.

 \sa gaiaToWkb, gaiaToHexWkb, gaiaFromEWKB, gaiaToEWKB

 \note you are responsible to destroy (before or after) any buffer allocated by
 gaiaParseHexEWKB()
 */
    GAIAGEO_DECLARE unsigned char *gaiaParseHexEWKB (const unsigned char
						     *blob_hex, int *blob_size);

/**
 Attempts to decode a Point from within an EWKB binary buffer

 \param geom pointer to an existing Geometry object; if succesfull the parsed Point
  will be inserted into this Geometry
 \param blob pointer to EWKB input buffer
 \param offset the offset (in bytes) on the input buffer where the Point definition is expected to start.
 \param blob_size lenght (in bytes) of the input buffer.
 \param endian (boolean) states if the EWKB input buffer is little- or big-endian encode.
 \param endian_arch (boolean) states if the target CPU has a little- or big-endian architecture.
 \param dims dimensions: one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M or GAIA_XY_Z_M

 \return -1 on failure; otherwise the offset where the next object starts.

 \sa gaiaEwkbGetLinestring, gaiaEwkbGetPolygon, gaiaEwkbGetMultiGeometry

 \note these functions are mainly intended for internal usage.
 */
    GAIAGEO_DECLARE int
	gaiaEwkbGetPoint (gaiaGeomCollPtr geom, unsigned char *blob,
			  int offset, int blob_size, int endian,
			  int endian_arch, int dims);

/**
 Attempts to decode a Point from within an EWKB binary buffer

 \param geom pointer to an existing Geometry object; if succesfull the parsed Linestring
  will be inserted into this Geometry
 \param blob pointer to EWKB input buffer
 \param offset the offset (in bytes) on the input buffer where the Point definition is expected to start.
 \param blob_size lenght (in bytes) of the input buffer.
 \param endian (boolean) states if the EWKB input buffer is little- or big-endian encode.
 \param endian_arch (boolean) states if the target CPU has a little- or big-endian architecture.
 \param dims dimensions: one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M or GAIA_XY_Z_M

 \return -1 on failure; otherwise the offset where the next object starts.

 \sa gaiaEwkbGetPoint, gaiaEwkbGetPolygon, gaiaEwkbGetMultiGeometry

 \note these functions are mainly intended for internal usage.
 */
    GAIAGEO_DECLARE int
	gaiaEwkbGetLinestring (gaiaGeomCollPtr geom, unsigned char *blob,
			       int offset, int blob_size, int endian,
			       int endian_arch, int dims);

/**
 Attempts to decode a Polygon from within an EWKB binary buffer

 \param geom pointer to an existing Geometry object; if succesfull the parsed Polygon
  will be inserted into this Geometry
 \param blob pointer to EWKB input buffer
 \param offset the offset (in bytes) on the input buffer where the Point definition is expected to start.
 \param blob_size lenght (in bytes) of the input buffer.
 \param endian (boolean) states if the EWKB input buffer is little- or big-endian encode.
 \param endian_arch (boolean) states if the target CPU has a little- or big-endian architecture.
 \param dims dimensions: one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M or GAIA_XY_Z_M

 \return -1 on failure; otherwise the offset where the next object starts.

 \sa gaiaEwkbGetPoint, gaiaEwkbGetPolygon, gaiaEwkbGetMultiGeometry
 */
    GAIAGEO_DECLARE int
	gaiaEwkbGetPolygon (gaiaGeomCollPtr geom, unsigned char *blob,
			    int offset, int blob_size, int endian,
			    int endian_arch, int dims);

/**
 Attempts to decode a MultiGeometry from within an EWKB binary buffer

 \param geom pointer to an existing Geometry object; if succesfull the parsed MultiGeometry
  will be inserted into this Geometry
 \param blob pointer to EWKB input buffer
 \param offset the offset (in bytes) on the input buffer where the Point definition is expected to start.
 \param blob_size lenght (in bytes) of the input buffer.
 \param endian (boolean) states if the EWKB input buffer is little- or big-endian encode.
 \param endian_arch (boolean) states if the target CPU has a little- or big-endian architecture.
 \param dims dimensions: one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M or GAIA_XY_Z_M

 \return -1 on failure; otherwise the offset where the next object starts.

 \sa gaiaEwkbGetPoint, gaiaEwkbGetLinestring, gaiaEwkbGetPolygon

 \note these functions are mainly intended for internal usage.
 */
    GAIAGEO_DECLARE int
	gaiaEwkbGetMultiGeometry (gaiaGeomCollPtr geom, unsigned char *blob,
				  int offset, int blob_size, int endian,
				  int endian_arch, int dims);

/**
 Creates a Geometry object from FGF notation

 \param blob pointer to FGF buffer
 \param size the BLOB's size (in bytes)

 \return the pointer to the newly created Geometry object: NULL on failure.

 \sa gaiaToFgf

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any
 contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromFgf (const unsigned char *blob,
						 unsigned int size);

/**
 Encodes a Geometry object into FGF notation

 \param geom pointer to Geometry object
 \param result on completion will containt a pointer to the FGF buffer [BLOB]:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes)
 \param coord_dims one of: GAIA_XY, GAIA_XY_Z, GAIA_XY_M, GAIA_XY_ZM

 \sa gaiaFromFgf

 \note the returned BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaToFgf (gaiaGeomCollPtr geom,
				    unsigned char **result, int *size,
				    int coord_dims);

/**
 Creates a Geometry object from WKT notation

 \param in_buffer pointer to WKT buffer
 \param type the expected Geometry Class Type
 \n if actual type defined in WKT doesn't corresponds to this, an error will
 be raised.

 \return the pointer to the newly created Geometry object: NULL on failure

 \sa gaiaOutWkt, gaiaOutWktStrict, gaiaParseEWKT, gaiaToEWKT

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any
 contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaParseWkt (const unsigned char
						  *in_buffer, short type);

/**
 Encodes a Geometry object into WKT notation

 \param out_buf pointer to dynamically growing Text buffer
 \param geom pointer to Geometry object

 \sa gaiaParseWkt, gaiaOutWktStrict, gaiaParseEWKT, gaiaToEWKT,
 gaiaOutWktEx

 \note this function will apply 3D WKT encoding as internally intended by
 SpatiaLite: not necessarily intended by other OGC-like implementations.
 \n Anyway, 2D WKT is surely standard and safely interoperable.
 */
    GAIAGEO_DECLARE void gaiaOutWkt (gaiaOutBufferPtr out_buf,
				     gaiaGeomCollPtr geom);

/**
 Encodes a Geometry object into WKT notation

 \param out_buf pointer to dynamically growing Text buffer
 \param geom pointer to Geometry object
 \param precision decimal digits to be used for coordinates

 \sa gaiaParseWkt, gaiaOutWktStrict, gaiaParseEWKT, gaiaToEWKT

 \note this function will apply 3D WKT encoding as internally intended by
 SpatiaLite: not necessarily intended by other OGC-like implementations.
 \n Anyway, 2D WKT is surely standard and safely interoperable.
 */
    GAIAGEO_DECLARE void gaiaOutWktEx (gaiaOutBufferPtr out_buf,
				       gaiaGeomCollPtr geom, int precision);

/**
 Encodes a Geometry object into strict 2D WKT notation

 \param out_buf pointer to dynamically growing Text buffer
 \param geom pointer to Geometry object
 \param precision decimal digits to be used for coordinates

 \sa gaiaParseWkt, gaiaOutWkt, gaiaParseEWKT, gaiaToEWKT

 \note this function will apply strict 2D WKT encoding, so to be surely
 standard and safely interoperable.
 \n Dimensions will be automatically casted to 2D [XY] when required.
 */
    GAIAGEO_DECLARE void gaiaOutWktStrict (gaiaOutBufferPtr out_buf,
					   gaiaGeomCollPtr geom, int precision);

/**
 Creates a Geometry object from EWKT notation

 \param in_buffer pointer to EWKT buffer

 \return the pointer to the newly created Geometry object: NULL on failure

 \sa gaiaParseWkt, gaiaOutWkt, gaiaOutWktStrict, gaiaToEWKT

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any
 contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaParseEWKT (const unsigned char
						   *in_buffer);

/**
 Encodes a Geometry object into EWKT notation

 \param out_buf pointer to dynamically growing Text buffer
 \param geom pointer to Geometry object

 \sa gaiaParseWkt, gaiaOutWkt, gaiaOutWktStrict, gaiaParseEWKT

 \note this function will apply PostGIS own EWKT encoding.
 */
    GAIAGEO_DECLARE void gaiaToEWKT (gaiaOutBufferPtr out_buf,
				     gaiaGeomCollPtr geom);

/**
 Encodes a WKT 3D Point [XYZ]

 \param out_buf pointer to dynamically growing Text buffer
 \param point pointer to Point object

 \sa gaiaOutLinestringZ, gaiaOutPolygonZ, gaiaOutPointZex
 
 \remark mainly intended for internal usage.
 */
    GAIAGEO_DECLARE void gaiaOutPointZ (gaiaOutBufferPtr out_buf,
					gaiaPointPtr point);

/**
 Encodes a WKT 3D Point [XYZ]

 \param out_buf pointer to dynamically growing Text buffer
 \param point pointer to Point object
 \param precision decimal digits to be used for coordinates

 \sa gaiaOutLinestringZ, gaiaOutPolygonZ
 
 \remark mainly intended for internal usage.
 */
    GAIAGEO_DECLARE void gaiaOutPointZex (gaiaOutBufferPtr out_buf,
					  gaiaPointPtr point, int precision);

/**
 Encodes a WKT 3D Linestring [XYZ]

 \param out_buf pointer to dynamically growing Text buffer
 \param linestring pointer to Linestring object

 \sa gaiaOutPointZ, gaiaOutPolygonZ, gaiaOutLinestringZex
 
 \remark mainly intended for internal usage.
 */
    GAIAGEO_DECLARE void gaiaOutLinestringZ (gaiaOutBufferPtr out_buf,
					     gaiaLinestringPtr linestring);

/**
 Encodes a WKT 3D Linestring [XYZ]

 \param out_buf pointer to dynamically growing Text buffer
 \param linestring pointer to Linestring object
 \param precision decimal digits to be used for coordinates

 \sa gaiaOutPointZ, gaiaOutPolygonZ
 
 \remark mainly intended for internal usage.
 */
    GAIAGEO_DECLARE void gaiaOutLinestringZex (gaiaOutBufferPtr out_buf,
					       gaiaLinestringPtr linestring,
					       int precision);

/**
 Encodes a WKT 3D Polygon [XYZ]

 \param out_buf pointer to dynamically growing Text buffer
 \param polygon pointer to Point object

 \sa gaiaOutPointZ, gaiaOutLinestringZ, gaiaOutPolygonZex
 
 \remark mainly intended for internal usage.
 */
    GAIAGEO_DECLARE void gaiaOutPolygonZ (gaiaOutBufferPtr out_buf,
					  gaiaPolygonPtr polygon);

/**
 Encodes a WKT 3D Polygon [XYZ]

 \param out_buf pointer to dynamically growing Text buffer
 \param polygon pointer to Point object
 \param precision decimal digits to be used for coordinates

 \sa gaiaOutPointZ, gaiaOutLinestringZ
 
 \remark mainly intended for internal usage.
 */
    GAIAGEO_DECLARE void gaiaOutPolygonZex (gaiaOutBufferPtr out_buf,
					    gaiaPolygonPtr polygon,
					    int precision);

/**
 Creates a Geometry object from KML notation

 \param in_buffer pointer to KML buffer

 \return the pointer to the newly created Geometry object: NULL on failure

 \sa gaiaOutBareKml, gaiaOutFullKml

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any
 contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaParseKml (const unsigned char
						  *in_buffer);

/**
 Encodes a Geometry object into KML notation

 \param out_buf pointer to dynamically growing Text buffer
 \param geom pointer to Geometry object 
 \param precision decimal digits to be used for coordinates

 \sa gaiaParseKml, gaiaOutFullKml

 \note this function will export the simplest KML notation (no descriptions).
 */
    GAIAGEO_DECLARE void gaiaOutBareKml (gaiaOutBufferPtr out_buf,
					 gaiaGeomCollPtr geom, int precision);

/**
 Encodes a Geometry object into KML notation

 \param out_buf pointer to dynamically growing Text buffer
 \param name text string to be set as KML \e name 
 \param desc text string to se set as KML \e description 
 \param geom pointer to Geometry object
 \param precision decimal digits to be used for coordinates

 \sa gaiaParseKml, gaiaOutBareKml

 \note this function will export the simplest KML notation (no descriptions).
 */
    GAIAGEO_DECLARE void gaiaOutFullKml (gaiaOutBufferPtr out_buf,
					 const char *name, const char *desc,
					 gaiaGeomCollPtr geom, int precision);

/**
 Creates a Geometry object from GML notation

 \param in_buffer pointer to GML buffer
 \param sqlite_handle handle to current DB connection

 \return the pointer to the newly created Geometry object: NULL on failure

 \sa gaiaParseGml_r, gaiaOutGml

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any
 contained child object.\n
 not reentrant and thread unsafe.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaParseGml (const unsigned char
						  *in_buffer,
						  sqlite3 * sqlite_handle);

/**
 Creates a Geometry object from GML notation

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param in_buffer pointer to GML buffer
 \param sqlite_handle handle to current DB connection

 \return the pointer to the newly created Geometry object: NULL on failure

 \sa gaiaParseGml, gaiaOutGml

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any
 contained child object.\n
 reentrant and thread-safe.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaParseGml_r (const void *p_cache,
						    const unsigned char
						    *in_buffer,
						    sqlite3 * sqlite_handle);

/**
 Encodes a Geometry object into GML notation

 \param out_buf pointer to dynamically growing Text buffer
 \param version GML version
 \param precision decimal digits to be used for coordinates
 \param geom pointer to Geometry object

 \sa gaiaParseGml

 \note if \e version is set to \b 3, then GMLv3 will be used;
 in any other case GMLv2 will be assumed by default.
 */
    GAIAGEO_DECLARE void gaiaOutGml (gaiaOutBufferPtr out_buf, int version,
				     int precision, gaiaGeomCollPtr geom);

/**
 Creates a Geometry object from GeoJSON notation

 \param in_buffer pointer to GeoJSON buffer

 \return the pointer to the newly created Geometry object: NULL on failure

 \sa gaiaOutGeoJSON

 \note you are responsible to destroy (before or after) any allocated Geometry,
 unless you've passed ownership of the Geometry object to some further object:
 in this case destroying the higher order object will implicitly destroy any
 contained child object.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaParseGeoJSON (const unsigned char
						      *in_buffer);

/**
 Encodes a Geometry object into GeoJSON notation

 \param out_buf pointer to dynamically growing Text buffer
 \param geom pointer to Geometry object
 \param precision decimal digits to be used for coordinates
 \param options GeoJSON specific options

 \sa gaiaParseGeoJSON

 \note \e options can assume the following values:
 \li 1 = BBOX, no CRS
 \li 2 = no BBOX, short form CRS
 \li 3 = BBOX, short form CRS
 \li 4 = no BBOX, long form CRS
 \li 5 = BBOX, long form CRS
 \li any other value: no BBOX and no CRS
 */
    GAIAGEO_DECLARE void gaiaOutGeoJSON (gaiaOutBufferPtr out_buf,
					 gaiaGeomCollPtr geom, int precision,
					 int options);
/**
 Encodes a Geometry object into SVG notation

 \param out_buf pointer to dynamically growing Text buffer
 \param geom pointer to Geometry object
 \param relative flag: relative or absolute coordinates
 \param precision decimal digits to be used for coordinates

 \note if \e relative is set to \b 1, then SVG relative coords will be used:
 in any other case SVG absolute coords will be assumed by default.
 */
    GAIAGEO_DECLARE void gaiaOutSvg (gaiaOutBufferPtr out_buf,
				     gaiaGeomCollPtr geom, int relative,
				     int precision);

/**
 Allocates a new DBF Field Value object [duplicating an existing one]
 
 \param org pointer to input DBF Field Value object.

 \return the pointer to newly created DBF Field object.

 \sa gaiaAllocDbfField, gaiaFreeDbfField, gaiaCloneDbfField, gaiaCloneValue,
 gaiaSetNullValue, gaiaSetIntValue, gaiaSetDoubleValue,
 gaiaSetStrValue

 \note the newly created object is an exact copy of the original one.
 */
    GAIAGEO_DECLARE gaiaValuePtr gaiaCloneValue (gaiaValuePtr org);

/**
 Resets a DBF Field Value object to its initial empty state

 \param p pointer to DBF Field Value object

 \sa gaiaAllocDbfField, gaiaCloneDbfField, gaiaCloneValue,
 gaiaSetNullValue, gaiaSetIntValue, gaiaSetDoubleValue,
 gaiaSetStrValue, gaiaResetDbfEntity
 */
    GAIAGEO_DECLARE void gaiaFreeValue (gaiaValuePtr p);

/**
 Allocates a new DBF Field object

 \param name text string: DBF Field name.
 \param type identifier of the corresponding DBF data type.
 \param offset corresponding offset into the DBF I/O buffer.
 \param length max field length (in bytes).
 \param decimals precision: number of decimal digits.

 \return the pointer to newly created DBF Field object.

 \sa gaiaFreeDbfField, gaiaCloneDbfField, gaiaFreeValue,
 gaiaSetNullValue, gaiaSetIntValue, gaiaSetDoubleValue,
 gaiaSetStrValue

 \note you are responsible to destroy (before or after) any allocated DBF Field,
 unless you've passed ownership to some further object: in this case destroying  the higher order object will implicitly destroy any contained child object.  
 \n supported DBF data types are:
 \li 'C' text string [default]
 \li 'N' numeric
 \li 'D' date
 \li 'L' boolean
 */
    GAIAGEO_DECLARE gaiaDbfFieldPtr gaiaAllocDbfField (char *name,
						       unsigned char type,
						       int offset,
						       unsigned char length,
						       unsigned char decimals);

/**
 Destroys a DBF Field object

 \param p pointer to DBF Field object

 \sa gaiaAllocDbfField, gaiaCloneDbfField, gaiaCloneValue,
 gaiaFreeValue, gaiaSetNullValue, gaiaSetIntValue, gaiaSetDoubleValue,
 gaiaSetStrValue
 */
    GAIAGEO_DECLARE void gaiaFreeDbfField (gaiaDbfFieldPtr p);

/**
 Allocates a new DBF Field object [duplicating an existing one]

 \param org pointer to input DBF Field object.

 \return the pointer to newly created DBF Field object.

 \sa gaiaAllocDbfField, gaiaFreeDbfField, gaiaCloneDbfField, 
 gaiaFreeValue, gaiaSetNullValue, gaiaSetIntValue, gaiaSetDoubleValue,
 gaiaSetStrValue

 \note the newly created object is an exact copy of the original one
 [this including an evantual Field Value].
 */
    GAIAGEO_DECLARE gaiaDbfFieldPtr gaiaCloneDbfField (gaiaDbfFieldPtr org);

/**
 Sets a NULL current value for a DBF Field object

 \param field pointer to DBF Field object
 
 \sa gaiaAllocDbfField, gaiaFreeDbfField, gaiaCloneDbfField,
 gaiaFreeValue, gaiaSetIntValue, gaiaSetDoubleValue,
 gaiaSetStrValue
 */
    GAIAGEO_DECLARE void gaiaSetNullValue (gaiaDbfFieldPtr field);

/**
 Sets an INTEGER current value for a DBF Field object

 \param field pointer to DBF Field object.
 \param value integer value to be set.

 \sa gaiaAllocDbfField, gaiaFreeDbfField, gaiaCloneDbfField,
 gaiaFreeValue, gaiaSetNullValue, gaiaSetDoubleValue,
 gaiaSetStrValue
 */
    GAIAGEO_DECLARE void gaiaSetIntValue (gaiaDbfFieldPtr field,
					  sqlite3_int64 value);

/**
 Sets a DOUBLE current value for a DBF Field object
 
 \param field pointer to DBF Field object.
 \param value double value to be set.
                                          
 \sa gaiaAllocDbfField, gaiaFreeDbfField, gaiaCloneDbfField, 
 gaiaFreeValue, gaiaSetNullValue, gaiaSetIntValue, gaiaSetStrValue
 */
    GAIAGEO_DECLARE void gaiaSetDoubleValue (gaiaDbfFieldPtr field,
					     double value);

/**
 Sets a TEXT current value for a DBF Field object

 \param field pointer to DBF Field object.
 \param str text string value to be set.

 \sa gaiaAllocDbfField, gaiaFreeDbfField, gaiaCloneDbfField,
 gaiaFreeValue, gaiaSetNullValue, gaiaSetIntValue, gaiaSetDoubleValue
 */
    GAIAGEO_DECLARE void gaiaSetStrValue (gaiaDbfFieldPtr field, char *str);

/**
 Creates an initially empty DBF List object

 \return the pointer to newly allocated DBF List object: NULL on failure.

 \sa gaiaFreeDbfList, gaiaIsValidDbfList, 
 gaiaResetDbfEntity, gaiaCloneDbfEntity, gaiaAddDbfField
 
 \note you are responsible to destroy (before or after) any allocated DBF List,
 unless you've passed ownership to some further object: in this case destroying
 the higher order object will implicitly destroy any contained child object. 
 */
    GAIAGEO_DECLARE gaiaDbfListPtr gaiaAllocDbfList (void);

/**
 Destroys a DBF List object
 
 \param list pointer to the DBF List object

 \sa gaiaAllocDbfList, gaiaIsValidDbfList,
 gaiaResetDbfEntity, gaiaCloneDbfEntity, gaiaAddDbfField

 \note attempting to destroy any DBF List object whose ownnership has already 
 been transferred to some other (higher order) object is a serious error,
 and will easily cause severe memory corruption. 
 */
    GAIAGEO_DECLARE void gaiaFreeDbfList (gaiaDbfListPtr list);

/**
 Checks a DBF List object for validity

 \param list pointer to the DBF List object.
 
 \return 0 if not valid: any other value if valid.

 \sa gaiaAllocDbfList, gaiaFreeDbfList, gaiaIsValidDbfList,
 gaiaResetDbfEntity, gaiaCloneDbfEntity, gaiaAddDbfField
 */
    GAIAGEO_DECLARE int gaiaIsValidDbfList (gaiaDbfListPtr list);

/**
 Inserts a further DBF Field object into a DBF List object

 \param list pointer to the DBF List object.
 \param name text string: DBF Field name.
 \param type identifier of the corresponding DBF data type.
 \param offset corresponding offset into the DBF I/O buffer.
 \param length max field length (in bytes).
 \param decimals precision: number of decimal digits.

 \return the pointer to newly created DBF Field object.

 \sa gaiaAllocDbfField

 \note supported DBF data types are:
 \li 'C' text string [default]
 \li 'N' numeric
 \li 'D' date
 \li 'L' boolean
 */
    GAIAGEO_DECLARE gaiaDbfFieldPtr gaiaAddDbfField (gaiaDbfListPtr list,
						     char *name,
						     unsigned char type,
						     int offset,
						     unsigned char length,
						     unsigned char decimals);

/** 
 Resets a DBF List object to its initial empty state

 \param list pointer to the DBF List object.

 \sa gaiaFreeValue

 \note any DBF Field associated to the List object will be reset to its
 initial empty state (i.e. \e no \e value at all).
 */
    GAIAGEO_DECLARE void gaiaResetDbfEntity (gaiaDbfListPtr list);

/**
 Allocates a new DBF List object [duplicating an existing one]

 \param org pointer to input DBF List object.

 \return the pointer to newly created DBF List object.

 \sa gaiaCloneDbfField, gaiaCloneValue,

 \note the newly created object is an exact copy of the original one.
 \n this including any currently set Field Value.
 */
    GAIAGEO_DECLARE gaiaDbfListPtr gaiaCloneDbfEntity (gaiaDbfListPtr org);

/**
 Allocates a new Shapefile object.

 \return the pointer to newly created Shapefile object.

 \sa gaiaFreeShapefile, gaiaOpenShpRead, gaiaOpenShpWrite,
 gaiaReadShpEntity, gaiaShpAnalyze, gaiaWriteShpEntity, gaiaFlushShpHeaders

 \note you are responsible to destroy (before or after) any allocated Shapefile.
 \n you should phisically open the Shapefile in \e read or \e write mode
 before performing any actual I/O operation.
 */
    GAIAGEO_DECLARE gaiaShapefilePtr gaiaAllocShapefile (void);

/**
 Destroys a Shapefile object 

 \param shp pointer to the Shapefile object.

 \sa gaiaAllocShapefile, gaiaOpenShpRead, gaiaOpenShpWrite,
 gaiaReadShpEntity, gaiaShpAnalyze, gaiaWriteShpEntity, gaiaFlushShpHeaders
 
 \note destroying the Shapefile object will close any related file:
 anyway you a responsible to explicitly call gaiaFlushShpHeader
 before destroyng a Shapefile opened in \e write mode.
 */
    GAIAGEO_DECLARE void gaiaFreeShapefile (gaiaShapefilePtr shp);

/** 
 Open a Shapefile in read mode

 \param shp pointer to the Shapefile object.
 \param path \e abstract pathname to the corresponding file-system files.
 \param charFrom GNU ICONV name identifying the input charset encoding.
 \param charTo GNU ICONV name identifying the output charset encoding.

 \sa gaiaAllocShapefile, gaiaFreeShapefile, gaiaOpenShpWrite,
 gaiaReadShpEntity, gaiaShpAnalyze, gaiaWriteShpEntity, gaiaFlushShpHeaders
 
 \note on failure the object member \e Valid will be set to 0; and the
 object member \e LastError will contain the appropriate error message.
 \n the \e abstract pathname should not contain any suffix at all.
 */
    GAIAGEO_DECLARE void gaiaOpenShpRead (gaiaShapefilePtr shp,
					  const char *path,
					  const char *charFrom,
					  const char *charTo);

/**
 Open a Shapefile in read mode

 \param shp pointer to the Shapefile object.
 \param path \e abstract pathname to the corresponding file-system files.
 \param shape the SHAPE code; expected to be one of GAIA_SHP_POINT,
 GAIA_SHP_POLYLINE, GAIA_SHP_POLYGON, GAIA_SHP_MULTIPOINT, GAIA_SHP_POINTZ,
 GAIA_SHP_POLYLINEZ, GAIA_SHP_POLYGONZ, GAIA_SHP_MULTIPOINTZ, 
 GAIA_SHP_POINTM, GAIA_SHP_POLYLINEM, GAIA_SHP_POLYGONM, GAIA_SHP_MULTIPOINTM
 \param list pointer to DBF List object representing the corresponding
 data attributes.
 \param charFrom GNU ICONV name identifying the input charset encoding.
 \param charTo GNU ICONV name identifying the output charset encoding.

 \sa gaiaAllocShapefile, gaiaFreeShapefile, gaiaOpenShpRead, 
 gaiaReadShpEntity, gaiaShpAnalyze, gaiaWriteShpEntity, gaiaFlushShpHeaders
 
 \note on failure the object member \e Valid will be set to 0; and the
 object member \e LastError will contain the appropriate error message.
 \n the \e abstract pathname should not contain any suffix at all.
 */
    GAIAGEO_DECLARE void gaiaOpenShpWrite (gaiaShapefilePtr shp,
					   const char *path, int shape,
					   gaiaDbfListPtr list,
					   const char *charFrom,
					   const char *charTo);

/**
 Reads a feature from a Shapefile object

 \param shp pointer to the Shapefile object.
 \param current_row the row number identifying the feature to be read.
 \param srid feature's SRID 

 \return 0 on failure: any other value on success.

 \sa gaiaAllocShapefile, gaiaFreeShapefile, gaiaOpenShpRead, gaiaOpenShpWrite,
 gaiaShpAnalyze, gaiaWriteShpEntity, gaiaFlushShpHeaders

 \note on completion the Shapefile's \e Dbf member will contain the feature
 read:
 \li the \e Dbf->Geometry member will contain the corresponding Geometry
 \li and the \e Dbf->First member will point to the linked list containing
 the corresponding data attributes [both data formats and values].

 \remark the Shapefile object should be opened in \e read mode.
 */
    GAIAGEO_DECLARE int gaiaReadShpEntity (gaiaShapefilePtr shp,
					   int current_row, int srid);

/**
 Reads a feature from a Shapefile object

 \param shp pointer to the Shapefile object.
 \param current_row the row number identifying the feature to be read.
 \param srid feature's SRID 
 \param text_dates is TRUE all DBF dates will be considered as TEXT

 \return 0 on failure: any other value on success.

 \sa gaiaAllocShapefile, gaiaFreeShapefile, gaiaOpenShpRead, gaiaOpenShpWrite,
 gaiaShpAnalyze, gaiaWriteShpEntity, gaiaFlushShpHeaders

 \note on completion the Shapefile's \e Dbf member will contain the feature
 read:
 \li the \e Dbf->Geometry member will contain the corresponding Geometry
 \li and the \e Dbf->First member will point to the linked list containing
 the corresponding data attributes [both data formats and values].

 \remark the Shapefile object should be opened in \e read mode.
 */
    GAIAGEO_DECLARE int gaiaReadShpEntity_ex (gaiaShapefilePtr shp,
					      int current_row, int srid,
					      int text_dates);

/**
 Prescans a Shapefile object gathering informations

 \param shp pointer to the Shapefile object.

 \sa gaiaAllocShapefile, gaiaFreeShapefile, gaiaOpenShpRead, gaiaOpenShpWrite,
 gaiaReadShpEntity, gaiaWriteShpEntity, gaiaFlushShpHeaders

 \note on completion the Shapefile's \e EffectiveType will containt the
 Geometry type corresponding to features actually found.

 \remark the Shapefile object should be opened in \e read mode.
 */
    GAIAGEO_DECLARE void gaiaShpAnalyze (gaiaShapefilePtr shp);

/**
 Writes a feature into a Shapefile object
                                            
 \param shp pointer to the Shapefile object.
 \param entity pointer to DBF List object containing both Geometry and Field 
 values.

 \return 0 on failure: any other value on success.

 \sa gaiaAllocShapefile, gaiaFreeShapefile, gaiaOpenShpRead, gaiaOpenShpWrite,
 gaiaReadShpEntity, gaiaShpAnalyze, gaiaFlushShpHeaders

 \remark the Shapefile object should be opened in \e write mode.
 */
    GAIAGEO_DECLARE int gaiaWriteShpEntity (gaiaShapefilePtr shp,
					    gaiaDbfListPtr entity);

/**
 Writes into an output Shapefile any required header / footer

 \param shp pointer to the Shapefile object.

 \sa gaiaAllocShapefile, gaiaFreeShapefile, gaiaOpenShpRead, gaiaOpenShpWrite,
 gaiaReadShpEntity, gaiaShpAnalyze, gaiaWriteShpEntity

 \note forgetting to call gaiaFlushShpHeader for any Shapefile opened in
 \e write mode immediately before destroying the object, will surely 
 cause severe file corruption.
 */
    GAIAGEO_DECLARE void gaiaFlushShpHeaders (gaiaShapefilePtr shp);

/**
 Allocates a new DBF File object.

 \return the pointer to newly created DBF File object.

 \sa gaiaFreeDbf, gaiaOpenDbfRead, gaiaOpenDbfWrite,
 gaiaReadDbfEntity, gaiaWriteDbfEntity, gaiaFlushDbfHeader

 \note you are responsible to destroy (before or after) any allocated DBF File.
 \n you should phisically open the DBF File in \e read or \e write mode
 before performing any actual I/O operation.
 */
    GAIAGEO_DECLARE gaiaDbfPtr gaiaAllocDbf (void);

/**
 Destroys a DBF File object 
    
 \param dbf pointer to the DBF File object.
                                          
 \sa gaiaAllocDbf, gaiaFreeDbf, gaiaOpenDbfWrite,
 gaiaReadDbfEntity, gaiaWriteDbfEntity, gaiaFlushDbfHeader
 
 \note destroying the Shapefile object will close any related file:
 anyway you a responsible to explicitly call gaiaFlushShpHeader
 before destroyng a Shapefile opened in \e write mode.
 */
    GAIAGEO_DECLARE void gaiaFreeDbf (gaiaDbfPtr dbf);

/**
 Open a DBF File in read mode

 \param dbf pointer to the DBF File object.
 \param path pathname to the corresponding file-system file.
 \param charFrom GNU ICONV name identifying the input charset encoding.
 \param charTo GNU ICONV name identifying the output charset encoding.

 \sa gaiaAllocDbf, gaiaFreeDbf, gaiaOpenDbfWrite,
 gaiaReadDbfEntity, gaiaWriteDbfEntity, gaiaFlushDbfHeader

 \note on failure the object member \e Valid will be set to 0; and the
 object member \e LastError will contain the appropriate error message.
 */
    GAIAGEO_DECLARE void gaiaOpenDbfRead (gaiaDbfPtr dbf,
					  const char *path,
					  const char *charFrom,
					  const char *charTo);

/** 
 Open a DBF File in write mode

 \param dbf pointer to the DBF File object.
 \param path pathname to the corresponding file-system file.
 \param charFrom GNU ICONV name identifying the input charset encoding.
 \param charTo GNU ICONV name identifying the output charset encoding.

 \sa gaiaAllocDbf, gaiaFreeDbf, gaiaOpenDbfRead, 
 gaiaReadDbfEntity, gaiaWriteDbfEntity, gaiaFlushDbfHeader
 
 \note on failure the object member \e Valid will be set to 0; and the
 object member \e LastError will contain the appropriate error message.
 */
    GAIAGEO_DECLARE void gaiaOpenDbfWrite (gaiaDbfPtr dbf,
					   const char *path,
					   const char *charFrom,
					   const char *charTo);

/**
 Reads a record from a DBF File object

 \param dbf pointer to the DBF File object.
 \param current_row the row number identifying the record to be read.
 \param deleted on completion this variable will contain 0 if the record
 just read is valid: any other value if the record just read is marked as
 \e logically \e deleted.

 \return 0 on failure: any other value on success.

 \sa gaiaAllocDbf, gaiaFreeDbf, gaiaOpenDbfRead, gaiaOpenDbfWrite,
 gaiaFlushDbfHeader

 \note on completion the DBF File \e First member will point to the 
 linked list containing the corresponding data attributes [both data 
 formats and values].

 \remark the DBF File object should be opened in \e read mode.
 */
    GAIAGEO_DECLARE int gaiaReadDbfEntity (gaiaDbfPtr dbf, int current_row,
					   int *deleted);

/**
 Reads a record from a DBF File object

 \param dbf pointer to the DBF File object.
 \param current_row the row number identifying the record to be read.
 \param deleted on completion this variable will contain 0 if the record
 \param text_dates is TRUE all DBF dates will be considered as TEXT
 just read is valid: any other value if the record just read is marked as
 \e logically \e deleted.

 \return 0 on failure: any other value on success.

 \sa gaiaAllocDbf, gaiaFreeDbf, gaiaOpenDbfRead, gaiaOpenDbfWrite,
 gaiaFlushDbfHeader

 \note on completion the DBF File \e First member will point to the 
 linked list containing the corresponding data attributes [both data 
 formats and values].

 \remark the DBF File object should be opened in \e read mode.
 */
    GAIAGEO_DECLARE int gaiaReadDbfEntity_ex (gaiaDbfPtr dbf, int current_row,
					      int *deleted, int text_dates);

/**
 Writes a record into a DBF File object

 \param dbf pointer to the DBF File object.
 \param entity pointer to DBF List object containing Fields and corresponding
 values.

 \return 0 on failure: any other value on success.

 \sa gaiaAllocDbf, gaiaFreeDbf, gaiaOpenDbfRead, gaiaOpenDbfWrite,
 gaiaReadDbfEntity, gaiaFlushDbfHeader

 \remark the DBF File object should be opened in \e write mode.
 */
    GAIAGEO_DECLARE int gaiaWriteDbfEntity (gaiaDbfPtr dbf,
					    gaiaDbfListPtr entity);

/**
 Writes into an output DBF File any required header / footer

 \param dbf pointer to the DBF File object.

 \sa gaiaAllocDbf, gaiaFreeDbf, gaiaOpenDbfRead, gaiaOpenDbfWrite,
 gaiaReadDbfEntity, gaiaWriteDbfEntity

 \note forgetting to call gaiaFlushDbfHeader for any DBF File opened in
 \e write mode immediately before destroying the object, will surely 
 cause severe file corruption.
 */
    GAIAGEO_DECLARE void gaiaFlushDbfHeader (gaiaDbfPtr dbf);



#ifndef OMIT_ICONV		/* ICONV enabled: supporting text reader */

/** 
 Creates a Text Reader object

 \param path to the corresponding file-system file.
 \param field_separator the character acting as a separator between adjacent 
 fields.
 \param text_separator the character used to quote text strings.
 \param decimal_separator the character used as a separator between integer
 and decimal digits for real numeric values.
 \param first_line_titles 0 if the first line contains regular values:
 any other value if the first line contains column names.
 \param encoding GNU ICONV name identifying the input charset encoding.

 \return the pointer to the newly created Text Reader object: NULL on failure

 \sa gaiaTextReaderDestroy, gaiaTextReaderParse,
 gaiaTextReaderGetRow, gaiaTextReaderFetchField

 \note you are responsible to destroy (before or after) any allocated Text
 Reader object.
 */
    GAIAGEO_DECLARE gaiaTextReaderPtr gaiaTextReaderAlloc (const char *path,
							   char field_separator,
							   char text_separator,
							   char
							   decimal_separator,
							   int
							   first_line_titles,
							   const char
							   *encoding);

/**
 Destroys a Text Reader object

 \param reader pointer to Text Reader object.

 \sa gaiaTextReaderAlloc, gaiaTextReaderParse,
 gaiaTextReaderGetRow, gaiaTextReaderFetchField
 */
    GAIAGEO_DECLARE void gaiaTextReaderDestroy (gaiaTextReaderPtr reader);

/**
 Prescans the external file associated to a Text Reade object

 \param reader pointer to Text Reader object.

 \return 0 on failure: any other value on success.

 \sa gaiaTextReaderAlloc, gaiaTextReaderDestroy, 
 gaiaTextReaderGetRow, gaiaTextReaderFetchField

 \note this preliminary step is required so to ensure:
 \li file consistency: checking expected formatting rules.
 \li identifying the number / type / name of fields [aka columns].
 \li identifying the actual number of lines within the file.
 */
    GAIAGEO_DECLARE int gaiaTextReaderParse (gaiaTextReaderPtr reader);

/**
 Reads a line from a Text Reader object
 
 \param reader pointer to Text Reader object.
 \param row_num the Line Number identifying the Line to be read.

 \return 0 on failure: any other value on success.

 \sa gaiaTextReaderAlloc, gaiaTextReaderDestroy, gaiaTextReaderParse,
 gaiaTextReaderFetchField

 \note this function will load the requested Line into the current buffer:
 you can then use gaiaTextReaderFetchField in order to retrieve
 any individual field [aka column] value.
 */
    GAIAGEO_DECLARE int gaiaTextReaderGetRow (gaiaTextReaderPtr reader,
					      int row_num);

/**
 Retrieves an individual field value from the current Line

 \param reader pointer to Text Reader object.
 \param field_num relative field [aka column] index: first field has index 0.
 \param type on completion this variable will contain the value type.
 \param value on completion this variable will contain the current field value.

 \return 0 on failure: any other value on success.

 \sa gaiaTextReaderAlloc, gaiaTextReaderDestroy, gaiaTextReaderParse,
 gaiaTextReaderGetRow
 */
    GAIAGEO_DECLARE int gaiaTextReaderFetchField (gaiaTextReaderPtr reader,
						  int field_num, int *type,
						  const char **value);

#endif				/* end ICONV (text reader) */

#ifdef __cplusplus
}
#endif

#endif				/* _GG_FORMATS_H */
