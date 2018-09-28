/** \file
    \brief Implementation of classes that interpret values in a string.

    <pre>
                 __________                                 ____  ___
      _____  __ _\______   \_____ _______  ______ __________\   \/  /
     /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     /
    |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \
    |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
          \/                     \/           \/     \/           \_/
    Copyright (C) 2016, Ingo Berg
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
    </pre>
    */
#include "mpValReader.h"
#include "mpError.h"


MUP_NAMESPACE_START

//------------------------------------------------------------------------------
//
//  Reader for floating point values
//
//------------------------------------------------------------------------------

DblValReader::DblValReader()
:IValueReader()
{}

//------------------------------------------------------------------------------
DblValReader::~DblValReader()
{}

//------------------------------------------------------------------------------
bool DblValReader::IsValue(const char_type *a_szExpr, int &a_iPos, Value &a_Val)
{
    stringstream_type stream(a_szExpr + a_iPos);
    float_type fVal(0);
    std::streamoff iEnd(0);

    stream >> fVal;

    if (stream.fail())
        return false;

    if (stream.eof())
    {
        // This part sucks but tellg will return -1 if eof is set,
        // so i need a special treatment for the case that the number
        // just read here is the last part of the string
        for (; a_szExpr[a_iPos] != 0; ++a_iPos);
    }
    else
    {
        iEnd = stream.tellg();   // Position after reading
        assert(iEnd > 0);
        a_iPos += (int)iEnd;
    }

    // Finally i have to check if the next sign is the "i" for a imaginary unit
    // if so this is an imaginary value
    if (a_szExpr[a_iPos] == 'i')
    {
        a_Val = cmplx_type(0.0, fVal);
        a_iPos++;
    }
    else
    {
        a_Val = cmplx_type(fVal, 0.0);
    }

    return true;
}

//------------------------------------------------------------------------------
IValueReader* DblValReader::Clone(TokenReader *pTokenReader) const
{
    IValueReader *pReader = new DblValReader(*this);
    pReader->SetParent(pTokenReader);

    return pReader;
}

//------------------------------------------------------------------------------
//
//  Reader for boolean values
//
//------------------------------------------------------------------------------

BoolValReader::BoolValReader()
    :IValueReader()
{}

//------------------------------------------------------------------------------
BoolValReader::~BoolValReader()
{}

//------------------------------------------------------------------------------
bool BoolValReader::IsValue(const char_type *a_szExpr, int &a_iPos, Value &a_Val)
{
    string_type sExpr(a_szExpr + a_iPos);

    if (sExpr.find(_T("true")) == 0)
    {
        a_Val = true;
        a_iPos += 4;
        return true;
    }
    else if (sExpr.find(_T("false")) == 0)
    {
        a_Val = false;
        a_iPos += 5;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
IValueReader* BoolValReader::Clone(TokenReader *pTokenReader) const
{
    IValueReader *pReader = new BoolValReader(*this);
    pReader->SetParent(pTokenReader);

    return pReader;
}

//------------------------------------------------------------------------------
//
//  Reader for hex values
//
//------------------------------------------------------------------------------

HexValReader::HexValReader()
    :IValueReader()
{}

//------------------------------------------------------------------------------
/** \brief Try to read a hex value from a given position in the expression.
    \param a_szExpr The Expression
    \param [in/out] a_iPos The current position in the expression
    \param [out] a_val The value that was read

    Hex values must start with a "0x" characters. The position a_iPos is advanded in case
    a hex value was found.
    */
bool HexValReader::IsValue(const char_type *a_szExpr, int &a_iPos, Value &a_val)
{
    std::size_t len = std::char_traits<char_type>::length(a_szExpr);
    if (a_iPos >= (int)len || a_szExpr[a_iPos + 1] != 'x' || a_szExpr[a_iPos + 1] == 0 || a_szExpr[a_iPos] != '0')
        return 0;

    unsigned iVal(0);

    stringstream_type::pos_type nPos(0);
    stringstream_type ss(a_szExpr + a_iPos + 2);
    ss >> std::hex >> iVal;

    if (ss.fail())
        return false;

    if (ss.eof())
    {
        // This part sucks but tellg will return -1 if eof is set,
        // so i need a special treatment for those cases.
        for (; a_szExpr[a_iPos] != 0; ++a_iPos);
    }
    else
    {
        nPos = ss.tellg();
        assert(nPos > 0);
        a_iPos += (int)(2 + nPos);
    }

    a_val = (float_type)iVal;
    return true;
}

//------------------------------------------------------------------------------
IValueReader* HexValReader::Clone(TokenReader *pTokenReader) const
{
    IValueReader *pReader = new HexValReader(*this);
    pReader->SetParent(pTokenReader);
    return pReader;
}

//------------------------------------------------------------------------------
//
//  Reader for binary values
//
//------------------------------------------------------------------------------

BinValReader::BinValReader()
    :IValueReader()
{}

//------------------------------------------------------------------------------
BinValReader::~BinValReader()
{}

//------------------------------------------------------------------------------
bool BinValReader::IsValue(const char_type *a_szExpr, int &a_iPos, Value &a_Val)
{
    const char_type *szExpr = a_szExpr + a_iPos;

    if (szExpr[0] != '0' || (szExpr[1] != 'b' && szExpr[1] != 'B'))
        return false;

    // <ibg 2014-05-26/> Number of bits hardcoded to 32, i can't 
    //                   store 64 bit integers in double values without 
    //                   loss. There is no point in accepting them.
    unsigned iVal = 0, iBits = 32 /*sizeof(iVal)*8*/, i;
    for (i = 0; (szExpr[i + 2] == '0' || szExpr[i + 2] == '1') && i <= iBits; ++i)
    {
        iVal |= (unsigned)(szExpr[i + 2] == '1') << ((iBits - 1) - i);
    }

    if (i == 0)
        return false;

    if (i > iBits)
    {
        throw ParserError(_T("Binary to integer conversion error (overflow)."));
    }

    a_Val = (float_type)((int)(iVal >> (iBits - i)));
    a_iPos += i + 2;

    return true;
}

//------------------------------------------------------------------------------
IValueReader* BinValReader::Clone(TokenReader *pTokenReader) const
{
    IValueReader *pReader = new BinValReader(*this);
    pReader->SetParent(pTokenReader);

    return pReader;
}

//------------------------------------------------------------------------------
//
//  Reader for string values
//
//------------------------------------------------------------------------------

StrValReader::StrValReader()
    :IValueReader()
{}

//------------------------------------------------------------------------------
StrValReader::~StrValReader()
{}

//------------------------------------------------------------------------------
string_type StrValReader::Unescape(const char_type *szExpr, int &nPos)
{
    string_type out;
    bool bEscape = false;

    for (char_type c = szExpr[nPos]; c != 0; c = szExpr[++nPos])
    {
        switch (c)
        {
        case '\\':
            if (!bEscape)
            {
                bEscape = true;
                break;
            }
            // fall throught!

        case '"':
            if (!bEscape)
            {
                ++nPos;
                return out;
            }
            // fall through!

        default:
            if (bEscape)
            {
                switch (c)
                {
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;
                case '"':  out += '\"'; break;
                case '\\': out += '\\'; break;
                default:
                    throw ParserError(ErrorContext(ecUNKNOWN_ESCAPE_SEQUENCE, nPos));
                }

                bEscape = false;
            }
            else
            {
                out += c;
            }
        }
    }

    throw ParserError(ErrorContext(ecUNTERMINATED_STRING, nPos));
}

//------------------------------------------------------------------------------
bool StrValReader::IsValue(const char_type *a_pszExpr, int &a_iPos, Value &a_Val)
{
    const char_type *szExpr = a_pszExpr + a_iPos;
    if (szExpr[0] != '"')
        return false;

    a_Val = Unescape(a_pszExpr, ++a_iPos);
    return true;
}

//------------------------------------------------------------------------------
IValueReader* StrValReader::Clone(TokenReader *pTokenReader) const
{
    IValueReader *pReader = new StrValReader(*this);
    pReader->SetParent(pTokenReader);

    return pReader;
}
} // namespace mu
