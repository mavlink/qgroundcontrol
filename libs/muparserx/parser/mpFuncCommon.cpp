/*
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
*/
#include "mpFuncCommon.h"

#include <cassert>
#include <string>
#include <iostream>

#include "mpValue.h"
#include "mpParserBase.h"


MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  //
  // FunParserID
  //
  //------------------------------------------------------------------------------

  FunParserID::FunParserID()
    :ICallback(cmFUNC, _T("parserid"), 0)
  {}

  //------------------------------------------------------------------------------
  void FunParserID::Eval(ptr_val_type &ret, const ptr_val_type * /*a_pArg*/, int /*a_iArgc*/)
  {
    string_type sVer = _T("muParserX V") + GetParent()->GetVersion();
    *ret = sVer;
  }

  //------------------------------------------------------------------------------
  const char_type* FunParserID::GetDesc() const
  {
    return _T("parserid() - muParserX version information");
  }

  //------------------------------------------------------------------------------
  IToken* FunParserID::Clone() const
  {
    return new FunParserID(*this);
  }

  //------------------------------------------------------------------------------
  //
  // Max Function
  //
  //------------------------------------------------------------------------------

  FunMax::FunMax() : ICallback(cmFUNC, _T("max"), -1)
  {}

  //------------------------------------------------------------------------------
  void FunMax::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int a_iArgc)
  {
    if (a_iArgc < 1)
        throw ParserError(ErrorContext(ecTOO_FEW_PARAMS, GetExprPos(), GetIdent()));

    float_type max(-1e30), val(0);
    for (int i=0; i<a_iArgc; ++i)
    {
      switch(a_pArg[i]->GetType())
      {
      case 'f': val = a_pArg[i]->GetFloat();   break;
      case 'i': val = a_pArg[i]->GetFloat(); break;
      case 'n': break; // ignore not in list entries (missing parameter)
      case 'c':
      default:
        {
          ErrorContext err;
          err.Errc = ecTYPE_CONFLICT_FUN;
          err.Arg = i+1;
          err.Type1 = a_pArg[i]->GetType();
          err.Type2 = 'f';
          throw ParserError(err);
        }
      }
      max = std::max(max, val);    
    }
    
    *ret = max;
  }

  //------------------------------------------------------------------------------
  const char_type* FunMax::GetDesc() const
  {
    return _T("max(x,y,...,z) - Returns the maximum value from all of its function arguments.");
  }

  //------------------------------------------------------------------------------
  IToken* FunMax::Clone() const
  {
    return new FunMax(*this);
  }

  //------------------------------------------------------------------------------
  //
  // Min Function
  //
  //------------------------------------------------------------------------------

  FunMin::FunMin() : ICallback(cmFUNC, _T("min"), -1)
  {}

  //------------------------------------------------------------------------------
  /** \brief Returns the minimum value of all values. 
      \param a_pArg Pointer to an array of Values
      \param a_iArgc Number of values stored in a_pArg
  */
  void FunMin::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int a_iArgc)
  {
    if (a_iArgc < 1)
        throw ParserError(ErrorContext(ecTOO_FEW_PARAMS, GetExprPos(), GetIdent()));

    float_type min(1e30), val(min);

    for (int i=0; i<a_iArgc; ++i)
    {
      switch(a_pArg[i]->GetType())
      {
      case 'f': 
      case 'i': val = a_pArg[i]->GetFloat();   break;
      default:
        {
          ErrorContext err;
          err.Errc = ecTYPE_CONFLICT_FUN;
          err.Arg = i+1;
          err.Type1 = a_pArg[i]->GetType();
          err.Type2 = 'f';
          throw ParserError(err);
        }
      }
      min = std::min(min, val);    
    }
    
    *ret = min;
  }

  //------------------------------------------------------------------------------
  const char_type* FunMin::GetDesc() const
  {
    return _T("min(x,y,...,z) - Returns the minimum value from all of its function arguments.");
  }

  //------------------------------------------------------------------------------
  IToken* FunMin::Clone() const
  {
    return new FunMin(*this);
  }

  //------------------------------------------------------------------------------
  //
  // class FunSum
  //
  //------------------------------------------------------------------------------

  FunSum::FunSum() 
    :ICallback(cmFUNC, _T("sum"), -1)
  {}

  //------------------------------------------------------------------------------
  /** \brief Returns the minimum value of all values. 
      \param a_pArg Pointer to an array of Values
      \param a_iArgc Number of values stored in a_pArg
  */
  void FunSum::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int a_iArgc)
  {
    if (a_iArgc < 1)
        throw ParserError(ErrorContext(ecTOO_FEW_PARAMS, GetExprPos(), GetIdent()));

    float_type sum(0);

    for (int i=0; i<a_iArgc; ++i)
    {
      switch(a_pArg[i]->GetType())
      {
      case 'f': 
      case 'i': sum += a_pArg[i]->GetFloat();   break;
      default:
        {
          ErrorContext err;
          err.Errc = ecTYPE_CONFLICT_FUN;
          err.Arg = i+1;
          err.Type1 = a_pArg[i]->GetType();
          err.Type2 = 'f';
          throw ParserError(err);
        }
      }
    }
    
    *ret = sum;
  }

  //------------------------------------------------------------------------------
  const char_type* FunSum::GetDesc() const
  {
    return _T("sum(x,y,...,z) - Returns the sum of all arguments.");
  }

  //------------------------------------------------------------------------------
  IToken* FunSum::Clone() const
  {
    return new FunSum(*this);
  }

  //------------------------------------------------------------------------------
  //
  // SizeOf
  //
  //------------------------------------------------------------------------------

  FunSizeOf::FunSizeOf()
    :ICallback(cmFUNC, _T("sizeof"), 1)
  {}

  //------------------------------------------------------------------------------
  FunSizeOf::~FunSizeOf()
  {}

  //------------------------------------------------------------------------------
  /** \brief Returns the number of elements stored in the first parameter. */
  void FunSizeOf::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int a_iArgc)
  {
    assert(a_iArgc==1);
    *ret = (float_type)a_pArg[0]->GetArray().GetRows();
  }

  //------------------------------------------------------------------------------
  const char_type* FunSizeOf::GetDesc() const
  {
    return _T("sizeof(a) - Returns the number of elements in a.");
  }

  //------------------------------------------------------------------------------
  IToken* FunSizeOf::Clone() const
  {
    return new FunSizeOf(*this);
  }

MUP_NAMESPACE_END
