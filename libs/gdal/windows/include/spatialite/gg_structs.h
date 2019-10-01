/*
 gg_structs.h -- Gaia common support for geometries: structures
  
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
 \file gg_structs.h

 Geometry structures
 */

#ifndef _GG_STRUCTS_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_STRUCTS_H
#endif

#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 Container for OGC POINT Geometry
 */
    typedef struct gaiaPointStruct
    {
/* an OpenGis POINT */
/** X coordinate */
	double X;		/* X,Y coordinates */
/** Y coordinate */
	double Y;
/** Z coordinate: only for XYZ and XYZM dims */
	double Z;		/* Z coordinate */
/** M measure: only for XYM and XYZM dims */
	double M;		/* M measure */
/** one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M, GAIA_XY_ZM */
	int DimensionModel;	/* (x,y), (x,y,z), (x,y,m) or (x,y,z,m) */
/** pointer to next item [double linked list] */
	struct gaiaPointStruct *Next;	/* for double-linked list */
/** pointer to previous item [double linked list] */
	struct gaiaPointStruct *Prev;	/* for double-linked list */
    } gaiaPoint;
/**
 Typedef for OGC POINT structure

 \sa gaiaPoint
 */
    typedef gaiaPoint *gaiaPointPtr;

/**
 Container for dynamically growing line/ring
 */
    typedef struct gaiaDynamicLineStruct
    {
/* a generic DYNAMIC LINE object */
/** invalid object */
	int Error;
/** the SRID */
	int Srid;
/** pointer to first POINT [double linked list] */
	gaiaPointPtr First;	/* Points linked list - first */
/** pointer to last POINT [double linked list] */
	gaiaPointPtr Last;	/* Points linked list - last */
    } gaiaDynamicLine;
/**
 Typedef for dynamically growing line/ring structure

 \sa gaiaDynamicLine
 */
    typedef gaiaDynamicLine *gaiaDynamicLinePtr;

/**
 Container for OGC LINESTRING Geometry
 */
    typedef struct gaiaLinestringStruct
    {
/* an OpenGis LINESTRING */
/** number of points [aka vertices] */
	int Points;		/* number of vertices */
/** COORDs mem-array */
	double *Coords;		/* X,Y [vertices] array */
/** MBR: min X */
	double MinX;		/* MBR - BBOX */
/** MBR: min Y */
	double MinY;		/* MBR - BBOX */
/** MBR: max X */
	double MaxX;		/* MBR - BBOX */
/** MBR: max X */
	double MaxY;		/* MBR - BBOX */
/** one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M, GAIA_XY_ZM */
	int DimensionModel;	/* (x,y), (x,y,z), (x,y,m) or (x,y,z,m) */
/** pointer to next item [linked list] */
	struct gaiaLinestringStruct *Next;	/* for linked list */
    } gaiaLinestring;
/**
 Typedef for OGC LINESTRING structure

 \sa gaiaLinestring
 */
    typedef gaiaLinestring *gaiaLinestringPtr;

/**
 Container for OGC RING Geometry
 */
    typedef struct gaiaRingStruct
    {
/* a GIS ring - OpenGis LINESTRING, closed */
/** number of points [aka vertices] */
	int Points;		/* number of vertices */
/** COORDs mem-array */
	double *Coords;		/* X,Y [vertices] array */
/** clockwise / counterclockwise */
	int Clockwise;		/* clockwise / counterclockwise */
/** MBR: min X */
	double MinX;		/* MBR - BBOX */
/** MBR: min Y */
	double MinY;		/* MBR - BBOX */
/** MBR: max X */
	double MaxX;		/* MBR - BBOX */
/** MBR: max Y */
	double MaxY;		/* MBR - BBOX */
/** one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M, GAIA_XY_ZM */
	int DimensionModel;	/* (x,y), (x,y,z), (x,y,m) or (x,y,z,m) */
/** pointer to next item [linked list] */
	struct gaiaRingStruct *Next;	/* for linked list */
/** pointer to belonging Polygon */
	struct gaiaPolygonStruct *Link;	/* polygon reference */
    } gaiaRing;
/**
 Typedef for OGC RING structure

 \sa gaiaRing
 */
    typedef gaiaRing *gaiaRingPtr;

/**
 Container for OGC POLYGON Geometry
 */
    typedef struct gaiaPolygonStruct
    {
/* an OpenGis POLYGON */
/** the exterior ring (mandatory) */
	gaiaRingPtr Exterior;	/* exterior ring */
/** number of interior rings (may be, none) */
	int NumInteriors;	/* number of interior rings */
/** array of interior rings */
	gaiaRingPtr Interiors;	/* interior rings array */
/** index of first unused interior ring */
	int NextInterior;	/* first free interior ring */
/** MBR: min X */
	double MinX;		/* MBR - BBOX */
/** MBR: min Y */
	double MinY;		/* MBR - BBOX */
/** MBR: max X */
	double MaxX;		/* MBR - BBOX */
/** MBR: max Y */
	double MaxY;		/* MBR - BBOX */
/** one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M, GAIA_XY_ZM */
	int DimensionModel;	/* (x,y), (x,y,z), (x,y,m) or (x,y,z,m) */
/** pointer to next item [linked list] */
	struct gaiaPolygonStruct *Next;	/* for linked list */
    } gaiaPolygon;
/**
 Typedef for OGC POLYGON structure
 
 \sa gaiaPolygon
 */
    typedef gaiaPolygon *gaiaPolygonPtr;

/**
 Container for OGC GEOMETRYCOLLECTION Geometry
 */
    typedef struct gaiaGeomCollStruct
    {
/* OpenGis GEOMETRYCOLLECTION */
/** the SRID */
	int Srid;		/* the SRID value for this GEOMETRY */
/** CPU endian arch */
	char endian_arch;	/* littleEndian - bigEndian arch for target CPU */
/** BLOB Geometry endian arch */
	char endian;		/* littleEndian - bigEndian */
/** BLOB-Geometry buffer */
	const unsigned char *blob;	/* WKB encoded buffer */
/** BLOB-Geometry buffer size (in bytes) */
	unsigned long size;	/* buffer size */
/** current offset [BLOB parsing] */
	unsigned long offset;	/* current offset [for parsing] */
/** pointer to first POINT [linked list]; may be NULL */
	gaiaPointPtr FirstPoint;	/* Points linked list - first */
/** pointer to last POINT [linked list]; may be NULL */
	gaiaPointPtr LastPoint;	/* Points linked list - last */
/** pointer to first LINESTRING [linked list]; may be NULL */
	gaiaLinestringPtr FirstLinestring;	/* Linestrings linked list - first */
/** pointer to last LINESTRING [linked list]; may be NULL */
	gaiaLinestringPtr LastLinestring;	/* Linestrings linked list - last */
/** pointer to first POLYGON [linked list]; may be NULL */
	gaiaPolygonPtr FirstPolygon;	/* Polygons linked list - first */
/** pointer to last POLYGON [linked list]; may be NULL */
	gaiaPolygonPtr LastPolygon;	/* Polygons linked list - last */
/** MBR: min X */
	double MinX;		/* MBR - BBOX */
/** MBR: min Y */
	double MinY;		/* MBR - BBOX */
/** MBR: max X */
	double MaxX;		/* MBR - BBOX */
/** MBR: max Y */
	double MaxY;		/* MBR - BBOX */
/** one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M, GAIA_XY_ZM */
	int DimensionModel;	/* (x,y), (x,y,z), (x,y,m) or (x,y,z,m) */
/** any valid Geometry Class type */
	int DeclaredType;	/* the declared TYPE for this Geometry */
/** pointer to next item [linked list] */
	struct gaiaGeomCollStruct *Next;	/* Vanuatu - used for linked list */
    } gaiaGeomColl;
/**
 Typedef for OGC GEOMETRYCOLLECTION structure

 \sa gaiaGeomCool
 */
    typedef gaiaGeomColl *gaiaGeomCollPtr;

/**
 Container similar to LINESTRING [internally used]
 */
    typedef struct gaiaPreRingStruct
    {
/* a LINESTRING used to build rings */
/** pointer to LINESTRING */
	gaiaLinestringPtr Line;	/* a LINESTRING pointer */
/** already used/visited item */
	int AlreadyUsed;	/* a switch to mark an already used line element */
/** pointer to next item [linked list] */
	struct gaiaPreRingStruct *Next;	/* for linked list */
    } gaiaPreRing;
/**
 Typedef for gaiaPreRing structure

 \sa gaiaPreRing
 */
    typedef gaiaPreRing *gaiaPreRingPtr;

/**
 Container for variant (multi-type) value
 */
    typedef struct gaiaValueStruct
    {
/* a DBF field multitype value */
/** data type: one of GAIA_NULL_VALUE, GAIA_INT_VALUE, GAIA_DOUBLE_VALUE, GAIA_TEXT_VALUE */
	short Type;		/* the type */
/** TEXT type value */
	char *TxtValue;		/* the text value */
/** INT type value */
	sqlite3_int64 IntValue;	/* the integer value */
/** DOUBLE type value */
	double DblValue;	/* the double value */
    } gaiaValue;
/**
 Typedef for variant (multi-type) value structure 
 */
    typedef gaiaValue *gaiaValuePtr;

/**
 Container for DBF field
 */
    typedef struct gaiaDbfFieldStruct
    {
/* a DBF field definition - shapefile attribute */
/** field name */
	char *Name;		/* field name */
/** DBF data type */
	unsigned char Type;	/* field type */
/** DBF buffer offset [where the field value starts] */
	int Offset;		/* buffer offset [this field begins at *buffer+offset* and extends for *length* bytes */
/** total DBF buffer field length (in bytes) */
	unsigned char Length;	/* field total length [in bytes] */
/** precision (decimal digits) */
	unsigned char Decimals;	/* decimal positions */
/** current variant [multi-type] value */
	gaiaValuePtr Value;	/* the current multitype value for this attribute */
/** pointer to next item [linked list] */
	struct gaiaDbfFieldStruct *Next;	/* pointer to next element in linked list */
    } gaiaDbfField;
/**
 Typedef for DBF field structure 
 */
    typedef gaiaDbfField *gaiaDbfFieldPtr;

/**
 Container for a list of DBF fields
 */
    typedef struct gaiaDbfListStruct
    {
/* a linked list to contain the DBF fields definitions - shapefile attributes */
/** current RowID */
	int RowId;		/* the current RowId */
/** current Geometry */
	gaiaGeomCollPtr Geometry;	/* geometry for current entity */
/** pointer to first DBF field [linked list] */
	gaiaDbfFieldPtr First;	/* pointer to first element in linked list */
/** pointer to last DBF field [linked list] */
	gaiaDbfFieldPtr Last;	/* pointer to last element in linker list */
    } gaiaDbfList;
/**
 Typedef for a list of DBF fields
 
 \sa gaiaDbfList
 */
    typedef gaiaDbfList *gaiaDbfListPtr;

/**
 Container for DBF file handling
 */
    typedef struct gaiaDbfStruct
    {
/* DBF TYPE */
/** DBF endian arch */
	int endian_arch;
/** validity flag: 1 = ready to be processed */
	int Valid;		/* 1 = ready to process */
/** DBF file pathname */
	char *Path;		/* the DBF path */
/** FILE handle */
	FILE *flDbf;		/* the DBF file handle */
/** list of DBF fields */
	gaiaDbfListPtr Dbf;	/* the DBF attributes list */
/** I/O buffer */
	unsigned char *BufDbf;	/* the DBF I/O buffer */
/** header size (in bytes) */
	int DbfHdsz;		/* the DBF header length */
/** record length (in bytes) */
	int DbfReclen;		/* the DBF record length */
/** current file size */
	int DbfSize;		/* current DBF size */
/** current Record Number */
	int DbfRecno;		/* current DBF record number */
/** handle to ICONV converter object */
	void *IconvObj;		/* opaque reference to ICONV converter */
/** last error message (may be NULL) */
	char *LastError;	/* last error message */
    } gaiaDbf;
/** 
 Typedef for DBF file handler structure

 \sa gaiaDbf
 */
    typedef gaiaDbf *gaiaDbfPtr;

/**
 Container for SHP file handling
 */
    typedef struct gaiaShapefileStruct
    {
/* SHAPEFILE TYPE */
/** SHP endian arch */
	int endian_arch;
/** validity flag: 1 = ready to be processed */
	int Valid;		/* 1 = ready to process */
/** read or write mode */
	int ReadOnly;		/* read or write mode */
/** SHP 'abstract' path (no suffixes) */
	char *Path;		/* the shapefile abstract path [no suffixes] */
/** FILE handle to SHX file */
	FILE *flShx;		/* the SHX file handle */
/** FILE handle to SHP file */
	FILE *flShp;		/* the SHP file handle */
/** FILE handle to DBF file */
	FILE *flDbf;		/* the DBF file handle */
/** the SHP shape code */
	int Shape;		/* the SHAPE code for the whole shapefile */
/** list of DBF fields */
	gaiaDbfListPtr Dbf;	/* the DBF attributes list */
/** DBF I/O buffer */
	unsigned char *BufDbf;	/* the DBF I/O buffer */
/** DBF header size (in bytes) */
	int DbfHdsz;		/* the DBF header length */
/** DBF record length (in bytes) */
	int DbfReclen;		/* the DBF record length */
/** DBF current file size (in bytes) */
	int DbfSize;		/* current DBF size */
/** DBF current Record Number */
	int DbfRecno;		/* current DBF record number */
/** SHP I/O buffer */
	unsigned char *BufShp;	/* the SHP I/O buffer */
/** SHP current buffer size (in bytes) */
	int ShpBfsz;		/* the SHP buffer current size */
/** SHP current file size */
	int ShpSize;		/* current SHP size */
/** SHX current file size */
	int ShxSize;		/* current SHX size */
/** Total Extent: min X */
	double MinX;		/* the MBR/BBOX for the whole shapefile */
/** Total Extent: min Y */
	double MinY;
/** Total Extent: max X */
	double MaxX;
/** Total Extent: max Y */
	double MaxY;
/** handle to ICONV converter object */
	void *IconvObj;		/* opaque reference to ICONV converter */
/** last error message (may be NULL) */
	char *LastError;	/* last error message */
/** SHP actual OGC Geometry type */
	int EffectiveType;	/* the effective Geometry-type, as determined by gaiaShpAnalyze() */
/** SHP actual dims: one of GAIA_XY, GAIA_XY_Z, GAIA_XY_M, GAIA_XY_ZM */
	int EffectiveDims;	/* the effective Dimensions [XY, XYZ, XYM, XYZM], as determined by gaiaShpAnalyze() */
    } gaiaShapefile;
/**
 Typedef for SHP file handler structure

 \sa gaiaShapefile
 */
    typedef gaiaShapefile *gaiaShapefilePtr;

/**
 Container for dynamically growing output buffer
 */
    typedef struct gaiaOutBufferStruct
    {
/* a struct handling a dynamically growing output buffer */
/** current buffer */
	char *Buffer;
/** current write offset */
	int WriteOffset;
/** current buffer size (in bytes) */
	int BufferSize;
/** validity flag */
	int Error;
    } gaiaOutBuffer;
/**
 Typedef for dynamically growing output buffer structure

 \sa gaiaOutBuffer
 */
    typedef gaiaOutBuffer *gaiaOutBufferPtr;

#ifndef OMIT_ICONV		/* ICONV enabled: supporting text reader */

/** Virtual Text driver: MAX number of fields */
#define VRTTXT_FIELDS_MAX	65535
/** Virtual Text driver: MAX block size (in bytes) */
#define VRTTXT_BLOCK_MAX 65535

/** Virtual Text driver: TEXT value */
#define VRTTXT_TEXT		1
/** Virtual Text driver: INTEGER value */
#define VRTTXT_INTEGER	2
/** Virtual Text driver: DOUBLE value */
#define VRTTXT_DOUBLE	3
/** Virtual Text driver: NULL value */
#define VRTTXT_NULL	4

/**
 Container for Virtual Text record (line)
 */
    struct vrttxt_line
    {
/* a struct representing a full LINE (aka Record) */
/** current offset (parsing) */
	off_t offset;
/** line length (in bytes) */
	int len;
/** array of field offsets (where each field starts) */
	int field_offsets[VRTTXT_FIELDS_MAX];
/** number of field into the record */
	int num_fields;
/** validity flag */
	int error;
    };

/**
 Container for Virtual Text record (line) offsets 
 */
    struct vrttxt_row
    {
/* a struct storing Row offsets */
/** Line Number */
	int line_no;
/** start offset */
	off_t offset;
/** record (line) length (in bytes) */
	int len;
/** number of fields into this record */
	int num_fields;
    };

/**
 Container for Virtual Text block of records
 */
    struct vrttxt_row_block
    {
/*
/ for efficiency sake, individual Row offsets 
/ are grouped in reasonably sized blocks
*/
/** array of records [lines] */
	struct vrttxt_row rows[VRTTXT_BLOCK_MAX];
/** number of records into the array */
	int num_rows;
/** min Line Number */
	int min_line_no;
/** max Line Number */
	int max_line_no;
/** pointer to next item [linked list] */
	struct vrttxt_row_block *next;
    };

/** 
 Container for Virtual Text column (field) header
 */
    struct vrttxt_column_header
    {
/* a struct representing a Column (aka Field) header */
/** column name */
	char *name;
/** data type: one of GAIA_NULL_VALUE, GAIA_INT_VALUE, GAIA_DOUBLE_VALUE, GAIA_TEXT_VALUE */
	int type;
    };

/**
 Container for Virtual Text file handling
 */
    typedef struct vrttxt_reader
    {
/* the main TXT-Reader struct */
/** array of columns (fields) */
	struct vrttxt_column_header columns[VRTTXT_FIELDS_MAX];
/** FILE handle */
	FILE *text_file;
/** handle to ICONV converter object */
	void *toUtf8;		/* the UTF-8 ICONV converter */
/** field separator character */
	char field_separator;
/** text separator character (quote) */
	char text_separator;
/** decimal separator */
	char decimal_separator;
/** TRUE if the first line contains column names */
	int first_line_titles;
/** validity flag */
	int error;
/** pointer to first block of records [linked list] */
	struct vrttxt_row_block *first;
/** pointer to last block of records [linked list] */
	struct vrttxt_row_block *last;
/** array of pointers to individual records [lines] */
	struct vrttxt_row **rows;
/** number of records */
	int num_rows;
/** current Line Number */
	int line_no;
/** max number of columns (fields) */
	int max_fields;
/** current buffer size */
	int current_buf_sz;
/** current buffer offset [parsing] */
	int current_buf_off;
/** I/O buffer */
	char *line_buffer;
/** current field buffer */
	char *field_buffer;
/** array of field offsets [current record] */
	int field_offsets[VRTTXT_FIELDS_MAX];
/** array of field lengths [current record] */
	int field_lens[VRTTXT_FIELDS_MAX];
/** max field [current record] */
	int max_current_field;
/** current record [line] ready for parsing */
	int current_line_ready;
    } gaiaTextReader;
/**
 Typedef for Virtual Text file handling structure

 \sa gaiaTextReader
 */
    typedef gaiaTextReader *gaiaTextReaderPtr;

#endif				/* end ICONV (text reader) */

/**
 Layer Extent infos
 */
    typedef struct gaiaLayerExtentInfos
    {
/** row count (aka feature count) */
	int Count;
/** Extent: min X */
	double MinX;		/* MBR - BBOX */
/** Extent: min Y */
	double MinY;		/* MBR - BBOX */
/** Extent: max X */
	double MaxX;		/* MBR - BBOX */
/** Extent: max Y */
	double MaxY;		/* MBR - BBOX */
    } gaiaLayerExtent;

/**
 Typedef for Layer Extent infos

 \sa gaiaLayerExtent
 */
    typedef gaiaLayerExtent *gaiaLayerExtentPtr;

/**
 Layer Auth infos
 */
    typedef struct gaiaLayerAuthInfos
    {
/** Read-Only layer: TRUE or FALSE */
	int IsReadOnly;
/** Hidden layer: TRUE or FALSE */
	int IsHidden;
    } gaiaLayerAuth;

/**
 Typedef for Layer Auth infos

 \sa gaiaLayerAuth
 */
    typedef gaiaLayerAuth *gaiaLayerAuthPtr;

/**
 Attribute/Field MaxSize/Length infos
 */
    typedef struct gaiaAttributeFieldMaxSizeInfos
    {
/** MaxSize / MaxLength */
	int MaxSize;
    } gaiaAttributeFieldMaxSize;

/**
 Typedef for Attribute/Field MaxSize/Length infos

 \sa gaiaAttributeFieldMaxSize
 */
    typedef gaiaAttributeFieldMaxSize *gaiaAttributeFieldMaxSizePtr;

/**
 Attribute/Field Integer range infos
 */
    typedef struct gaiaAttributeFieldIntRangeInfos
    {
/** Minimum value */
	sqlite3_int64 MinValue;
/** Maximum value */
	sqlite3_int64 MaxValue;
    } gaiaAttributeFieldIntRange;

/**
 Typedef for Attribute/Field Integer range infos

 \sa gaiaAttributeFieldIntRange
 */
    typedef gaiaAttributeFieldIntRange *gaiaAttributeFieldIntRangePtr;

/**
 Attribute/Field Double range infos
 */
    typedef struct gaiaAttributeFieldDoubleRangeInfos
    {
/** Minimum value */
	double MinValue;
/** Maximum value */
	double MaxValue;
    } gaiaAttributeFieldDoubleRange;

/**
 Typedef for Attribute/Field Double range infos

 \sa gaiaAttributeFieldDoubleRange
 */
    typedef gaiaAttributeFieldDoubleRange *gaiaAttributeFieldDoubleRangePtr;

/**
 LayerAttributeField infos
 */
    typedef struct gaiaLayerAttributeFieldInfos
    {
/** ordinal position */
	int Ordinal;
/** SQL name of the corresponding column */
	char *AttributeFieldName;
/** total count of NULL values */
	int NullValuesCount;
/** total count of INTEGER values */
	int IntegerValuesCount;
/** total count of DOUBLE values */
	int DoubleValuesCount;
/** total count of TEXT values */
	int TextValuesCount;
/** total count of BLOB values */
	int BlobValuesCount;
/** pointer to MaxSize/Length infos (may be NULL) */
	gaiaAttributeFieldMaxSizePtr MaxSize;
/** pointer to range of Integer values infos (may be NULL) */
	gaiaAttributeFieldIntRangePtr IntRange;
/** pointer to range of Double values infos (may be NULL) */
	gaiaAttributeFieldDoubleRangePtr DoubleRange;
/** pointer to next item (linked list) */
	struct gaiaLayerAttributeFieldInfos *Next;
    } gaiaLayerAttributeField;

/**
 Typedef for Layer AttributeField infos

 \sa gaiaLayerAttributeField
 */
    typedef gaiaLayerAttributeField *gaiaLayerAttributeFieldPtr;

/**
 Vector Layer item
 */
    typedef struct gaiaVectorLayerItem
    {
/** one of GAIA_VECTOR_UNKNOWN, GAIA_VECTOR_TABLE, GAIA_VECTOR_VIEW, 
    GAIA_VECTOR_VIRTUAL */
	int LayerType;
/** SQL name of the corresponding table */
	char *TableName;
/** SQL name of the corresponding Geometry column */
	char *GeometryName;
/** SRID value */
	int Srid;
/** one of GAIA_VECTOR_UNKNOWN, GAIA_VECTOR_POINT, GAIA_VECTOR_LINESTRING, 
    GAIA_VECTOR_POLYGON, GAIA_VECTOR_MULTIPOINT, GAIA_VECTOR_MULTILINESTRING, 
    GAIA_VECTOR_MULTIPOLYGON, GAIA_VECTOR_GEOMETRYCOLLECTION, GAIA_VECTOR_GEOMETRY
*/
	int GeometryType;
/** one of GAIA_VECTOR_UNKNOWN, GAIA_XY, GAIA_XY_Z, GAIA_XY_M, GAIA_XY_ZM */
	int Dimensions;
/** one of GAIA_VECTOR_UNKNOWN, GAIA_SPATIAL_INDEX_NONE, GAIA_SPATIAL_INDEX_RTREE, 
    GAIA_SPATIAL_INDEX_MBRCACHE
*/
	int SpatialIndex;
/** pointer to Extent infos (may be NULL) */
	gaiaLayerExtentPtr ExtentInfos;
/** pointer to Auth infos (may be NULL) */
	gaiaLayerAuthPtr AuthInfos;
/** pointer to first Field/Attribute (linked list) */
	gaiaLayerAttributeFieldPtr First;
/** pointer to last Field/Attribute (linked list) */
	gaiaLayerAttributeFieldPtr Last;
/** pointer to next item (linked list) */
	struct gaiaVectorLayerItem *Next;
    } gaiaVectorLayer;

/**
 Typedef for Vector Layer item

 \sa gaiaVectorLayer
 */
    typedef gaiaVectorLayer *gaiaVectorLayerPtr;

/**
 Container for Vector Layers List
 */
    typedef struct gaiaVectorLayersListStr
    {
/** pointer to first vector layer (linked list) */
	gaiaVectorLayerPtr First;
/** pointer to last vector layer (linked list) */
	gaiaVectorLayerPtr Last;
/** pointer to currently set vector layer */
	gaiaVectorLayerPtr Current;
    } gaiaVectorLayersList;

/**
 Typedef for Vector Layers List

 \sa gaiaVectorLayersList
 */
    typedef gaiaVectorLayersList *gaiaVectorLayersListPtr;

#ifdef __cplusplus
}
#endif

#endif				/* _GG_STRUCTS_H */
