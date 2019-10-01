/* 
 spatialite.h -- Gaia spatial support for SQLite 
  
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
 \file spatialite.h

 Main SpatiaLite header file
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef _WIN32
#ifdef DLL_EXPORT
#define SPATIALITE_DECLARE __declspec(dllexport)
#else
#define SPATIALITE_DECLARE extern
#endif
#else
#define SPATIALITE_DECLARE __attribute__ ((visibility("default")))
#endif
#endif

#ifndef _SPATIALITE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _SPATIALITE_H
#endif

#define SPLITE_AXIS_1		0x51
#define SPLITE_AXIS_2		0x52
#define SPLITE_AXIS_NAME	0x3e
#define SPLITE_AXIS_ORIENTATION	0x3f

#ifdef __cplusplus
extern "C"
{
#endif

#include <spatialite/gaiageo.h>

#ifdef LOADABLE_EXTENSION
    SPATIALITE_DECLARE int
	sqlite3_spatialite_init (sqlite3 * db, char **pzErrMsg,
				 const sqlite3_api_routines * pApi);
#endif

/**
 Initializes the library

 \note you are always expected to explicitly call this function
 before attempting to call any SpatiaLite own function.
 */
    SPATIALITE_DECLARE void spatialite_initialize (void);

/**
 Finalizes the library

 \note you are always expected to explicitly call this function
 immediately before exiting the main application.\n
 This function will free any memory allocation and will release
 any system resource internally used by the library.\n 
 */
    SPATIALITE_DECLARE void spatialite_shutdown (void);

/**
 Return the current library version.

 \return the version string.
 */
    SPATIALITE_DECLARE const char *spatialite_version (void);

/**
 Return the target CPU name.

 \return the target CPU string.
 */
    SPATIALITE_DECLARE const char *spatialite_target_cpu (void);

/**
 Initializes the internal memory block supporting each connection

 \sa spatialite_init_ex, spatialite_cleanup_ex

 */
    SPATIALITE_DECLARE void *spatialite_alloc_connection (void);

/**
 Initializes a SpatiaLite connection. 

 This function is now \b DEPRECATED because is not reentrant (not thread safe);
 use spatialite_init_ex() for all new development.

 \param verbose if TRUE a short start-up message is shown on stderr

 \sa spatialite_cleanup, spatialite_init_ex

 \note You absolutely must invoke this function before attempting to perform
 any other SpatiaLite's call.

 */
    SPATIALITE_DECLARE void spatialite_init (int verbose);

/**
 Initializes a SpatiaLite connection. 

 \param db_handle handle to the current SQLite connection
 \param ptr a memory pointer returned by spatialite_alloc_connection()
 \param verbose if TRUE a short start-up message is shown on stderr

 \sa spatialite_alloc_connection, spatialite_cleanup_ex, spatialite_init

 \note You absolutely must invoke this function before attempting to perform
 any other SpatiaLite's call.

 */
    SPATIALITE_DECLARE void spatialite_init_ex (sqlite3 * db_handle,
						const void *ptr, int verbose);

/**
 Initializes the GEOS library. 
 
 \note You are never supposed to invoke this function (internally handled).

 */
    SPATIALITE_DECLARE void spatialite_init_geos (void);

/**
 Cleanup a SpatiaLite connection

 This function is now \b DEPRECATED; use spatialite_cleanup_ex() for all new development.

 This function performs general cleanup, essentially undoing the effect
 of spatialite_init().

 \sa spatialite_init
*/
    SPATIALITE_DECLARE void spatialite_cleanup (void);

/**
 Cleanup a SpatiaLite connection

 This function performs general cleanup, essentially undoing the effect
 of spatialite_init_ex().

 \param ptr the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()

 \sa spatialite_init_ex, spatialite_alloc_connection
*/
    SPATIALITE_DECLARE void spatialite_cleanup_ex (const void *ptr);

/**
 Dumps a full geometry-table into an external Shapefile

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param column the name of the geometry column
 \param shp_path pathname of the Shapefile to be exported (no suffix) 
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param geom_type "POINT", "LINESTRING", "POLYGON", "MULTIPOLYGON" or NULL
 \param verbose if TRUE a short report is shown on stderr
 \param rows on completion will contain the total number of exported rows
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_shapefile (sqlite3 * sqlite, char *table,
					   char *column, char *shp_path,
					   char *charset, char *geom_type,
					   int verbose, int *rows,
					   char *err_msg);

/**
 Loads an external Shapefile into a newly created table

 \param sqlite handle to current DB connection
 \param shp_path pathname of the Shapefile to be imported (no suffix) 
 \param table the name of the table to be created
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param srid the SRID to be set for Geometries
 \param column the name of the geometry column
 \param coerce2d if TRUE any Geometry will be casted to 2D [XY]
 \param compressed if TRUE compressed Geometries will be created
 \param verbose if TRUE a short report is shown on stderr
 \param spatial_index if TRUE an R*Tree Spatial Index will be created
 \param rows on completion will contain the total number of imported rows
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success

 \sa load_shapefile_ex, load_shapefile_ex2

 \note this function simply calls load_shapefile_ex by passing 
  implicit gype="AUTO" and pk_column=NULL arguments
 */
    SPATIALITE_DECLARE int load_shapefile (sqlite3 * sqlite, char *shp_path,
					   char *table, char *charset, int srid,
					   char *column, int coerce2d,
					   int compressed, int verbose,
					   int spatial_index, int *rows,
					   char *err_msg);

/**
 Loads an external Shapefile into a newly created table

 \param sqlite handle to current DB connection
 \param shp_path pathname of the Shapefile to be imported (no suffix) 
 \param table the name of the table to be created
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param srid the SRID to be set for Geometries
 \param geo_column the name of the geometry column
 \param gtype expected to be one of: "LINESTRING", "LINESTRINGZ", 
  "LINESTRINGM", "LINESTRINGZM", "MULTILINESTRING", "MULTILINESTRINGZ",
  "MULTILINESTRINGM", "MULTILINESTRINGZM", "POLYGON", "POLYGONZ", "POLYGONM", 
  "POLYGONZM", "MULTIPOLYGON", "MULTIPOLYGONZ", "MULTIPOLYGONM", 
  "MULTIPOLYGONZM" or "AUTO".
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param coerce2d if TRUE any Geometry will be casted to 2D [XY]
 \param compressed if TRUE compressed Geometries will be created
 \param verbose if TRUE a short report is shown on stderr
 \param spatial_index if TRUE an R*Tree Spatial Index will be created
 \param rows on completion will contain the total number of imported rows
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success

 \sa load_shapefile, load_shapefile_ex2

 \note the Shapefile format doesn't supports any distinction between
  LINESTRINGs and MULTILINESTRINGs, or between POLYGONs and MULTIPOLYGONs;
  as does not allows to clearly distinguish if the M-measure is required.
 \n So a first preliminary scan of the Shapefile is required in order to
  correctly identify the actual payload (gtype = "AUTO", default case).
 \n By explicitly specifying some expected geometry type this first scan
  will be skipped at all thus introducing a noticeable performance gain.
 \n Anyway, declaring a mismatching geometry type will surely cause a failure.
 */
    SPATIALITE_DECLARE int load_shapefile_ex (sqlite3 * sqlite, char *shp_path,
					      char *table, char *charset,
					      int srid, char *geo_column,
					      char *gtype, char *pk_column,
					      int coerce2d, int compressed,
					      int verbose, int spatial_index,
					      int *rows, char *err_msg);

/**
 Loads an external Shapefile into a newly created table

 \param sqlite handle to current DB connection
 \param shp_path pathname of the Shapefile to be imported (no suffix) 
 \param table the name of the table to be created
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param srid the SRID to be set for Geometries
 \param geo_column the name of the geometry column
 \param gtype expected to be one of: "LINESTRING", "LINESTRINGZ", 
  "LINESTRINGM", "LINESTRINGZM", "MULTILINESTRING", "MULTILINESTRINGZ",
  "MULTILINESTRINGM", "MULTILINESTRINGZM", "POLYGON", "POLYGONZ", "POLYGONM", 
  "POLYGONZM", "MULTIPOLYGON", "MULTIPOLYGONZ", "MULTIPOLYGONM", 
  "MULTIPOLYGONZM" or "AUTO".
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param coerce2d if TRUE any Geometry will be casted to 2D [XY]
 \param compressed if TRUE compressed Geometries will be created
 \param verbose if TRUE a short report is shown on stderr
 \param spatial_index if TRUE an R*Tree Spatial Index will be created
 \param text_dates is TRUE all DBF dates will be considered as TEXT
 \param rows on completion will contain the total number of imported rows
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success

 \sa load_shapefile, load_shapefile_ex

 \note the Shapefile format doesn't supports any distinction between
  LINESTRINGs and MULTILINESTRINGs, or between POLYGONs and MULTIPOLYGONs;
  as does not allows to clearly distinguish if the M-measure is required.
 \n So a first preliminary scan of the Shapefile is required in order to
  correctly identify the actual payload (gtype = "AUTO", default case).
 \n By explicitly specifying some expected geometry type this first scan
  will be skipped at all thus introducing a noticeable performance gain.
 \n Anyway, declaring a mismatching geometry type will surely cause a failure.
 */
    SPATIALITE_DECLARE int load_shapefile_ex2 (sqlite3 * sqlite, char *shp_path,
					       char *table, char *charset,
					       int srid, char *geo_column,
					       char *gtype, char *pk_column,
					       int coerce2d, int compressed,
					       int verbose, int spatial_index,
					       int text_date, int *rows,
					       char *err_msg);

/**
 Loads an external DBF file into a newly created table

 \param sqlite handle to current DB connection
 \param dbf_path pathname of the DBF file to be imported
 \param table the name of the table to be created
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param verbose if TRUE a short report is shown on stderr
 \param rows on completion will contain the total number of actually exported rows
 \param err_msg on completion will contain an error message (if any)

 \sa load_dbf_ex, load_dbf_ex2

 \note this function simply calls load_dbf_ex by passing an
  implicit pk_column=NULL argument

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int load_dbf (sqlite3 * sqlite, char *dbf_path,
				     char *table, char *charset, int verbose,
				     int *rows, char *err_msg);

/**
 Loads an external DBF file into a newly created table

 \param sqlite handle to current DB connection
 \param dbf_path pathname of the DBF file to be imported
 \param table the name of the table to be created
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param verbose if TRUE a short report is shown on stderr
 \param rows on completion will contain the total number of actually exported rows
 \param err_msg on completion will contain an error message (if any)

 \sa load_dbf, load_dbf_ex2

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int load_dbf_ex (sqlite3 * sqlite, char *dbf_path,
					char *table, char *pk_column,
					char *charset, int verbose, int *rows,
					char *err_msg);

/**
 Loads an external DBF file into a newly created table

 \param sqlite handle to current DB connection
 \param dbf_path pathname of the DBF file to be imported
 \param table the name of the table to be created
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param verbose if TRUE a short report is shown on stderr
 \param text_dates is TRUE all DBF dates will be considered as TEXT
 \param rows on completion will contain the total number of imported rows
 \param err_msg on completion will contain an error message (if any)

 \sa load_dbf, load_dbf_ex

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int load_dbf_ex2 (sqlite3 * sqlite, char *dbf_path,
					 char *table, char *pk_column,
					 char *charset, int verbose,
					 int text_date, int *rows,
					 char *err_msg);

/**
 Dumps a full table into an external DBF file

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param dbf_path pathname of the DBF to be exported 
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param err_msg on completion will contain an error message (if any)
 
 \sa dump_dbf_ex

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_dbf (sqlite3 * sqlite, char *table,
				     char *dbf_path, char *charset,
				     char *err_msg);

/**
 Dumps a full table into an external DBF file

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param dbf_path pathname of the DBF to be exported 
 \param charset a valid GNU ICONV charset to be used for DBF text strings
 \param rows on completion will contain the total number of exported rows
 \param err_msg on completion will contain an error message (if any)
 
 \sa dump_dbf

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_dbf_ex (sqlite3 * sqlite, char *table,
					char *dbf_path, char *charset,
					int *rows, char *err_msg);

/**
 Loads an external spreadsheet (.xls) file into a newly created table

 \param sqlite handle to current DB connection
 \param path pathname of the spreadsheet file to be imported
 \param table the name of the table to be created
 \param worksheetIndex the index identifying the worksheet to be imported
 \param first_titles if TRUE the first line is assumed to contain column names
 \param rows on completion will contain the total number of actually exported rows
 \param err_msg on completion will contain an error message (if any)

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int load_XL (sqlite3 * sqlite, const char *path,
				    const char *table,
				    unsigned int worksheetIndex,
				    int first_titles, unsigned int *rows,
				    char *err_msg);

/**
 A portable replacement for C99 round()

 \param value a double value

 \return the nearest integeral value
 */
    SPATIALITE_DECLARE double math_round (double value);

/**
 A portable replacement for C99 llabs()

 \param value a 64 bit integer value

 \return the corresponding absolute value
 */
    SPATIALITE_DECLARE sqlite3_int64 math_llabs (sqlite3_int64 value);

/**
 Inserts the inlined EPSG dataset into the "spatial_ref_sys" table

 \param sqlite handle to current DB connection
 \param verbose if TRUE a short report is shown on stderr

 \return 0 on failure, any other value on success

 \sa spatial_ref_sys_init2

 \note this function is internally invoked by the SQL function 
  InitSpatialMetadata(), and is not usually intended for direct use.
  This functions is now deprecated, and will simply call
  spatial_ref_sys_init2(sqlite, GAIA_EPSG_ANY, verbose).
 */
    SPATIALITE_DECLARE int spatial_ref_sys_init (sqlite3 * sqlite, int verbose);

/**
 Inserts the inlined EPSG dataset into the "spatial_ref_sys" table

 \param sqlite handle to current DB connection
 \param mode can be one of GAIA_EPSG_ANY, GAIA_EPSG_NONE or GAIA_EPSG_WGS84_ONLY
 \param verbose if TRUE a short report is shown on stderr

 \return 0 on failure, any other value on success

 \note this function is internally invoked by the SQL function 
  InitSpatialMetadata(), and is not usually intended for direct use.
 */
    SPATIALITE_DECLARE int spatial_ref_sys_init2 (sqlite3 * sqlite, int mode,
						  int verbose);

/**
 Inserts some inlined EPSG definition into the "spatial_ref_sys" table

 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int insert_epsg_srid (sqlite3 * sqlite, int srid);

/**
 checks a SRID definition from the "spatial_ref_sys" table
 determining if it is of the geographic type

 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 
 \param geographic on successful completion will contain TRUE or FALSE

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int srid_is_geographic (sqlite3 * sqlite, int srid,
					       int *geographic);

/**
 checks a SRID definition from the "spatial_ref_sys" table
 determining if it is of the projected type

 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 
 \param projected on successful completion will contain TRUE or FALSE

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int srid_is_projected (sqlite3 * sqlite, int srid,
					      int *projected);

/**
 checks a SRID definition from the "spatial_ref_sys" table
 determining if the axes order is X-Y or Y-X

 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 
 \param flipped on successful completion will contain 0 (FALSE) if axes order 
 is X-Y, any other value (TRUE) if axes order is Y-X.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int srid_has_flipped_axes (sqlite3 * sqlite, int srid,
						  int *flipped);

/**
 checks a SRID definition from the "spatial_ref_sys" table
 then returning the corresponding Spheroid name
 
 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 

 \return the Spheroid name on succes, NULL on failure 
 
 \note you are responsible for freeing the returned name.
 */
    SPATIALITE_DECLARE char *srid_get_spheroid (sqlite3 * sqlite, int srid);

/**
 checks a SRID definition from the "spatial_ref_sys" table
 then returning the corresponding Prime Meridian name
 
 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 

 \return the Prime Meridian name on succes, NULL on failure 
 
 \note you are responsible for freeing the returned name.
 */
    SPATIALITE_DECLARE char *srid_get_prime_meridian (sqlite3 * sqlite,
						      int srid);

/**
 checks a SRID definition from the "spatial_ref_sys" table
 then returning the corresponding Projection name
 
 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 

 \return the Projection name on succes, NULL on failure 
 
 \note you are responsible for freeing the returned name.
 */
    SPATIALITE_DECLARE char *srid_get_projection (sqlite3 * sqlite, int srid);

/**
 checks a SRID definition from the "spatial_ref_sys" table
 then returning the corresponding Datum name
 
 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 

 \return the Datum name on succes, NULL on failure 
 
 \note you are responsible for freeing the returned name.
 */
    SPATIALITE_DECLARE char *srid_get_datum (sqlite3 * sqlite, int srid);

/**
 checks a SRID definition from the "spatial_ref_sys" table
 then returning the corresponding Unit name
 
 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition 

 \return the Unit name on succes, NULL on failure 
 
 \note you are responsible for freeing the returned name.
 */
    SPATIALITE_DECLARE char *srid_get_unit (sqlite3 * sqlite, int srid);

/**
 checks a SRID definition from the "spatial_ref_sys" table
 then returning an Axis definition
 
 \param sqlite handle to current DB connection
 \param srid the SRID value uniquely identifying the required EPSG definition
 \param axis should be one of SPLITE_AXIS_1 or SPLITE_AXIS_2
 \param mode should be one of SPLITE_AXIS_NAME or SPLITE_AXIS_ORIENTATION 

 \return the reqested name on succes, NULL on failure 
 
 \note you are responsible for freeing the returned name.
 */
    SPATIALITE_DECLARE char *srid_get_axis (sqlite3 * sqlite, int srid,
					    char axis, char mode);

/**
 Checks if a column is actually defined into the given table

 \param sqlite handle to current DB connection
 \param table the table to be checked
 \param column the column to be checked

 \return 0 on success, any other value on success

 \note internally used to detect if some KML attribute defaults to a constant value
 */
    SPATIALITE_DECLARE int
	is_kml_constant (sqlite3 * sqlite, char *table, char *column);

/**
 Dumps a full geometry-table into an external KML file

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param geom_col the name of the geometry column
 \param kml_path pathname of the KML file to be exported 
 \param name_col column to be used for KML "name" (may be null)
 \param desc_col column to be used for KML "description" (may be null)
 \param precision number of decimal digits for coordinates
 
 \sa dump_kml_ex

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_kml (sqlite3 * sqlite, char *table,
				     char *geom_col, char *kml_path,
				     char *name_col, char *desc_col,
				     int precision);

/**
 Dumps a full geometry-table into an external KML file

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param geom_col the name of the geometry column
 \param kml_path pathname of the KML file to be exported 
 \param name_col column to be used for KML "name" (may be null)
 \param desc_col column to be used for KML "description" (may be null)
 \param precision number of decimal digits for coordinates
 \param rows on completion will contain the total number of exported rows
 
 \sa dump_kml

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_kml_ex (sqlite3 * sqlite, char *table,
					char *geom_col, char *kml_path,
					char *name_col, char *desc_col,
					int precision, int *rows);

/**
 Checks for duplicated rows into the same table

 \param sqlite handle to current DB connection
 \param table name of the table to be checked
 \param dupl_count on completion will contain the number of duplicated rows found

 \sa remove_duplicated_rows
 \note two (or more) rows are assumed to be duplicated if any column

 value (excluding any Primary Key column) is exacly the same
 */
    SPATIALITE_DECLARE void check_duplicated_rows (sqlite3 * sqlite,
						   char *table,
						   int *dupl_count);

/**
 Remove duplicated rows from a table

 \param sqlite handle to current DB connection
 \param table name of the table to be cleaned

 \sa check_duplicated_rows, remove_duplicated_rows_ex

 \note when two (or more) duplicated rows exist, only the first occurence
 will be preserved, then deleting any further occurrence.
 */
    SPATIALITE_DECLARE void remove_duplicated_rows (sqlite3 * sqlite,
						    char *table);

/**
 Remove duplicated rows from a table

 \param sqlite handle to current DB connection
 \param table name of the table to be cleaned
 \param removed on successful completion will contain the total
 count of removed duplicate rows

 \sa check_duplicated_rows, remove_duplicated_rows_ex2

 \note when two (or more) duplicated rows exist, only the first occurence
 will be preserved, then deleting any further occurrence.
 */
    SPATIALITE_DECLARE void remove_duplicated_rows_ex (sqlite3 * sqlite,
						       char *table,
						       int *removed);

/**
 Remove duplicated rows from a table

 \param sqlite handle to current DB connection
 \param table name of the table to be cleaned
 \param removed on successful completion will contain the total
 count of removed duplicate rows
 \param transaction boolena; if set to TRUE will internally handle
 a SQL Transaction

 \sa check_duplicated_rows, remove_duplicated_rows

 \note when two (or more) duplicated rows exist, only the first occurence
 will be preserved, then deleting any further occurrence.
 */
    SPATIALITE_DECLARE void remove_duplicated_rows_ex2 (sqlite3 * sqlite,
							char *table,
							int *removed,
							int transaction);

/**
 Creates a derived table surely containing elementary Geometries

 \param sqlite handle to current DB connection
 \param inTable name of the input table 
 \param geometry name of the Geometry column
 \param outTable name of the output table to be created
 \param pKey name of the Primary Key column in the output table
 \param multiId name of the column identifying origins in the output table
 
 \sa elementary_geometries_ex

 \note if the input table contains some kind of complex Geometry
 (MULTIPOINT, MULTILINESTRING, MULTIPOLYGON or GEOMETRYCOLLECTION),
 then many rows are inserted into the output table: each single 
 row will contain the same attributes and an elementaty Geometry.
 All the rows created by expanding the same input row will expose
 the same value in the "multiId" column.
 */
    SPATIALITE_DECLARE void elementary_geometries (sqlite3 * sqlite,
						   char *inTable,
						   char *geometry,
						   char *outTable, char *pKey,
						   char *multiId);

/**
 Creates a derived table surely containing elementary Geometries

 \param sqlite handle to current DB connection
 \param inTable name of the input table 
 \param geometry name of the Geometry column
 \param outTable name of the output table to be created
 \param pKey name of the Primary Key column in the output table
 \param multiId name of the column identifying origins in the output table
 \param rows on completion will contain the total number of inserted rows
 
 \sa elementary_geometries_ex2

 \note if the input table contains some kind of complex Geometry
 (MULTIPOINT, MULTILINESTRING, MULTIPOLYGON or GEOMETRYCOLLECTION),
 then many rows are inserted into the output table: each single 
 row will contain the same attributes and an elementaty Geometry.
 All the rows created by expanding the same input row will expose
 the same value in the "multiId" column.
 */
    SPATIALITE_DECLARE void elementary_geometries_ex (sqlite3 * sqlite,
						      char *inTable,
						      char *geometry,
						      char *outTable,
						      char *pKey, char *multiId,
						      int *rows);

/**
 Creates a derived table surely containing elementary Geometries

 \param sqlite handle to current DB connection
 \param inTable name of the input table 
 \param geometry name of the Geometry column
 \param outTable name of the output table to be created
 \param pKey name of the Primary Key column in the output table
 \param multiId name of the column identifying origins in the output table
 \param rows on completion will contain the total number of inserted rows
 \param transaction boolena; if set to TRUE will internally handle
 a SQL Transaction
 
 \sa elementary_geometries

 \note if the input table contains some kind of complex Geometry
 (MULTIPOINT, MULTILINESTRING, MULTIPOLYGON or GEOMETRYCOLLECTION),
 then many rows are inserted into the output table: each single 
 row will contain the same attributes and an elementaty Geometry.
 All the rows created by expanding the same input row will expose
 the same value in the "multiId" column.
 */
    SPATIALITE_DECLARE void elementary_geometries_ex2 (sqlite3 * sqlite,
						       char *inTable,
						       char *geometry,
						       char *outTable,
						       char *pKey,
						       char *multiId, int *rows,
						       int transaction);

/**
 Dumps a full geometry-table into an external GeoJSON file

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param geom_col the name of the geometry column
 \param outfile_path pathname for the GeoJSON file to be written to
 \param precision number of decimal digits for coordinates
 \param option the format to use for output
 
 \sa dump_geojson_rx

 \note valid values for option are:
   - 0 no option
   - 1 GeoJSON MBR
   - 2 GeoJSON Short CRS (e.g EPSG:4326)
   - 3 MBR + Short CRS
   - 4 GeoJSON Long CRS (e.g urn:ogc:def:crs:EPSG::4326)
   - 5 MBR + Long CRS

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_geojson (sqlite3 * sqlite, char *table,
					 char *geom_col, char *outfile_path,
					 int precision, int option);

/**
 Dumps a full geometry-table into an external GeoJSON file

 \param sqlite handle to current DB connection
 \param table the name of the table to be exported
 \param geom_col the name of the geometry column
 \param outfile_path pathname for the GeoJSON file to be written to
 \param precision number of decimal digits for coordinates
 \param option the format to use for output
 \param rows on completion will contain the total number of exported rows
 
 \sa dump_geojson

 \note valid values for option are:
   - 0 no option
   - 1 GeoJSON MBR
   - 2 GeoJSON Short CRS (e.g EPSG:4326)
   - 3 MBR + Short CRS
   - 4 GeoJSON Long CRS (e.g urn:ogc:def:crs:EPSG::4326)
   - 5 MBR + Long CRS

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int dump_geojson_ex (sqlite3 * sqlite, char *table,
					    char *geom_col, char *outfile_path,
					    int precision, int option,
					    int *rows);

/**
 Updates the LAYER_STATICS metadata table

 \param sqlite handle to current DB connection
 \param table name of the table to be processed
 \param column name of the geometry to be processed

 \note this function will explore the given table/geometry determining
 the number of rows and the full layer extent; a corresponding table/geometry
 entry is expected to be already declared in the GEOMETRY_COLUMNS table.
 These informations will be permanently stored into the LAYER_STATISTICS
 table; if such table does not yet exists will be implicitly created.
   - if table is NULL, any entry found within GEOMETRY_COLUMNS
     will be processed.
   - if table is not NULL and column is NULL, any geometry
     belonging to the given table will be processed.
   - if both table and column are not NULL, then only the
     given entry will be processed.

 \sa gaiaStatisticsInvalidate, gaiaGetLayerExtent, gaiaGetVectorLayersList

 \return 0 on failure, the total count of processed entries on success
 */
    SPATIALITE_DECLARE int update_layer_statistics (sqlite3 * sqlite,
						    const char *table,
						    const char *column);

/**
 Immediately and unconditionally invalidates the already existing Statistics

 \param handle SQLite handle to current DB connection.
 \param table VectorLayer Table (or View, or VirtualShape).
 \param geometry Geometry Column name.
 
 \return 0 on success, any other value on success

 \sa update_layer_statistics, gaiaGetLayerExtent, gaiaGetVectorLayersList

 \note if the table arg is NULL all Statistics for any VectorLayer defined within 
 the DB will be invalidated; otherwise only a single Layer will be affectedd (if existing).
 \n By defining the geometry arg (not NULL) you can further restrict your selection.
 */
    SPATIALITE_DECLARE int gaiaStatisticsInvalidate (sqlite3 * handle,
						     const char *table,
						     const char *geometry);

/**
 Queries the Metadata tables returning the Layer Full Extent

 \param handle SQLite handle to current DB connection.
 \param table VectorLayer Table (or View, or VirtualShape).
 \param geometry Geometry Column name.
 \param mode if TRUE a PESSIMISTIC statistics update will be implied,
  otherwise OPTIMISTIC.
 
 \return the pointer to the newly created Geometry (Envelope): NULL on failure

 \sa update_layer_statistic, gaiaStatisticsInvalidate, gaiaGetVectorLayersList

 \note you are responsible to destroy (before or after) any allocated
 Geometry returned by gaiaGetLayerExtent().
 \n The geometry arg is optional when the table simply has a single Geometry Column,
  and can be NULL in this case.
 \n When the mode arg is set to FALSE (default) then the returned infos
  will be simply retrieved from the staticized statistic tables (faster, but could be inaccurate).
 \n If the mode arg is set to TRUE a preliminary attempt to update the
  statistic tables will be always performed (probably slower, but surely accurate).
 \n If the named Layer doesn't exist, or if it's completely empty (not containing
 any valid Geometry) NULL will be returned.
 */
    SPATIALITE_DECLARE gaiaGeomCollPtr gaiaGetLayerExtent (sqlite3 * handle,
							   const char *table,
							   const char *geometry,
							   int mode);

/**
 Queries the Metadata tables supporting Vector Layers

 \param handle SQLite handle to current DB connection.
 \param table VectorLayer Table (or View, or VirtualShape).
 \param geometry Geometry Column name.
 \param mode one of GAIA_VECTORS_LIST_OPTIMISTIC or GAIA_VECTORS_LIST_PESSIMISTIC.
 
 \return the pointer to the newly created VectorLayersList object: NULL on failure

 \sa gaiaFreeVectorLayersList, update_layer_statistics, gaiaStatisticsInvalidate, 
 gaiaGetLayerExtent, gaiaGetVectorLayersList

 \note you are responsible to destroy (before or after) any allocated
 VectorLayersList returned by gaiaGetVectorLayersList().
 \n If the table arg is NULL all VectorLayers defined within the DB will be reported;
  otherwise only a single Layer will be reported (if existing).
 \n By defining the geometry arg (not NULL) you can further restrict the returned report.
 \n When the mode arg is set to GAIA_VECTORS_LIST_OPTIMISTIC (default) then the returned infos
  will be simply retrieved from the staticized statistic tables (faster, but could be inaccurate).
 \n If the mode arg is set to GAIA_VECTORS_LIST_PESSIMISTIC a preliminary attempt to update the
  statistic tables will be always performed (probably slower, but surely accurate).
 */
    SPATIALITE_DECLARE gaiaVectorLayersListPtr gaiaGetVectorLayersList (sqlite3
									*
									handle,
									const
									char
									*table,
									const
									char
									*geometry,
									int
									mode);

/**
 Creates (or re-creates) the "splite_metacatalog" and 
 "splite_metacalog_statistics" tables.

 \param handle SQLite handle to current DB connection.
 
 \return 0 (FALSE) on failure, any other value (TRUE) on success

 \sa gaiaUpdateMetaCatalogStatistics, gaiaUpdateMetaCatalogStatisticsFromMaster
 */
    SPATIALITE_DECLARE int gaiaCreateMetaCatalogTables (sqlite3 * handle);

/**
 Updates the "splite_metacatalog_statistics" table.

 \param handle SQLite handle to current DB connection.
 \param table name of the table to be processed.
 \param column name of the column to be processed.
 
 \return 0 (FALSE) on failure, any other value (TRUE) on success

 \sa gaiaCreateMetaCatalogTables, gaiaUpdateMetaCatalogStatisticsFromMaster
 */
    SPATIALITE_DECLARE int gaiaUpdateMetaCatalogStatistics (sqlite3 * handle,
							    const char *table,
							    const char *column);

/**
 Updates the "splite_metacatalog_statistics" table (using a Master Table).

 \param handle SQLite handle to current DB connection.
 \param master_table name of the master-table controlling the whole process.
 \param table_name name of the column into the master-table containing table-names.
 \param column_name name of the column into the master-table containing column-names.
 
 \return 0 (FALSE) on failure, any other value (TRUE) on success

 \sa gaiaCreateMetaCatalogTables, gaiaUpdateMetaCatalogStatistics
 */
    SPATIALITE_DECLARE int gaiaUpdateMetaCatalogStatisticsFromMaster (sqlite3 *
								      handle,
								      const char
								      *master_table,
								      const char
								      *table_name,
								      const char
								      *column_name);

/**
 Destroys a VectorLayersList object

 \param ptr pointer to the VectorLayersList object to be destroyed

 \sa gaiaGetVectorLayersList
 */
    SPATIALITE_DECLARE void gaiaFreeVectorLayersList (gaiaVectorLayersListPtr
						      ptr);

/**
 Drops a layer-table, removing any related dependency

 \param sqlite handle to current DB connection
 \param table name of the table to be removed

 \note this function will drop a SpatialTable, SpatialView or VirtualShape being
 properly registered within the Metadata tables.
 \n an eventual Spatial Index will be dropped as well, and any row referring the
 selected table will be removed from the Metadata tables.

 \return 0 on failure, any other value on success

 \sa gaiaDropTableEx

 \note this one simply is a convenience method alway defaulting to
 gaiaDropTableEx(sqlite, "main", table);
 */
    SPATIALITE_DECLARE int gaiaDropTable (sqlite3 * sqlite, const char *table);

/**
 Drops a layer-table, removing any related dependency

 \param sqlite handle to current DB connection
 \param prefix schema prefix identifying the target DB\n
 "main" always identifies the main DB (primary, not Attached).
 \param table name of the table to be removed

 \note this function will drop a SpatialTable, SpatialView or VirtualShape being
 properly registered within the Metadata tables.
 \n an eventual Spatial Index will be dropped as well, and any row referring the
 selected table will be removed from the Metadata tables.

 \return 0 on failure, any other value on success

 \sa gaiaDropTableEx2
 */
    SPATIALITE_DECLARE int gaiaDropTableEx (sqlite3 * sqlite,
					    const char *prefix,
					    const char *table);

/**
 Drops a layer-table, removing any related dependency

 \param sqlite handle to current DB connection
 \param prefix schema prefix identifying the target DB\n
 "main" always identifies the main DB (primary, not Attached).
 \param table name of the table to be removed
 \param transaction boolena; if set to TRUE will internally handle
 a SQL Transaction

 \note this function will drop a SpatialTable, SpatialView or VirtualShape being
 properly registered within the Metadata tables.
 \n an eventual Spatial Index will be dropped as well, and any row referring the
 selected table will be removed from the Metadata tables.

 \return 0 on failure, any other value on success

 \sa gaiaDropTable
 */
    SPATIALITE_DECLARE int gaiaDropTableEx2 (sqlite3 * sqlite,
					     const char *prefix,
					     const char *table,
					     int transaction);

/**
 Checks a Geometry Column for validity

 \param sqlite handle to current DB connection
 \param table name of the table 
 \param geometry name of the column to be checked
 \param report_path pathname of the report-file
 \param n_rows if this variable is not NULL on successful completion will
 contain the total number of rows found into the checkeck table
 \param n_invalids if this variable is not NULL on successful completion will
 contain the total number of invalid Geometries found into the checkeck table
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \sa check_geometry_column_r, check_all_geometry_columns,
 sanitize_geometry_column, sanitize_all_geometry_columns

 \note this function will check a Geometry Column (layer) for validity;
 a HTML report will be produced.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()\n
 not reentrant and thread unsafe.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int check_geometry_column (sqlite3 * sqlite,
						  const char *table,
						  const char *geom,
						  const char *report_path,
						  int *n_rows, int *n_invalids,
						  char **err_msg);

/**
 Checks a Geometry Column for validity

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param sqlite handle to current DB connection
 \param table name of the table 
 \param geometry name of the column to be checked
 \param report_path pathname of the report-file
 \param n_rows if this variable is not NULL on successful completion will
 contain the total number of rows found into the checkeck table
 \param n_invalids if this variable is not NULL on successful completion will
 contain the total number of invalid Geometries found into the checkeck table
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \sa check_geometry_column, check_all_geometry_columns,
 sanitize_geometry_column, sanitize_all_geometry_columns

 \note this function will check a Geometry Column (layer) for validity;
 a HTML report will be produced.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()\n
 reentrant and thread-safe.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int check_geometry_column_r (const void *p_cache,
						    sqlite3 * sqlite,
						    const char *table,
						    const char *geom,
						    const char *report_path,
						    int *n_rows,
						    int *n_invalids,
						    char **err_msg);

/**
 Checks all Geometry Columns for validity

 \param sqlite handle to current DB connection
 \param output_dir pathname of the directory to be created for report-files
 \param n_invalids if this variable is not NULL on successful completion will
 contain the total number of invalid Geometries found
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \sa check_all_geometry_columns_r, check_geometry_column,
 sanitize_geometry_column, sanitize_all_geometry_columns

 \note this function will check all Geometry Columns (vector layers) for validity;
 a HTML report will be produced.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()\n
 not reentrant and thread unsafe.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int check_all_geometry_columns (sqlite3 * sqlite,
						       const char *output_dir,
						       int *n_invalids,
						       char **err_msg);

/**
 Checks all Geometry Columns for validity

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param sqlite handle to current DB connection
 \param output_dir pathname of the directory to be created for report-files
 \param n_invalids if this variable is not NULL on successful completion will
 contain the total number of invalid Geometries found
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \sa check_all_geometry_columns, check_geometry_column,
 sanitize_geometry_column, sanitize_all_geometry_columns

 \note this function will check all Geometry Columns (vector layers) for validity;
 a HTML report will be produced.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()\n
 reentrant and thread-safe.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int check_all_geometry_columns_r (const void *p_cache,
							 sqlite3 * sqlite,
							 const char *output_dir,
							 int *n_invalids,
							 char **err_msg);

/**
 Sanitizes a Geometry Column making all invalid geometries to be valid

 \param sqlite handle to current DB connection
 \param table name of the table 
 \param geometry name of the column to be checked
 \param tmp_table name of the temporary table
 \param report_path pathname of the report-file
 \param n_invalids if this variable is not NULL on successful completion will
 contain the total number of invalid Geometries found into the sanitize table
 \param n_repaired if this variable is not NULL on successful completion will
 contain the total number of repaired Geometries
 \param n_discarded if this variable is not NULL on successful completion will
 contain the total number of repaired Geometries (by discarding fragments)
 \param n_failures if this variable is not NULL on successful completion will
 contain the total number of repair failures (i.e. Geometries beyond possible repair)
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \sa sanitize_geometry_column_r, check_geometry_column,
 check_all_geometry_columns, sanitize_all_geometry_columns

 \note this function will attempt to make valid all invalid geometries
 found within a Geometry Column (layer); a temporary table is required.
 \n if the process has full success the temprary table will be deleted;
 otherwise it will be preserved for further inspection.
 a HTML report will be produced as well.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()\n
 not reentrant and thread unsafe.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int sanitize_geometry_column (sqlite3 * sqlite,
						     const char *table,
						     const char *geom,
						     const char *tmp_table,
						     const char *report_path,
						     int *n_invalids,
						     int *n_repaired,
						     int *n_discarded,
						     int *n_failures,
						     char **err_msg);

/**
 Sanitizes a Geometry Column making all invalid geometries to be valid

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param sqlite handle to current DB connection
 \param table name of the table 
 \param geometry name of the column to be checked
 \param tmp_table name of the temporary table
 \param report_path pathname of the report-file
 \param n_invalids if this variable is not NULL on successful completion will
 contain the total number of invalid Geometries found into the sanitize table
 \param n_repaired if this variable is not NULL on successful completion will
 contain the total number of repaired Geometries
 \param n_discarded if this variable is not NULL on successful completion will
 contain the total number of repaired Geometries (by discarding fragments)
 \param n_failures if this variable is not NULL on successful completion will
 contain the total number of repair failures (i.e. Geometries beyond possible repair)
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \sa sanitize_geometry_column, check_geometry_column,
 check_all_geometry_columns, sanitize_all_geometry_columns

 \note this function will attempt to make valid all invalid geometries
 found within a Geometry Column (layer); a temporary table is required.
 \n if the process has full success the temprary table will be deleted;
 otherwise it will be preserved for further inspection.
 a HTML report will be produced as well.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()\n
 reentrant and thread-safe.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int sanitize_geometry_column_r (const void *p_cache,
						       sqlite3 * sqlite,
						       const char *table,
						       const char *geom,
						       const char *tmp_table,
						       const char *report_path,
						       int *n_invalids,
						       int *n_repaired,
						       int *n_discarded,
						       int *n_failures,
						       char **err_msg);

/**
 Sanitizes all Geometry Columns making all invalid geometries to be valid

 \param sqlite handle to current DB connection
 \param tmp_prefix name-prefix for temporary tables
 \param output_dir pathname of the directory to be created for report-files
 \param not_repaired if this variable is not NULL on successful completion will
 contain the total count of repair failures (i.e. Geometries beyond possible repair)
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \sa sanitize_all_geometry_columns_r, check_geometry_column,
 check_all_geometry_columns, sanitize_geometry_column

 \note this function will attempt to make valid all invalid geometries
 found within all Geometry Columns (vector layers); a temporary table is 
 required so to support each input table.
 \n if the process has full success the temprary table will be deleted;
 otherwise it will be preserved for further inspection.
 a HTML report will be produced as well.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()\n
 not reentrant and thread unsafe.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int sanitize_all_geometry_columns (sqlite3 * sqlite,
							  const char
							  *tmp_prefix,
							  const char
							  *output_dir,
							  int *not_repaired,
							  char **err_msg);

/**
 Sanitizes all Geometry Columns making all invalid geometries to be valid

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param sqlite handle to current DB connection
 \param tmp_prefix name-prefix for temporary tables
 \param output_dir pathname of the directory to be created for report-files
 \param not_repaired if this variable is not NULL on successful completion will
 contain the total count of repair failures (i.e. Geometries beyond possible repair)
 \param err_msg if this variable is not NULL and the return status is ZERO
 (failure), an appropriate error message will be returned

 \sa sanitize_all_geometry_columns, check_geometry_column,
 check_all_geometry_columns, sanitize_geometry_column

 \note this function will attempt to make valid all invalid geometries
 found within all Geometry Columns (vector layers); a temporary table is 
 required so to support each input table.
 \n if the process has full success the temprary table will be deleted;
 otherwise it will be preserved for further inspection.
 a HTML report will be produced as well.
 \n an eventual error message returned via err_msg requires to be deallocated
 by invoking free()\n
 reentrant and thread-safe.

 \return 0 on failure, any other value on success
 */
    SPATIALITE_DECLARE int sanitize_all_geometry_columns_r (const void *p_cache,
							    sqlite3 * sqlite,
							    const char
							    *tmp_prefix,
							    const char
							    *output_dir,
							    int *not_repaired,
							    char **err_msg);

    SPATIALITE_DECLARE int gaiaGPKG2Spatialite (sqlite3 * handle_in,
						const char *gpkg_in_path,
						sqlite3 * handle_out,
						const char *splite_out_path);
    SPATIALITE_DECLARE int gaiaSpatialite2GPKG (sqlite3 * handle_in,
						const char *splite_in_path,
						sqlite3 * handle_out,
						const char *gpkg_out_path);

#ifdef __cplusplus
}
#endif

#endif				/* _SPATIALITE_H */
