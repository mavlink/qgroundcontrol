/*
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
*/
#include "mpError.h"
#include "mpIToken.h"


MUP_NAMESPACE_START

  const ParserErrorMsg ParserErrorMsg::m_Instance;

  //------------------------------------------------------------------------------
  const ParserErrorMsg& ParserErrorMsg::Instance()
  {
    return m_Instance;
  }

  //------------------------------------------------------------------------------
  string_type ParserErrorMsg::operator[](unsigned a_iIdx) const
  {
    return (a_iIdx<m_vErrMsg.size()) ? m_vErrMsg[a_iIdx] : string_type();
  }


  //---------------------------------------------------------------------------
  ParserErrorMsg::~ParserErrorMsg()
  {}

  //---------------------------------------------------------------------------
  /** \brief Assignement operator is deactivated.
  */
  ParserErrorMsg& ParserErrorMsg::operator=(const ParserErrorMsg& )
  {
    assert(false);
    return *this;
  }

  //---------------------------------------------------------------------------
  ParserErrorMsg::ParserErrorMsg(const ParserErrorMsg&)
  {}

  //---------------------------------------------------------------------------
  ParserErrorMsg::ParserErrorMsg()
    :m_vErrMsg(0)
  {
    m_vErrMsg.resize(ecCOUNT);

    m_vErrMsg[ecUNASSIGNABLE_TOKEN]      = _T("Undefined token \"$IDENT$\" found at position $POS$");
    m_vErrMsg[ecINTERNAL_ERROR]          = _T("Internal error");
    m_vErrMsg[ecINVALID_NAME]            = _T("Invalid function-, variable- or constant name");
    m_vErrMsg[ecINVALID_FUN_PTR]         = _T("Invalid pointer to callback function");
    m_vErrMsg[ecINVALID_VAR_PTR]         = _T("Invalid pointer to variable");
    m_vErrMsg[ecUNEXPECTED_OPERATOR]     = _T("Unexpected operator \"$IDENT$\" found at position $POS$");
    m_vErrMsg[ecUNEXPECTED_EOF]          = _T("Unexpected end of expression at position $POS$");
    m_vErrMsg[ecUNEXPECTED_COMMA]        = _T("Unexpected comma at position $POS$");
    m_vErrMsg[ecUNEXPECTED_PARENS  ]     = _T("Unexpected parenthesis \"$IDENT$\" at position $POS$");
    m_vErrMsg[ecUNEXPECTED_FUN]          = _T("Unexpected function \"$IDENT$\" at position $POS$");
    m_vErrMsg[ecUNEXPECTED_VAL]          = _T("Unexpected value \"$IDENT$\" found at position $POS$");
    m_vErrMsg[ecUNEXPECTED_VAR]          = _T("Unexpected variable \"$IDENT$\" found at position $POS$");
    m_vErrMsg[ecUNEXPECTED_STR]          = _T("Unexpected string token found at position $POS$");
    m_vErrMsg[ecUNEXPECTED_CONDITIONAL]  = _T("The \"$IDENT$\" operator must be preceeded by a closing bracket");
    m_vErrMsg[ecUNEXPECTED_NEWLINE]      = _T("Unexprected newline");
    m_vErrMsg[ecMISSING_PARENS]          = _T("Missing parenthesis");
    m_vErrMsg[ecMISSING_ELSE_CLAUSE]     = _T("If-then-else operator is missing an else clause");
    m_vErrMsg[ecMISPLACED_COLON]         = _T("Misplaced colon at position $POS$");
    m_vErrMsg[ecTOO_MANY_PARAMS]         = _T("Too many parameters for function \"$IDENT$\"");
    m_vErrMsg[ecTOO_FEW_PARAMS]          = _T("Too few parameters for function \"$IDENT$\"");
    m_vErrMsg[ecDIV_BY_ZERO]             = _T("Divide by zero");
    m_vErrMsg[ecDOMAIN_ERROR]            = _T("Domain error");
    m_vErrMsg[ecNAME_CONFLICT]           = _T("Name conflict");
    m_vErrMsg[ecOPT_PRI]                 = _T("Invalid value for operator priority (must be greater or equal to zero)");
    m_vErrMsg[ecBUILTIN_OVERLOAD]        = _T("Binary operator identifier conflicts with a built in operator");
    m_vErrMsg[ecUNTERMINATED_STRING]     = _T("Unterminated string starting at position $POS$");
    m_vErrMsg[ecSTRING_EXPECTED]         = _T("String function called with a non string type of argument");
    m_vErrMsg[ecVAL_EXPECTED]            = _T("Numerical function called with a non value type of argument");
    m_vErrMsg[ecTYPE_CONFLICT]           = _T("Value \"$IDENT$\" is of type '$TYPE1$'. There is no implicit conversion to type '$TYPE2$'");
    m_vErrMsg[ecTYPE_CONFLICT_FUN]       = _T("Argument $ARG$ of function/operator \"$IDENT$\" is of type '$TYPE1$' whereas type '$TYPE2$' was expected");
    m_vErrMsg[ecTYPE_CONFLICT_IDX]       = _T("Index to variable \"$IDENT$\" must be a positive integer value");
    m_vErrMsg[ecGENERIC]                 = _T("Parser error");
    m_vErrMsg[ecINVALID_TYPE]            = _T("Invalid argument type");
    m_vErrMsg[ecINVALID_TYPECAST]        = _T("Value type conversion from type '$TYPE1$' to '$TYPE2$' is not supported!");
    m_vErrMsg[ecARRAY_SIZE_MISMATCH]     = _T("Array size mismatch");
    m_vErrMsg[ecNOT_AN_ARRAY]            = _T("Using the index operator on the scalar variable \"$IDENT$\" is not allowed");
    m_vErrMsg[ecUNEXPECTED_SQR_BRACKET]  = _T("Unexpected \"]\"");
    m_vErrMsg[ecAPI_INVALID_PROTOTYPE]   = _T("Invalid prototype (use something like: \"f:fff\")");
    m_vErrMsg[ecAPI_NOT_AN_ARRAY]        = _T("Not an array");
    m_vErrMsg[ecAPI_INVALID_DIMENSION]   = _T("Invalid matrix dimensions");
    m_vErrMsg[ecINDEX_OUT_OF_BOUNDS]     = _T("Index to variable \"$IDENT$\" is out of bounds");
    m_vErrMsg[ecMISSING_SQR_BRACKET]     = _T("Missing \"]\"");
    m_vErrMsg[ecASSIGNEMENT_TO_VALUE]    = _T("Assignment operator \"$IDENT$\" can't be used in this context");
    m_vErrMsg[ecEVAL]                    = _T("Can't evaluate function/operator \"$IDENT$\": $HINT$");

    #if defined(_DEBUG)
      for (int i=0; i<ecCOUNT; ++i)
        if (!m_vErrMsg[i].length())
          assert(false);
    #endif
  }

  //---------------------------------------------------------------------------
  //
  //  Error context
  //
  //---------------------------------------------------------------------------

  /** \brief Constructs an empty Error context structure. */
  ErrorContext::ErrorContext(EErrorCodes a_iErrc, 
                             int a_iPos, 
                             string_type a_sIdent)
    :Expr()
    ,Ident(a_sIdent)
    ,Hint()
    ,Errc(a_iErrc)
    ,Type1(0)
    ,Type2(0)
    ,Arg(-1)
    ,Pos(a_iPos)
  {}

  //---------------------------------------------------------------------------
  ErrorContext::ErrorContext(EErrorCodes iErrc, 
                             int iPos, 
                             string_type sIdent,
                             char_type cType1,
                             char_type cType2,
                             int nArg)
    :Expr()
    ,Ident(sIdent)
    ,Hint()
    ,Errc(iErrc)
    ,Type1(cType1)
    ,Type2(cType2)
    ,Arg(nArg)
    ,Pos(iPos)
  {}

  //---------------------------------------------------------------------------
  //
  //  ParserError class
  //
  //---------------------------------------------------------------------------

  ParserError::ParserError()
    :m_Err()
    ,m_sMsg()
    ,m_ErrMsg(ParserErrorMsg::Instance())
  {}

  //------------------------------------------------------------------------------
  ParserError::ParserError(const string_type &sMsg) 
    :m_Err()
    ,m_sMsg(sMsg)
    ,m_ErrMsg(ParserErrorMsg::Instance())
  {}

  //------------------------------------------------------------------------------
  ParserError::ParserError(const ErrorContext &a_Err) 
    :m_Err(a_Err)
    ,m_sMsg()
    ,m_ErrMsg(ParserErrorMsg::Instance())
  {
    m_sMsg = m_ErrMsg[a_Err.Errc];
  }

  //------------------------------------------------------------------------------
  ParserError::ParserError(const ParserError &a_Obj)
    :m_Err(a_Obj.m_Err)
    ,m_sMsg(a_Obj.m_sMsg)
    ,m_ErrMsg(ParserErrorMsg::Instance())
  {}

  //------------------------------------------------------------------------------
  ParserError& ParserError::operator=(const ParserError &a_Obj)
  {
    if (this==&a_Obj)
      return *this;

    m_sMsg = a_Obj.m_sMsg;
    m_Err = a_Obj.m_Err;
    return *this;
  }

  //------------------------------------------------------------------------------
  /** \brief Replace all occurences of a substring with another string. */
  void ParserError::ReplaceSubString( string_type &sSource,
                                      const string_type &sFind,
                                      const string_type &sReplaceWith) const
  {
    string_type sResult;
    string_type::size_type iPos(0), iNext(0);

    for(;;)
    {
      iNext = sSource.find(sFind, iPos);
      sResult.append(sSource, iPos, iNext-iPos);

      if( iNext==string_type::npos )
        break;

      sResult.append(sReplaceWith);
      iPos = iNext + sFind.length();
    } 

    sSource.swap(sResult);
  }


  //------------------------------------------------------------------------------
  /** \brief Replace all occurences of a substring with another string. */
  void ParserError::ReplaceSubString( string_type &sSource,
                                      const string_type &sFind,
                                      int iReplaceWith) const
  {
    stringstream_type stream;
    stream << iReplaceWith;
    ReplaceSubString(sSource, sFind, stream.str());
  }
  
  //------------------------------------------------------------------------------
  /** \brief Replace all occurences of a substring with another string. */
  void ParserError::ReplaceSubString( string_type &sSource,
                                      const string_type &sFind,
                                      char_type cReplaceWith) const
  {
    stringstream_type stream;
    stream << cReplaceWith;
    ReplaceSubString(sSource, sFind, stream.str());
  }
  
  //------------------------------------------------------------------------------
  void ParserError::Reset()
  {
    m_sMsg = _T("");
    m_Err = ErrorContext();
  }
      
  //------------------------------------------------------------------------------
  const string_type& ParserError::GetExpr() const 
  {
    return m_Err.Expr;
  }

  //------------------------------------------------------------------------------
  string_type ParserError::GetMsg() const
  {
    string_type sMsg(m_sMsg);
    ReplaceSubString(sMsg, _T("$EXPR$"),  m_Err.Expr);
    ReplaceSubString(sMsg, _T("$IDENT$"), m_Err.Ident);
    ReplaceSubString(sMsg, _T("$POS$"),   m_Err.Pos);
    ReplaceSubString(sMsg, _T("$ARG$"),   m_Err.Arg);
    ReplaceSubString(sMsg, _T("$TYPE1$"), m_Err.Type1);
    ReplaceSubString(sMsg, _T("$TYPE2$"), m_Err.Type2);
    ReplaceSubString(sMsg, _T("$HINT$"),  m_Err.Hint);
    return sMsg;
  }

  //------------------------------------------------------------------------------
  ErrorContext& ParserError::GetContext()
  {
    return m_Err;
  }

  //------------------------------------------------------------------------------
  /** \brief Return the expression position related to the error. 

    If the error is not related to a distinct position this will return -1
  */
  int ParserError::GetPos() const
  {
    return m_Err.Pos;
  }

  //------------------------------------------------------------------------------
  /** \brief Return string related with this token (if available). */
  const string_type& ParserError::GetToken() const
  {
    return m_Err.Ident;
  }

  //------------------------------------------------------------------------------
  /** \brief Return the error code. */
  EErrorCodes ParserError::GetCode() const
  {
    return m_Err.Errc;
  }
}  // namespace mu
