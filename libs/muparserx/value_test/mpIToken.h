/*
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
#ifndef MUP_ITOKEN_H
#define MUP_ITOKEN_H

#include <list>
#include "mpTypes.h"
#include "mpFwdDecl.h"

MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  /** \brief Generic token interface for expression tokens. 
      \author (C) 2010 Ingo Berg 

      Tokens can either be Functions, operators, values, variables or necessary 
      base tokens like brackets. ´The IToken baseclass implements reference 
      counting. Only TokenPtr<...> templates may be used as pointers to tokens.
  */
  class IToken
  {
  friend std::ostream& operator<<(std::ostream &a_Stream, const IToken &a_Val);
  friend std::wostream& operator<<(std::wostream &a_Stream, const IToken &a_Val);

  friend class TokenPtr<IToken>;
  friend class TokenPtr<IValue>;
  friend class TokenPtr<IOprtBin>;
  friend class TokenPtr<IFunction>;
  friend class TokenPtr<Value>;
  friend class TokenPtr<Variable>;

  public:

    enum EFlags
    {
      flNONE = 0,
      flVOLATILE = 1
    };

    virtual IToken* Clone() const = 0;
    virtual string_type ToString() const;
    virtual string_type AsciiDump() const;
    
    virtual ICallback* AsICallback();
    virtual IValue* AsIValue();
    virtual IPrecedence* AsIPrecedence();

    virtual void Compile(const string_type &sArg);

    ECmdCode GetCode() const;
    int GetExprPos() const;
    
    const string_type& GetIdent() const;
    long GetRef() const;
    void SetIdent(const string_type &a_sIdent);
    void SetExprPos(int nPos);

    void AddFlags(int flags);
    bool IsFlagSet(int flags) const;

  protected:

    explicit IToken(ECmdCode a_iCode);
    virtual ~IToken();
    IToken(ECmdCode a_iCode, string_type a_sIdent);
    IToken(const IToken &ref);

    void ResetRef();

  private:

    /** \brief Release the token. 
    
      This Function either deletes the token or releases it to
      the value cache for reuse without deletion.
    */
    virtual void Release();

    void IncRef() const;
    long DecRef() const;

    ECmdCode m_eCode;
    string_type m_sIdent;
    int m_nPosExpr;           ///< Original position of the token in the expression
    mutable long m_nRefCount; ///< Reference counter.
    int m_flags;

#ifdef MUP_LEAKAGE_REPORT
    static std::list<IToken*> s_Tokens;

  public:
    static void LeakageReport();
#endif
  };


  //---------------------------------------------------------------------------
  /** \brief Default token implentation. 
  */
  class GenericToken : public IToken
  {
  public:
      GenericToken(ECmdCode a_iCode, string_type a_sIdent);
      explicit GenericToken(ECmdCode a_iCode);
      GenericToken(const GenericToken &a_Tok);      
      virtual ~GenericToken();
      virtual IToken* Clone() const;
      virtual string_type AsciiDump() const;
  };

  //------------------------------------------------------------------------------
  template<typename T>
  class TokenPtr
  {
  public:
     
      typedef T* token_type;

      //---------------------------------------------------------------------------
      explicit TokenPtr(token_type p = 0)
        :m_pTok(p)
      {
        if (m_pTok)
          m_pTok->IncRef();
      }

      //---------------------------------------------------------------------------
      TokenPtr(const TokenPtr &p)
        :m_pTok(p.m_pTok)
      {
        if (m_pTok)
          m_pTok->IncRef();
      }

      //---------------------------------------------------------------------------
     ~TokenPtr()
      {
        if (m_pTok && m_pTok->DecRef()==0)
          m_pTok->Release();
      }

      //---------------------------------------------------------------------------
      token_type operator->() const
      {
        return static_cast<token_type>(m_pTok);
      }
  
      //---------------------------------------------------------------------------
      T& operator*() const
      {
        assert(m_pTok);
        return *(static_cast<token_type>(m_pTok));
      }

      //---------------------------------------------------------------------------
      token_type Get() const
      {
        return static_cast<token_type>(m_pTok);
      }

      //---------------------------------------------------------------------------
      /** \brief Release the managed pointer and assign a new pointer. */
      void Reset(token_type tok)
      {
        if (m_pTok && m_pTok->DecRef()==0) 
        {
          m_pTok->Release();
          //delete m_pTok;
        }

        tok->IncRef();
        m_pTok = tok;
      }

      //---------------------------------------------------------------------------
      TokenPtr& operator=(const TokenPtr &p)
      {
        if (p.m_pTok) 
          p.m_pTok->IncRef();

        if (m_pTok && m_pTok->DecRef()==0) 
        {
          m_pTok->Release();
          //delete m_pTok;
        }

        m_pTok = p.m_pTok;

        return *this;
      }

  private:
      IToken *m_pTok;
  };

MUP_NAMESPACE_END

#endif // include guard
