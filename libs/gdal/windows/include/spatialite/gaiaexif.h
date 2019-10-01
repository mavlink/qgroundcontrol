/* 
 gaiaexif.h -- Gaia common EXIF Metadata reading functions
  
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
 \file gaiaexif.h

 EXIF/image: supporting functions and constants
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef DLL_EXPORT
#define GAIAEXIF_DECLARE __declspec(dllexport)
#else
#define GAIAEXIF_DECLARE extern
#endif
#endif

#ifndef _GAIAEXIF_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GAIAEXIF_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constants used for BLOB value types */
/** generic hexadecimal BLOB */
#define GAIA_HEX_BLOB		0
/** this BLOB does actually contain a GIF image */
#define GAIA_GIF_BLOB		1
/** this BLOB does actually containt a PNG image */
#define GAIA_PNG_BLOB		2
/** this BLOB does actually contain a generic JPEG image */
#define GAIA_JPEG_BLOB		3
/** this BLOB does actually contain a JPEG-EXIF image */
#define GAIA_EXIF_BLOB		4
/** this BLOB does actually contain a JPEG-EXIF image including GPS data */
#define GAIA_EXIF_GPS_BLOB	5
/** this BLOB does actually contain a ZIP compressed file */
#define GAIA_ZIP_BLOB		6
/** this BLOB does actually contain a PDF document */
#define GAIA_PDF_BLOB		7
/** this BLOB does actually contain a SpatiaLite Geometry */
#define GAIA_GEOMETRY_BLOB	8
/** this BLOB does actually contain a TIFF image */
#define GAIA_TIFF_BLOB		9
/** this BLOB does actually contain a WebP image */
#define GAIA_WEBP_BLOB		10
/** this BLOB does actually contain a JP2 (Jpeg2000) image */
#define GAIA_JP2_BLOB		11
/** this BLOB does actually contain a SpatiaLite XmlBLOB */
#define GAIA_XML_BLOB		12
/** this BLOB does actually contain a GPKG Geometry */
#define GAIA_GPB_BLOB		13

/* constants used for EXIF value types */
/** unrecognized EXIF value */
#define GAIA_EXIF_NONE		0
/** EXIF value of the BYTE type */
#define GAIA_EXIF_BYTE		1
/** EXIF value of the SHORT type */
#define GAIA_EXIF_SHORT		2
/** EXIF value of the STRING type */
#define GAIA_EXIF_STRING	3
/** EXIF value of the LONG type */
#define GAIA_EXIF_LONG		4
/** EXIF value of the RATIONAL type */
#define GAIA_EXIF_RATIONAL	5
/** EXIF value of the SLONG type */
#define GAIA_EXIF_SLONG		9
/** EXIF value of the SRATIONAL type */
#define GAIA_EXIF_SRATIONAL	10

/**
 Container for an EXIF tag
 */
    typedef struct gaiaExifTagStruct
    {
/* an EXIF TAG */
	/** GPS data included (0/1) */
	char Gps;
	/** EXIF tag ID */
	unsigned short TagId;
	/** EXIF value type */
	unsigned short Type;
	/** number of values */
	unsigned short Count;
	/** tag offset [big- little-endian encoded] */
	unsigned char TagOffset[4];
	/** array of BYTE values */
	unsigned char *ByteValue;
	/** array of STRING values */
	char *StringValue;
	/** array of SHORT values */
	unsigned short *ShortValues;
	/** array of LONG values ] */
	unsigned int *LongValues;
	/** array of RATIONAL values [numerators] */
	unsigned int *LongRationals1;
	/** array of RATIONAL values [denominators] */
	unsigned int *LongRationals2;
	/** array of Signed SHORT values */
	short *SignedShortValues;
	/** array of Signed LONG values */
	int *SignedLongValues;
	/** array of Signed RATIONAL values [numerators] */
	int *SignedLongRationals1;
	/** array of Signed RATIONAL values [denominators] */
	int *SignedLongRationals2;
	/** array of FLOAT values */
	float *FloatValues;
	/** array of DOUBLE values */
	double *DoubleValues;
	/** pointer to next item into the linked list */
	struct gaiaExifTagStruct *Next;
    } gaiaExifTag;
/**
 Typedef for EXIF tag structure.

 \sa gaiaExifTagStruct
 */
    typedef gaiaExifTag *gaiaExifTagPtr;

/**
 Container for a list of EXIF tags
 */
    typedef struct gaiaExifTagListStruct
    {
/* an EXIF TAG LIST */
	/** pointer to first item into the linked list */
	gaiaExifTagPtr First;
	/** pointer to the last item into the linked list */
	gaiaExifTagPtr Last;
	/** number of items */
	int NumTags;
	/** an array of pointers to items */
	gaiaExifTagPtr *TagsArray;
    } gaiaExifTagList;
/**
 Typedef for EXIF tag structure

 \sa gaiaExifTagListStruct
 */
    typedef gaiaExifTagList *gaiaExifTagListPtr;

/* function prototipes */

/**
 Creates a list of EXIF tags by parsing a BLOB of the JPEG-EXIF type

 \param blob the BLOB to be parsed
 \param size the BLOB size (in bytes)

 \return a list of EXIF tags: or NULL if any error is encountered

 \sa gaiaExifTagsFree

 \note you must explicitly destroy the list when it's any longer used.
 */
    GAIAEXIF_DECLARE gaiaExifTagListPtr gaiaGetExifTags (const unsigned char
							 *blob, int size);

/**
 Destroy a list of EXIF tags

 \param tag_list the list to be destroied

 \sa gaiaGetExifTags

 \note the pointer passed to this function must be one returned by a
 previous call to gaiaGetExifTags
 */
    GAIAEXIF_DECLARE void gaiaExifTagsFree (gaiaExifTagListPtr tag_list);

/**
 Return the total number of EXIF tags into the list

 \param tag_list pointer to an EXIF tag list.

 \return the EXIF tag count.

 \sa gaiaGetExifTags, gaiaExifTagsFree
 */
    GAIAEXIF_DECLARE int gaiaGetExifTagsCount (gaiaExifTagListPtr tag_list);

/**
 Retrieves an EXIF tag by its relative position into the list

 \param tag_list pointer to an EXIF tag list.
 \param pos relative item position [first item is 0]

 \return a pointer to the corresponding EXIF tag: NULL if not found

 \sa gaiaGetExifTags, gaiaExifTagsFree, gaiaExifTagsCount
 */
    GAIAEXIF_DECLARE gaiaExifTagPtr gaiaGetExifTagByPos (gaiaExifTagListPtr
							 tag_list,
							 const int pos);

/**
 Return the total number of EXIF tags into the list

 \param tag_list pointer to an EXIF tag list.

 \return the EXIF tag count.

 \sa gaiaGetExifTags, gaiaExifTagsFree
 */
    GAIAEXIF_DECLARE int gaiaGetExifTagsCount (gaiaExifTagListPtr tag_list);

/**
 Retrieves an EXIF tag by its Tag ID

 \param tag_list pointer to an EXIF tag list.
 \param tag_id the Tag ID to be found

 \return a pointer to the corresponding EXIF tag: NULL if not found

 \sa gaiaGetExifTags, gaiaExifTagsFree
 */
    GAIAEXIF_DECLARE gaiaExifTagPtr gaiaGetExifTagById (const gaiaExifTagListPtr
							tag_list,
							const unsigned short
							tag_id);

/**
 Retrieves an EXIF-GPS tag by its Tag ID

 \param tag_list pointer to an EXIF tag list.
 \param tag_id the GPS Tag ID to be found

 \return a pointer to the corresponding EXIF tag: NULL if not found

 \sa gaiaGetExifTags, gaiaExifTagsFree
 */
    GAIAEXIF_DECLARE gaiaExifTagPtr gaiaGetExifGpsTagById (const
							   gaiaExifTagListPtr
							   tag_list,
							   const unsigned short
							   tag_id);

/**
 Retrieves an EXIF tag by its name

 \param tag_list pointer to an EXIF tag list.
 \param tag_name the Tag Name to be found

 \return a pointer to the corresponding EXIF tag: NULL if not found

 \sa gaiaGetExifTags, gaiaExifTagsFree
 */
    GAIAEXIF_DECLARE gaiaExifTagPtr gaiaGetExifTagByName (const
							  gaiaExifTagListPtr
							  tag_list,
							  const char *tag_name);

/**
 Return the Tag ID from an EXIF tag

 \param tag pointer to an EXIF tag
 
 \return the Tag ID

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName
 */
    GAIAEXIF_DECLARE unsigned short gaiaExifTagGetId (const gaiaExifTagPtr tag);

/**
 Return the Tag Name from an EXIF tag

 \param tag pointer to an EXIF tag
 \param tag_name receiving buffer: the Tag Name will be copied here
 \param len length of the receiving buffer

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName
 */
    GAIAEXIF_DECLARE void gaiaExifTagGetName (const gaiaExifTagPtr tag,
					      char *tag_name, int len);

/**
 Checks if an EXIF tag actually is an EXIF-GPS tag

 \param tag pointer to an EXIF tag

 \return 0 if false: any other value if true

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName
 */
    GAIAEXIF_DECLARE int gaiaIsExifGpsTag (const gaiaExifTagPtr tag);

/**
 Return the value type for an EXIF tag

 \param tag pointer to an EXIF tag

 \return the value type: one of GAIA_EXIF_NONE, GAIA_EXIF_BYTE,
 GAIA_EXIF_SHORT, GAIA_EXIF_STRING, GAIA_EXIF_LONG, GAIA_EXIF_RATIONAL,
 GAIA_EXIF_SLONG, GAIA_EXIF_SRATIONAL

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName
 */
    GAIAEXIF_DECLARE unsigned short gaiaExifTagGetValueType (const
							     gaiaExifTagPtr
							     tag);

/**
 Return the total count of values from an EXIF tag
 
 \param tag pointer to an EXIF tag

 \return the number of available values

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName
 */
    GAIAEXIF_DECLARE unsigned short gaiaExifTagGetNumValues (const
							     gaiaExifTagPtr
							     tag);

/**
 Return a BYTE value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the BYTE value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE unsigned char gaiaExifTagGetByteValue (const gaiaExifTagPtr
							    tag, const int ind,
							    int *ok);

/**
 Return a STRING value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param str receiving buffer: the STRING value will be copied here.
 \param len length of the receiving buffer
 \param ok on completion will contain 0 on failure: any other value on success.

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE void gaiaExifTagGetStringValue (const gaiaExifTagPtr tag,
						     char *str, int len,
						     int *ok);

/**
 Return a SHORT value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the SHORT value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE unsigned short gaiaExifTagGetShortValue (const
							      gaiaExifTagPtr
							      tag,
							      const int ind,
							      int *ok);

/**
 Return a LONG value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the LONG value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE unsigned int gaiaExifTagGetLongValue (const gaiaExifTagPtr
							   tag, const int ind,
							   int *ok);

/**
 Return a RATIONAL [numerator] value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the RATIONAL [numerator] value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE unsigned int gaiaExifTagGetRational1Value (const
								gaiaExifTagPtr
								tag,
								const int ind,
								int *ok);

/**
 Return a RATIONAL [denominator] value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the RATIONAL [denominator] value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE unsigned int gaiaExifTagGetRational2Value (const
								gaiaExifTagPtr
								tag,
								const int ind,
								int *ok);

/**
 Return a RATIONAL value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the RATIONAL value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE double gaiaExifTagGetRationalValue (const gaiaExifTagPtr
							 tag, const int ind,
							 int *ok);

/**
 Return a Signed SHORT value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the Signed SHORT value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE short gaiaExifTagGetSignedShortValue (const gaiaExifTagPtr
							   tag, const int ind,
							   int *ok);

/**
 Return a Signed LONG value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the Signed LONG value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE int gaiaExifTagGetSignedLongValue (const gaiaExifTagPtr
							tag, const int ind,
							int *ok);

/**
 Return a SRATIONAL [numerator] value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the SRATIONAL [numerator] value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE int gaiaExifTagGetSignedRational1Value (const
							     gaiaExifTagPtr tag,
							     const int ind,
							     int *ok);

/**
 Return a SRATIONAL [denominator] value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the SRATIONAL [denominator] value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE int gaiaExifTagGetSignedRational2Value (const
							     gaiaExifTagPtr tag,
							     const int ind,
							     int *ok);

/**
 Return a Signed RATIONAL value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the Signed RATIONAL value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE double gaiaExifTagGetSignedRationalValue (const
							       gaiaExifTagPtr
							       tag,
							       const int ind,
							       int *ok);

/**
 Return a FLOAT value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the FLOAT value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE float gaiaExifTagGetFloatValue (const gaiaExifTagPtr tag,
						     const int ind, int *ok);

/**
 Return a DOUBLE value from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param ind value index [first value has index 0].
 \param ok on completion will contain 0 on failure: any other value on success.

 \return the DOUBLE value

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaExifTagGetValueType, gaiaExifTagGetNumValues
 */
    GAIAEXIF_DECLARE double gaiaExifTagGetDoubleValue (const gaiaExifTagPtr tag,
						       const int ind, int *ok);

/**
 Return a human readable description from an EXIF tag

 \param tag pointer to an EXIF tag.
 \param str receiving buffer: the STRING value will be copied here.
 \param len length of the receiving buffer
 \param ok on completion will contain 0 on failure: any other value on success.

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName
 */
    GAIAEXIF_DECLARE void gaiaExifTagGetHumanReadable (const gaiaExifTagPtr tag,
						       char *str, int len,
						       int *ok);

/**
 Attempts to guess the actual content-type of some BLOB

 \param blob the BLOB to be parsed 
 \param size length of the BLOB (in bytes)

 \return the BLOB type: one of GAIA_HEX_BLOB, GAIA_GIF_BLOB, GAIA_PNG_BLOB,
 GAIA_JPEG_BLOB, GAIA_EXIF_BLOB, GAIA_EXIF_GPS_BLOB, GAIA_ZIP_BLOB,
 GAIA_PDF_BLOB, GAIA_GEOMETRY_BLOB, GAIA_TIFF_BLOB, GAIA_WEBP_BLOB,
 GAIA_JP2_BLOB, GAIA_XML_BLOB, GAIA_GPB_BLOB
 */
    GAIAEXIF_DECLARE int gaiaGuessBlobType (const unsigned char *blob,
					    int size);
/**
 Return longitude and latitude from an EXIF-GPS tag

 \param blob the BLOB to be parsed 
 \param size length of the BLOB (in bytes)
 \param longitude on success will contain the longitude coordinate
 \param latitude on success will contain the latitude coordinate

 \return 0 on failure: any other value on success

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaIsExifGpsTag
 */
    GAIAEXIF_DECLARE int gaiaGetGpsCoords (const unsigned char *blob, int size,
					   double *longitude, double *latitude);
/**
 Return a text string representing DMS coordinates from an EXIF-GPS tag

 \param blob the BLOB to be parsed 
 \param size length of the BLOB (in bytes)
 \param latlong receiving buffer: the text string will be copied here.
 \param ll_size length of the receiving buffer

 \return 0 on failure: any other value on success

 \sa gaiaGetExifTagById, gaiaGetExifGpsTagById, gaiaGetExifTagByName, 
 gaiaIsExifGpsTag
 */
    GAIAEXIF_DECLARE int gaiaGetGpsLatLong (const unsigned char *blob, int size,
					    char *latlong, int ll_size);

#ifdef __cplusplus
}
#endif

#endif				/* _GAIAEXIF_H */
