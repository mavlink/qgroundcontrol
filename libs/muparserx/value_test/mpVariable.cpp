/** \file
    \brief Implementation of the muParserX variable class.

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
#include "mpVariable.h"
#include "mpError.h"

#include "mpValue.h"


MUP_NAMESPACE_START

  //---------------------------------------------------------------------------
  /** \brief Create a variable and bind a value to it.
      \param pVal Pointer of the value to bind to this variable.

    It is possible to create an empty variable object by setting pVal to null.
    Such variable objects must be bound later in order to be of any use.
  */
  Variable::Variable(IValue *pVal)
    :IValue(cmVAR)
    ,m_pVal(pVal)
  {
    AddFlags(IToken::flVOLATILE);
  }

  //---------------------------------------------------------------------------
  Variable::Variable(const Variable &obj)
    :IValue(cmVAR)
  {
    Assign(obj);
    AddFlags(IToken::flVOLATILE);
  }

  //---------------------------------------------------------------------------
  Variable& Variable::operator=(const Variable &obj)
  {
    Assign(obj);
    return *this;
  }

  //---------------------------------------------------------------------------
  /** \brief Assign a value to the variable. 
      \param ref Reference to the value to be assigned
  */
  IValue& Variable::operator=(const Value &ref)
  {
    assert(m_pVal);
    *m_pVal = ref;
    return *this;
  }

  //---------------------------------------------------------------------------
  IValue& Variable::operator=(int_type val)
  {
    assert(m_pVal);
    return m_pVal->operator=(val);
  }

  //---------------------------------------------------------------------------
  IValue& Variable::operator=(float_type val)
  {
    assert(m_pVal);
    return m_pVal->operator=(val);
  }

  //---------------------------------------------------------------------------
  IValue& Variable::operator=(string_type val)
  {
    assert(m_pVal);
    return m_pVal->operator=(val);
  }

  //---------------------------------------------------------------------------
  IValue& Variable::operator=(bool_type val)
  {
    assert(m_pVal);
    return m_pVal->operator=(val);
  }

  //---------------------------------------------------------------------------
  IValue& Variable::operator=(const array_type &val)
  {
    assert(m_pVal);
    return m_pVal->operator=(val);
  }

  //---------------------------------------------------------------------------
  IValue& Variable::operator=(const cmplx_type &val)
  {
    assert(m_pVal);
    return m_pVal->operator=(val);
  }

  //---------------------------------------------------------------------------
  IValue& Variable::operator[](std::size_t i)
  {
    return m_pVal->operator[](i);
  }

  //---------------------------------------------------------------------------
  Variable::~Variable()
  {}

  //---------------------------------------------------------------------------
  void Variable::Assign(const Variable &ref)
  {
    if (this==&ref)
      return;

    m_pVal = ref.m_pVal;
  }

  //---------------------------------------------------------------------------
  /** \brief Returns a character representing the type of the variable. 
      \throw nothrow  
  */
  char_type Variable::GetType() const
  {
    return (m_pVal) ? m_pVal->GetType() : 'v';
  }

  //---------------------------------------------------------------------------
  /** \brief Returns the Value pointer bound to this variable. 
      \throw nothrow
  */
  IValue* Variable::GetPtr() const
  {
    return m_pVal;
  }

  //---------------------------------------------------------------------------
  int_type Variable::GetInteger() const
  {
    return m_pVal->GetInteger();
  }

  //---------------------------------------------------------------------------
  float_type Variable::GetFloat() const
  {
    return m_pVal->GetFloat();
  }

  //---------------------------------------------------------------------------
  float_type Variable::GetImag() const
  {
    return m_pVal->GetImag();
  }

  //---------------------------------------------------------------------------
  const cmplx_type& Variable::GetComplex() const
  {
    return m_pVal->GetComplex();
  }

  //---------------------------------------------------------------------------
  const string_type& Variable::GetString() const
  {
    return m_pVal->GetString();
  }

  //---------------------------------------------------------------------------
  bool Variable::GetBool() const
  {
    return m_pVal->GetBool();
  }

  //---------------------------------------------------------------------------
  const array_type& Variable::GetArray() const
  {
    return m_pVal->GetArray();
  }

  //---------------------------------------------------------------------------
  void Variable::SetFloat(float_type a_fVal)
  {
    assert(m_pVal);
    *m_pVal = a_fVal;
  }

  //---------------------------------------------------------------------------
  void Variable::SetString(const string_type &a_sVal)
  {
    assert(m_pVal);
    *m_pVal = a_sVal;
  }

  //---------------------------------------------------------------------------
  void Variable::SetBool(bool a_bVal)
  {
    assert(m_pVal);
    *m_pVal = a_bVal;
  }

  //---------------------------------------------------------------------------
  void Variable::Bind(IValue *pValue)
  {
    m_pVal = pValue;
  }

  //---------------------------------------------------------------------------
  bool Variable::IsVolatile() const
  {
    return true;
  }

  //---------------------------------------------------------------------------
  IToken* Variable::Clone() const
  {
    return new Variable(*this);
  }

  //---------------------------------------------------------------------------
  Value* Variable::AsValue()
  {
    return NULL;
  }

  //---------------------------------------------------------------------------
  string_type Variable::AsciiDump() const
  {
    stringstream_type ss;

    ss << g_sCmdCode[ GetCode() ];
    ss << _T(" [addr=0x") << std::hex << this << std::dec;
    ss << _T("; id=\"") << GetIdent() << _T("\"");
    ss << _T("; type=\"") << GetType() << _T("\"");
    ss << _T("; val=");

    switch(GetType())
    {
    case 'i': ss << (int_type)GetFloat(); break;
    case 'f': ss << GetFloat(); break;
    case 'a': ss << _T("(array)"); break;
    case 's': ss << _T("\"") << GetString() << _T("\""); break;
    }

    ss << ((IsFlagSet(IToken::flVOLATILE)) ? _T("; ") : _T("; not ")) << _T("volatile");
    ss << _T("]");

    return ss.str();
  }
MUP_NAMESPACE_END
