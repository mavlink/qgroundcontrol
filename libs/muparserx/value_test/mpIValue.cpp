/** \file
    \brief Implementation of the virtual base class used for all parser values.

<pre>
               __________                                 ____  ___
    _____  __ _\______   \_____ _______  ______ __________\   \/  /
   /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     / 
  |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \ 
  |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
        \/                     \/           \/     \/           \_/

  muParserX - A C++ math parser library with array and string support
  Copyright 2010 Ingo Berg

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE
  as published by the Free Software Foundation, either version 3 of 
  the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see http://www.gnu.org/licenses.
</pre>
*/
#include "mpIValue.h"

//--- Standard includes ---------------------------------------------------------------------------
#include <cassert>

//--- muParserX framework -------------------------------------------------------------------------
#include "mpError.h"
#include "mpValue.h"


MUP_NAMESPACE_START

#ifndef _UNICODE

  //-----------------------------------------------------------------------------------------------
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

  //-----------------------------------------------------------------------------------------------
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

  //-----------------------------------------------------------------------------------------------
  IValue::IValue(ECmdCode a_iCode)
    :IToken(a_iCode)
  {
    assert(a_iCode==cmVAL || a_iCode==cmVAR);
  }

  //-----------------------------------------------------------------------------------------------
  IValue::IValue(ECmdCode a_iCode, const string_type &a_sIdent)
    :IToken(a_iCode, a_sIdent)
  {
    assert(a_iCode==cmVAL || a_iCode==cmVAR);
  }

  //-----------------------------------------------------------------------------------------------
  IValue::~IValue()
  {}

  //-----------------------------------------------------------------------------------------------
  ICallback* IValue::AsICallback()
  {
    return NULL;
  }

  //-----------------------------------------------------------------------------------------------
  IValue* IValue::AsIValue()
  {
    return this;
  }

  //-----------------------------------------------------------------------------------------------
  string_type IValue::ToString() const
  {
    stringstream_type ss;
    switch (GetType())
    {
    case 'a':  
               {
                 const array_type &arr(GetArray());
                 ss << _T("["); 
                 for (std::size_t i=0; i<arr.size(); ++i)
                 {
                   ss << arr[i];
                   if (i!=arr.size()-1)
                   {
                     if (arr[i].GetType()!='a')
                       ss << _T("; ");
                     else
                       ss << _T("\n ");
                   }
                 }
                 ss << _T("]"); 
               }
               break;
    case 'c':
              {
                float_type re = GetFloat(),
                           im = GetImag();
								
								// realteil nicht ausgeben, wenn es eine rein imaginäre Zahl ist
								if (im==0 || re!=0 || (im==0 && re==0))
									ss << re;

                if (im!=0)
                {
	                if (im>0 && re!=0)
	                  ss << _T("+");
									
									if (im!=1)
	                  ss << im;

                  ss << _T("i");
                }
              }
              break;
            
    case 'i':  
    case 'f':  ss << GetFloat(); break;
    case 's':  ss << _T("\"") << GetString() << _T("\""); break;
    case 'b':  ss << ((GetBool()==true) ? _T("true"):_T("false")); break;
    case 'v':  ss << _T("void"); break;
    default:   ss << _T("internal error: unknown value type."); break;
    }

    return ss.str();
  }

  //-----------------------------------------------------------------------------------------------
  bool IValue::operator==(const IValue &a_Val) const
  {
    char_type type1 = GetType(),
              type2 = a_Val.GetType(); 

    if (type1==type2 || IsScalar() && a_Val.IsScalar())
    {
      switch(GetType())
      {
      case 'i': 
      case 'f': return GetFloat()   == a_Val.GetFloat();
      case 'c': return GetComplex() == a_Val.GetComplex();
      case 's': return GetString()  == a_Val.GetString();
      case 'b': return GetBool()    == a_Val.GetBool();
      case 'v': return false;
      case 'a': if (GetDim()!=a_Val.GetDim())
                {
                  return false;
                }
                else
                {
                  for (std::size_t i=0; i<GetDim(); ++i)
                  {
                    if (const_cast<IValue*>(this)->operator[](i)!=const_cast<IValue&>(a_Val)[i])
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

  //-----------------------------------------------------------------------------------------------
  bool IValue::operator!=(const IValue &a_Val) const
  {
      char_type type1 = GetType(),
                type2 = a_Val.GetType(); 

    if (type1==type2 || IsScalar() && a_Val.IsScalar())
    {
      switch(GetType())
      {
      case 's': return GetString() != a_Val.GetString();
      case 'i': 
      case 'f': return GetFloat() != a_Val.GetFloat();
      case 'c': return (GetFloat() != a_Val.GetFloat()) || (GetImag() != a_Val.GetImag());
      case 'b': return GetBool() != a_Val.GetBool();
      case 'v': return true;
      case 'a': if (GetDim()!=a_Val.GetDim())
                {
                  return true;
                }
                else
                {
                  for (std::size_t i=0; i<GetDim(); ++i)
                  {
                    if (const_cast<IValue*>(this)->operator[](i)!=const_cast<IValue&>(a_Val)[i])
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

  //-----------------------------------------------------------------------------------------------
  bool IValue::operator<(const IValue &a_Val) const
  {
    char_type type1 = GetType(),
              type2 = a_Val.GetType(); 

    if (type1==type2 || IsScalar() && a_Val.IsScalar())
    {
      switch(GetType())
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
      err.Errc    = ecTYPE_CONFLICT_FUN;
      err.Arg     = (type1!='f' && type1!='i') ? 1 : 2;
      err.Type1   = type2;
      err.Type2   = type1;
      throw ParserError(err);
    }
  }

  //-----------------------------------------------------------------------------------------------
  bool IValue::operator> (const IValue &a_Val) const
  {
    char_type type1 = GetType(),
              type2 = a_Val.GetType(); 

    if (type1==type2 || IsScalar() && a_Val.IsScalar())
    {
      switch(GetType())
      {
      case 's': return GetString() > a_Val.GetString();
      case 'i': 
      case 'f': 
      case 'c': return GetFloat() > a_Val.GetFloat();
      case 'b': return GetBool()  > a_Val.GetBool();
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
      err.Errc    = ecTYPE_CONFLICT_FUN;
      err.Arg     = (type1!='f' && type1!='i') ? 1 : 2;
      err.Type1   = type2;
      err.Type2   = type1;
      throw ParserError(err);
    }
  }

  //-----------------------------------------------------------------------------------------------
  bool IValue::operator>=(const IValue &a_Val) const
  {
    char_type type1 = GetType(),
              type2 = a_Val.GetType(); 

    if (type1==type2 || IsScalar() && a_Val.IsScalar())
    {
      switch(GetType())
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
      err.Errc    = ecTYPE_CONFLICT_FUN;
      err.Arg     = (type1!='f' && type1!='i') ? 1 : 2;
      err.Type1   = type2;
      err.Type2   = type1;
      throw ParserError(err);
    }
  }

  //-----------------------------------------------------------------------------------------------
  bool IValue::operator<=(const IValue &a_Val) const
  {
    char_type type1 = GetType(),
              type2 = a_Val.GetType(); 

    if (type1==type2 || IsScalar() && a_Val.IsScalar())
    {
      switch(GetType())
      {
      case 's': return GetString() <= a_Val.GetString();
      case 'i': 
      case 'f': 
      case 'c': return GetFloat() <= a_Val.GetFloat();
      case 'b': return GetBool()  <= a_Val.GetBool();
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
      err.Errc    = ecTYPE_CONFLICT_FUN;
      err.Arg     = (type1!='f' && type1!='i') ? 1 : 2;
      err.Type1   = type2;
      err.Type2   = type1;
      throw ParserError(err);
    }
  }

  //-----------------------------------------------------------------------------------------------
  IValue& IValue::operator=(const IValue &ref)
  {
    if (this==&ref)
      return *this;

    switch(ref.GetType())
    {
    case 'i': 
    case 'f': 
    case 'c': return *this = cmplx_type(ref.GetFloat(), ref.GetImag());
    case 's': return *this = ref.GetString();
    case 'a': return *this = ref.GetArray();
    case 'b': return *this = ref.GetBool();
    case 'v': 
      throw ParserError(_T("Assignment from void type is not possible"));

    default:  
      throw ParserError(_T("Internal error: unexpected data type identifier in IValue& operator=(const IValue &ref)"));
    }
  }

MUP_NAMESPACE_END
