/** \file
    \brief A file containing macros used by muParserX

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
#ifndef MUP_DEFINES_H
#define MUP_DEFINES_H

#include <cassert>


#if defined(_UNICODE)
  #if !defined(_T)
  #define _T(x) L##x
  #endif // not defined _T
  #define MUP_STRING_TYPE std::wstring
#else
  #ifndef _T
  /** \brief Macro needed for the "unicodification" of strings.
  */
  #define _T(x) x
  #endif
  
  /** \brief The string type used by muParserX. 
  
    This macro is needed for UNICODE support.
  */
  #define MUP_STRING_TYPE std::string
#endif

/** \brief A macro containing the version of muParserX. */
#define MUP_PARSER_VERSION _T("1.10.1 (20110709)")

/** \brief A macro for setting the parser namespace. */
#define MUP_NAMESPACE_START namespace mup {

/** \brief Closing bracket for the parser namespace macro. */
#define MUP_NAMESPACE_END }

/** \brief A macro for casting between different token types.
    \param TYPE The token type
    \param POINTER Pointer to the token object

  This macro uses a dynamic_cast in debugbuilds and a static_cast
  in release builds for doing the cast operation.
*/
#define MUP_TOK_CAST(TYPE, POINTER)  static_cast<TYPE>(POINTER);

#if defined(_DEBUG)

  /** \brief Debug macro to force an abortion of the programm with a certain message.
  */
  #define MUP_FAIL(MSG)    \
          bool MSG=false;  \
          assert(MSG);

  /** \brief An assertion that does not kill the program.

      This macro is neutralised in UNICODE builds. It's
      too difficult to translate.
  */
  #define MUP_ASSERT(COND)                         \
          if (!(COND))                             \
          {                                        \
            stringstream_type ss;                  \
            ss << _T("Assertion \"") _T(#COND) _T("\" failed: ") \
               << __FILE__ << _T(" line ")         \
               << __LINE__ << _T(".");             \
            throw ParserError( ss.str() );         \
          }
  #define MUP_LEAKAGE_REPORT
#else
  #define MUP_FAIL(MSG)
  #define MUP_ASSERT(COND)
#endif

  /** \brief Include tests for features about to be implemented in 
             the future in the unit test.
  */
  //#define MUP_NICE_TO_HAVE
#endif


