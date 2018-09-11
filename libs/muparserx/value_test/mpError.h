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
#ifndef MUP_ERROR_H
#define MUP_ERROR_H

#include <cassert>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

#include "mpTypes.h"


MUP_NAMESPACE_START

    /** \brief Error codes. 
    
      This is the complete list of all error codes used by muParserX
    */
    enum EErrorCodes
    {
      // Expression syntax errors
      ecUNEXPECTED_OPERATOR    =  0, ///< Unexpected binary operator found
      ecUNASSIGNABLE_TOKEN     =  1, ///< Token cant be identified.
      ecUNEXPECTED_EOF         =  2, ///< Unexpected end of expression. (Example: "2+sin(")
      ecUNEXPECTED_COMMA       =  3, ///< An unexpected comma has been found. (Example: "1,23")
      ecUNEXPECTED_VAL         =  4, ///< An unexpected value token has been found
      ecUNEXPECTED_VAR         =  5, ///< An unexpected variable token has been found
      ecUNEXPECTED_PARENS      =  6, ///< Unexpected Parenthesis, opening or closing
      ecUNEXPECTED_STR         =  7, ///< A string has been found at an inapropriate position
      ecUNEXPECTED_CONDITIONAL =  8,
      ecUNEXPECTED_NEWLINE     =  9, 
      ecSTRING_EXPECTED        = 10, ///< A string function has been called with a different type of argument
      ecVAL_EXPECTED           = 11, ///< A numerical function has been called with a non value type of argument
      ecMISSING_PARENS         = 12, ///< Missing parens. (Example: "3*sin(3")
      ecMISSING_ELSE_CLAUSE    = 13, 
      ecMISPLACED_COLON        = 14,
      ecUNEXPECTED_FUN         = 15, ///< Unexpected function found. (Example: "sin(8)cos(9)")
      ecUNTERMINATED_STRING    = 16, ///< unterminated string constant. (Example: "3*valueof("hello)")
      ecTOO_MANY_PARAMS        = 17, ///< Too many function parameters
      ecTOO_FEW_PARAMS         = 18, ///< Too few function parameters. (Example: "ite(1<2,2)")
      ecTYPE_CONFLICT          = 19, ///< Generic type conflict       
      ecTYPE_CONFLICT_FUN      = 20, ///< Function argument type conflict.
      ecTYPE_CONFLICT_IDX      = 21, ///< Function argument type conflict.
      ecINVALID_TYPE           = 22,       
      ecINVALID_TYPECAST       = 23, ///< Invalid Value token cast.
      ecARRAY_SIZE_MISMATCH    = 24, ///< Array size mismatch during a vector operation
      ecNOT_AN_ARRAY           = 25, ///< Using the index operator on a scalar variable
      ecUNEXPECTED_SQR_BRACKET = 26, ///< Invalid use of the index operator 

      // Invalid Parser input Parameters
      ecINVALID_NAME           = 27, ///< Invalid function, variable or constant name.
      ecBUILTIN_OVERLOAD       = 28, ///< Trying to overload builtin operator
      ecINVALID_FUN_PTR        = 29, ///< Invalid callback function pointer 
      ecINVALID_VAR_PTR        = 30, ///< Invalid variable pointer 

      ecNAME_CONFLICT          = 31, ///< Name conflict
      ecOPT_PRI                = 32, ///< Invalid operator priority
      ecASSIGNEMENT_TO_VALUE   = 33, ///< Assignment to operator (3=4 instead of a=4)

      // 
      ecDOMAIN_ERROR           = 34, ///< catch division by zero, sqrt(-1), log(0) (currently unused)
      ecDIV_BY_ZERO            = 35, ///< Division by zero (currently unused)
      ecGENERIC                = 36, ///< Generic error

      ecAPI_INVALID_PROTOTYPE  = 37, ///< API error: tried to create a callback with an invalid prototype definition
      ecAPI_NOT_AN_ARRAY       = 38, ///< Trying to access a non array type as an array
      ecAPI_INVALID_DIMENSION  = 39, ///< Trying to access a non array type as an array
      ecINDEX_OUT_OF_BOUNDS    = 40, ///< Array index is out of bounds
      ecMISSING_SQR_BRACKET    = 41, ///< The index operator was not closed properly (i.e. "v[3")
      ecEVAL                   = 42, ///< Error while evaluating function / operator

      // internal errors
      ecINTERNAL_ERROR         = 43, ///< Internal error of any kind.

      // The last two are special entries 
      ecCOUNT,                    ///< This is no error code, It just stores just the total number of error codes
      ecUNDEFINED              = -1, ///< Undefined message, placeholder to detect unassigned error messages
    };

    //---------------------------------------------------------------------------
    class ParserErrorMsg
    {
    public:
        typedef ParserErrorMsg self_type;

      ~ParserErrorMsg();

        static const ParserErrorMsg& Instance();
        string_type operator[](unsigned a_iIdx) const;

    private:
        std::vector<string_type>  m_vErrMsg;
        static const self_type m_Instance;

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
      char_type Type1;   ///> For type conflicts only! This is the type that was expected.
      char_type Type2;   ///> For type conflicts only! This is the type that was actually found.
      int Arg;           ///> The number of arguments that were expected.
      int Pos;           ///> Position inside the expression where the error occured.
    };

    //---------------------------------------------------------------------------
    /** \brief Error class of the parser. 
        \author Ingo Berg

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
        const ParserErrorMsg &m_ErrMsg;
    };		
} // namespace mu

#endif

