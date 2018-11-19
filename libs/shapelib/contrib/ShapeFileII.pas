{
/******************************************************************************
 * $Id: ShapeFileII.pas,v 1.4 2016-12-05 12:44:07 erouault Exp $
 *
 * Project:  Shapelib
 * Purpose:  Delphi Pascal interface to Shapelib.
 * Author:   Kevin Meyer (Kevin@CyberTracker.co.za)
 *
 ******************************************************************************
 * Copyright (c) 2002, Keven Meyer (Kevin@CyberTracker.co.za)
 *
 * This software is available under the following "MIT Style" license,
 * or at the option of the licensee under the LGPL (see COPYING).  This
 * option is discussed in more detail in shapelib.html.
 *
 * --
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log: ShapeFileII.pas,v $
 * Revision 1.4  2016-12-05 12:44:07  erouault
 * * Major overhaul of Makefile build system to use autoconf/automake.
 *
 * * Warning fixes in contrib/
 *
 * Revision 1.3  2003-05-14 20:04:51  warmerda
 * Changed fpSHP and fpSHX to integer at suggestion of Ivan Lucena.
 *
 * Revision 1.2  2002/01/21 14:09:26  warmerda
 * Fixed name.
 *
 * Revision 1.1  2002/01/17 14:30:37  warmerda
 * New
 *
 */
}
unit ShapeFileII;

interface
//uses	{ uses clause }
//    ;
{ Set compiler to pack on byte boundaries only }
{$ALIGN OFF}
{$OVERFLOWCHECKS OFF}
{$J-}
const
    SHPT_NULL	 = 0;
    SHPT_POINT	 = 1;
    SHPT_ARC	 = 3;
    SHPT_POLYGON	 = 5;
    SHPT_MULTIPOINT	 = 8;
    SHPT_POINTZ	 = 11;
    SHPT_ARCZ	 = 13;
    SHPT_POLYGONZ	 = 15;
    SHPT_MULTIPOINTZ  = 18;
    SHPT_POINTM	 = 21;
    SHPT_ARCM	 = 23;
    SHPT_POLYGONM	 = 25;
    SHPT_MULTIPOINTM  = 28;
    SHPT_MULTIPATCH  = 31;
    XBASE_FLDHDR_SZ = 32;
    szAccessBRW = 'rb+';

// *********************** SHP support ************************

type
SHPObject = record
    nSHPType,
    nShapeId,
    nParts : LongWord;
    panPartStart,
    panPartType : array of LongWord;
    nVertices : LongWord;
    padfX, padfY, padfZ, padfM : array of double;
    dfXMin, dfYMin, dfZMin, dfMMin : double;
    dfXMax, dfYMax, dfZMax, dfMMax : double;
end;
SHPObjectHandle = ^SHPObject;

SHPBoundsArr = double;

SHPInfo = record
    fpSHP,
    fpSHX : integer;

    nShapeType,
    nFileSize,
    nRecords,
    nMaxRecords : LongWord;
    panRecOffset,
    panRecSize : array of LongWord;
    adBoundsMin, adBoundsMax : SHPBoundsArr;
    bUpdated : LongWord;
end;
SHPHandle = ^SHPInfo;

// *********************** DBF support ************************

DBFInfo = record
    fp : FILE;
    nRecords,
    nRecordLength,
    nHeaderLength,
    nFields : LongWord;

    panFieldOffset,
    panFieldSize,
    panFieldDecimals : array of LongWord;

    pachFieldType : LongWord;
    pszHeader : PChar;
    nCurrentRecord,
    bCurrentRecordModified : LongWord;
    pszCurrentRecord : PChar;

    bNoHeader,
    bUpdated : LongWord;
end;
DBFHandle = ^DBFInfo;

DBFFieldType = (DBFTString,  DBFTInteger, DBFTDouble,  DBFTInvalid) ;

// *********************** SHP func declarations ************************

{$ALIGN ON}

function SHPOpen(pszShapeFile, pszAccess : PChar) : SHPHandle;cdecl;
procedure SHPGetInfo(hSHP : SHPHandle; var pnEntities, pnShapeType : LongWord; var padfMinBoud, padfMaxBound : SHPBoundsArr);cdecl;
procedure SHPClose(hSHP : SHPHandle);cdecl;
function SHPReadObject(hSHP : SHPHandle; iShape : LongWord): SHPObjectHandle;cdecl;
function SHPCreate(pszShapeFile : PChar; nShapeType : LongWord):SHPHandle;cdecl;
function SHPWriteObject(hSHP : SHPHandle; iShape : LongWord; psObject : SHPObjectHandle): LongWord;cdecl;
function SHPCreateSimpleObject(nSHPType, nVertices : LongWord; var padfX, padfY, padfZ : double):SHPObjectHandle;cdecl;
procedure SHPDestroy(psObject : SHPObjectHandle);cdecl;

procedure SHPComputeExtents(psObject : SHPObjectHandle);cdecl;
function SHPCreateObject(nSHPType, iShape, nParts : LongWord; var panPartStart, panPartType : LongWord; nVertices : LongWord; var padfX, padfY, padfZ, padfM : SHPBoundsArr): SHPObjectHandle;cdecl;

function SHPTypeStr(pnShapeType : LongWord): string;

// *********************** DBF func declarations ************************

function DBFOpen(pszDBFFile, pszAccess : PChar): DBFHandle;cdecl;
function DBFCreate(pszDBFFile : PChar): DBFHandle ;cdecl;
function DBFGetFieldCount(hDBF : DBFHandle) : LongWord ;cdecl;
function DBFGetRecordCount(hDBF : DBFHandle) : LongWord;cdecl;
function DBFGetFieldIndex(hDBF: DBFHandle; pszFieldName : PChar): LongWord;cdecl;
function DBFGetFieldInfo(hDBF : DBFHandle; iField : LongWord; pszFieldName : PChar;
                              var pnWidth, pnDecimals : LongWord): DBFFieldType;cdecl;
function DBFAddField(hDBF : DBFHandle; pszFieldName : PChar;
                 eType : DBFFieldType; nWidth, nDecimals  : LongWord): LongWord;cdecl;

function DBFReadIntegerAttribute(hDBF : DBFHandle;iShape,  iField  : LongWord ): LongWord;cdecl;
function DBFReadDoubleAttribute(hDBF : DBFHandle; iShape,  iField  : LongWord ):double;cdecl;
function DBFReadStringAttribute(hDBF : DBFHandle; iShape,  iField  : LongWord ) : pchar;cdecl;
function DBFIsAttributeNULL(hDBF : DBFHandle; iShape,  iField  : LongWord ): LongWord;cdecl;
function DBFWriteIntegerAttribute(hDBF : DBFHandle;iShape, iField, nFieldValue : LongWord): LongWord;cdecl;
function DBFWriteDoubleAttribute(hDBF : DBFHandle;iShape, iField   : LongWord;
                             dFieldValue : double): LongWord ;cdecl;
function DBFWriteStringAttribute(hDBF : DBFHandle;iShape, iField   : LongWord;
                             pszFieldValue : PChar): LongWord ;cdecl;
function DBFWriteNULLAttribute(hDBF : DBFHandle; iShape,  iField  : LongWord ) : LongWord;cdecl;
procedure DBFClose(hDBF : DBFHandle);cdecl;
function DBFGetNativeFieldType(hDBF : DBFHandle;  iField : LongWord) : Char;cdecl;

// *********************** SHP implementation ************************
implementation
// *****************************************************************************
function SHPCreateSimpleObject(nSHPType, nVertices : LongWord; var padfX, padfY, padfZ : double):SHPObjectHandle;external 'shapelib.dll' name 'SHPCreateSimpleObject';
function SHPOpen(pszShapeFile, pszAccess : PChar) : SHPHandle; external 'shapelib.dll' name 'SHPOpen';
procedure SHPGetInfo(hSHP : SHPHandle; var pnEntities, pnShapeType : LongWord; var padfMinBoud, padfMaxBound : SHPBoundsArr);external 'shapelib.dll' name 'SHPGetInfo';
procedure SHPClose(hSHP : SHPHandle);external 'shapelib.dll' name 'SHPClose';
function SHPReadObject(hSHP : SHPHandle; iShape : LongWord) : SHPObjectHandle;external 'shapelib.dll' name 'SHPReadObject';
function SHPCreate(pszShapeFile : PChar; nShapeType : LongWord):SHPHandle;external 'shapelib.dll' name 'SHPCreate';
function SHPWriteObject(hSHP : SHPHandle; iShape : LongWord; psObject : SHPObjectHandle): LongWord;cdecl;external 'shapelib.dll' name 'SHPWriteObject';
procedure SHPDestroy(psObject : SHPObjectHandle);external 'shapelib.dll' name 'SHPDestroyObject';
procedure SHPComputeExtents(psObject : SHPObjectHandle);external 'shapelib.dll' name 'SHPComputeExtents';
function SHPCreateObject(nSHPType, iShape, nParts : LongWord; var panPartStart, panPartType : LongWord; nVertices : LongWord; var padfX, padfY, padfZ, padfM : SHPBoundsArr): SHPObjectHandle;external 'shapelib.dll' name 'SHPCreateObject';
// *****************************************************************************
function SHPTypeStr(pnShapeType : LongWord): string;
begin
  case pnShapeType of
    SHPT_NULL           : result := 'NULL';
    SHPT_POINT          : result := 'POINT';
    SHPT_ARC            : result := 'ARC';
    SHPT_POLYGON        : result := 'POLYGON';
    SHPT_MULTIPOINT     : result := 'MULTIPOINT';
    SHPT_POINTZ         : result := 'POINTZ';
    SHPT_ARCZ           : result := 'ARCZ';
    SHPT_POLYGONZ       : result := 'POLYGONZ';
    SHPT_MULTIPOINTZ    : result := 'MULTIPOINTZ';
    SHPT_POINTM         : result := 'POINTM';
    SHPT_ARCM           : result := 'ARCM';
    SHPT_POLYGONM       : result := 'POLYGONM';
    SHPT_MULTIPOINTM    : result := 'MULTIPOINTM';
    SHPT_MULTIPATCH     : result := 'MULTIPATCH';
    else
      result := '--unknown--';
  end;
end;
// *****************************************************************************
// *****************************************************************************
function DBFOpen(pszDBFFile, pszAccess : PChar): DBFHandle;external 'shapelib.dll';
function DBFCreate(pszDBFFile : PChar): DBFHandle ;external 'shapelib.dll';
function DBFGetFieldCount(hDBF : DBFHandle) : LongWord ;external 'shapelib.dll';
function DBFGetRecordCount(hDBF : DBFHandle) : LongWord;external 'shapelib.dll';
function DBFGetFieldIndex(hDBF: DBFHandle; pszFieldName : PChar): LongWord;external 'shapelib.dll';
function DBFGetFieldInfo(hDBF : DBFHandle; iField : LongWord; pszFieldName : PChar; var pnWidth, pnDecimals : LongWord): DBFFieldType;external 'shapelib.dll';
function DBFAddField(hDBF : DBFHandle; pszFieldName : PChar; eType : DBFFieldType; nWidth, nDecimals  : LongWord): LongWord;external 'shapelib.dll';
function DBFReadIntegerAttribute(hDBF : DBFHandle;iShape,  iField  : LongWord ): LongWord;external 'shapelib.dll';
function DBFReadDoubleAttribute(hDBF : DBFHandle; iShape,  iField  : LongWord ):double;external 'shapelib.dll';
function DBFReadStringAttribute(hDBF : DBFHandle; iShape,  iField  : LongWord ) : pchar;external 'shapelib.dll';
function DBFIsAttributeNULL(hDBF : DBFHandle; iShape,  iField  : LongWord ): LongWord;external 'shapelib.dll';
function DBFWriteIntegerAttribute(hDBF : DBFHandle;iShape, iField, nFieldValue : LongWord): LongWord;external 'shapelib.dll';
function DBFWriteDoubleAttribute(hDBF : DBFHandle;iShape, iField   : LongWord; dFieldValue : double): LongWord ;external 'shapelib.dll';
function DBFWriteStringAttribute(hDBF : DBFHandle;iShape, iField   : LongWord; pszFieldValue : PChar): LongWord ;external 'shapelib.dll';
function DBFWriteNULLAttribute(hDBF : DBFHandle; iShape,  iField  : LongWord ) : LongWord;external 'shapelib.dll';
procedure DBFClose(hDBF : DBFHandle);external 'shapelib.dll';
function DBFGetNativeFieldType(hDBF : DBFHandle;  iField : LongWord) : Char;external 'shapelib.dll';
// *****************************************************************************

end.
