/*
 gg_xml.h -- Gaia common support for XML documents
  
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
 \file gg_xml.h

 Geometry handling functions: XML document
 */

#ifndef _GG_XML_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_XML_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constant values for XmlBLOB */

/** XmlBLOB internal marker: START */
#define GAIA_XML_START		0x00
/** XmlBLOB internal marker: END */
#define GAIA_XML_END		0xDD
/** XmlBLOB internal marker: HEADER */
#define GAIA_XML_HEADER		0xAC
/** XmlBLOB internal marker: LEGACY HEADER */
#define GAIA_XML_LEGACY_HEADER	0xAB
/** XmlBLOB internal marker: SCHEMA */
#define GAIA_XML_SCHEMA		0xBA
/** XmlBLOB internal marker: FILEID */
#define GAIA_XML_FILEID		0xCA
/** XmlBLOB internal marker: PARENTID */
#define GAIA_XML_PARENTID	0xDA
/** XmlBLOB internal marker: TITLE */
#define GAIA_XML_NAME		0xDE
/** XmlBLOB internal marker: TITLE */
#define GAIA_XML_TITLE		0xDB
/** XmlBLOB internal marker: ABSTRACT */
#define GAIA_XML_ABSTRACT	0xDC
/** XmlBLOB internal marker: GEOMETRY */
#define GAIA_XML_GEOMETRY	0xDD
/** XmlBLOB internal marker: CRC32 */
#define GAIA_XML_CRC32		0xBC
/** XmlBLOB internal marker: PAYLOAD */
#define GAIA_XML_PAYLOAD	0xCB

/* bitmasks for XmlBLOB-FLAG */

/** XmlBLOB FLAG - LITTLE_ENDIAN bitmask */
#define GAIA_XML_LITTLE_ENDIAN		0x01
/** XmlBLOB FLAG - COMPRESSED bitmask */
#define GAIA_XML_COMPRESSED		0x02
/** XmlBLOB FLAG - VALIDATED bitmask */
#define GAIA_XML_VALIDATED		0x04
/** XmlBLOB FLAG - ISO METADATA bitmask */
#define GAIA_XML_ISO_METADATA		0x80
/** XmlBLOB FLAG - SLDSE VECTOR STYLE bitmask */
#define GAIA_XML_SLD_SE_RASTER_STYLE	0x10
/** XmlBLOB FLAG - SLDSE VECTOR STYLE bitmask */
#define GAIA_XML_SLD_SE_VECTOR_STYLE	0x40
/** XmlBLOB FLAG - SLD STYLE bitmask */
#define GAIA_XML_SLD_STYLE		0x48
/** XmlBLOB FLAG - SVG bitmask */
#define GAIA_XML_SVG			0x20


/* function prototypes */

#ifndef DOXYGEN_SHOULD_IGNORE_THIS
#ifdef ENABLE_LIBXML2		/* LIBXML2 enabled: supporting XML documents */
#endif

/**
 return the LIBXML2 version string

 \return a text string identifying the current LIBXML2 version

 \note the version string corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE char *gaia_libxml2_version (void);

/**
 Creates an XmlBLOB buffer

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param xml pointer to the XML document (XmlBLOB payload).
 \param xml_len lenght of the XML document (in bytes).
 \param compressed if TRUE the returned XmlBLOB will be zip-compressed.
 \param schemaURI if not NULL the XML document will be assumed to be valid
  only if it successfully passes a formal Schema valitadion.
 \param result on completion will containt a pointer to XmlBLOB:
 NULL on failure.
 \param size on completion this variable will contain the XmlBLOB's size (in bytes)
 \param parsing_errors on completion this variable will contain all error/warning
 messages emitted during the XML Parsing step. Can be set to NULL so to ignore any message.
 \param schema_validation_errors on completion this variable will contain all error/warning
 messages emitted during the XML Schema Validation step. Can be set to NULL so to ignore any message.

 \sa gaiaXmlFromBlob, gaiaXmlTextFromBlob, gaiaXmlBlobGetLastParseError, 
 gaiaXmlBlobGetLastValidateError

 \note the XmlBLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaXmlToBlob (const void *p_cache,
					const unsigned char *xml, int xml_len,
					int compressed, const char *schemaURI,
					unsigned char **result, int *size,
					char **parsing_errors,
					char **schema_validation_errors);

/**
 Extract an XMLDocument from within an XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).
 \param indent if a negative value is passed the XMLDocument will 
 be extracted exactly as it was when loaded. Otherwise it will be 
 properly formatted using the required intenting (max. 8); ZERO
 means that the whole XML Document will consist of a single line.

 \return the pointer to the newly created XMLDocument buffer: NULL on failure

 \sa gaiaXmlToBlob, gaiaXmlFromBlob

 \note the returned XMLDocument will always be encoded as UTF-8 (irrespectively
 from the internal encoding declaration), so to allow any further processing as 
 SQLite TEXT.

 \note the XMLDocument buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlTextFromBlob (const unsigned char *blob,
					       int size, int indent);

/**
 Extract an XMLDocument from within an XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).
 \param indent if a negative value is passed the XMLDocument will 
 be extracted exactly as it was when loaded. Otherwise it will be 
 properly formatted using the required intenting (max. 8); ZERO
 means that the whole XML Document will consist of a single line.
 \param result pointer to the memory buffer containing the XML Document
 \param res_size dimension (in bytes) of the XML Document memory buffer
 (both values will be passed back after successful completion).


 \sa gaiaXmlToBlob, gaiaXmlTextFromBlob

 \note the returned XMLDocument will always respect the internal encoding declaration,
 and may not support any further processing as SQLite TEXT if it's not UTF-8.

 \note the XMLDocument buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE void gaiaXmlFromBlob (const unsigned char *blob,
					  int size, int indent,
					  unsigned char **result,
					  int *res_size);

/**
 Checks if a BLOB actually is a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE

 \sa gaiaIsCompressedXmlBlob, gaiaIsSchemaValidatedXmlBlob, 
 gaiaIsIsoMetadataXmlBlob, gaiaIsSldSeVectorStyleXmlBlob, 
 gaiaIsSldSeRasterStyleXmlBlob, gaiaIsSldStyleXmlBlob,
 gaiaIsSvgXmlBlob
 */
    GAIAGEO_DECLARE int gaiaIsValidXmlBlob (const unsigned char *blob,
					    int size);

/**
 Checks if a valid XmlBLOB buffer is compressed or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid XmlBLOB; -1 in any other case.

 \sa gaiaIsValidXmlBlob, gaiaIsSchemaValidatedXmlBlob, 
 gaiaIsIsoMetadataXmlBlob, gaiaIsSldSeVectorStyleXmlBlob, 
 gaiaIsSldSeRasterStyleXmlBlob, gaiaIsSldStyleXmlBlob,
 gaiaIsSvgXmlBlob
 */
    GAIAGEO_DECLARE int gaiaIsCompressedXmlBlob (const unsigned char *blob,
						 int size);

/**
 Checks if a valid XmlBLOB buffer does contain an ISO Metadata or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid XmlBLOB; -1 in any other case.

 \sa gaiaIsValidXmlBlob, gaiaIsSchemaValidatedXmlBlob, 
 gaiaIsCompressedXmlBlob, gaiaIsSldSeVectorStyleXmlBlob, 
 gaiaIsSldSeRasterStyleXmlBlob, gaiaIsSldStyleXmlBlob,
 gaiaIsSvgXmlBlob
 */
    GAIAGEO_DECLARE int gaiaIsIsoMetadataXmlBlob (const unsigned char *blob,
						  int size);

/**
 Checks if a valid XmlBLOB buffer does contain an SLD/SE Style or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid XmlBLOB of the 
 Vector type; -1 in any other case.

 \sa gaiaIsValidXmlBlob, gaiaIsSchemaValidatedXmlBlob, 
 gaiaIsCompressedXmlBlob, gaiaIsIsoMetadataXmlBlob, 
 gaiaIsSldSeRasterStyleXmlBlob, gaiaIsSvgXmlBlob
 */
    GAIAGEO_DECLARE int gaiaIsSldSeVectorStyleXmlBlob (const unsigned char
						       *blob, int size);

/**
 Checks if a valid XmlBLOB buffer does contain an SLD/SE Style or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid XmlBLOB of the
 Raster type; -1 in any other case.

 \sa gaiaIsValidXmlBlob, gaiaIsSchemaValidatedXmlBlob, 
 gaiaIsCompressedXmlBlob, gaiaIsIsoMetadataXmlBlob, 
 gaiaIsSldSeVectorStyleXmlBlob, gaiaIsSldStyleXmlBlob,
 gaiaIsSvgXmlBlob
 */
    GAIAGEO_DECLARE int gaiaIsSldSeRasterStyleXmlBlob (const unsigned char
						       *blob, int size);

/**
 Checks if a valid XmlBLOB buffer does contain an SLD Style or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid XmlBLOB of the
 SLD type; -1 in any other case.

 \sa gaiaIsValidXmlBlob, gaiaIsSchemaValidatedXmlBlob, 
 gaiaIsCompressedXmlBlob, gaiaIsIsoMetadataXmlBlob, 
 gaiaIsSldSeVectorStyleXmlBlob, gaiaIsSldSeRasterXmlBlob,
 gaiaIsSvgXmlBlob
 */
    GAIAGEO_DECLARE int gaiaIsSldStyleXmlBlob (const unsigned char
					       *blob, int size);

/**
 Checks if a valid XmlBLOB buffer does contain an SVG Symbol or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid XmlBLOB; -1 in any other case.

 \sa gaiaIsValidXmlBlob, gaiaIsSchemaValidatedXmlBlob, 
 gaiaIsCompressedXmlBlob, gaiaIsIsoMetadataXmlBlob, 
 gaiaIsSldSeVectorStyleXmlBlob, gaiaIsSldStyleXmlBlob,
 gaiaIsSldSeRasterStyleXmlBlob
 */
    GAIAGEO_DECLARE int gaiaIsSvgXmlBlob (const unsigned char *blob, int size);

/**
 Return another XmlBLOB buffer compressed / uncompressed

 \param blob pointer to the input XmlBLOB buffer.
 \param in_size input XmlBLOB's size (in bytes).
 \param compressed if TRUE the returned XmlBLOB will be zip-compressed.
 \param result on completion will containt a pointer to the output XmlBLOB:
 NULL on failure.
 \param out_size on completion this variable will contain the output XmlBLOB's size (in bytes)

 \sa gaiaXmlToBlob, gaiaIsCompressedXmlBlob

 \note the XmlBLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE void gaiaXmlBlobCompression (const unsigned char *blob,
						 int in_size, int compressed,
						 unsigned char **result,
						 int *out_size);

/**
 Checks if a valid XmlBLOB buffer has successfully passed a formal Schema validation or not

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return TRUE or FALSE if the BLOB actually is a valid XmlBLOB but not schema-validated; 
  -1 in any other case.

 \sa gaiaIsValidXmlBlob, gaiaIsSvgXmlBlob, 
 gaiaIsCompressedXmlBlob, gaiaIsIsoMetadataXmlBlob, 
 gaiaIsSldSeVectorStyleXmlBlob, gaiaIsSldSeRasterStyleXmlBlob,
 gaiaIsSldStyleXmlBlob 
 */
    GAIAGEO_DECLARE int gaiaIsSchemaValidatedXmlBlob (const unsigned char *blob,
						      int size);

/**
 Return the XMLDocument size (in bytes) from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the XMLDocument size (in bytes) for any valid XmlBLOB; 
  -1 if the BLOB isn't a valid XmlBLOB.
 */
    GAIAGEO_DECLARE int gaiaXmlBlobGetDocumentSize (const unsigned char *blob,
						    int size);

/**
 Return the SchemaURI from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the SchemaURI for any valid XmlBLOB containing a SchemaURI; 
  NULL in any other case.

 \sa gaiaXmlGetInternalSchemaURI

 \note the returned SchemaURI corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetSchemaURI (const unsigned char
						   *blob, int size);

/**
 Return the Internal SchemaURI from a valid XmlDocument

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param xml pointer to the XML document 
 \param xml_len lenght of the XML document (in bytes).

 \return the SchemaURI eventually defined within a valid XMLDocument; 
  NULL if the XMLDocument is invalid, or if it doesn't contain any SchemaURI.

 \sa gaiaXmlBlobGetSchemaURI

 \note the returned SchemaURI corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlGetInternalSchemaURI (const void *p_cache,
						       const unsigned char *xml,
						       int xml_len);

/**
 Return the FileIdentifier from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the FileIdentifier for any valid XmlBLOB containing a FileIdentifier; 
  NULL in any other case.

 \sa gaiaIsIsoMetadataXmlBlob, gaiaXmlBlobSetFileId, gaiaXmlBlobAddFileId

 \note the returned FileIdentifier corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetFileId (const unsigned char
						*blob, int size);

/**
 Return the ParentIdentifier from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the ParentIdentifier for any valid XmlBLOB containing a ParentIdentifier; 
  NULL in any other case.

 \sa gaiaIsIsoMetadataXmlBlob, gaiaXmlBlobSetParentId, gaiaXmlBlobAddParentId

 \note the returned ParentIdentifier corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetParentId (const unsigned char
						  *blob, int size);

/**
 Return a new XmlBLOB (ISO Metadata) by replacing the FileId value

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param blob pointer to the input XmlBLOB buffer.
 \param size input XmlBLOB's size (in bytes).
 \param identifier the new FileId value to be set.
 \param new_blob on completion will contain a pointer to the output XmlBLOB buffer.
 \param new_size on completion will containg the output XmlBlob's size (in bytes).

 \return TRUE for success; FALSE for any failure cause.

 \sa gaiaIsIsoMetadataXmlBlob, gaiaXmlBlobGetFileId, gaiaXmlBlobAddFileId

 \note the output XmlBLOB corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE int gaiaXmlBlobSetFileId (const void *p_cache,
					      const unsigned char *blob,
					      int size, const char *identifier,
					      unsigned char **new_blob,
					      int *new_size);

/**
 Return a new XmlBLOB (ISO Metadata) by replacing the ParentId value

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param blob pointer to the inputXmlBLOB buffer.
 \param size input XmlBLOB's size (in bytes).
 \param identifier the new ParentId value to be set.
 \param new_blob on completion will contain a pointer to the output XmlBLOB buffer.
 \param new_size on completion will containg the output XmlBlob's size (in bytes).

 \return TRUE for success; FALSE for any failure cause.

 \sa gaiaIsIsoMetadataXmlBlob, gaiaXmlBlobGetParentId, gaiaXmlBlobAddParentId

 \note the returned XmlBLOB corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE int gaiaXmlBlobSetParentId (const void *p_cache,
						const unsigned char *blob,
						int size,
						const char *identifier,
						unsigned char **new_blob,
						int *new_size);

/**
 Return a new XmlBLOB (ISO Metadata) by inserting a FileId value

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param blob pointer to the input XmlBLOB buffer.
 \param size input XmlBLOB's size (in bytes).
 \param identifier the new FileId value to be inserted.
 \param ns_id prefix corresponding to FileIdentifier NameSpace (may be NULL)
 \param uri_id URI corresponding to the FileIdentifier NameSpace (may be NULL)
 \param ns_charstr prefix corresponding to CharacterString NameSpace (may be NULL)
 \param uri_charstr URI corresponding to CharacterString NameSpace (may be NULL)
 \param new_blob on completion will contain a pointer to the output XmlBLOB buffer.
 \param new_size on completion will containg the output XmlBlob's size (in bytes).

 \return TRUE for success; FALSE for any failure cause.

 \sa gaiaIsIsoMetadataXmlBlob, gaiaXmlBlobGetFileId, gaiaXmlBlobSetFileId

 \note the output XmlBLOB corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE int gaiaXmlBlobAddFileId (const void *p_cache,
					      const unsigned char *blob,
					      int size, const char *identifier,
					      const char *ns_id,
					      const char *uri_id,
					      const char *ns_charstr,
					      const char *uri_charstr,
					      unsigned char **new_blob,
					      int *new_size);

/**
 Return a new XmlBLOB (ISO Metadata) by inserting a ParentId value

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param blob pointer to the inputXmlBLOB buffer.
 \param size input XmlBLOB's size (in bytes).
 \param identifier the new ParentId value to be inserted.
 \param ns_id prefix corresponding to FileIdentifier NameSpace (may be NULL)
 \param uri_id URI corresponding to the FileIdentifier NameSpace (may be NULL)
 \param ns_charstr prefix corresponding to CharacterString NameSpace (may be NULL)
 \param uri_charstr URI corresponding to CharacterString NameSpace (may be NULL)
 \param new_blob on completion will contain a pointer to the output XmlBLOB buffer.
 \param new_size on completion will containg the output XmlBlob's size (in bytes).

 \return TRUE for success; FALSE for any failure cause.

 \sa gaiaIsIsoMetadataXmlBlob, gaiaXmlBlobGetParentId, gaiaXmlBlobSetParentId

 \note the returned XmlBLOB corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE int gaiaXmlBlobAddParentId (const void *p_cache,
						const unsigned char *blob,
						int size,
						const char *identifier,
						const char *ns_id,
						const char *uri_id,
						const char *ns_charstr,
						const char *uri_charstr,
						unsigned char **new_blob,
						int *new_size);

/**
 Return the Name from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the Name for any valid XmlBLOB containing a Name; 
  NULL in any other case.

 \sa gaiaIsIsoMetadataXmlBlob, gaiaIsSldSeVectorStyleXmlBlob, 
 gaiaIsSldSeRasterStyleXmlBlob, gaiaIsSldStyleXmlBlob

 \note the returned Name corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetName (const unsigned char
					      *blob, int size);

/**
 Return the Title from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the Title for any valid XmlBLOB containing a Title; 
  NULL in any other case.

 \sa gaiaIsIsoMetadataXmlBlob, gaiaIsSldSeVectorStyleXmlBlob, 
 gaiaIsSldSeRasterStyleXmlBlob, gaiaIsSldStyleXmlBlob

 \note the returned Title corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetTitle (const unsigned char
					       *blob, int size);

/**
 Return the Abstract from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the Abstract for any valid XmlBLOB containing an Abstract; 
  NULL in any other case.

 \sa gaiaIsIsoMetadataXmlBlob, gaiaIsSldSeVectorStyleXmlBlob, 
 gaiaIsSldSeRasterStyleXmlBlob, gaiaIsSldStyleXmlBlob

 \note the returned Abstract corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetAbstract (const unsigned char
						  *blob, int size);

/**
 Return the Geometry Buffer from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).
 \param blob_geom on completion this variable will contain
 a pointer to the returned Geometry Buffer (NULL if no Geometry
 was defined within the XmlBLOB)
 \param blob_size on completion this variable will contain
 the size (in bytes) of the returned Geometry Buffer

 \sa gaiaIsIsoMetadataXmlBlob

 \note the returned Geometry Buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE void gaiaXmlBlobGetGeometry (const unsigned char
						 *blob, int size,
						 unsigned char **blob_geom,
						 int *blob_size);

/**
 Return the Charset Encoding from a valid XmlBLOB buffer

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).

 \return the Charset Encoding for any valid XmlBLOB explicitly defining an Encoding; 
  NULL in any other case.

 \note the returned Encoding corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetEncoding (const unsigned char
						  *blob, int size);

/**
 Return the most recent XML Parse error/warning (if any)

 \param ptr a memory pointer returned by spatialite_alloc_connection()

 \return the most recent XML Parse error/warning message (if any); 
  NULL in any other case.

 \sa gaiaXmlBlobGetLastValidateError, gaiaIsValidXPathExpression, 
 gaiaXmlBlobGetLastXPathError

 \note the returned error/warning message corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetLastParseError (const void *p_cache);

/**
 Return the most recent XML Validate error/warning (if any)

 \param p_cache a memory pointer returned by spatialite_alloc_connection()

 \return the most recent XML Validate error/warning message (if any); 
  NULL in any other case.

 \sa gaiaXmlBlobGetLastParseError, gaiaIsValidXPathExpression, 
 gaiaXmlBlobGetLastXPathError

 \note the returned error/warning message corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetLastValidateError (const void *p_cache);

/**
 Checks if a Text string could be a valid XPathExpression

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param xpath_expr pointer to the XPathExpression to be checked.

 \return TRUE or FALSE if the Text string actually is a valid XPathExpression; 
  -1 in any other case.

 \sa gaiaXmlBlobGetLastXPathError
 */
    GAIAGEO_DECLARE int gaiaIsValidXPathExpression (const void *p_cache,
						    const char *xpath_expr);

/**
 Return the most recent XPath error/warning (if any)

 \param p_cache a memory pointer returned by spatialite_alloc_connection()

 \return the most recent XPath error/warning message (if any); 
  NULL in any other case.

 \sa gaiaXmlBlobGetLastParseError, gaiaXmlBlobGetLastValidateError,
 gaiaIsValidXPathExpression

 \note the returned error/warning message corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.
 */
    GAIAGEO_DECLARE char *gaiaXmlBlobGetLastXPathError (const void *p_cache);

/**
 Load an external XML Document

 \param path_or_url pointer to the external XML Document (could be a pathname or an URL).
 \param result on completion will containt a pointer to a BLOB:
 NULL on failure.
 \param size on completion this variable will contain the BLOB's size (in bytes).
 \param parsing_errors on completion this variable will contain all error/warning
 messages emitted during the XML Parsing step. Can be set to NULL so to ignore any message.

 \sa gaiaXmlFromBlob, gaiaXmlStore

 \note the BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE int gaiaXmlLoad (const void *p_cache,
				     const char *path_or_url,
				     unsigned char **result, int *size,
				     char **parsing_errors);

/**
 Stores an external XML Document

 \param blob pointer to the XmlBLOB buffer.
 \param size XmlBLOB's size (in bytes).
 \param path pathname of the export file
 \param indent if a negative value is passed the XMLDocument will 
 be extracted exactly as it was when loaded. Otherwise it will be 
 properly formatted using the required intenting (max. 8); ZERO
 means that the whole XML Document will consist of a single line.

 \sa gaiaXmlToBlob, gaiaXmlTextFromBlob

 \note the returned XMLDocument will always respect the internal encoding declaration,
 and may not support any further processing as SQLite TEXT if it's not UTF-8.

 \note the XMLDocument buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it before or after.

 \sa gaiaXmlFromBlob, gaiaXmlLoad

 \note the BLOB buffer corresponds to dynamically allocated memory:
 so you are responsible to free() it [unless SQLite will take care
 of memory cleanup via buffer binding].
 */
    GAIAGEO_DECLARE int gaiaXmlStore (const unsigned char *blob, int size,
				      const char *path, int indent);

#endif				/* end LIBXML2: supporting XML documents */

#ifdef __cplusplus
}
#endif

#endif				/* _GG_XML_H */
