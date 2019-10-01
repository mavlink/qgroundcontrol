/*
 gg_wfs.h -- Gaia common support for WFS
  
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
 \file gg_wfs.h

 WFS support
 */

#ifndef _GG_WFS_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_WFS_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct gaia_wfs_catalog gaiaWFScatalog;
    typedef gaiaWFScatalog *gaiaWFScatalogPtr;

    typedef struct gaia_wfs_item gaiaWFSitem;
    typedef gaiaWFSitem *gaiaWFSitemPtr;

    typedef struct gaia_wfs_schema gaiaWFSschema;
    typedef gaiaWFSschema *gaiaWFSschemaPtr;

    typedef struct gaia_wfs_column gaiaWFScolumn;
    typedef gaiaWFScolumn *gaiaWFScolumnPtr;

/**
 Loads data from some WFS source 

 \param sqlite handle to current DB connection
 \param path_or_url pointer to some WFS-GetFeature XML Document (could be a pathname or an URL).
 \param alt_describe_uri an alternative URI for DescribeFeatureType to be used
 if no one is found within the XML document returned by GetFeature.
 \param layer_name the name of the WFS layer. 
 \param swap_axes if TRUE the X and Y axes will be swapped 
 \param table the name of the table to be created
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param spatial_index if TRUE an R*Tree Spatial Index will be created
 \param rows on completion will contain the total number of actually imported rows
 \param err_msg on completion will contain an error message (if any)
 \param progress_callback pointer to a callback function to be invoked immediately
 after processing each WFS page (could be NULL)
 \param callback_ptr an arbitrary pointer (to be passed as the second argument
 by the callback function).
 
 \sa create_wfs_catalog, load_from_wfs_paged, reset_wfs_http_connection

 \return 0 on failure, any other value on success
 
 \note an eventual error message returned via err_msg requires to be deallocated
 by invoking free()
 \n please note: this one simply is a convenience method, and exactly corresponds
 to load_from_wfs_paged() setting a negative page size. 
 */
    SPATIALITE_DECLARE int load_from_wfs (sqlite3 * sqlite,
					  const char *path_or_url,
					  const char *alt_describe_uri,
					  const char *layer_name, int swap_axes,
					  const char *table,
					  const char *pk_column_name,
					  int spatial_index, int *rows,
					  char **err_msg,
					  void (*progress_callback) (int,
								     void *),
					  void *callback_ptr);

/**
 Loads data from some WFS source (using WFS paging)

 \param sqlite handle to current DB connection
 \param path_or_url pointer to some WFS-GetFeature XML Document (could be a pathname or an URL).
 \param alt_describe_uri an alternative URI for DescribeFeatureType to be used
 if no one is found within the XML document returned by GetFeature.
 \param layer_name the name of the WFS layer.
 \param swap_axes if TRUE the X and Y axes will be swapped 
 \param table the name of the table to be created
 \param pk_column name of the Primary Key column; if NULL or mismatching
 then "PK_UID" will be assumed by default.
 \param spatial_index if TRUE an R*Tree Spatial Index will be created
 \param page_size max number of features for each single WFS call; if zero or
 negative a single monolithic page is assumed (i.e. paging will not be applied).
 \param rows on completion will contain the total number of actually imported rows
 \param err_msg on completion will contain an error message (if any)
 \param progress_callback pointer to a callback function to be invoked immediately
 after processing each WFS page (could be NULL)
 \param callback_ptr an arbitrary pointer (to be passed as the second argument
 by the callback function).
 
 \sa create_wfs_catalog, load_from_wfs, reset_wfs_http_connection

 \return 0 on failure, any other value on success
 
 \note an eventual error message returned via err_msg requires to be deallocated
 by invoking free()

 \note the progress_callback function must have this signature: 
 \b void \b myfunct(\b int \b count, \b void \b *ptr);
 \n and will cyclically report how many features have been processed since the initial call start.
 */
    SPATIALITE_DECLARE int load_from_wfs_paged (sqlite3 * sqlite,
						const char *path_or_url,
						const char *alt_describe_uri,
						const char *layer_name,
						int swap_axes,
						const char *table,
						const char *pk_column_name,
						int spatial_index,
						int page_size, int *rows,
						char **err_msg,
						void (*progress_callback) (int,
									   void
									   *),
						void *callback_ptr);

/**
 Creates a Catalog for some WFS service 

 \param path_or_url pointer to some WFS-GetCapabilities XML Document (could be a pathname or an URL). 
 \param err_msg on completion will contain an error message (if any)

 \return the pointer to the corresponding WFS-Catalog object: NULL on failure
 
 \sa destroy_wfs_catalog, get_wfs_catalog_count, get_wfs_catalog_item, load_from_wfs,
 reset_wfs_http_connection, get_wfs_version
 
 \note an eventual error message returned via err_msg requires to be deallocated
 by invoking free().\n
 you are responsible to destroy (before or after) any WFS-Catalog returned by create_wfs_catalog().
 */
    SPATIALITE_DECLARE gaiaWFScatalogPtr create_wfs_catalog (const char
							     *path_or_url,
							     char **err_msg);

/**
 Destroys a WFS-Catalog object freeing any allocated resource 

 \param handle the pointer to a valid WFS-Catalog returned by a previous call
 to create_wfs_catalog()
 
 \sa create_wfs_catalog
 */
    SPATIALITE_DECLARE void destroy_wfs_catalog (gaiaWFScatalogPtr handle);

/**
 Return the WFS-Version string as reported by GetCapabilities

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the WFS Version string: NULL is undefined
 
 \sa create_wfs_catalog
 */
    SPATIALITE_DECLARE const char *get_wfs_version (gaiaWFScatalogPtr handle);

/**
 Return the base URL for any WFS-GetFeature call

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the base URL for any WFS-GetFeature call: NULL is undefined
 
 \sa create_wfs_catalog, get_wfs_base_describe_url, get_wfs_request_url
 */
    SPATIALITE_DECLARE const char *get_wfs_base_request_url (gaiaWFScatalogPtr
							     handle);

/**
 Return the base URL for any WFS-DescribeFeatureType call

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the base URL for any WFS-DescribeFeatureType call: NULL is undefined
 
 \sa create_wfs_catalog, get_wfs_base_request_url, get_wfs_describe_url
 */
    SPATIALITE_DECLARE const char *get_wfs_base_describe_url (gaiaWFScatalogPtr
							      handle);

/**
 Return a GetFeature URL (GET)

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().
 \param name the NAME uniquely identifying the required WFS layer.
 \param version could be "1.0.0" or "1.1.0"; if NULL or invalid "1.1.0"
 will be assumed.
 \param srid the preferred SRS to be used for WFS geometries; if negative
 or mismatching will be simply ignored.
 \param max_features the WFS MAXFEATURES argument; any negative or zero
 value will be ignored.

 \return the GetFeature URL: NULL if any error is found.
 
 \sa get_wfs_base_request_url, get_wfs_describe_url

 \note you are responsible to destroy (before or after) any allocated
 memory returned by get_wfs_request_url().
 */
    SPATIALITE_DECLARE char *get_wfs_request_url (gaiaWFScatalogPtr handle,
						  const char *name,
						  const char *version,
						  int srid, int max_features);

/**
 Return a DescribeFeatureType URL (GET)

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().
 \param name the NAME uniquely identifying the required WFS layer.
 \param version could be "1.0.0" or "1.1.0"; if NULL or invalid "1.1.0"
 will be assumed.

 \return the DescribeFeatureType URL: NULL if any error is found.
 
 \sa get_wfs_base_describe_url, get_wfs_request_url

 \note you are responsible to destroy (before or after) any allocated
 memory returned by get_wfs_describe_url().
 */
    SPATIALITE_DECLARE char *get_wfs_describe_url (gaiaWFScatalogPtr handle,
						   const char *name,
						   const char *version);

/**
 Return the total count of items (aka Layers) defined within a WFS-Catalog object

 \param handle the pointer to a valid WFS-Catalog returned by a previous call
 to create_wfs_catalog()

 \return the total count of items (aka Layers) defined within a WFS-Catalog object: 
 a negative number if the WFS-Catalog isn't valid
 
 \sa create_wfs_catalog, get_wfs_catalog_item
 */
    SPATIALITE_DECLARE int get_wfs_catalog_count (gaiaWFScatalogPtr handle);

/**
 Return the pointer to some specific Layer defined within a WFS-Catalog object

 \param handle the pointer to a valid WFS-Catalog returned by a previous call
 to create_wfs_catalog()
 \param index the relative index identifying the required WFS-Layer (the first 
 Item in the WFS-Catalaog object has index ZERO).

 \return the pointer to the required WFS-Layer object: NULL if the passed index
 isn't valid
 
 \sa create_wfs_catalog, get_wfs_catalog_count, get_wfs_item_name, get_wfs_item_title, 
 get_wfs_item_abstract, get_wfs_layer_srid_count, get_wfs_layer_srid, 
 get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE gaiaWFSitemPtr get_wfs_catalog_item (gaiaWFScatalogPtr
							    handle, int index);

/**
 Return the name corresponding to some WFS-Item (aka Layer) object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the name corresponding to the WFS-Layer object
 
 \sa get_wfs_layer_title, get_wfs_layer_abstract, get_wfs_layer_srid_count, get_wfs_layer_srid, 
 get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE const char *get_wfs_item_name (gaiaWFSitemPtr handle);

/**
 Return the title corresponding to some WFS-Item (aka Layer) object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the title corresponding to the WFS-Layer object
 
 \sa get_wfs_item_name, get_wfs_item_abstract, get_wfs_layer_srid_count, get_wfs_layer_srid, 
 get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE const char *get_wfs_item_title (gaiaWFSitemPtr handle);

/**
 Return the abstract corresponding to some WFS-Item (aka Layer) object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the abstract corresponding to the WFS-Layer object
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_layer_srid_count, get_wfs_layer_srid, 
 get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE const char *get_wfs_item_abstract (gaiaWFSitemPtr
							  handle);

/**
 Return the total count of SRIDs supported by a WFS-Item object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the total count of SRIDs supported by a WFS-Item object: 
 a negative number if the WFS-Item isn't valid
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_item_abstract, 
 get_wfs_layer_srid, get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE int get_wfs_layer_srid_count (gaiaWFSitemPtr handle);

/**
 Return one of the SRIDs supported by a WFS-Item object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().
 \param index the relative index identifying the required SRID (the first 
 SRID value supported by a WFS-Item object has index ZERO).

 \return the SRID-value: a negative number if the required SRID-value isn't defined.
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_item_abstract, 
 get_wfs_layer_srid_count, get_wfs_keyword_count, get_wfs_keyword
 */
    SPATIALITE_DECLARE int get_wfs_layer_srid (gaiaWFSitemPtr handle,
					       int index);

/**
 Return the total count of Keywords associated to a WFS-Item object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().

 \return the total count of Keyword associated to a WFS-Item object: 
 a negative number if the WFS-Item isn't valid
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_item_abstract, 
 get_wfs_layer_srid_count, get_wfs_layer_srid, get_wfs_layer_keyword
 */
    SPATIALITE_DECLARE int get_wfs_keyword_count (gaiaWFSitemPtr handle);

/**
 Return one of the Keywords supported by a WFS-Item object

 \param handle the pointer to a valid WFS-Item returned by a previous call
 to get_wfs_catalog_item().
 \param index the relative index identifying the required Keyword (the first 
 Keyword associated to a WFS-Item object has index ZERO).

 \return the Keyword value: NULL if the required Keyword isn't defined.
 
 \sa get_wfs_item_name, get_wfs_item_title, get_wfs_item_abstract, 
 get_wfs_layer_srid_count, get_wfs_layer_srid, get_wfs_layer_keyword
 */
    SPATIALITE_DECLARE const char *get_wfs_keyword (gaiaWFSitemPtr handle,
						    int index);

/**
 Creates a Schema representing some WFS Layer 

 \param path_or_url pointer to some WFS-DescribeFeatureType XML Document (could be a pathname or an URL). 
 \param err_msg on completion will contain an error message (if any)

 \return the pointer to the corresponding WFS-Schema object: NULL on failure
 
 \sa destroy_wfs_schema,get_wfs_schema_column_count, get_wfs_schema_column_info,
 get_wfs_schema_geometry_info
 
 \note an eventual error message returned via err_msg requires to be deallocated
 by invoking free().\n
 you are responsible to destroy (before or after) any WFS-Schema returned by create_wfs_schema().
 */
    SPATIALITE_DECLARE gaiaWFSschemaPtr create_wfs_schema (const char
							   *path_or_url,
							   const char
							   *layer_name,
							   char **err_msg);

/**
 Destroys a WFS-schema object freeing any allocated resource 

 \param handle the pointer to a valid WFS-Catalog returned by a previous call
 to create_wfs_schema()
 
 \sa create_wfs_schema
 */
    SPATIALITE_DECLARE void destroy_wfs_schema (gaiaWFSschemaPtr handle);

/**
 Return the infos describing some WFS-GeometryColumn object

 \param handle the pointer to a valid WFS-Schema returned by a previous call
 to create_wfs_schema().
 \param name on completion will contain a pointer to the GeometryColumn name
 \param type on completion will contain the GeometryType set for the Column;
 could be one of GAIA_POINT, GAIA_LINESTRING, GAIA_POLYGON, GAIA_MULTIPOINT,
 GAIA_MULTILINESTRING, GAIA_MULTIPOLYGON or GAIA_GEOMETRYCOLLECTION
 \param srid on completion will contain the SRID-value set for the GeometryColumn
 \param dims on completion will contain the dimensions (2 or 3) set
 for the GeometryColumn
 \param nullable on completion will contain a Boolean value; if TRUE
 the Column may contain NULL-values.

 \return TRUE on success, FALSE if any error is encountered or if
 the WFS-Schema hasn't any Geometry-Column defined.
 
 \sa create_wfs_schema, get_wfs_schema_column_count, get_wfs_schema_column,
 get_wfs_schema_column_info
 */
    SPATIALITE_DECLARE int get_wfs_schema_geometry_info (gaiaWFSschemaPtr
							 handle,
							 const char **name,
							 int *type, int *srid,
							 int *dims,
							 int *nullable);

/**
 Return the total count of items (aka Columns) defined within a WFS-Schema object

 \param handle the pointer to a valid WFS-Schema returned by a previous call
 to create_wfs_schema()

 \return the total count of items (aka Columns) defined within a WFS-Schema object: 
 a negative number if the WFS-Schema isn't valid
 
 \sa create_wfs_schema, get_wfs_schema_geometry_info, 
 get_wfs_schema_column, get_wfs_schema_column_info 
 */
    SPATIALITE_DECLARE int get_wfs_schema_column_count (gaiaWFSschemaPtr
							handle);

/**
 Return the pointer to some specific Column defined within a WFS-Schema object

 \param handle the pointer to a valid WFS-Schema returned by a previous call
 to create_wfs_schema()
 \param index the relative index identifying the required WFS-Column (the first 
 Item in the WFS-Schema object has index ZERO).

 \return the pointer to the required WFS-Column object: NULL if the passed index
 isn't valid
 
 \sa create_wfs_schema, get_wfs_schema_geometry_info, 
 get_wfs_schema_column_count, get_wfs_schema_column_info
 */
    SPATIALITE_DECLARE gaiaWFScolumnPtr get_wfs_schema_column (gaiaWFSschemaPtr
							       handle,
							       int index);

/**
 Return the infos describing some WFS-Column object

 \param handle the pointer to a valid WFS-Column returned by a previous call
 to get_wfs_schema_column().
 \param name on completion will contain a pointer to the Column name
 \param type on completion will contain the datatype set for the Column;
 could be one of SQLITE_TEXT, SQLITE_INTEGER or SQLITE_FLOAT
 \param nullable on completion will contain a Boolean value; if TRUE
 the Column may contain NULL-values.

 \return TRUE on success, FALSE if any error is encountered
 
 \sa get_wfs_schema_column, get_wfs_schema_geometry_info
 */
    SPATIALITE_DECLARE int get_wfs_schema_column_info (gaiaWFScolumnPtr handle,
						       const char **name,
						       int *type,
						       int *nullable);

/**
 Resets the libxml2 "nano HTTP": useful when changing the HTTP_PROXY settings
 
 \sa create_wfs_catalog, load_from_wfs, load_from_wfs_paged
 */
    SPATIALITE_DECLARE void reset_wfs_http_connection (void);

#ifdef __cplusplus
}
#endif

#endif				/* _GG_WFS_H */
