/** \file
    \brief Definition of basic types used by muParserX

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
#ifndef MUP_VALUE_H
#define MUP_VALUE_H

//--- Standard includes ------------------------------------------------------------
#include <complex>
#include <list>

//--- Parser framework -------------------------------------------------------------
#include "mpIValue.h"
#include "mpTypes.h"


MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  /** \brief Value class of muParserX
  
    This class represents a value to be used with muParserX. It's a Variant like
    class able to store a variety of types.
  */
  class Value : public IValue
  {
  public:

    explicit Value(char_type cType = 'v');
    Value(int_type val);
    Value(bool_type val);
    Value(float_type val);
    Value(string_type val);
    Value(const char_type *val);
    Value(const cmplx_type &v);
    Value(const array_type &val);

    // Array constructor
    Value(int_type m, float_type v);

    // Matrix constructor
    Value(int_type m, int_type n, float_type v);

    Value(const Value &a_Val );
    Value(const IValue &a_Val);
    Value& operator=(const Value &a_Val);

    virtual ~Value();

    virtual IValue& operator[](std::size_t idx);
    virtual IValue& operator=(int_type a_iVal);
    virtual IValue& operator=(float_type a_fVal);
    virtual IValue& operator=(string_type a_sVal);
    virtual IValue& operator=(bool val);
    virtual IValue& operator=(const array_type &a_vVal);
    virtual IValue& operator=(const cmplx_type &val);
    virtual IValue& operator=(const char_type *a_szVal);

    virtual char_type GetType() const;
    virtual int_type GetInteger() const;
    virtual float_type GetFloat() const;
    virtual float_type GetImag() const;
    virtual bool GetBool() const;
    virtual const cmplx_type& GetComplex() const;
    virtual const string_type& GetString() const;
    virtual const array_type& GetArray() const;

    virtual bool IsVolatile() const;
    virtual IToken* Clone() const;
    virtual Value* AsValue();

    virtual string_type AsciiDump() const;
    void BindToCache(ValueCache *pCache);

  private:

    cmplx_type   m_val;    ///< Member variable for storing the value of complex, float, int and boolean values
    string_type *m_psVal;  ///< Variable for storing a string value
    array_type  *m_pvVal;  ///< A Vector for storing array variable content
    char_type    m_cType;  ///< A byte indicating the type os the represented value
    EFlags       m_iFlags; ///< Additional flags
    ValueCache  *m_pCache; ///< Pointer to the Value Cache

    void CheckType(char_type a_cType) const;
    void Assign(const Value &a_Val);
    void Reset();

    virtual void Release();
  }; // class Value

MUP_NAMESPACE_END

#endif


