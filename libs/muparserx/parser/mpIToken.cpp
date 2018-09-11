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
#include "mpIToken.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <list>
#include <cassert>

#include "mpIPrecedence.h"

MUP_NAMESPACE_START

#ifdef MUP_LEAKAGE_REPORT
  std::list<IToken*> IToken::s_Tokens;
#endif

#ifndef _UNICODE

  //---------------------------------------------------------------------------
  /** \brief Overloaded streaming operator for outputting the value type 
             into an std::ostream. 
      \param a_Stream The stream object
      \param a_Val The value object to be streamed

    This function is only present if _UNICODE is not defined.
  */
  std::ostream& operator<<(std::ostream &a_Stream, const IToken &tok)
  {
    return a_Stream << tok.ToString();
  }  

#else

  //---------------------------------------------------------------------------
  /** \brief Overloaded streaming operator for outputting the value type 
             into an std::ostream. 
      \param a_Stream The stream object
      \param a_Val The value object to be streamed

    This function is only present if _UNICODE is defined.
  */
  std::wostream& operator<<(std::wostream &a_Stream, const IToken &tok)
  {
    return a_Stream << tok.ToString();
  }  
#endif

#ifdef MUP_LEAKAGE_REPORT

  void IToken::LeakageReport()
  {
    using namespace std;

    console() << "\n";     
    console() << "Memory leakage report:\n\n";     
    if (IToken::s_Tokens.size())
    {
      list<IToken*>::const_iterator item = IToken::s_Tokens.begin();
      std::vector<int> stat(cmCOUNT, 0);
      for (; item!=IToken::s_Tokens.end(); ++item)
      {
        console() << "Addr: 0x" << hex << *item << "  Ident: \"" << (*item)->GetIdent() << "\"";
        console() << "\tCode: " << g_sCmdCode[(*item)->GetCode()] << "\n"; 
        stat[(*item)->GetCode()]++;
      }
      console() << "Leaked tokens: " << dec << (int)IToken::s_Tokens.size() << std::endl;
      
      for (int i=0; i<cmCOUNT; ++i)
        console() << g_sCmdCode[i] << ":" << stat[i] << "\n";

      console() << endl;
    }
    else
    {
      console() << "No tokens leaked!\n";
    }
  }

#endif

  //------------------------------------------------------------------------------
  IToken::IToken(ECmdCode a_iCode)
    :m_eCode(a_iCode)
    ,m_sIdent()
    ,m_nPosExpr(-1)
    ,m_nRefCount(0)
    ,m_flags(0)
  {
#ifdef MUP_LEAKAGE_REPORT
    IToken::s_Tokens.push_back(this);
#endif
  }

  //------------------------------------------------------------------------------
  IToken::IToken(ECmdCode a_iCode, string_type a_sIdent)
    :m_eCode(a_iCode)
    ,m_sIdent(a_sIdent)
    ,m_nPosExpr(-1)
    ,m_nRefCount(0)
    ,m_flags(0)
  {
#ifdef MUP_LEAKAGE_REPORT
    IToken::s_Tokens.push_back(this);
#endif
  }

  /** \brief Destructor (trivial). */
  IToken::~IToken()
  {
#ifdef MUP_LEAKAGE_REPORT
    std::list<IToken*>::iterator it = std::find(IToken::s_Tokens.begin(), IToken::s_Tokens.end(), this);
    IToken::s_Tokens.remove(this);
#endif
  }

  //------------------------------------------------------------------------------
  /** \brief Copy constructor. 
      \param ref The token to copy basic state information from.

    The copy constructor must be implemented in order not to screw up
    the reference count of the created object. CC's are used in the
    Clone function and they would start with a reference count != 0 
    introducing memory leaks if the default CC where used.
  */
  IToken::IToken(const IToken &ref)
  {
    m_eCode  = ref.m_eCode;
    m_sIdent = ref.m_sIdent;
    m_flags  = ref.m_flags;
    m_nPosExpr = ref.m_nPosExpr;

    // The following items must be initialised 
    // (rather than just beeing copied)
    m_nRefCount = 0;
  }

  //------------------------------------------------------------------------------
  void IToken::ResetRef()
  {
    m_nRefCount = 0;
  }

  //------------------------------------------------------------------------------
  void IToken::Release()
  {
    delete this;
  }

  //------------------------------------------------------------------------------
  string_type IToken::ToString() const
  {
    return AsciiDump();
  }

  //------------------------------------------------------------------------------
  int IToken::GetExprPos() const
  {
    return m_nPosExpr;
  }

  //------------------------------------------------------------------------------
  void IToken::SetExprPos(int nPos)
  {
    m_nPosExpr = nPos;
  }

  //------------------------------------------------------------------------------
  /** \brief return the token code. 
     
      \sa ECmdCode
  */
  ECmdCode IToken::GetCode() const
  {
    return m_eCode;
  }

  //------------------------------------------------------------------------------
  /** \brief Return the token identifier string. */
  const string_type& IToken::GetIdent() const
  {
    return m_sIdent;
  }

  //------------------------------------------------------------------------------
  void IToken::SetIdent(const string_type &a_sIdent)
  {
    m_sIdent = a_sIdent;
  }

  //------------------------------------------------------------------------------
  string_type IToken::AsciiDump() const
  {
    stringstream_type ss;
    ss << g_sCmdCode[m_eCode];
    return ss.str().c_str();
  }

  //------------------------------------------------------------------------------
  void IToken::IncRef() const
  {
    ++m_nRefCount;
  }

  //------------------------------------------------------------------------------
  long IToken::DecRef() const
  {
    return --m_nRefCount;
  }

  //------------------------------------------------------------------------------
  long IToken::GetRef() const
  {
    return m_nRefCount;
  }

  //---------------------------------------------------------------------------
  void IToken::AddFlags(int flags)
  {
    m_flags |= flags;
  }

  //---------------------------------------------------------------------------
  bool IToken::IsFlagSet(int flags) const
  {
    return (m_flags & flags)==flags;
  }

  //---------------------------------------------------------------------------
  ICallback* IToken::AsICallback()
  {
    return nullptr;
  }

  //---------------------------------------------------------------------------
  IValue* IToken::AsIValue()
  {
    return nullptr;
  }

  //---------------------------------------------------------------------------
  IPrecedence* IToken::AsIPrecedence()
  {
    return nullptr;
  }

  //------------------------------------------------------------------------------
  void IToken::Compile(const string_type & /*sArg*/)
  {
  }

  //---------------------------------------------------------------------------
  //
  // Generic token implementation
  //
  //---------------------------------------------------------------------------

  GenericToken::GenericToken(ECmdCode a_iCode, string_type a_sIdent)
    :IToken(a_iCode, a_sIdent)
  {}

  //---------------------------------------------------------------------------
  GenericToken::GenericToken(ECmdCode a_iCode)
    :IToken(a_iCode, _T(""))
  {}

  //---------------------------------------------------------------------------
  GenericToken::~GenericToken()
  {}

  //---------------------------------------------------------------------------
  GenericToken::GenericToken(const GenericToken &a_Tok)
    :IToken(a_Tok)
  {}

  //---------------------------------------------------------------------------
  IToken* GenericToken::Clone() const
  {
    return new GenericToken(*this);
  }
  
  //------------------------------------------------------------------------------
  string_type GenericToken::AsciiDump() const
  {
    stringstream_type ss;

    ss << g_sCmdCode[ GetCode() ];
    ss << _T(" [addr=0x") << std::hex << this << _T("]");

    return ss.str();
  }

MUP_NAMESPACE_END
