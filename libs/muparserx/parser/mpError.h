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
#ifndef MUP_ERROR_H
#define MUP_ERROR_H

#include <cassert>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

#include "mpTypes.h"
#include "mpParserMessageProvider.h"


MUP_NAMESPACE_START
  
  //---------------------------------------------------------------------------------------------
  class ParserErrorMsg 
  {
  public:
    ~ParserErrorMsg();

      static const ParserMessageProviderBase& Instance();
      static void Reset(ParserMessageProviderBase *pProvider);

      string_type GetErrorMsg(EErrorCodes eError) const;

  private:

      static std::unique_ptr<ParserMessageProviderBase> m_pInstance;
      ParserErrorMsg& operator=(const ParserErrorMsg &);
      ParserErrorMsg(const ParserErrorMsg&);
      ParserErrorMsg();
  };

  //---------------------------------------------------------------------------
  /** \brief Error context class. 
    
      This struct contains the data associated with parser erros. 
  */
  struct ErrorContext
  {
    /** \brief Creates an empty ErrorContext object.
      
      All Members are initialised to an invalid state.
    */
    ErrorContext(EErrorCodes a_iErrc = ecUNDEFINED, 
                  int a_iPos = -1, 
                  string_type a_sIdent = string_type() );

    ErrorContext(EErrorCodes a_iErrc, 
                  int a_iPos, 
                  string_type a_sIdent,
                  char_type cType1,
                  char_type cType2,
                  int nArg);

    string_type Expr;  ///> The expression string.
    string_type Ident; ///> The identifier of the token that caused the error.
    string_type Hint;  ///> Additional message
    EErrorCodes Errc;  ///> The error code
    char_type Type1;   ///> For type conflicts only! This is the type that was actually found.
    char_type Type2;   ///> For type conflicts only! This is the type that was expected.
    int Arg;           ///> The number of arguments that were expected.
    int Pos;           ///> Position inside the expression where the error occured.
  };

  //---------------------------------------------------------------------------
  /** \brief Error class of the parser. 
      \author IngecMISSINGo Berg

    Part of the math parser package.
  */
  class ParserError
  {
  private:
      //------------------------------------------------------------------------------
      /** \brief Replace all ocuurences of a substring with another string. */
      void ReplaceSubString(string_type &strSource, 
                            const string_type &strFind,
                            const string_type &strReplaceWith) const;
      void ReplaceSubString(string_type &sSource,
                            const string_type &sFind,
                            int iReplaceWith) const;
      void ReplaceSubString(string_type &sSource,
                            const string_type &sFind,
                            char_type cReplaceWith) const;
      void Reset();

  public:
      ParserError();
      ParserError(const string_type &sMsg);
      ParserError(const ErrorContext &a_Err);
      ParserError(const ParserError &a_Obj);
      ParserError& operator=(const ParserError &a_Obj);

      const string_type& GetExpr() const;
      string_type GetMsg() const;
      int GetPos() const;
      const string_type& GetToken() const;
      EErrorCodes GetCode() const;
      ErrorContext& GetContext();

  private:
      ErrorContext m_Err;  ///< Error context data
      string_type m_sMsg;  ///< The message string with all wildcards still in place.
      const ParserMessageProviderBase &m_ErrMsg;
  };		
} // namespace mu

#endif

