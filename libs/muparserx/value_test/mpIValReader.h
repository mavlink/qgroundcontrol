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
#ifndef MU_IPARSER_VALUE_READER_H
#define MU_IPARSER_VALUE_READER_H

#include "mpValue.h"
#include "mpIToken.h"
#include "mpTokenReader.h"


MUP_NAMESPACE_START

  class TokenReader;

  /** \brief Interface for custom value reader objects. 
  
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
