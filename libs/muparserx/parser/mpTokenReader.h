/** \file
    \brief Definition of the token reader used to break the expression string up 
           into tokens.

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

#ifndef MUP_TOKEN_READER_H
#define MUP_TOKEN_READER_H

//--- Standard includes ----------------------------------------------------
#include <cstdio>
#include <cstring>
#include <map>
#include <stack>
#include <string>
#include <list>

//--- muParserX framework --------------------------------------------------
#include "mpIToken.h"
#include "mpError.h"
#include "mpStack.h"
#include "mpFwdDecl.h"

MUP_NAMESPACE_START

  /** \brief Token reader for the ParserXBase class. */
  class TokenReader
  {
  friend class ParserXBase;

  public:

    typedef std::vector<ptr_tok_type> token_buf_type;

  private:

    TokenReader(const TokenReader &a_Reader);
    TokenReader& operator=(const TokenReader &a_Reader);
    void Assign(const TokenReader &a_Reader);
    void DeleteValReader();
    void SetParent(ParserXBase *a_pParent);

    int ExtractToken(const char_type *a_szCharSet, string_type &a_sTok, int a_iPos) const;

    void SkipCommentsAndWhitespaces();
    bool IsBuiltIn(ptr_tok_type &t);
    bool IsEOF(ptr_tok_type &t);
    bool IsNewline(ptr_tok_type &a_Tok);
    bool IsNewLine(ptr_tok_type &t);
    bool IsInfixOpTok(ptr_tok_type &t);
    bool IsFunTok(ptr_tok_type &t);
    bool IsPostOpTok(ptr_tok_type &t);
    bool IsOprt(ptr_tok_type &t);
    bool IsValTok(ptr_tok_type &t);
    bool IsVarOrConstTok(ptr_tok_type &t);
    bool IsUndefVarTok(ptr_tok_type &t);
    bool IsComment();

    const ptr_tok_type& Store(const ptr_tok_type &t, int pos);

    ParserXBase *m_pParser;  ///< Pointer to the parser bound to this token reader
    string_type m_sExpr;     ///< The expression beeing currently parsed
    int  m_nPos;             ///< Current parsing position in the expression
    int  m_nNumBra;          ///< Number of open parenthesis
    int  m_nNumIndex;        ///< Number of open index paranethesis    
	int  m_nNumCurly;        ///< Number of open curly brackets
    int  m_nNumIfElse;       ///< Coubter for if-then-else levels
    int  m_nSynFlags;        ///< Flags to controll the syntax flow

    token_buf_type m_vTokens;
    ECmdCode m_eLastTokCode;

    mutable fun_maptype  *m_pFunDef;
    mutable oprt_bin_maptype *m_pOprtDef;
    mutable oprt_ifx_maptype *m_pInfixOprtDef;
    mutable oprt_pfx_maptype *m_pPostOprtDef;
    mutable val_maptype  *m_pConstDef;
    val_vec_type *m_pDynVarShadowValues; ///< Value items created for holding values of variables created at parser runtime
    var_maptype  *m_pVarDef;             ///< The only non const pointer to parser internals

    readervec_type m_vValueReader;  ///< Value token identification function
    var_maptype m_UsedVar;
    float_type m_fZero;             ///< Dummy value of zero, referenced by undefined variables

  public:

    TokenReader(ParserXBase *a_pParent);
   ~TokenReader();
    TokenReader* Clone(ParserXBase *a_pParent) const;
    
    void AddValueReader(IValueReader *a_pReader);
    void AddSynFlags(int flag);
    int GetPos() const;
    const string_type& GetExpr() const;
    const var_maptype& GetUsedVar() const;
    const token_buf_type& GetTokens() const;
    void SetExpr(const string_type &a_sExpr);

    void ReInit();
    ptr_tok_type ReadNextToken();
  }; // class TokenReader

MUP_NAMESPACE_END

#endif


