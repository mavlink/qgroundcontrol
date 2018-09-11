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
#include "mpIOprt.h"

#include "mpError.h"


MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  //
  // Binary Operators
  //
  //------------------------------------------------------------------------------

  IOprtBin::IOprtBin(const char_type *a_szIdent, int nPrec, EOprtAsct eAsc)
    :ICallback(cmOPRT_BIN, a_szIdent, 2)
    ,IPrecedence()
    ,m_nPrec(nPrec)
    ,m_eAsc(eAsc)
  {}

  //------------------------------------------------------------------------------
  IOprtBin::~IOprtBin()
  {}

  //------------------------------------------------------------------------------
  string_type IOprtBin::AsciiDump() const
  {
    stringstream_type ss;

    ss << g_sCmdCode[ GetCode() ];
    ss << _T(" [addr=0x") << std::hex << this << std::dec;
    ss << _T("; pos=") << GetExprPos();
    ss << _T("; id=\"") << GetIdent() << _T("\"");
    ss << _T("; prec=") << GetPri();
    ss << _T("; argc=") << GetArgc();
    ss << _T("]");

    return ss.str();
  }

  //------------------------------------------------------------------------------
  int IOprtBin::GetPri() const
  {
    return m_nPrec;
  }

  //------------------------------------------------------------------------------
  EOprtAsct IOprtBin::GetAssociativity() const
  {
    return m_eAsc;
  }

  //---------------------------------------------------------------------------
  IPrecedence* IOprtBin::AsIPrecedence()
  {
    return this;
  }

  //------------------------------------------------------------------------------
  //
  // Unary Postfix Operators
  //
  //------------------------------------------------------------------------------

  IOprtPostfix::IOprtPostfix(const char_type *a_szIdent)
    :ICallback(cmOPRT_POSTFIX, a_szIdent, 1)
  {}

  //------------------------------------------------------------------------------
  IOprtPostfix::~IOprtPostfix()
  {}

  //------------------------------------------------------------------------------
  string_type IOprtPostfix::AsciiDump() const
  {
    stringstream_type ss;

    ss << g_sCmdCode[ GetCode() ];
    ss << _T(" [addr=0x") << std::hex << this << std::dec;
    ss << _T("; pos=") << GetExprPos();
    ss << _T("; id=\"") << GetIdent() << _T("\"");
    ss << _T("; argc=") << GetArgc();
    ss << _T("]");

    return ss.str();
  }

  //------------------------------------------------------------------------------
  //
  // Unary Infix Operators
  //
  //------------------------------------------------------------------------------

  IOprtInfix::IOprtInfix(const char_type *a_szIdent, int nPrec)
    :ICallback(cmOPRT_INFIX, a_szIdent, 1)
    ,IPrecedence()
    ,m_nPrec(nPrec)
  {}

  //------------------------------------------------------------------------------
  IOprtInfix::~IOprtInfix()
  {}

  //------------------------------------------------------------------------------
  string_type IOprtInfix::AsciiDump() const
  {
    stringstream_type ss;

    ss << g_sCmdCode[ GetCode() ];
    ss << _T(" [addr=0x") << std::hex << this << std::dec;
    ss << _T("; pos=") << GetExprPos();
    ss << _T("; id=\"") << GetIdent() << _T("\"");
    ss << _T("; argc=") << GetArgc();
    ss << _T("]");

    return ss.str();
  }

  //---------------------------------------------------------------------------
  IPrecedence* IOprtInfix::AsIPrecedence()
  {
    return this;
  }

  //------------------------------------------------------------------------------
  int IOprtInfix::GetPri() const
  {
    return m_nPrec;
  }

  //------------------------------------------------------------------------------
  EOprtAsct IOprtInfix::GetAssociativity() const
  {
    return oaNONE;
  }
}  // namespace mu
