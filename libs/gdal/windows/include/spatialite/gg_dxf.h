/*
 gg_dxf.h -- Gaia common support for DXF files
  
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
 \file gg_dxf.h

 Geometry handling functions: DXF files
 */

#ifndef _GG_DXF_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_DXF_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constant values for DXF */

/** import distinct layers */
#define GAIA_DXF_IMPORT_BY_LAYER	1
/** import layers mixed altogether by type */
#define GAIA_DXF_IMPORT_MIXED		2
/** auto-selects 2D or 3D */
#define GAIA_DXF_AUTO_2D_3D		3
/** always force 2D */
#define GAIA_DXF_FORCE_2D		4
/** always force 3D */
#define GAIA_DXF_FORCE_3D		5
/** don't apply any special Ring handling */
#define GAIA_DXF_RING_NONE		6
/** apply special "linked rings" handling */
#define GAIA_DXF_RING_LINKED		7
/** apply special "unlinked rings" handling */
#define GAIA_DXF_RING_UNLINKED		8


/** DXF version [Writer] */
#define GAIA_DXF_V12	1000

/* data structs */


/**
 wrapper for DXF Extra Attribute object
 */
    typedef struct gaia_dxf_extra_attr
    {
/** pointer to Extra Attribute Key value */
	char *key;
/** pointer to Extra Attribute Value string */
	char *value;
/** pointer to next item [linked list] */
	struct gaia_dxf_extra_attr *next;
    } gaiaDxfExtraAttr;
/**
 Typedef for DXF Extra Attribute object

 \sa gaiaDxfExtraAttr
 */
    typedef gaiaDxfExtraAttr *gaiaDxfExtraAttrPtr;

/**
 wrapper for DXF Insert object
 */
    typedef struct gaia_dxf_insert
    {
/** pointer to Block ID string */
	char *block_id;
/** X coordinate */
	double x;
/** Y coordinate */
	double y;
/** Z coordinate */
	double z;
/** X scale factor */
	double scale_x;
/** Y scale factor */
	double scale_y;
/** Z scale factor */
	double scale_z;
/** rotation angle */
	double angle;
/** boolean flag: contains Text objects */
	int hasText;
/** boolean flag: contains Point objects */
	int hasPoint;
/** boolean flag: contains Polyline (Linestring) objects */
	int hasLine;
/** boolean flag: contains Polyline (Polygon) objects */
	int hasPolyg;
/** boolean flag: contains Hatch objects */
	int hasHatch;
/** boolean flag: contains 3d Text objects */
	int is3Dtext;
/** boolean flag: contains 3d Point objects */
	int is3Dpoint;
/** boolean flag: contains 3d Polyline (Linestring) objects */
	int is3Dline;
/** boolean flag: contains 3d Polyline (Polygon) objects */
	int is3Dpolyg;
/** pointer to first Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr first;
/** pointer to last Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr last;
/** pointer to next item [linked list] */
	struct gaia_dxf_insert *next;
    } gaiaDxfInsert;
/**
 Typedef for DXF Insert object

 \sa gaiaDxfText
 */
    typedef gaiaDxfInsert *gaiaDxfInsertPtr;

/**
 wrapper for DXF Text object
 */
    typedef struct gaia_dxf_text
    {
/** pointer to Label string */
	char *label;
/** X coordinate */
	double x;
/** Y coordinate */
	double y;
/** Z coordinate */
	double z;
/** label rotation angle */
	double angle;
/** pointer to first Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr first;
/** pointer to last Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr last;
/** pointer to next item [linked list] */
	struct gaia_dxf_text *next;
    } gaiaDxfText;
/**
 Typedef for DXF Text object

 \sa gaiaDxfText
 */
    typedef gaiaDxfText *gaiaDxfTextPtr;

/**
 wrapper for DXF Point object
 */
    typedef struct gaia_dxf_point
    {
/** X coordinate */
	double x;
/** Y coordinate */
	double y;
/** Z coordinate */
	double z;
/** pointer to first Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr first;
/** pointer to last Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr last;
/** pointer to next item [linked list] */
	struct gaia_dxf_point *next;
    } gaiaDxfPoint;
/**
 Typedef for DXF Point object

 \sa gaiaDxfPoint
 */
    typedef gaiaDxfPoint *gaiaDxfPointPtr;

/**
 wrapper for DXF Circle object
 */
    typedef struct gaia_dxf_circle
    {
/** Center X coordinate */
	double cx;
/** Center Y coordinate */
	double cy;
/** Center Z coordinate */
	double cz;
/** radius */
	double radius;
    } gaiaDxfCircle;
/**
 Typedef for DXF Circle object

 \sa gaiaDxfCircle
 */
    typedef gaiaDxfCircle *gaiaDxfCirclePtr;

/**
 wrapper for DXF Arc object
 */
    typedef struct gaia_dxf_arc
    {
/** Center X coordinate */
	double cx;
/** Center Y coordinate */
	double cy;
/** Center Z coordinate */
	double cz;
/** radius */
	double radius;
/** start angle */
	double start;
/** stop angle */
	double stop;
    } gaiaDxfArc;
/**
 Typedef for DXF Arc object

 \sa gaiaDxfArc
 */
    typedef gaiaDxfArc *gaiaDxfArcPtr;

/**
 wrapper for DXF Polygon interior hole object
 */
    typedef struct gaia_dxf_hole
    {
/** total count of points */
	int points;
/** array of X coordinates */
	double *x;
/** array of Y coordinates */
	double *y;
/** array of Z coordinates */
	double *z;
/** pointer to next item [linked list] */
	struct gaia_dxf_hole *next;
    } gaiaDxfHole;
/**
 Typedef for DXF Point object

 \sa gaiaDxfHole
 */
    typedef gaiaDxfHole *gaiaDxfHolePtr;

/**
 wrapper for DXF Polyline object 
 could be a Linestring or a Polygon depending on the is_closed flag
 */
    typedef struct gaia_dxf_polyline
    {
/** open (Linestring) or closed (Polygon exterior ring) */
	int is_closed;
/** total count of points */
	int points;
/** array of X coordinates */
	double *x;
/** array of Y coordinates */
	double *y;
/** array of Z coordinates */
	double *z;
/** pointer to first Polygon hole [linked list] */
	gaiaDxfHolePtr first_hole;
/** pointer to last Polygon hole [linked list] */
	gaiaDxfHolePtr last_hole;
/** pointer to first Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr first;
/** pointer to last Extra Attribute [linked list] */
	gaiaDxfExtraAttrPtr last;
/** pointer to next item [linked list] */
	struct gaia_dxf_polyline *next;
    } gaiaDxfPolyline;
/**
 Typedef for DXF Polyline object

 \sa gaiaDxfPolyline
 */
    typedef gaiaDxfPolyline *gaiaDxfPolylinePtr;
/**
 wrapper for DXF Pattern Segment object
 */
    typedef struct gaia_dxf_hatch_segm
    {
/** start X */
	double x0;
/** start Y */
	double y0;
/** end X */
	double x1;
/** end Y */
	double y1;
/** pointer to next item [linked list] */
	struct gaia_dxf_hatch_segm *next;
    } gaiaDxfHatchSegm;
/**
 Typedef for DXF Hatch Segment object

 \sa gaiaDxfHatch
 */
    typedef gaiaDxfHatchSegm *gaiaDxfHatchSegmPtr;

/**
 wrapper for DXF Boundary Path object
 */
    typedef struct gaia_dxf_boundary_path
    {
/** pointer to first segment */
	gaiaDxfHatchSegmPtr first;
/** pointer to last segment */
	gaiaDxfHatchSegmPtr last;
/** pointer to next item [linked list] */
	struct gaia_dxf_boundary_path *next;
    } gaiaDxfBoundaryPath;
/**
 Typedef for DXF Boundary Path object

 \sa gaiaDxfBoundaryPath
 */
    typedef gaiaDxfBoundaryPath *gaiaDxfBoundaryPathPtr;

/**
 wrapper for DXF Pattern Hatch object
 */
    typedef struct gaia_dxf_hatch
    {
/** hatch pattern spacing */
	double spacing;
/** hatch line angle */
	double angle;
/** hatch line base X */
	double base_x;
/** hatch line base Y */
	double base_y;
/** hatch line offset X */
	double offset_x;
/** hatch line offset Y */
	double offset_y;
/** pointer to first Boundary */
	gaiaDxfBoundaryPathPtr first;
/** pointer to last Boundary */
	gaiaDxfBoundaryPathPtr last;
/** pointer to Boundary geometry */
	gaiaGeomCollPtr boundary;
/** pointer to first Pattern segment */
	gaiaDxfHatchSegmPtr first_out;
/** pointer to last Pattern segment */
	gaiaDxfHatchSegmPtr last_out;
/** pointer to next item [linked list] */
	struct gaia_dxf_hatch *next;
    } gaiaDxfHatch;
/**
 Typedef for DXF Hatch object

 \sa gaiaDxfHatch
 */
    typedef gaiaDxfHatch *gaiaDxfHatchPtr;

/**
 wrapper for DXF Block object
 */
    typedef struct gaia_dxf_block
    {
/** Boolean flag: this block is referenced by some Insert */
	int hasInsert;
/** pointer to Layer Name string */
	char *layer_name;
/** pointer to Block ID string */
	char *block_id;
/** pointer to first DXF Text object [linked list] */
	gaiaDxfTextPtr first_text;
/** pointer to last DXF Text object [linked list] */
	gaiaDxfTextPtr last_text;
/** pointer to first DXF Point object [linked list] */
	gaiaDxfPointPtr first_point;
/** pointer to last DXF Point object [linked list] */
	gaiaDxfPointPtr last_point;
/** pointer to first DXF Polyline (Linestring) object [linked list] */
	gaiaDxfPolylinePtr first_line;
/** pointer to last DXF Polyline (Linestring) object [linked list] */
	gaiaDxfPolylinePtr last_line;
/** pointer to first DXF Polyline (Polygon) object [linked list] */
	gaiaDxfPolylinePtr first_polyg;
/** pointer to last DXF Polyline (Polygon) object [linked list] */
	gaiaDxfPolylinePtr last_polyg;
/** pointer to first DXF Hatch object [linked list] */
	gaiaDxfHatchPtr first_hatch;
/** pointer to last DXF Hatch object [linked list] */
	gaiaDxfHatchPtr last_hatch;
/** boolean flag: contains 3d Text objects */
	int is3Dtext;
/** boolean flag: contains 3d Point objects */
	int is3Dpoint;
/** boolean flag: contains 3d Polyline (Linestring) objects */
	int is3Dline;
/** boolean flag: contains 3d Polyline (Polygon) objects */
	int is3Dpolyg;
/** pointer to next item [linked list] */
	struct gaia_dxf_block *next;
    } gaiaDxfBlock;
/**
 Typedef for DXF Block object

 \sa gaiaDxfBlock
 */
    typedef gaiaDxfBlock *gaiaDxfBlockPtr;

/**
 wrapper for DXF Layer object
 */
    typedef struct gaia_dxf_layer
    {
/** pointer to Layer Name string */
	char *layer_name;
/** pointer to first DXF Text object [linked list] */
	gaiaDxfTextPtr first_text;
/** pointer to last DXF Text object [linked list] */
	gaiaDxfTextPtr last_text;
/** pointer to first DXF Point object [linked list] */
	gaiaDxfPointPtr first_point;
/** pointer to lasst DXF Point object [linked list] */
	gaiaDxfPointPtr last_point;
/** pointer to first DXF Polyline (Linestring) object [linked list] */
	gaiaDxfPolylinePtr first_line;
/** pointer to last DXF Polyline (Linestring) object [linked list] */
	gaiaDxfPolylinePtr last_line;
/** pointer to first DXF Polyline (Polygon) object [linked list] */
	gaiaDxfPolylinePtr first_polyg;
/** pointer to last DXF Polyline (Polygon) object [linked list] */
	gaiaDxfPolylinePtr last_polyg;
/** pointer to first DXF Hatch object [linked list] */
	gaiaDxfHatchPtr first_hatch;
/** pointer to last DXF Hatch object [linked list] */
	gaiaDxfHatchPtr last_hatch;
/** pointer to first DXF Insert Text object [linked list] */
	gaiaDxfInsertPtr first_ins_text;
/** pointer to last DXF Insert Text object [linked list] */
	gaiaDxfInsertPtr last_ins_text;
/** pointer to first DXF Insert Point object [linked list] */
	gaiaDxfInsertPtr first_ins_point;
/** pointer to last DXF Insert Point object [linked list] */
	gaiaDxfInsertPtr last_ins_point;
/** pointer to first DXF Insert Polyline (Linestring) object [linked list] */
	gaiaDxfInsertPtr first_ins_line;
/** pointer to last DXF Insert Polyline (Linestring) object [linked list] */
	gaiaDxfInsertPtr last_ins_line;
/** pointer to first DXF Insert Polyline (Polygon) object [linked list] */
	gaiaDxfInsertPtr first_ins_polyg;
/** pointer to last DXF Insert Polyline (Polygon) object [linked list] */
	gaiaDxfInsertPtr last_ins_polyg;
/** pointer to first DXF Insert Hatch object [linked list] */
	gaiaDxfInsertPtr first_ins_hatch;
/** pointer to last DXF Insert Hatch object [linked list] */
	gaiaDxfInsertPtr last_ins_hatch;
/** boolean flag: contains 3d Text objects */
	int is3Dtext;
/** boolean flag: contains 3d Point objects */
	int is3Dpoint;
/** boolean flag: contains 3d Polyline (Linestring) objects */
	int is3Dline;
/** boolean flag: contains 3d Polyline (Polygon) objects */
	int is3Dpolyg;
/** boolean flag: contains 3d Insert Text objects */
	int is3DinsText;
/** boolean flag: contains 3d Insert Point objects */
	int is3DinsPoint;
/** boolean flag: contains 3d Insert Polyline (Linestring) objects */
	int is3DinsLine;
/** boolean flag: contains 3d Insert Polyline (Polygon) objects */
	int is3DinsPolyg;
/** boolean flag: contains Text Extra Attributes */
	int hasExtraText;
/** boolean flag: contains Point Extra Attributes */
	int hasExtraPoint;
/** boolean flag: contains Polyline (Linestring) Extra Attributes */
	int hasExtraLine;
/** boolean flag: contains Polyline (Polygon) Extra Attributes */
	int hasExtraPolyg;
/** boolean flag: contains Insert Text Extra Attributes */
	int hasExtraInsText;
/** boolean flag: contains Insert Text Extra Attributes */
	int hasExtraInsPoint;
/** boolean flag: contains Insert Polyline (Linestring) Extra Attributes */
	int hasExtraInsLine;
/** boolean flag: contains Insert Polyline (Polygon) Extra Attributes */
	int hasExtraInsPolyg;
/** pointer to next item [linked list] */
	struct gaia_dxf_layer *next;
    } gaiaDxfLayer;
/**
 Typedef for DXF Layer object

 \sa gaiaDxfLayer
 */
    typedef gaiaDxfLayer *gaiaDxfLayerPtr;

/**
 wrapper for DXF Parser object
 */
    typedef struct gaia_dxf_parser
    {
/** OUT: origin/input filename */
	char *filename;
/** OUT: pointer to first DXF Layer object [linked list] */
	gaiaDxfLayerPtr first_layer;
/** OUT: pointer to last DXF Layer object [linked list] */
	gaiaDxfLayerPtr last_layer;
/** OUT: pointer to first DXF Block object [linked list] */
	gaiaDxfBlockPtr first_block;
/** OUT: pointer to last DXF Block object [linked list] */
	gaiaDxfBlockPtr last_block;
/** IN: parser option - dimension handlig */
	int force_dims;
/** IN: parser option - the SRID */
	int srid;
/** IN: parser option - pointer the single Layer Name string */
	const char *selected_layer;
/** IN: parser option - pointer to prefix string for DB tables */
	const char *prefix;
/** IN: parser option - linked rings special handling */
	int linked_rings;
/** IN: parser option - unlinked rings special handling */
	int unlinked_rings;
/** internal parser variable */
	int line_no;
/** internal parser variable */
	int op_code_line;
/** internal parser variable */
	int op_code;
/** internal parser variable */
	int section;
/** internal parser variable */
	int tables;
/** internal parser variable */
	int blocks;
/** internal parser variable */
	int entities;
/** internal parser variable */
	int is_layer;
/** internal parser variable */
	int is_block;
/** internal parser variable */
	int is_text;
/** internal parser variable */
	int is_point;
/** internal parser variable */
	int is_polyline;
/** internal parser variable */
	int is_lwpolyline;
/** internal parser variable */
	int is_line;
/** internal parser variable */
	int is_circle;
/** internal parser variable */
	int is_arc;
/** internal parser variable */
	int is_vertex;
/** internal parser variable */
	int is_hatch;
/** internal parser variable */
	int is_hatch_boundary;
/** internal parser variable */
	int is_insert;
/** internal parser variable */
	int eof;
/** internal parser variable */
	int error;
/** internal parser variable */
	char *curr_layer_name;
/** internal parser variable */
	gaiaDxfText curr_text;
/** internal parser variable */
	gaiaDxfInsert curr_insert;
/** internal parser variable */
	gaiaDxfBlock curr_block;
/** internal parser variable */
	gaiaDxfPoint curr_point;
/** internal parser variable */
	gaiaDxfPoint curr_end_point;
/** internal parser variable */
	gaiaDxfCircle curr_circle;
/** internal parser variable */
	gaiaDxfArc curr_arc;
/** internal parser variable */
	int is_closed_polyline;
/** internal parser variable */
	gaiaDxfPointPtr first_pt;
/** internal parser variable */
	gaiaDxfPointPtr last_pt;
/** internal parser variable */
	char *extra_key;
/** internal parser variable */
	char *extra_value;
/** internal parser variable */
	gaiaDxfExtraAttrPtr first_ext;
/** internal parser variable */
	gaiaDxfExtraAttrPtr last_ext;
/** internal parser variable */
	gaiaDxfHatchPtr curr_hatch;
/** internal parser variable */
	int undeclared_layers;
    } gaiaDxfParser;
/**
 Typedef for DXF Layer object

 \sa gaiaDxfParser
 */
    typedef gaiaDxfParser *gaiaDxfParserPtr;

/**
 wrapper for DXF Write object
 */
    typedef struct gaia_dxf_write
    {
/** IN: output DXF file handle */
	FILE *out;
/** IN: coord's precision (number of decimal digits) */
	int precision;
/** IN: DXF version number */
	int version;
/** OUT: count of exported geometries */
	int count;
/** OUT: error flag */
	int error;
    } gaiaDxfWriter;
/**
 Typedef for DXF Writer object
 */
    typedef gaiaDxfWriter *gaiaDxfWriterPtr;


/* function prototypes */


/**
 Creates a DXF Parser object

 \param srid the SRID value to be used for all Geometries
 \param force_dims should be one of GAIA_DXF_AUTO_2D_3D, GAIA_DXF_FORCE_2D 
 or GAIA_DXF_FORCE_3D
 \param prefix an optional prefix to be used for DB target tables 
 (could be NULL)
 \param selected_layers if set, only the DXF Layer of corresponding name will 
 be imported (could be NULL)
 \param special_rings rings handling: should be one of GAIA_DXF_RING_NONE, 
 GAIA_DXF_RING_LINKED of GAIA_DXF_RING_UNLINKED

 \return the pointer to a DXF Parser object

 \sa gaiaDestroyDxfParser, gaiaParseDxfFile, gaiaLoadFromDxfParser

 \note the DXF Parser object corresponds to dynamically allocated memory:
 so you are responsible to destroy this object before or later by invoking
 gaiaDestroyDxfParser().
 */
    GAIAGEO_DECLARE gaiaDxfParserPtr gaiaCreateDxfParser (int srid,
							  int force_dims,
							  const char *prefix,
							  const char
							  *selected_layer,
							  int special_rings);

/**
 Destroying a DXF Parser object

 \param parser pointer to DXF Parser object

 \sa gaiaCreateDxfParser

 \note the pointer to the DXF Parser object to be finalized is expected
 to be the one returned by a previous call to gaiaCreateDxfParser.
 */
    GAIAGEO_DECLARE void gaiaDestroyDxfParser (gaiaDxfParserPtr parser);

/**
 Parsing a DXF file

 \param parser pointer to DXF Parser object
 \param dxf_path pathname of the DXF external file to be parsed

 \return 0 on failure, any other value on success

 \sa gaiaParseDxfFile_r,
 gaiaCreateDxfParser, gaiaDestroyDxfParser, gaiaLoadFromDxfParser

 \note the pointer to the DXF Parser object is expected to be the one 
 returned by a previous call to gaiaCreateDxfParser.
 A DXF Parser object can be used only a single time to parse a DXF file.\n
 not reentrant and thread unsafe.
 */
    GAIAGEO_DECLARE int gaiaParseDxfFile (gaiaDxfParserPtr parser,
					  const char *dxf_path);

/**
 Parsing a DXF file

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param parser pointer to DXF Parser object
 \param dxf_path pathname of the DXF external file to be parsed

 \return 0 on failure, any other value on success

 \sa gaiaParseDxfFile,
 gaiaCreateDxfParser, gaiaDestroyDxfParser, gaiaLoadFromDxfParser

 \note the pointer to the DXF Parser object is expected to be the one 
 returned by a previous call to gaiaCreateDxfParser.
 A DXF Parser object can be used only a single time to parse a DXF file.\n
 reentrant and thread-safe.
 */
    GAIAGEO_DECLARE int gaiaParseDxfFile_r (const void *p_cache,
					    gaiaDxfParserPtr parser,
					    const char *dxf_path);

/**
 Populating a DB so to permanently store all Geometries from a DXF Parser

 \param db_handle handle to a valid DB connection
 \param parser pointer to DXF Parser object
 \param mode should be one of GAIA_DXF_IMPORT_BY_LAYER or GAIA_DXF_IMPORT_MIXED
 \param append boolean flag: if set and some required DB table already exists 
  will attempt to append further rows into the existing table.
  otherwise an error will be returned.

 \return 0 on failure, any other value on success

 \sa gaiaCreateDxfParser, gaiaDestroyDxfParser, gaiaParseDxfFile

 \note the pointer to the DXF Parser object is expected to be the one 
 returned by a previous call to gaiaCreateDxfParser and previously used
 for a succesfull call to gaiaParseDxfFile
 */
    GAIAGEO_DECLARE int gaiaLoadFromDxfParser (sqlite3 * db_handle,
					       gaiaDxfParserPtr parser,
					       int mode, int append);

/**
 Initializing a DXF Writer Object

 \param writer pointer to the gaiaDxfWriter object to be initialized
 \param out file handle to DXF output file
 \param precision number of decimal digits for any coordinate
 \param version currently always expected to be GAIA_DXF_V12

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteHeader, gaiaExportDxf
 */
    GAIAGEO_DECLARE int gaiaDxfWriterInit (gaiaDxfWriterPtr dxf,
					   FILE * out, int precision,
					   int version);

/**
 Writing the DXF Header

 \param dxf pointer to a properly initialized gaiaDxfWriter object
 \param minx the minimum X coordinate contained within the DXF
 \param minx the minimum Y coordinate contained within the DXF
 \param minx the minimum Z coordinate contained within the DXF
 \param minx the maximum X coordinate contained within the DXF
 \param minx the maximum Y coordinate contained within the DXF
 \param minx the maximum Z coordinate contained within the DXF

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriterInit, gaiaDxfWriteFooter, gaiaDxfWriteTables, gaiaDxfWriteEntities
 */
    GAIAGEO_DECLARE int
	gaiaDxfWriteHeader (gaiaDxfWriterPtr dxf, double minx, double miny,
			    double minz, double maxx, double maxy, double maxz);

/**
 Writing a DXF Entities Section Header 

 \param dxf pointer to a properly initialized gaiaDxfWriter object

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteHeader
 */
    GAIAGEO_DECLARE int gaiaDxfWriteFooter (gaiaDxfWriterPtr dxf);

/**
 Writing the DXF Tables Section Header 

 \param dxf pointer to a properly initialized gaiaDxfWriter object

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteHeader, gaiaDxfWriteEndSection
 */
    GAIAGEO_DECLARE int gaiaDxfWriteTables (gaiaDxfWriterPtr dxf);

/**
 Writing a DXF Table/Layer definition 

 \param dxf pointer to a properly initialized gaiaDxfWriter object
 \param layer_name name of the layer

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteTables, gaiaDxfWriteEndSection
 */
    GAIAGEO_DECLARE int gaiaDxfWriteLayer (gaiaDxfWriterPtr dxf,
					   const char *layer_name);

/**
 Writing a DXF Entities Section Header 

 \param dxf pointer to a properly initialized gaiaDxfWriter object

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteHeader, gaiaDxfWriteEndSection, gaiaDxfWritePoint,
 gaiaDxfWriteText, gaiaDxfWriteLine, gaiaDxfWriteRing, gaiaDxfWriteGeometry 
 */
    GAIAGEO_DECLARE int gaiaDxfWriteEntities (gaiaDxfWriterPtr dxf);

/**
 Writing a DXF Entities Section Header 

 \param dxf pointer to a properly initialized gaiaDxfWriter object

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteTables, gaiaDxfWriteEntities
 */
    GAIAGEO_DECLARE int gaiaDxfWriteEndSection (gaiaDxfWriterPtr dxf);

/**
 Writing a DXF Point Entity 

 \param dxf pointer to a properly initialized gaiaDxfWriter object
 \param layer_name name of the corresponding layer
 \param x X coordinate value
 \param y Y coordinate value
 \param z Z coordinate value

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteEntities, gaiaDxfWriteEndSection, gaiaDxfWriteText, 
 gaiaDxfWriteLine, gaiaDxfWriteRing, gaiaDxfWriteGeometry 
 */
    GAIAGEO_DECLARE int gaiaDxfWritePoint (gaiaDxfWriterPtr dxf,
					   const char *layer_name, double x,
					   double y, double z);

/**
 Writing a DXF Text Entity 

 \param dxf pointer to a properly initialized gaiaDxfWriter object
 \param layer_name name of the corresponding layer
 \param x X coordinate value
 \param y Y coordinate value
 \param z Z coordinate value
 \param label text string containing the label value
 \param text_height height of the text in map units
 \param angle text rotation angle

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteEntities, gaiaDxfWriteEndSection, gaiaDxfWritePoint, 
 gaiaDxfWriteLine, gaiaDxfWriteRing, gaiaDxfWriteGeometry 
 */
    GAIAGEO_DECLARE int gaiaDxfWriteText (gaiaDxfWriterPtr dxf,
					  const char *layer_name, double x,
					  double y, double z, const char *label,
					  double text_height, double angle);

/**
 Writing a DXF Polyline (opened) Entity 

 \param dxf pointer to a properly initialized gaiaDxfWriter object
 \param layer_name name of the corresponding layer
 \param line pointer to the internal Linestring to be exported into the DXF

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteEntities, gaiaDxfWriteEndSection, gaiaDxfWritePoint, 
 gaiaDxfWriteText, gaiaDxfWriteRing, gaiaDxfWriteGeometry 
 */
    GAIAGEO_DECLARE int
	gaiaDxfWriteLine (gaiaDxfWriterPtr dxf, const char *layer_name,
			  gaiaLinestringPtr line);

/**
 Writing a DXF Polyline (closed) Entity 

 \param dxf pointer to a properly initialized gaiaDxfWriter object
 \param layer_name name of the corresponding layer
 \param line pointer to the internal Ring to be exported into the DXF

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteEntities, gaiaDxfWriteEndSection, gaiaDxfWritePoint, 
 gaiaDxfWriteText, gaiaDxfWriteLine, gaiaDxfWriteGeometry
 */
    GAIAGEO_DECLARE int
	gaiaDxfWriteRing (gaiaDxfWriterPtr dxf, const char *layer_name,
			  gaiaRingPtr ring);

/**
 Writing a DXF generic Entity

 \param dxf pointer to a properly initialized gaiaDxfWriter object
 \param layer_name name of the corresponding layer
 \param line pointer to the internal Ring to be exported into the DXF
 \param label text string containing the label value (could be NULL)
 \param text_height only for Text Labels: ingnored in any other case.
 \param text_rotation only for Text Labels: ingnored in any other case.

 \return 0 on failure, any other value on success

 \sa gaiaDxfWriteEntities, gaiaDxfWriteEndSection, gaiaDxfWritePoint, 
 gaiaDxfWriteText, gaiaDxfWriteLine, gaiaDxfWriteRing
 */
    GAIAGEO_DECLARE int
	gaiaDxfWriteGeometry (gaiaDxfWriterPtr dxf, const char *layer_name,
			      const char *label, double text_height,
			      double text_rotation, gaiaGeomCollPtr geometry);

/**
 Exporting a complex DXF file 

 \param dxf pointer to a properly initialized gaiaDxfWriter object
 \param db_hanlde handle to the current DB connection
 \param sql a text string defining the SQL query to be used for
 extracting all geometries/entities to be exported into the output DXF
 \param layer_col_name name of the SQL resultset column containing the Layer name
 \param geom_col_name name of the SQL resultset column containing Geometries
 \param label_col_name name of the SQL resultset column containing Label values
 (could be NULL)
 \param text_height_col_name name of the SQL resultset column containing Text Height values
 (could be NULL)
 \param text_rotation_col_name name of the SQL resultset column containing Text Rotation values
 (could be NULL)
 \param geom_filter an optional arbitrary Geometry to be used as a Spatial Filter
 (could be NULL) 

 \return 0 on failure; the total count of exported  entities on success

 \sa gaiaDxfWriterInit
 */
    GAIAGEO_DECLARE int
	gaiaExportDxf (gaiaDxfWriterPtr dxf, sqlite3 * db_handle,
		       const char *sql, const char *layer_col_name,
		       const char *geom_col_name, const char *label_col_name,
		       const char *text_height_col_name,
		       const char *text_rotation_col_name,
		       gaiaGeomCollPtr geom_filter);

#ifdef __cplusplus
}
#endif

#endif				/* _GG_DXF_H */
