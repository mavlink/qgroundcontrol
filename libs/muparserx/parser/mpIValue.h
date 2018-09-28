/** \file
    \brief Definition of the virtual base class used for all parser values.

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
#ifndef MUP_IVALUE_H
#define MUP_IVALUE_H

#include "mpIToken.h"
#include "mpFwdDecl.h"

MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  /** \brief Interface to be implemented by all classes representing values. 
  
    IValue is the common base class of both the Value and Variable classes.
  */
  class IValue : public IToken
  {
  friend std::ostream& operator<<(std::ostream &a_Stream, const IValue &a_Val);
  friend std::wostream& operator<<(std::wostream &a_Stream, const IValue &a_Val);

  public:

    explicit IValue(ECmdCode a_iCode);
    IValue(ECmdCode a_iCode, const string_type &a_sIdent);
    
    bool operator==(const IValue &a_Val) const;
    bool operator!=(const IValue &a_Val) const;
    bool operator< (const IValue &a_Val) const;
    bool operator> (const IValue &a_Val) const;
    bool operator<=(const IValue &a_Val) const;
    bool operator>=(const IValue &a_Val) const;

    virtual ICallback* AsICallback();
    virtual IValue* AsIValue();
    virtual Value* AsValue() = 0;

    virtual IValue& operator=(int_type val) = 0;
    virtual IValue& operator=(float_type val) = 0;
    virtual IValue& operator=(string_type val) = 0;
    virtual IValue& operator=(bool_type val) = 0;
    virtual IValue& operator=(const cmplx_type &val) = 0;
    virtual IValue& operator=(const matrix_type &val) = 0;
            IValue& operator=(const IValue &ref);

    virtual IValue& operator+=(const IValue &ref) = 0;
    virtual IValue& operator-=(const IValue &ref) = 0;
    virtual IValue& operator*=(const IValue &ref) = 0;

    virtual IValue& At(int nRow, int nCol = 0) = 0;
    virtual IValue& At(const IValue &nRows, const IValue &nCols) = 0;

    virtual int_type GetInteger() const = 0;
    virtual float_type GetFloat() const = 0;
    virtual float_type GetImag() const = 0;
    virtual bool GetBool() const = 0;
    virtual const cmplx_type& GetComplex() const = 0;
    virtual const string_type&  GetString() const = 0;
    virtual const matrix_type& GetArray() const = 0;
    virtual char_type GetType() const = 0;
    virtual int GetRows() const = 0;
    virtual int GetCols() const = 0;

    virtual string_type ToString() const;
  
    //---------------------------------------------------------------------------
    /** \brief Returns the dimension of the value represented by a value object.
        
        The value represents the dimension of the object. Possible value are:
        <ul>
          <li>0 - scalar</li>
          <li>1 - vector</li>
          <li>2 - matrix</li>
        </ul>
    */
    inline int GetDim() const
    {
      return (IsMatrix()) ? GetArray().GetDim() : 0;
    }

    //---------------------------------------------------------------------------
    virtual bool  IsVariable() const = 0;

    //---------------------------------------------------------------------------
    /** \brief Returns true if the type is either floating point or interger. 
        \throw nothrow
    */
    inline bool IsNonComplexScalar() const
    {
      char_type t = GetType();
      return t=='f' || t=='i';
    }

    //---------------------------------------------------------------------------
    /** \brief Returns true if the type is not a vector and not a string.
        \throw nothrow
    */
    inline bool IsScalar() const
    {
      char_type t = GetType();
      return t=='f' || t=='i' || t=='c';
    }

    //---------------------------------------------------------------------------
    /** \brief Returns true if this value is a noncomplex integer. 
        \throw nothrow
    */
    inline bool IsInteger() const
    {
      // checking the type is is insufficient. The integer could be disguised
      // as a float or a complex value
      return IsScalar() && GetImag()==0 && GetFloat()==(int_type)GetFloat();
    }

    //---------------------------------------------------------------------------
    /** \brief Returns true if this value is an array. 
        \throw nothrow
    */  
    inline bool IsMatrix() const 
    {
      return GetType() == 'm';  
    }

    //---------------------------------------------------------------------------
    /** \brief Returns true if this value is a complex value. 
        \throw nothrow
    */
    inline bool IsComplex() const
    {
      return GetType() == 'c' && GetImag()!=0;
    }

    //---------------------------------------------------------------------------
    /** \brief Returns true if this value is a string value. 
        \throw nothrow
    */
    inline bool IsString() const 
    {
      return GetType() == 's';  
    }

  protected:
    virtual ~IValue();
  }; // class IValue

  //---------------------------------------------------------------------------------------------
  Value operator*(const IValue& lhs, const IValue& rhs);
}  // namespace mu

#endif


