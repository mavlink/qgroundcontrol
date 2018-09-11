/** \file
    \brief Definition of the muParserX variable class.

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

#ifndef MP_VARIABLE_H
#define MP_VARIABLE_H

#include "mpIValue.h"
#include "mpTypes.h"


MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  /** \brief The variable class represents a parser variable. 
  
    This class stores a pointer to a value object and refers all
    operations to this value object.
  */
  class Variable : public IValue
  {
  public:

    Variable(IValue *pVal);

    Variable(const Variable &a_Var);
    Variable& operator=(const Variable &a_Var);

    virtual IValue& operator[](std::size_t idx);
    virtual IValue& operator=(const Value &val);
    virtual IValue& operator=(const array_type &val);
    virtual IValue& operator=(const cmplx_type &val);
    virtual IValue& operator=(int_type val);
    virtual IValue& operator=(float_type val);
    virtual IValue& operator=(string_type val);
    virtual IValue& operator=(bool_type val);

    virtual ~Variable();

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

    //void Set(Value &val);
    void SetFloat(float_type a_fVal);
    void SetString(const string_type &a_sVal);
    void SetBool(bool a_bVal);

    void Bind(IValue *pValue);

    IValue* GetPtr() const;
    string_type AsciiDump() const;

  private:

    IValue *m_pVal;    ///< Pointer to the value object bound to this variable

    void Assign(const Variable &a_Var);
    void CheckType(char_type a_cType) const;
  }; // class Variable

MUP_NAMESPACE_END

#endif


