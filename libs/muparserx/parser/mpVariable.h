/** \file
    \brief Definition of the muParserX variable class.

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

    virtual IValue& At(int nRow, int nCol);
    virtual IValue& At(const IValue &nRows, const IValue &nCols);

    virtual IValue& operator=(const Value &val);
    virtual IValue& operator=(const matrix_type &val);
    virtual IValue& operator=(const cmplx_type &val);
    virtual IValue& operator=(int_type val);
    virtual IValue& operator=(float_type val);
    virtual IValue& operator=(string_type val);
    virtual IValue& operator=(bool_type val);
    virtual IValue& operator+=(const IValue &ref);
    virtual IValue& operator-=(const IValue &ref);
    virtual IValue& operator*=(const IValue &val);

    virtual ~Variable();

    virtual char_type GetType() const;
    
    virtual int_type GetInteger() const;
    virtual float_type GetFloat() const;
    virtual float_type GetImag() const;
    virtual bool GetBool() const;
    virtual const cmplx_type& GetComplex() const;
    virtual const string_type& GetString() const;
    virtual const matrix_type& GetArray() const;
    virtual int GetRows() const;
    virtual int GetCols() const;

    virtual bool IsVariable() const;
    virtual IToken* Clone() const;
    virtual Value* AsValue();

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


