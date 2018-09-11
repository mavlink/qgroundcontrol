/** \file
    \brief Definition of basic types used by muParserX

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
#ifndef MUP_TYPES_H
#define MUP_TYPES_H

//--- Standard include ------------------------------------------------------
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <complex>

//--- muParserX framework ---------------------------------------------------
#include "suSortPred.h"  // We need the string utils sorting predicates
#include "mpDefines.h"


MUP_NAMESPACE_START

// Forward declarations
  class IValueReader;
  class IOprtBin;
  class IOprtPostfix;
  class IOprtInfix;
  class IFunction;
  class IToken;
  class IValue;
  class ParserXBase;
  class Value;
  class Variable;
  class TokenReader;

  // smart pointer types
  template<typename T>
  class TokenPtr;

  /** \brief Type of a managed pointer storing parser tokens. */
  typedef TokenPtr<IToken>   ptr_tok_type;

  /** \brief Type of a managed pointer storing value tokens. */
  typedef TokenPtr<IValue>   ptr_val_type;

  /** \brief Type of a managed pointer storing binary operator tokens. */
  typedef TokenPtr<IOprtBin> ptr_binop_type;

  /** \brief Type for a vector of tokens. */
  typedef std::vector<ptr_tok_type> token_vec_type;

  /** \brief Type for a vector of value items. */
  typedef std::vector<ptr_val_type> val_vec_type;

  // parser type definitions

  /** \brief Parser datatype for floating point value. */
  typedef /*long*/ double float_type;

  /** \brief Parser datatype for integer valuse. */
  typedef int int_type;

  /** \brief The basic type used for representing complex numbers. */
  typedef std::complex<float_type> cmplx_type;

  /** \brief Parser boolean datatype. 

      This must be bool! The only reason for this typedef is that I need the name bool_type
      for a preprocessor macro definition to avoid inconsistent naming of the macro parameters.
  */
  typedef bool bool_type;   

  /** \brief The parsers array type. */
  typedef std::vector<Value> array_type;

  /** \brief Parser datatype for strings. */
  typedef MUP_STRING_TYPE string_type;

  /** \brief Character type of the parser. */
  typedef string_type::value_type char_type;

  typedef std::basic_stringstream<char_type, 
                                  std::char_traits<char_type>,
                                  std::allocator<char_type> > stringstream_type;

  /** \brief Type of a vector holding pointers to value reader objects. */
  typedef std::vector<IValueReader*> readervec_type;

  /** \brief type for the parser variable storage. */
  typedef std::map<string_type, ptr_tok_type> var_maptype;  

  /** \brief type of a container used to store parser values.  */
  typedef std::map<string_type, ptr_tok_type> val_maptype;

  /** \brief Type of a container that binds Callback object pointer
             to operator identifiers. */
  typedef std::map<string_type, ptr_tok_type> fun_maptype;

  /** \brief Type of a container that binds Callback object pointer
             to operator identifiers. */
  typedef std::map<string_type, ptr_tok_type, su::pred::SortByLength<string_type> > oprt_bin_maptype;

  typedef std::multimap<string_type, ptr_tok_type, su::pred::SortByLength<string_type> > oprt_bin_multimap;

  /** \brief Type of a map for storing postfix operators by their name. */
  typedef std::map<string_type, ptr_tok_type> oprt_pfx_maptype;

  /** \brief Type of a map for storing infix operators by their name. */
  typedef std::map<string_type, ptr_tok_type> oprt_ifx_maptype;

  //------------------------------------------------------------------------------
  /** \brief Bytecode values.
      \attention The order of the operator entries must match the order in 
                 ParserXBase::c_DefaultOprt!
  */
  enum ECmdCode
  {
    // The following are codes for built in binary operators
    // apart from built in operators the user has the opportunity to
    // add user defined operators.
    cmBO                =  0,  ///< Operator item:  opening bracket
    cmBC                =  1,  ///< Operator item:  closing bracket
    cmIO                =  2,  ///< Operator item:  index operator opening
    cmIC                =  3,  ///< Operator item:  index operator closing
    cmARG_SEP           =  4,  ///< Operator item:  comma
    cmIF                =  5,  ///< Ternary if then else operator
    cmELSE              =  6,  ///< Ternary if then else operator 
    cmENDIF             =  7,  ///< Ternary if then else operator 
    cmJMP               =  8,  ///< Reserved for future use
    cmVAR               =  9,  ///< variable item
    cmVAL               = 10,  ///< value item
    cmFUNC              = 11,  ///< Code for a function item
    cmOPRT_BIN          = 12,  ///< Binary operator
    cmOPRT_INFIX        = 13,  ///< Infix operator
    cmOPRT_POSTFIX      = 14,  ///< Postfix operator
    cmEOE               = 15,  ///< End of expression

    // The following codes are reserved in case i will ever turn this
    // into a scripting language
    cmSCRIPT_GOTO       = 16,  ///< Reserved for future use
    cmSCRIPT_LABEL      = 17,  ///< Reserved for future use
    cmSCRIPT_FOR        = 18,  ///< Reserved for future use
    cmSCRIPT_IF         = 19,  ///< Reserved for future use
    cmSCRIPT_ELSE       = 20,  ///< Reserved for future use
    cmSCRIPT_ELSEIF     = 21,  ///< Reserved for future use
    cnSCRIPT_ENDIF      = 22,  ///< Reserved for future use
    cmSCRIPT_NEWLINE    = 23,  ///< Newline
    cmSCRIPT_FUNCTION   = 24,  ///< Reserved for future use

    // misc codes
    cmUNKNOWN           = 25,  ///< uninitialized item
    cmCOUNT             = 26   ///< Dummy entry for counting the enum values
  }; // ECmdCode


  //------------------------------------------------------------------------------
  /** \brief Strings assigned to the enum codes of ECmdCode. 

      Used for debugging purposes only.
  */
  extern const char_type *g_sCmdCode[];

  //------------------------------------------------------------------------------
  enum EPackages
  {
    pckCOMMON          = 1 << 0,
    pckUNIT            = 1 << 1,
    pckCOMPLEX         = 1 << 2,
    pckNON_COMPLEX     = 1 << 3,
    pckSTRING          = 1 << 4,
    pckALL_COMPLEX     = pckCOMMON | pckCOMPLEX | pckSTRING | pckUNIT,
    pckALL_NON_COMPLEX = pckCOMMON | pckNON_COMPLEX | pckSTRING | pckUNIT
  };

  //------------------------------------------------------------------------------
  /** \brief Syntax codes. 

    The syntax codes control the syntax check done during the first time parsing of 
    the expression string. They are flags that indicate which tokens are allowed next
    if certain tokens are identified.
  */
  enum ESynCodes
  {
    noBO      = 1 <<  0,  ///< to avoid i.e. "cos(7)(" 
    noBC      = 1 <<  1,  ///< to avoid i.e. "sin)" or "()"
    noIO      = 1 <<  2,  ///< No opening bracket "["
    noIC      = 1 <<  3,  ///< No closing bracket "]"
    noVAL     = 1 <<  4,  ///< to avoid i.e. "tan 2" or "sin(8)3.14"
    noVAR     = 1 <<  5,  ///< to avoid i.e. "sin a" or "sin(8)a"
    noCOMMA   = 1 <<  6,  ///< to avoid i.e. ",," or "+," ...
    noFUN     = 1 <<  7,  ///< to avoid i.e. "sqrt cos" or "(1)sin"	
    noOPT     = 1 <<  8,  ///< to avoid i.e. "(+)"
    noPFX     = 1 <<  9,  ///< to avoid i.e. "(5!!)" "sin!"
    noIFX     = 1 << 10,  ///< to avoid i.e. "++4" "!!4"
    noEND     = 1 << 11,  ///< to avoid unexpected end of expression
    noIF      = 1 << 12,  
    noELSE    = 1 << 13,  
    noNEWLINE = 1 << 14,  ///< to avoid i.e. "a+\nb" or "sin(\na)"

    sfSTART_OF_LINE = noOPT | noBC | noPFX | noCOMMA | noIO | noIC | noIF | noELSE,
    sfALLOW_NONE    = ~0        ///< All of he above flags set
  };	

  //------------------------------------------------------------------------------
  /** \brief Binary operator associativity values. */
  enum EOprtAsct
  {
    oaNONE  = 0,
    oaLEFT  = 1,
    oaRIGHT = 2,
  };

  //------------------------------------------------------------------------------
  /** \brief Parser operator precedence values. 
  
    These are predefined values for the operator precedence.
  */
  enum EOprtPrecedence
  {
    // assignment operators
    prASSIGN       = -1,

    // if-then-else
    prIF_THEN_ELSE =  0,

    // binary operators
    prLOGIC_OR     =  1,
    prLOGIC_AND    =  2,
    prBIT_OR       =  3,
    prBIT_AND      =  4, 

    prRELATIONAL1  =  5, ///< For "==", "!=" 
    prRELATIONAL2  =  6, ///< Relational operators "<", "<=", ">", ">="
    prSHIFT        =  7, ///< Shift operators "<<", ">>"

    prADD_SUB      =  8, ///< addition
    prMUL_DIV      =  9, ///< multiplication/division
    prPOW          = 10, ///< power operator priority (highest)

    // infix operators
    prINFIX        =  9, ///< Signs have a higher priority than ADD_SUB, but lower than power operator
    prPOSTFIX      =  9  ///< Postfix operator priority (currently unused)
  };

#if defined(_UNICODE)

  //------------------------------------------------------------------------------
  /** \brief Encapsulate wcout. */
  inline std::wostream& console()
  {
    return std::wcout;
  }

  //------------------------------------------------------------------------------
  /** \brief Encapsulate cin. */
  inline std::wistream& console_in()
  {
    return std::wcin;
  }

#else

  /** \brief Encapsulate cout. 
  
    Used for supporting UNICODE more easily.
  */
  inline std::ostream& console()
  {
    return std::cout;
  }

  /** \brief Encapsulate cin. 

    Used for supporting UNICODE more easily.
  */
  inline std::istream& console_in()
  {
    return std::cin;
  }

#endif // _UNICODE

}  // namespace mu

#endif
