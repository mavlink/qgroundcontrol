/** \file
    \brief Implementation of the virtual base class used for all parser values.

    <pre>
                __________                                 ____  ___
     _____  __ _\______   \_____ _______  ______ __________\   \/  /
    /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     /
    |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \
    |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
          \/                     \/           \/     \/           \_/
    Copyright (C) 2016 Ingo Berg
    All rights reserved.

    muParserX - A C++ math parser library with array and string support
    Copyright (c) 2016, Ingo Berg
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
#include "mpIValue.h"

//--- Standard includes ------------------------------------------------------
#include <cassert>
#include <iostream>
#include <iomanip>
#include <limits>

//--- muParserX framework -----------------------------------------------------
#include "mpValue.h"
#include "mpError.h"
#include "mpValue.h"


MUP_NAMESPACE_START

#ifndef _UNICODE

//---------------------------------------------------------------------------
/** \brief Overloaded streaming operator for outputting the value type 
           into an std::ostream. 
           \param a_Stream The stream object
           \param a_Val The value object to be streamed

           This function is only present if _UNICODE is not defined.
           */
           std::ostream& operator<<(std::ostream &a_Stream, const IValue &a_Val)
{
    return a_Stream << a_Val.ToString();
}  

#else

//---------------------------------------------------------------------------
/** \brief Overloaded streaming operator for outputting the value type
           into an std::ostream.
           \param a_Stream The stream object
           \param a_Val The value object to be streamed

           This function is only present if _UNICODE is defined.
           */
           std::wostream& operator<<(std::wostream &a_Stream, const IValue &a_Val)
{
    return a_Stream << a_Val.ToString();
}
#endif

//---------------------------------------------------------------------------------------------
Value operator*(const IValue& lhs, const IValue& rhs)
{
    return Value(lhs) *= rhs;
}

//---------------------------------------------------------------------------
IValue::IValue(ECmdCode a_iCode)
    :IToken(a_iCode)
{
    assert(a_iCode == cmVAL);
}

//---------------------------------------------------------------------------
IValue::IValue(ECmdCode a_iCode, const string_type &a_sIdent)
    :IToken(a_iCode, a_sIdent)
{
    assert(a_iCode == cmVAL);
}

//---------------------------------------------------------------------------
IValue::~IValue()
{}

//---------------------------------------------------------------------------
ICallback* IValue::AsICallback()
{
    return nullptr;
}

//---------------------------------------------------------------------------
IValue* IValue::AsIValue()
{
    return this;
}

//---------------------------------------------------------------------------
string_type IValue::ToString() const
{
    stringstream_type ss;
    switch (GetType())
    {
    case 'm':
    {
        const matrix_type &arr(GetArray());

        if (arr.GetRows() > 1)
            ss << _T("{");

        for (int i = 0; i < arr.GetRows(); ++i)
        {
            if (arr.GetCols()>1)
                ss << _T("{");

            for (int j = 0; j < arr.GetCols(); ++j)
            {
                ss << arr.At(i, j).ToString();
                if (j != arr.GetCols() - 1)
                    ss << _T(", ");
            }

            if (arr.GetCols()>1)
                ss << _T("}");

            if (i != arr.GetRows() - 1)
                ss << _T("; ");
        }

        if (arr.GetRows() > 1)
            ss << _T("} ");
    }
    break;

    case 'c':
    {
        float_type re = GetFloat(),
            im = GetImag();

        // realteil nicht ausgeben, wenn es eine rein imaginäre Zahl ist
        if (im == 0 || re != 0 || (im == 0 && re == 0))
            ss << re;

        if (im != 0)
        {
            if (im > 0 && re != 0)
                ss << _T("+");

            if (im != 1)
                ss << im;

            ss << _T("i");
        }
    }
    break;

    case 'i':
    case 'f':  ss << std::setprecision(std::numeric_limits<float_type>::digits10) << GetFloat(); break;
    case 's':  ss << _T("\"") << GetString() << _T("\""); break;
    case 'b':  ss << ((GetBool() == true) ? _T("true") : _T("false")); break;
    case 'v':  ss << _T("void"); break;
    default:   ss << _T("internal error: unknown value type."); break;
    }

    return ss.str();
}

//---------------------------------------------------------------------------
bool IValue::operator==(const IValue &a_Val) const
{
    char_type type1 = GetType(),
        type2 = a_Val.GetType();

    if (type1 == type2 || (IsScalar() && a_Val.IsScalar()))
    {
        switch (GetType())
        {
        case 'i':
        case 'f': return GetFloat() == a_Val.GetFloat();
        case 'c': return GetComplex() == a_Val.GetComplex();
        case 's': return GetString() == a_Val.GetString();
        case 'b': return GetBool() == a_Val.GetBool();
        case 'v': return false;
        case 'm': if (GetRows() != a_Val.GetRows() || GetCols() != a_Val.GetCols())
        {
            return false;
        }
                  else
                  {
                      for (int i = 0; i < GetRows(); ++i)
                      {
                          if (const_cast<IValue*>(this)->At(i) != const_cast<IValue&>(a_Val).At(i))
                              return false;
                      }

                      return true;
                  }
        default:
            ErrorContext err;
            err.Errc = ecINTERNAL_ERROR;
            err.Pos = -1;
            err.Type1 = GetType();
            err.Type2 = a_Val.GetType();
            throw ParserError(err);
        } // switch this type
    }
    else
    {
        return false;
    }
}

//---------------------------------------------------------------------------
bool IValue::operator!=(const IValue &a_Val) const
{
    char_type type1 = GetType(),
        type2 = a_Val.GetType();

    if (type1 == type2 || (IsScalar() && a_Val.IsScalar()))
    {
        switch (GetType())
        {
        case 's': return GetString() != a_Val.GetString();
        case 'i':
        case 'f': return GetFloat() != a_Val.GetFloat();
        case 'c': return (GetFloat() != a_Val.GetFloat()) || (GetImag() != a_Val.GetImag());
        case 'b': return GetBool() != a_Val.GetBool();
        case 'v': return true;
        case 'm': if (GetRows() != a_Val.GetRows() || GetCols() != a_Val.GetCols())
        {
            return true;
        }
                  else
                  {
                      for (int i = 0; i < GetRows(); ++i)
                      {
                          if (const_cast<IValue*>(this)->At(i) != const_cast<IValue&>(a_Val).At(i))
                              return true;
                      }

                      return false;
                  }
        default:
            ErrorContext err;
            err.Errc = ecINTERNAL_ERROR;
            err.Pos = -1;
            err.Type2 = GetType();
            err.Type1 = a_Val.GetType();
            throw ParserError(err);
        } // switch this type
    }
    else
    {
        return true;
    }
}

//---------------------------------------------------------------------------
bool IValue::operator<(const IValue &a_Val) const
{
    char_type type1 = GetType();
    char_type type2 = a_Val.GetType();

    if (type1 == type2 || (IsScalar() && a_Val.IsScalar()))
    {
        switch (GetType())
        {
        case 's': return GetString() < a_Val.GetString();
        case 'i':
        case 'f':
        case 'c': return GetFloat() < a_Val.GetFloat();
        case 'b': return GetBool() < a_Val.GetBool();

        default:
            ErrorContext err;
            err.Errc = ecINTERNAL_ERROR;
            err.Pos = -1;
            err.Type1 = GetType();
            err.Type2 = a_Val.GetType();
            throw ParserError(err);
        } // switch this type
    }
    else
    {
        ErrorContext err;
        err.Errc = ecTYPE_CONFLICT_FUN;
        err.Arg = (type1 != 'f' && type1 != 'i') ? 1 : 2;
        err.Type1 = type2;
        err.Type2 = type1;
        throw ParserError(err);
    }
}

//---------------------------------------------------------------------------
bool IValue::operator> (const IValue &a_Val) const
{
    char_type type1 = GetType(),
        type2 = a_Val.GetType();

    if (type1 == type2 || (IsScalar() && a_Val.IsScalar()))
    {
        switch (GetType())
        {
        case 's': return GetString() > a_Val.GetString();
        case 'i':
        case 'f':
        case 'c': return GetFloat() > a_Val.GetFloat();
        case 'b': return GetBool() > a_Val.GetBool();
        default:
            ErrorContext err;
            err.Errc = ecINTERNAL_ERROR;
            err.Pos = -1;
            err.Type1 = GetType();
            err.Type2 = a_Val.GetType();
            throw ParserError(err);

        } // switch this type
    }
    else
    {
        ErrorContext err;
        err.Errc = ecTYPE_CONFLICT_FUN;
        err.Arg = (type1 != 'f' && type1 != 'i') ? 1 : 2;
        err.Type1 = type2;
        err.Type2 = type1;
        throw ParserError(err);
    }
}

//---------------------------------------------------------------------------
bool IValue::operator>=(const IValue &a_Val) const
{
    char_type type1 = GetType(),
        type2 = a_Val.GetType();

    if (type1 == type2 || (IsScalar() && a_Val.IsScalar()))
    {
        switch (GetType())
        {
        case 's': return GetString() >= a_Val.GetString();
        case 'i':
        case 'f':
        case 'c': return GetFloat() >= a_Val.GetFloat();
        case 'b': return GetBool() >= a_Val.GetBool();
        default:
            ErrorContext err;
            err.Errc = ecINTERNAL_ERROR;
            err.Pos = -1;
            err.Type1 = GetType();
            err.Type2 = a_Val.GetType();
            throw ParserError(err);

        } // switch this type
    }
    else
    {
        ErrorContext err;
        err.Errc = ecTYPE_CONFLICT_FUN;
        err.Arg = (type1 != 'f' && type1 != 'i') ? 1 : 2;
        err.Type1 = type2;
        err.Type2 = type1;
        throw ParserError(err);
    }
}

//---------------------------------------------------------------------------
bool IValue::operator<=(const IValue &a_Val) const
{
    char_type type1 = GetType(),
        type2 = a_Val.GetType();

    if (type1 == type2 || (IsScalar() && a_Val.IsScalar()))
    {
        switch (GetType())
        {
        case 's': return GetString() <= a_Val.GetString();
        case 'i':
        case 'f':
        case 'c': return GetFloat() <= a_Val.GetFloat();
        case 'b': return GetBool() <= a_Val.GetBool();
        default:
            ErrorContext err;
            err.Errc = ecINTERNAL_ERROR;
            err.Pos = -1;
            err.Type1 = GetType();
            err.Type2 = a_Val.GetType();
            throw ParserError(err);

        } // switch this type
    }
    else
    {
        ErrorContext err;
        err.Errc = ecTYPE_CONFLICT_FUN;
        err.Arg = (type1 != 'f' && type1 != 'i') ? 1 : 2;
        err.Type1 = type2;
        err.Type2 = type1;
        throw ParserError(err);
    }
}

//---------------------------------------------------------------------------
IValue& IValue::operator=(const IValue &ref)
{
    if (this == &ref)
        return *this;

    switch (ref.GetType())
    {
    case 'i':
    case 'f':
    case 'c': return *this = cmplx_type(ref.GetFloat(), ref.GetImag());
    case 's': return *this = ref.GetString();
    case 'm': return *this = ref.GetArray();
    case 'b': return *this = ref.GetBool();
    case 'v':
        throw ParserError(_T("Assignment from void type is not possible"));

    default:
        throw ParserError(_T("Internal error: unexpected data type identifier in IValue& operator=(const IValue &ref)"));
    }
}


MUP_NAMESPACE_END
