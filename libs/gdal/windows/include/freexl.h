/* 
/ freexl.h
/
/ public declarations
/
/ version  1.0, 2011 July 26
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ ------------------------------------------------------------------------------
/ 
/ Version: MPL 1.1/GPL 2.0/LGPL 2.1
/ 
/ The contents of this file are subject to the Mozilla Public License Version
/ 1.1 (the "License"); you may not use this file except in compliance with
/ the License. You may obtain a copy of the License at
/ http://www.mozilla.org/MPL/
/ 
/ Software distributed under the License is distributed on an "AS IS" basis,
/ WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
/ for the specific language governing rights and limitations under the
/ License.
/
/ The Original Code is the FreeXL library
/
/ The Initial Developer of the Original Code is Alessandro Furieri
/ 
/ Portions created by the Initial Developer are Copyright (C) 2011
/ the Initial Developer. All Rights Reserved.
/ 
/ Contributor(s):
/ Brad Hards 
/
/ Alternatively, the contents of this file may be used under the terms of
/ either the GNU General Public License Version 2 or later (the "GPL"), or
/ the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
/ in which case the provisions of the GPL or the LGPL are applicable instead
/ of those above. If you wish to allow use of your version of this file only
/ under the terms of either the GPL or the LGPL, and not to allow others to
/ use your version of this file under the terms of the MPL, indicate your
/ decision by deleting the provisions above and replace them with the notice
/ and other provisions required by the GPL or the LGPL. If you do not delete
/ the provisions above, a recipient may use your version of this file under
/ the terms of any one of the MPL, the GPL or the LGPL.
/ 
*/

/**
 \file freexl.h 
 
 Function declarations and constants for FreeXL library
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef DLL_EXPORT
#define FREEXL_DECLARE __declspec(dllexport)
#else
#define FREEXL_DECLARE extern
#endif
#endif

#ifndef _FREEXL_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _FREEXL_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constants */
/** query is not applicable, or information is not available */
#define FREEXL_UNKNOWN			0

/* CFBF constants */
/** CFBF file is version 3 */
#define FREEXL_CFBF_VER_3		3
/** CFBF file is version 4 */
#define FREEXL_CFBF_VER_4		4

/** CFBF file uses 512 byte sectors */
#define FREEXL_CFBF_SECTOR_512		512
/** CFBF file uses 4096 (4K) sectors */
#define FREEXL_CFBF_SECTOR_4096		4096

/* BIFF versions */
/** BIFF file is version 2 */
#define FREEXL_BIFF_VER_2		2
/** BIFF file is version 3 */
#define FREEXL_BIFF_VER_3		3
/** BIFF file is version 4 */
#define FREEXL_BIFF_VER_4		4
/** BIFF file is version 5 */
#define FREEXL_BIFF_VER_5		5
/** BIFF file is version 9 */
#define FREEXL_BIFF_VER_8		8

/* BIFF MaxRecordSize */
/** Maximum BIFF record size is 2080 bytes */
#define FREEXL_BIFF_MAX_RECSZ_2080	2080
/** Maximum BIFF record size is 8224 bytes */
#define FREEXL_BIFF_MAX_RECSZ_8224	8224

/* BIFF DateMode */
/** BIFF date mode starts at 1 Jan 1900 */
#define FREEXL_BIFF_DATEMODE_1900	1900
/** BIFF date mode starts at 2 Jan 1904 */
#define FREEXL_BIFF_DATEMODE_1904	1904

/* BIFF Obsfuscated */
/** BIFF file is password protected */
#define FREEXL_BIFF_OBFUSCATED		3003
/** BIFF file is not password protected */
#define FREEXL_BIFF_PLAIN		3004

/* BIFF CodePage */
/** BIFF file uses plain ASCII encoding */
#define FREEXL_BIFF_ASCII		0x016F
/** BIFF file uses CP437 (OEM US format) encoding */
#define FREEXL_BIFF_CP437		0x01B5
/** BIFF file uses CP720 (Arabic DOS format) encoding */
#define FREEXL_BIFF_CP720		0x02D0
/** BIFF file uses CP737 (Greek DOS format) encoding */
#define FREEXL_BIFF_CP737		0x02E1
/** BIFF file uses CP775 (Baltic DOS format) encoding */
#define FREEXL_BIFF_CP775		0x0307
/** BIFF file uses CP850 (Western Europe DOS format) encoding */
#define FREEXL_BIFF_CP850		0x0352
/** BIFF file uses CP852 (Central Europe DOS format) encoding */
#define FREEXL_BIFF_CP852		0x0354
/** BIFF file uses CP855 (OEM Cyrillic format) encoding */
#define FREEXL_BIFF_CP855		0x0357
/** BIFF file uses CP857 (Turkish DOS format) encoding */
#define FREEXL_BIFF_CP857		0x0359
/** BIFF file uses CP858 (OEM Multiligual Latin 1 format) encoding */
#define FREEXL_BIFF_CP858		0x035A
/** BIFF file uses CP860 (Portuguese DOS format) encoding */
#define FREEXL_BIFF_CP860		0x035C
/** BIFF file uses CP861 (Icelandic DOS format) encoding */
#define FREEXL_BIFF_CP861		0x035D
/** BIFF file uses CP862 (Hebrew DOS format) encoding */
#define FREEXL_BIFF_CP862		0x035E
/** BIFF file uses CP863 (French Canadian DOS format) encoding */
#define FREEXL_BIFF_CP863		0x035F
/** BIFF file uses CP864 (Arabic DOS format) encoding */
#define FREEXL_BIFF_CP864		0x0360
/** BIFF file uses CP865 (Nordic DOS format) encoding */
#define FREEXL_BIFF_CP865		0x0361
/** BIFF file uses CP866 (Cyrillic DOS format) encoding */
#define FREEXL_BIFF_CP866		0x0362
/** BIFF file uses CP869 (Modern Greek DOS format) encoding */
#define FREEXL_BIFF_CP869		0x0365
/** BIFF file uses CP874 (Thai Windows format) encoding */
#define FREEXL_BIFF_CP874		0x036A
/** BIFF file uses CP932 (Shift JIS format) encoding */
#define FREEXL_BIFF_CP932		0x03A4
/** BIFF file uses CP936 (Simplified Chinese GB2312 format) encoding */
#define FREEXL_BIFF_CP936		0x03A8
/** BIFF file uses CP949 (Korean) encoding */
#define FREEXL_BIFF_CP949		0x03B5
/** BIFF file uses CP950 (Traditional Chinese Big5 format) encoding */
#define FREEXL_BIFF_CP950		0x03B6
/** BIFF file uses Unicode (UTF-16LE format) encoding */
#define FREEXL_BIFF_UTF16LE		0x04B0
/** BIFF file uses CP1250 (Central Europe Windows) encoding */
#define FREEXL_BIFF_CP1250		0x04E2
/** BIFF file uses CP1251 (Cyrillic Windows) encoding */
#define FREEXL_BIFF_CP1251		0x04E3
/** BIFF file uses CP1252 (Windows Latin 1) encoding */
#define FREEXL_BIFF_CP1252		0x04E4
/** BIFF file uses CP1252 (Windows Greek) encoding */
#define FREEXL_BIFF_CP1253		0x04E5
/** BIFF file uses CP1254 (Windows Turkish) encoding */
#define FREEXL_BIFF_CP1254		0x04E6
/** BIFF file uses CP1255 (Windows Hebrew) encoding */
#define FREEXL_BIFF_CP1255		0x04E7
/** BIFF file uses CP1256 (Windows Arabic) encoding */
#define FREEXL_BIFF_CP1256		0x04E8
/** BIFF file uses CP1257 (Windows Baltic) encoding */
#define FREEXL_BIFF_CP1257		0x04E9
/** BIFF file uses CP1258 (Windows Vietnamese) encoding */
#define FREEXL_BIFF_CP1258		0x04EA
/** BIFF file uses CP1361 (Korean Johab) encoding */
#define FREEXL_BIFF_CP1361		0x0551
/** BIFF file uses Mac Roman encoding */
#define FREEXL_BIFF_MACROMAN		0x2710

/* CELL VALUE Types */
/** Cell has no value (empty cell) */
#define FREEXL_CELL_NULL		101
/** Cell contains an integer value */
#define FREEXL_CELL_INT			102
/** Cell contains a floating point number */
#define FREEXL_CELL_DOUBLE		103
/** Cell contains a text value */
#define FREEXL_CELL_TEXT		104
/** Cell contains a reference to a Single String Table entry (BIFF8) */
#define FREEXL_CELL_SST_TEXT		105
/** Cell contains a number intended to represent a date */
#define FREEXL_CELL_DATE		106
/** Cell contains a number intended to represent a date and time */
#define FREEXL_CELL_DATETIME		107
/** Cell contains a number intended to represent a time */
#define FREEXL_CELL_TIME		108

/* INFO params */
/** Information query for CFBF version */
#define FREEXL_CFBF_VERSION		32001
/** Information query for CFBF sector size */
#define FREEXL_CFBF_SECTOR_SIZE		32002
/** Information query for CFBF FAT entry count */
#define FREEXL_CFBF_FAT_COUNT		32003
/** Information query for BIFF version */
#define FREEXL_BIFF_VERSION		32005
/** Information query for BIFF maximum record size */
#define FREEXL_BIFF_MAX_RECSIZE		32006
/** Information query for BIFF date mode */
#define FREEXL_BIFF_DATEMODE		32007
/** Information query for BIFF password protection state */
#define FREEXL_BIFF_PASSWORD		32008
/** Information query for BIFF character encoding */
#define FREEXL_BIFF_CODEPAGE		32009
/** Information query for BIFF sheet count */
#define FREEXL_BIFF_SHEET_COUNT		32010
/** Information query for BIFF Single String Table entry count (BIFF8) */
#define FREEXL_BIFF_STRING_COUNT	32011
/** Information query for BIFF format count */
#define FREEXL_BIFF_FORMAT_COUNT	32012
/** Information query for BIFF extended format count */
#define FREEXL_BIFF_XF_COUNT		32013

/* Error codes */
#define FREEXL_OK			0 /**< No error, success */
#define FREEXL_FILE_NOT_FOUND		-1 /**< .xls file does not exist or is
						not accessible for reading */
#define FREEXL_NULL_HANDLE		-2 /**< Null xls_handle argument */
#define FREEXL_INVALID_HANDLE		-3 /**< Invalid xls_handle argument */
#define FREEXL_INSUFFICIENT_MEMORY	-4 /**< some kind of memory allocation
                                                failure */
#define FREEXL_NULL_ARGUMENT		-5 /**< an unexpected null argument */
#define FREEXL_INVALID_INFO_ARG		-6 /**< invalid "what" parameter */
#define FREEXL_INVALID_CFBF_HEADER	-7 /**< the .xls file does not contain a
                                                valid CFBF header */
#define FREEXL_CFBF_READ_ERROR		-8 /**< Read error. Usually indicates a
                                                corrupt or invalid .xls file */
#define FREEXL_CFBF_SEEK_ERROR		-9 /**< Seek error. Usually indicates a
                                                corrupt or invalid .xls file */
#define FREEXL_CFBF_INVALID_SIGNATURE	-10 /**< The .xls file does contain a
                                                 CFBF header, but the header is
                                                 broken or corrupted in some way
                                                 */
#define FREEXL_CFBF_INVALID_SECTOR_SIZE	-11 /**< The .xls file does contain a
                                                 CFBF header, but the header is
                                                 broken or corrupted in some way
                                                 */
#define FREEXL_CFBF_EMPTY_FAT_CHAIN	-12 /**< The .xls file does contain a
                                                 CFBF header, but the header is
                                                 broken or corrupted in some way
                                                 */
#define FREEXL_CFBF_ILLEGAL_FAT_ENTRY	-13 /**< The file contains an invalid
                                                 File Allocation Table record */
#define FREEXL_BIFF_INVALID_BOF		-14 /**< The file contains an invalid
                                                 BIFF format entry */
#define FREEXL_BIFF_INVALID_SST		-15 /**< The file contains an invalid
                                                 Single String Table */
#define FREEXL_BIFF_ILLEGAL_SST_INDEX	-16 /**< The requested Single String 
                                                 Table entry is not available */
#define FREEXL_BIFF_WORKBOOK_NOT_FOUND	-17 /**< BIFF does not contain a valid
                                                 workbook */
#define FREEXL_BIFF_ILLEGAL_SHEET_INDEX	-18 /**< The requested worksheet is not
                                                 available in the workbook */
#define FREEXL_BIFF_UNSELECTED_SHEET	-19 /**< There is no currently active
                                              worksheet. Possibly a forgotten
                                              call to 
                                              freexl_select_active_worksheet()
                                             */
#define FREEXL_INVALID_CHARACTER	-20 /**< Charset conversion detected an
                                                 illegal character (not within
                                                 the declared charset) */
#define FREEXL_UNSUPPORTED_CHARSET	-21 /**< The requested charset
                                                 conversion is not available. */
#define FREEXL_ILLEGAL_CELL_ROW_COL	-22 /**< The requested cell is outside
                                                 the valid range for the sheet*/
#define FREEXL_ILLEGAL_RK_VALUE		-23 /**< Conversion of the RK value 
                                                 failed. Possibly a corrupt file
                                                 or a bug in FreeXL. */
#define FREEXL_ILLEGAL_MULRK_VALUE	-23 /**< Conversion of the MULRK value 
                                                 failed. Possibly a corrupt file
                                                 or a bug in FreeXL. */
#define FREEXL_INVALID_MINI_STREAM	-24 /**< The MiniFAT stream is invalid.
                                                 Possibly a corrupt file. */
#define FREEXL_CFBF_ILLEGAL_MINI_FAT_ENTRY	-25 /**< The MiniFAT stream 
                                                     contains an invalid entry.
                                                     Possibly a corrupt file. */

    /**
     Container for a cell value
     
     freexl_get_cell_value() takes a pointer to this structure, and fills
     in the appropriate values.
     
     \code
	FreeXL_CellValue val;
	freexl_get_cell_value(..., &val);
	switch (val.type)
	{
	    case FREEXL_CELL_INT:
		printf("Int=%d\n", val.value.int_value;
		break;
	    case FREEXL_CELL_DOUBLE:
		printf("Double=%1.2f\n", val.value.double_value;
		break;
	    case FREEXL_CELL_TEXT:
	    case FREEXL_CELL_SST_TEXT:
		printf("Text='%s'\n", val.value.text_value;
		break;
	    case FREEXL_CELL_DATE:
	    case FREEXL_CELL_DATETIME:
	    case FREEXL_CELL_TIME:
		printf("DateOrTime='%s'\n", val.value.text_value;
		break;
	    case FREEXL_CELL_NULL:
		printf("NULL\n");
		break;
	    default:
		printf("Invalid data-type\n");
		break;
	}
     \endcode
     */
    struct FreeXL_CellValue_str
    {
	/**
	 The type of data stored in this cell. Can be one of the following:
	 - FREEXL_CELL_NULL the cell contains a NULL value.
	 - FREEXL_CELL_INT the cell contains an INTEGER value.
	 - FREEXL_CELL_DOUBLE the cell contains a DOUBLE value.
	 - FREEXL_CELL_TEXT or FREEXL_CELL_SST_TEXT the cell contains a text 
	 string (always UTF-8 encoded)
	 - FREEXL_CELL_DATE the cell contains a date, encoded as a 'YYYY-MM-DD'
	 string value
	 - FREEXL_CELL_DATETIME the cell contains a date and time, encoded as a
	 'YYYY-MM-DD HH:MM:SS' string value
	 - FREEXL_CELL_TIME the cell contains a time, encoded as a 'HH:MM:SS'
	 string value
	 */
	unsigned char type;
	union
	{
	    int int_value; /**< if type is FREEXL_CELL_INT, then the
	    corresponding value will be returned as int_value */
	    double double_value; /**< if type is FREEXL_CELL_DOUBLE, then the
	    corresponding value will be returned as double_value */
	    const char *text_value; /**< if type is FREEXL_CELL_TEXT,
	    FREEXL_CELL_SST_TEXT, FREEXL_CELL_DATE, FREEXL_CELL_DATETIME or
	    FREEXL_CELL_TIME the corresponding value will be returned as
	    text_value */
	} value; /**< The value of the data stored in the cell. Which part of
	              the union is valid is determined by the type value. */
    };

    /**
     Typedef for cell value structure.
     
     \sa FreeXL_CellValue_str
     */
    typedef struct FreeXL_CellValue_str FreeXL_CellValue;


    /**
     Return the current library version.
     
     \return the version string.
     */
    FREEXL_DECLARE const char *freexl_version (void);

    /**
     Open the .xls file, preparing for future functions
     
     \param path full or relative pathname of the input .xls file.
     \param xls_handle an opaque reference (handle) to be used in each
     subsequent function (return value).

     \return FREEXL_OK will be returned on success, otherwise any appropriate
     error code on failure.

     \note You are expected to freexl_close() even on failure, so as to
     correctly release any dynamic memory allocation.
     */
    FREEXL_DECLARE int freexl_open (const char *path, const void **xls_handle);

    /**
     Open the .xls file for metadata query only
     
     This is similar to freexl_open(), except that an abbreviated parsing
     step is performed. This makes it faster, but does not support queries
     for cell values. 
     
     \param path full or relative pathname of the input .xls file.
     \param xls_handle an opaque reference (handle) to be used in each
     subsequent function (return value).

     \return FREEXL_OK will be returned on success, otherwise any appropriate
     error code on failure.

     \note You are expected to freexl_close() even on failure, so as to
     correctly release any dynamic memory allocation.
     */
    FREEXL_DECLARE int freexl_open_info (const char *path,
					 const void **xls_handle);

    /** 
     Close the .xls file and releasing any allocated resource

    \param xls_handle the handle previously returned by freexl_open()

    \return FREEXL_OK will be returned on success
    
    \note After calling freexl_close() any related resource will be released,
    and the handle will no longer be valid.
    */
    FREEXL_DECLARE int freexl_close (const void *xls_handle);

    /**
     Query general information about the Workbook and Worksheets

     \param xls_handle the handle previously returned by freexl_open()
     \param what the info to be queried.
     \param info the corresponding information value (return value)
     
     \note FREEXL_UNKNOWN will be returned in \p info if the information is 
     not available, not appropriate or not supported for the file type.
     
     \return FREEXL_OK will be returned on success

    Valid values for \p what are:
    - FREEXL_CFBF_VERSION (returning FREEXL_CFBF_VER_3 or FREEXL_CFBF_VER_4)
    - FREEXL_CFBF_SECTOR_SIZE (returning FREEXL_CFBF_SECTOR_512 or
    FREEXL_CFBF_SECTOR_4096)
    - FREEXL_CFBF_FAT_COUNT (returning the total count of FAT entries in the
    file)
    - FREEXL_BIFF_VERSION (return one of FREEXL_BIFF_VER_2, FREEXL_BIFF_VER_3,
    FREEXL_BIFF_VER_4, FREEXL_BIFF_VER_5, FREEXL_BIFF_VER_8)
    - FREEXL_BIFF_MAX_RECSIZE (returning FREEXL_BIFF_MAX_RECSZ_2080 or
    FREEXL_BIFF_MAX_RECSZ_8224)
    - FREEXL_BIFF_DATEMODE (returning FREEXL_BIFF_DATEMODE_1900 or
    FREEXL_BIFF_DATEMODE_1904)
    - FREEXL_BIFF_PASSWORD (returning FREEXL_BIFF_OBFUSCATED or
    FREEXL_BIFF_PLAIN)
    - FREEXL_BIFF_CODEPAGE (returning FREEXL_BIFF_ASCII, one of
    FREEXL_BIFF_CP*,FREEXL_BIFF_UTF16LE
    or FREEXL_BIFF_MACROMAN)
    - FREEXL_BIFF_SHEET_COUNT (returning the total number of worksheets)
    - FREEXL_BIFF_STRING_COUNT (returning the total number of Single String
    Table entries)
    - FREEXL_BIFF_FORMAT_COUNT (returning the total number of format entries)
    - FREEXL_BIFF_XF_COUNT (returning the number of extended format entries)
    */
    FREEXL_DECLARE int freexl_get_info (const void *xls_handle,
					unsigned short what,
					unsigned int *info);

    /**
     Query worksheet name

    \param xls_handle the handle previously returned by freexl_open()
    \param sheet_index the index identifying the worksheet (base 0)
    \param string the name of the worksheet (return value)
    
    \return FREEXL_OK will be returned on success
    */
    FREEXL_DECLARE int freexl_get_worksheet_name (const void *xls_handle,
						  unsigned short sheet_index,
						  const char **string);

    /**
     Set the currently active worksheets
      
     Within a FreeXL handle, only one worksheet can be active at a time.
     Functions that fetch data are implictly working on the selected worksheet.

     \param xls_handle the handle previously returned by freexl_open()
     \param sheet_index the index identifying the worksheet (base 0)
     
     \return FREEXL_OK will be returned on success
     */
    FREEXL_DECLARE int freexl_select_active_worksheet (const void *xls_handle,
						       unsigned short
						       sheet_index);

    /**
     Query the currently active worksheet index
     
     \param xls_handle the handle previously returned by freexl_open()
     \param sheet_index the index corresponding to the currently active
     Worksheet (return value)
     
     \return FREEXL_OK will be returned on success
     
     \sa freexl_select_active_worksheet() for how to select the worksheet
    */
    FREEXL_DECLARE int freexl_get_active_worksheet (const void *xls_handle,
						    unsigned short
						    *sheet_index);

    /**
     Query worksheet dimensions
     
     This function returns the number of rows and columns for the currently
     selected worksheet. 

     \param xls_handle the handle previously returned by freexl_open()
     \param rows the total row count (return value)
     \param columns the total column count (return value)

     \return FREEXL_OK will be returned on success

     \note Worksheet dimensions are zero based, so if you
     have a worksheet that is four columns and two rows (i.e. from A1 in the
     top left corner to B4 in the bottom right corner), this will return rows
     equal to 1 and columns equal to 3). This is to support C style looping.
     */
    FREEXL_DECLARE int freexl_worksheet_dimensions (const void *xls_handle,
						    unsigned int *rows,
						    unsigned short *columns);

    /**
     Retrieve string entries from SST
     
     \param xls_handle the handle previously returned by freexl_open()
     \param string_index the index identifying the String entry (base 0).
     \param string the corresponding String value (return value)
     
     \return FREEXL_OK will be returned on success
     
     \note This function is not normally required, since freexl_get_cell_value
     will return the string where appropriate. It is mainly intended for
     debugging purposes.
     
    */
    FREEXL_DECLARE int freexl_get_SST_string (const void *xls_handle,
					      unsigned short string_index,
					      const char **string);

    /**
     Retrieve FAT entries from FAT chain

     \param xls_handle the handle previously returned by freexl_open()
     \param sector_index the index identifying the Sector entry (base 0).
     \param next_sector_index the index identifying the next Sector to be
     accessed in logical order (return value).
    
     \note The following values imply special meaning:
     - 0xffffffff free / unused sector
     - 0xfffffffe end of chain
     - 0xfffffffd sector used by FAT (map of sectors)
     - 0xfffffffc double-indirect FAT sector (map of FAT sectors)
    
     \return FREEXL_OK will be returned on success

     \note This function is not normally required, since FreeXL will handle
     FAT table entries transparent to the user. It is mainly intended for
     debugging purposes.
     
     */
    FREEXL_DECLARE int freexl_get_FAT_entry (const void *xls_handle,
					     unsigned int sector_index,
					     unsigned int *next_sector_index);

    /**
     Retrieve individual cell values from the currently active worksheet 

     \param xls_handle the handle previously returned by freexl_open()
     \param row row number of the cell to query (zero base)
     \param column column number of the cell to query (zero base)
     \param value the cell type and value (return value)

     \return FREEXL_OK will be returned on success
    */
    FREEXL_DECLARE int freexl_get_cell_value (const void *xls_handle,
					      unsigned int row,
					      unsigned short column,
					      FreeXL_CellValue * value);

#ifdef __cplusplus
}
#endif

#endif				/* _FREEXL_H */
