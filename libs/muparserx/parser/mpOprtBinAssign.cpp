/** \file
    \brief This file contains the implementation of binary assignment 
           operators used in muParser.

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
#include "mpOprtBinAssign.h"


MUP_NAMESPACE_START

  //---------------------------------------------------------------------
  //
  //  class OprtAssign
  //
  //---------------------------------------------------------------------

  OprtAssign::OprtAssign() 
    :IOprtBin(_T("="), (int)prASSIGN, oaLEFT)
  {}

  //---------------------------------------------------------------------
  const char_type* OprtAssign::GetDesc() const 
  { 
    return _T("'=' assignement operator"); 
  }

  //---------------------------------------------------------------------
  IToken* OprtAssign::Clone() const
  { 
    return new OprtAssign(*this); 
  }
  
  //---------------------------------------------------------------------
  void OprtAssign::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    Variable *pVar = dynamic_cast<Variable*>(a_pArg[0].Get());

    // assigment to non variable type
    if (!pVar)
    {
      ErrorContext err;
      err.Arg   = 1;
      err.Ident = _T("=");
      err.Errc  = ecASSIGNEMENT_TO_VALUE;
      throw ParserError(err);
    }

    *pVar = *a_pArg[1]; //pVar->SetFloat(a_pArg[1]->GetFloat());
    *ret = *pVar; 
  }

  //---------------------------------------------------------------------
  //
  //  class OprtAssignAdd
  //
  //---------------------------------------------------------------------

  OprtAssignAdd::OprtAssignAdd() 
    :IOprtBin(_T("+="), (int)prASSIGN, oaLEFT) 
  {}

  //---------------------------------------------------------------------
  void OprtAssignAdd::Eval(ptr_val_type& ret, const ptr_val_type *a_pArg, int)   
  {
    Variable *pVar = dynamic_cast<Variable*>(a_pArg[0].Get());

    // assigment to non variable type
    if (!pVar)
    {
      ErrorContext err;
      err.Arg   = 1;
      err.Ident = _T("+=");
      err.Errc  = ecASSIGNEMENT_TO_VALUE;
      throw ParserError(err);
    }

    *pVar = cmplx_type(a_pArg[0]->GetFloat() + a_pArg[1]->GetFloat(),
                       a_pArg[0]->GetImag() + a_pArg[1]->GetImag());
    *ret = *pVar;
  }

  //---------------------------------------------------------------------
  const char_type* OprtAssignAdd::GetDesc() const 
  { 
    return _T("assignement operator"); 
  }

  //---------------------------------------------------------------------
  IToken* OprtAssignAdd::Clone() const            
  { 
    return new OprtAssignAdd(*this); 
  }

  //---------------------------------------------------------------------
  //
  //  class OprtAssignAdd
  //
  //---------------------------------------------------------------------

  OprtAssignSub::OprtAssignSub() 
    :IOprtBin(_T("-="), (int)prASSIGN, oaLEFT) 
  {}

  //---------------------------------------------------------------------
  void OprtAssignSub::Eval(ptr_val_type& ret, const ptr_val_type *a_pArg, int)   
  {
    Variable *pVar = dynamic_cast<Variable*>(a_pArg[0].Get());
    if (!pVar)
    {
      ErrorContext err;
      err.Arg   = 1;
      err.Ident = _T("-=");
      err.Errc  = ecASSIGNEMENT_TO_VALUE;
      throw ParserError(err);
    }

    *pVar = cmplx_type(a_pArg[0]->GetFloat() - a_pArg[1]->GetFloat(),
                       a_pArg[0]->GetImag() - a_pArg[1]->GetImag());
    *ret = *pVar; 
  }

  //---------------------------------------------------------------------
  const char_type* OprtAssignSub::GetDesc() const 
  { 
    return _T("assignement operator"); 
  }

  //---------------------------------------------------------------------
  IToken* OprtAssignSub::Clone() const            
  { 
     return new OprtAssignSub(*this); 
  }

  //---------------------------------------------------------------------
  //
  //  class OprtAssignAdd
  //
  //---------------------------------------------------------------------

  OprtAssignMul::OprtAssignMul() 
    :IOprtBin(_T("*="), (int)prASSIGN, oaLEFT) 
  {}

  //---------------------------------------------------------------------
  void OprtAssignMul::Eval(ptr_val_type& ret, const ptr_val_type *a_pArg, int)
  {
    Variable *pVar = dynamic_cast<Variable*>(a_pArg[0].Get());
    if (!pVar)
    {
      ErrorContext err;
      err.Arg   = 1;
      err.Ident = _T("*=");
      err.Errc  = ecASSIGNEMENT_TO_VALUE;
      throw ParserError(err);
    }

    float_type a = a_pArg[0]->GetFloat(),
               b = a_pArg[0]->GetImag(),
               c = a_pArg[1]->GetFloat(),
               d = a_pArg[1]->GetImag();
    *pVar = cmplx_type(a*c-b*d, a*d-b*c); 
    *ret = *pVar;
  }

  //---------------------------------------------------------------------
  const char_type* OprtAssignMul::GetDesc() const 
  { 
    return _T("multiply and assign operator"); 
  }

  //---------------------------------------------------------------------
  IToken* OprtAssignMul::Clone() const
  {  
    return new OprtAssignMul(*this); 
  }

  //---------------------------------------------------------------------
  //
  //  class OprtAssignDiv
  //
  //---------------------------------------------------------------------

  OprtAssignDiv::OprtAssignDiv() : IOprtBin(_T("/="), (int)prASSIGN, oaLEFT) 
  {}

  //------------------------------------------------------------------------------
  void OprtAssignDiv::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    Variable *pVar = dynamic_cast<Variable*>(a_pArg[0].Get());
    if (!pVar)
    {
      ErrorContext err;
      err.Arg   = 1;
      err.Ident = _T("/=");
      err.Errc  = ecASSIGNEMENT_TO_VALUE;
      throw ParserError(err);
    }

    float_type a = a_pArg[0]->GetFloat(),
               b = a_pArg[0]->GetImag(),
               c = a_pArg[1]->GetFloat(),
               d = a_pArg[1]->GetImag(),
               n = c*c + d*d;
    *pVar = cmplx_type((a*c+b*d)/n, (b*c-a*d)/n); 
    *ret = *pVar; 
  }

  //------------------------------------------------------------------------------
  const char_type* OprtAssignDiv::GetDesc() const 
  { 
     return _T("multiply and divide operator"); 
  }

  //------------------------------------------------------------------------------
  IToken* OprtAssignDiv::Clone() const
  {  
    return new OprtAssignDiv(*this); 
  }
MUP_NAMESPACE_END
