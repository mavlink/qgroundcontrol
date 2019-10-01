/* 
 gaiaaux.h -- Gaia common utility functions
  
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
 \file gaiaaux.h

 Auxiliary/helper functions
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef DLL_EXPORT
#define GAIAAUX_DECLARE __declspec(dllexport)
#else
#define GAIAAUX_DECLARE extern
#endif
#endif

#ifndef _GAIAAUX_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GAIAAUX_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constants */
/** SQL single quoted string (text constant) */
#define GAIA_SQL_SINGLE_QUOTE	1001
/** SQL double quoted string (SQL name) */
#define GAIA_SQL_DOUBLE_QUOTE	1002

/* function prototypes */

/**
 Retrieves the Locale Charset

 \return the GNU ICONV name identifying the locale charset
 */
    GAIAAUX_DECLARE const char *gaiaGetLocaleCharset (void);

/**
 Converts a text string from one charset to another

 \param buf the text string to be converted
 \param fromCs the GNU ICONV name identifying the input charset
 \param toCs the GNU ICONV name identifying the output charset

 \return 0 on failure, any other value on success.

 \note this function uses an internal buffer limited to 64KB;
 so it's not safe passing extremely huge-sized text string.
 */
    GAIAAUX_DECLARE int gaiaConvertCharset (char **buf, const char *fromCs,
					    const char *toCs);

/**
 Creates a persistent UTF8 converter object

 \param fromCS the GNU ICONV name identifying the input charset

 \return the handle of the converter object, or NULL on failure

 \sa gaiaFreeUTF8Converter

 \note you must properly destroy the converter object 
 when it isn't any longer used.
 */
    GAIAAUX_DECLARE void *gaiaCreateUTF8Converter (const char *fromCS);

/**
 Destroys an UTF8 converter object

 \param cvtCS the handle identifying the UTF8 convert object  
 (returned by a previous call to gaiaCreateUTF8Converter).

 \sa gaiaCreateUTF8Converter
 */
    GAIAAUX_DECLARE void gaiaFreeUTF8Converter (void *cvtCS);

/**
 Converts a text string to UTF8

 \param cvtCS the handle identifying the UTF8 convert object  
 (returned by a previous call to gaiaCreateUTF8Converter).
 \param buf the input text string
 \param len length (in bytes) of input string
 \param err on completion will contain 0 on success, any other value on failure

 \return the null-terminated UTF8 encoded string: NULL on failure

 \sa gaiaCreateUTF8Converter, gaiaFreeUTF8Converter
 
 \note this function can safely handle strings of arbitrary length,
 and will return the converted string into a dynamically allocated buffer 
 created by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaConvertToUTF8 (void *cvtCS, const char *buf,
					     int len, int *err);

/**
 Checks if a name is a reserved SQLite name

 \param name the name to be checked

 \return 0 if no: any other value if yes

 \sa gaiaIsReservedSqlName, gaiaIllegalSqlName
 */
    GAIAAUX_DECLARE int gaiaIsReservedSqliteName (const char *name);

/**
 Checks if a name is a reserved SQL name

 \param name the name to be checked

 \return 0 if no: any other value if yes

 \sa gaiaIsReservedSqliteName, gaiaIllegalSqlName
 */
    GAIAAUX_DECLARE int gaiaIsReservedSqlName (const char *name);

/**
 Checks if a name is an illegal SQL name

 \param name the name to be checked

 \return 0 if no: any other value if yes

 \sa gaiaIsReservedSqliteName, gaiaIsReservedSqlName
 */
    GAIAAUX_DECLARE int gaiaIllegalSqlName (const char *name);

/**
 Properly formats an SQL text constant

 \param value the text string to be formatted

 \return the formatted string: NULL on failure

 \sa gaiaQuotedSql, gaiaDequotedSql
 
 \note this function simply is a convenience method corresponding to: 
 gaiaQuotedSQL(value, GAIA_SQL_SINGLE_QUOTE);
 
 \remark passing a string like "Sant'Andrea" will return 'Sant''Andrea'
 */
    GAIAAUX_DECLARE char *gaiaSingleQuotedSql (const char *value);

/**
 Properly formats an SQL name

 \param value the SQL name to be formatted

 \return the formatted string: NULL on failure

 \sa gaiaQuotedSql, gaiaDequotedSql
 
 \note this function simply is a convenience method corresponding to: 
 gaiaQuotedSQL(value, GAIA_SQL_DOUBLE_QUOTE);

 \remark passing a string like "Sant\"Andrea" will return "Sant""Andrea"
 */
    GAIAAUX_DECLARE char *gaiaDoubleQuotedSql (const char *value);

/**
 Properly formats an SQL generic string

 \param value the string to be formatted
 \param quote GAIA_SQL_SINGLE_QUOTE or GAIA_SQL_DOUBLE_QUOTE

 \return the formatted string: NULL on failure

 \sa gaiaSingleQuotedSql, gaiaDoubleQuotedSql, gaiaDequotedSql

 \note this function can safely handle strings of arbitrary length,
 and will return the formatted string into a dynamically allocated buffer 
 created by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaQuotedSql (const char *value, int quote);

/**
 Properly formats an SQL generic string (dequoting)

 \param value the string to be dequoted

 \return the formatted string: NULL on failure

 \sa gaiaSingleQuotedSql, gaiaDoubleQuotedSql, gaiaQuotedSql

 \note this function can safely handle strings of arbitrary length,
 and will return the formatted string into a dynamically allocated buffer 
 created by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaDequotedSql (const char *value);

/*
/ DEPRECATED FUNCTION: gaiaCleanSqlString()
/ this function must not be used for any new project
/ it's still maintained for backward compatibility,
/ but will be probably removed in future versions
*/

/**
 deprecated function

 \param value the string to be formatted

 \sa gaiaQuotedSql

 \note this function is still supported simply for backward compatibility.
 it's intrinsically unsafe (passing huge strings potentially leads to 
 buffer overflows) and you are strongly encouraged to use gaiaQuotedSql()
 as a safest replacement.
 */
    GAIAAUX_DECLARE void gaiaCleanSqlString (char *value);

/**
 SQL log: statement start

 \param sqlite handle of the current DB connection
 \param user_agent name of the invoking application, e.g. "spatialite_gui" or "spatialite CLI"
 \param utf8Sql the SQL statement bein executed
 \param sqllog_pk after completion this variable will contain the value
  of the Primary Key identifying the corresponding Log event

 \sa gaiaUpdateSqlLog

 \note this function inserts an \b event into the SQL Log, and
  is expected to be invoked immediately \b before executing the SQL
  statement itself.
 */
    GAIAAUX_DECLARE void gaiaInsertIntoSqlLog (sqlite3 * sqlite,
					       const char *user_agent,
					       const char *utf8Sql,
					       sqlite3_int64 * sqllog_pk);

/**
 SQL log: statement start

 \param sqlite handle of the current DB connection
 \param sqllog_pk the Primary Key identifying the corresponding Log event.
 \n expected to be exactely the same returned by the most recent call to gaiaInsertIntoSqlLog()
 \param success expected to be TRUE if the SQL statement was successfully executed.
 \param errMsg expected to be the error message returned by SQLite on failure, NULL on success.

 \sa gaiaInsertIntoSqlLog

 \note this function completes an \b event inserted into the SQL Log, and
  is expected to be invoked immediately \b after executing the SQL
  statement itself.
 */
    GAIAAUX_DECLARE void gaiaUpdateSqlLog (sqlite3 * sqlite,
					   sqlite3_int64 sqllog_pk, int success,
					   const char *errMsg);

/**
 Creates a persistent MD5 checksum object

 \return the handle of an MD5 checksum object, or NULL on failure

 \sa gaiaFreeMD5Checksum, gaiaUpdateMD5Checksum, gaiaFinalizeMD5Checksum

 \note you must properly destroy the MD5 object 
 when it isn't any longer used.
 */
    GAIAAUX_DECLARE void *gaiaCreateMD5Checksum (void);

/**
 Destroys an MD5 checksum object

 \param md5 the handle of the MD5 checksum object (returned by
 a previous call to gaiaCreateMD5Checksum).

 \sa gaiaCreateMD5Checksum
 */
    GAIAAUX_DECLARE void gaiaFreeMD5Checksum (void *md5);

/**
 Updates an MD5 checksum object

 \param md5 the handle of the MD5 checksum object (returned by
 a previous call to gaiaCreateMD5Checksum).
 \param blob an arbitrary sequence of binary data
 \param blob_size the length (in bytes) of the binary data

 \sa gaiaCreateMD5Checksum, gaiaFreeMD5Checksum, gaiaFinalizeMD5Checksum

 \note you can repeatedly invoke gaiaUpdateMD5Checksum more than a single
 time and always using the same MD5 object. 
 In this case the final MD5 checksum returned by gaiaGetMD5Checsum will be 
 the total checksum for any data processed by the MD5 object since its
 initialization.
 */
    GAIAAUX_DECLARE void gaiaUpdateMD5Checksum (void *md5,
						const unsigned char *blob,
						int blob_len);

/**
 Return an MD5 checksum value

 \param md5 the handle of the MD5 checksum object (returned by
 a previous call to gaiaCreateMD5Checksum).

 \return an hexadecimal text string representing the MD checksum:
 NULL on failure

 \sa gaiaCreateMD5Checksum, gaiaUpdateMD5Checksum, gaiaFreeMD5Checksum

 \note this function will return the MD5 checksum into a dynamically allocated 
 buffer created by malloc(). 
 You are required to explicitly free() any string returned by this function.

 \note gaiaFinalizeMD5Checksum will implicitly reset the MD5 object to its
 initial state.
 */
    GAIAAUX_DECLARE char *gaiaFinalizeMD5Checksum (void *md5);

/**
 Return longitude and latitude angles from a DMS string

 \param dms a text string representing a valid DMS (Degrees/Minutes/Seconds)
 expression.
 \param longitude on completion this variable will contain the longitude angle
 expressed in Decimal Degrees.
 \param latitude on completion this variable will contain the latitude angle
 expressed in Decimal Degrees.

 \return ZERO (FALSE) on failure, any other different value (TRUE) on success.

 \sa gaiaConvertToDMS

 \note this function will return a dynamically allocated buffer created 
 by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE int gaiaParseDMS (const char *dms, double *longitude,
				      double *latitude);

/**
 Return a DMS string

 \param longitude the angle of longitude expressed in Decimal Degrees.
 \param latitude the angle of latitude expressed in Decimal Degrees.

 \return the corresponding DMS (Degrees/Minutes/Seconds) text string,
 or NULL on failure

 \sa gaiaLongitudeFromDMS, gaiaLatitudeFromDMS

 \note this function will return a dynamically allocated buffer created 
 by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaConvertToDMS (double longitude, double latitude);

/**
 Return a percent-encoded URL

 \param url the URL to be percent-encoded

 \return the corresponding percent-encoded URL text string,
 or NULL on failure

 \sa gaiaDecodeURL

 \note this function will return a dynamically allocated buffer created 
 by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaEncodeURL (const char *url);

/**
 Return a clean URL from its percent-encoded representation

 \param encoded the percent-encoded URL to be decoded

 \return the corresponding clean URL text string,
 or NULL on failure

 \sa gaiaEncodeURL

 \note this function will return a dynamically allocated buffer created 
 by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaDecodeURL (const char *encoded);

/**
 Return the DirName component (if any) from a Path

 \param path full or relative pathname

 \return the corresponding DirName text string,
 or NULL on failure

 \sa gaiaFullFileNameFromPath, gaiaFileNameFromPath, gaiaFileExtFromPath

 \note this function will return a dynamically allocated buffer created 
 by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaDirNameFromPath (const char *path);

/**
 Return the FullFileName from a Path

 \param path full or relative pathname

 \return the corresponding FullFileName (including an eventual extension),
  or NULL on failure

 \sa gaiaDirNameFromPath, gaiaFileNameFromPath, gaiaFileExtFromPath

 \note this function will return a dynamically allocated buffer created 
 by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaFullFileNameFromPath (const char *path);

/**
 Return the FileName from a Path

 \param path full or relative pathname

 \return the corresponding FileName (excluding an eventual extension),
  or NULL on failure

 \sa gaiaDirNameFromPath, gaiaFullFileNameFromPath, gaiaFileExtFromPath

 \note this function will return a dynamically allocated buffer created 
 by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaFileNameFromPath (const char *path);

/**
 Return the FileExtension from a Path

 \param path full or relative pathname

 \return the corresponding FileExtension (if any), or NULL on failure

 \sa gaiaDirNameFromPath, gaiaFullFileNameFromPath, gaiaFileNameFromPath

 \note this function will return a dynamically allocated buffer created 
 by malloc(). 
 You are required to explicitly free() any string returned by this function.
 */
    GAIAAUX_DECLARE char *gaiaFileExtFromPath (const char *path);

#ifdef __cplusplus
}
#endif

#endif				/* _GAIAAUX_H */
