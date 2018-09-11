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
#ifndef MU_IPARSER_VALUE_READER_H
#define MU_IPARSER_VALUE_READER_H

#include "mpValue.h"
#include "mpIToken.h"
#include "mpTokenReader.h"

/** \defgroup valreader Value reader classes

  This group lists all classes that detect and parse values in an expression string.
*/


MUP_NAMESPACE_START

  class TokenReader;

  /** \brief Interface for custom value reader objects. 
      \ingroup valreader

    Value readers are objects used for identifying values 
    in an expression.
  */
  class IValueReader
  {
  public:

    IValueReader();
    IValueReader(const IValueReader &ref);

    virtual ~IValueReader();

    /** \brief Check a certain position in an expression for the presence of a value. 
        \param a_iPos [in/out] Reference to an integer value representing the current 
                      position of the parser in the expression.
        \param a_Val If a value is found it is stored in a_Val
        \return true if a value was found
    */
    virtual bool IsValue(const char_type *a_szExpr,
                         int &a_iPos, 
                         Value &a_Val ) = 0;

    /** \brief Clone this ValueReader object. 
        \return Pointer to the cloned value reader object.
    */
    virtual IValueReader* Clone(TokenReader *pParent) const = 0;
    
    /** \brief Assign this value reader object to a token 
               reader object. 
    
      The token reader does the tokenization of the expression.
      It uses this value reader to detect values.
    */
    virtual void SetParent(TokenReader *pTokenReader);

  protected:

    const IToken* TokenHistory(std::size_t pos) const;

  private:

    TokenReader *m_pTokenReader;  ///< Pointer to the TokenReader class used for token recognition
  }; // class IValueReader

MUP_NAMESPACE_END

#endif
