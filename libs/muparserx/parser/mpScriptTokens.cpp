/*
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
*/
#include "mpScriptTokens.h"
#include "mpTypes.h"


MUP_NAMESPACE_START

  //---------------------------------------------------------------------------
  TokenNewline::TokenNewline()
    :IToken(cmSCRIPT_NEWLINE)
    ,m_nOffset(0)
  {}

  //---------------------------------------------------------------------------
  IToken* TokenNewline::Clone() const
  {
    return new TokenNewline(*this);
  }

  //---------------------------------------------------------------------------
  void TokenNewline::SetStackOffset(int nOffset)
  {
    m_nOffset = nOffset;
  }

  //---------------------------------------------------------------------------
  int TokenNewline::GetStackOffset() const
  {
    return m_nOffset;
  }

  //---------------------------------------------------------------------------
  string_type TokenNewline::AsciiDump() const
  {
    stringstream_type ss;

    ss << g_sCmdCode[ GetCode() ];
    ss << _T(" [addr=0x") << std::hex << this << std::dec;
    ss << _T("; pos=") << GetExprPos();
    ss << _T("; offset=") << m_nOffset;
    ss << _T("]");
    return ss.str();
  }
  
MUP_NAMESPACE_END
