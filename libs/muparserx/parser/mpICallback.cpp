/** \file
    \brief Implementation of the interface for parser callback objects.

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
#include "mpICallback.h"
#include <cassert>

#include "mpParserBase.h"


MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  ICallback::ICallback(ECmdCode a_iCode, 
                       const char_type *a_szName, 
                       int a_nArgc)
    :IToken(a_iCode, a_szName)
    ,m_pParent(nullptr)
    ,m_nArgc(a_nArgc)
    ,m_nArgsPresent(-1)
  {}

  //------------------------------------------------------------------------------
  ICallback::~ICallback()
  {}

  //---------------------------------------------------------------------------
  ICallback* ICallback::AsICallback()
  {
    return this;
  }

  //---------------------------------------------------------------------------
  IValue* ICallback::AsIValue()
  {
    return nullptr;
  }

  //------------------------------------------------------------------------------
  /** \brief Returns a pointer to the parser object owning this callback. 
      \pre [assert] m_pParent must be defined
  */
  ParserXBase* ICallback::GetParent()
  {
    assert(m_pParent);
    return m_pParent;
  }

  //------------------------------------------------------------------------------
  void  ICallback::SetArgc(int argc)
  {
    m_nArgc = argc;
  }

  //------------------------------------------------------------------------------
  /** \brief Returns the m´number of arguments required by this callback. 
      \return Number of arguments or -1 if the number of arguments is variable.  
  */
  int ICallback::GetArgc() const
  {
    return m_nArgc;
  }

  //------------------------------------------------------------------------------
  /** \brief Assign a parser object to the callback.
      \param a_pParent The parser that belongs to this callback object.

    The parent object can be used in order to access internals of the parser
    from within a callback object. Thus enabling callbacks to delete 
    variables or functions if this is desired.
  */
  void ICallback::SetParent(parent_type *a_pParent)
  {
    assert(a_pParent);
    m_pParent = a_pParent;
  }

  //------------------------------------------------------------------------------
  string_type ICallback::AsciiDump() const
  {
    stringstream_type ss;

    ss << g_sCmdCode[ GetCode() ];
    ss << _T(" [addr=0x") << std::hex << this << std::dec;
    ss << _T("; pos=") << GetExprPos();
    ss << _T("; id=\"") << GetIdent() << "\"";
    ss << _T("; argc=") << GetArgc() << " (found: " << m_nArgsPresent << ")";
    ss << _T("]");

    return ss.str();
  }

  //------------------------------------------------------------------------------
  void ICallback::SetNumArgsPresent(int argc)
  {
    m_nArgsPresent = argc;
  }

  //------------------------------------------------------------------------------
  int ICallback::GetArgsPresent() const
  {
    if (m_nArgc!=-1)
      return m_nArgc;
    else
      return m_nArgsPresent;
  }
} // namespace mu
