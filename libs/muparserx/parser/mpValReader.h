/** \file
    \brief Definition of classes that interpret values in a string.

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
#ifndef MU_PARSER_IMPL_READER_H
#define MU_PARSER_IMPL_READER_H

#include "mpIValReader.h"



MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  //
  //  Reader for floating point values
  //
  //------------------------------------------------------------------------------

  /** \brief A class for reading floating point values from an expression string.
      \ingroup valreader
  */
  class DblValReader : public IValueReader
  {
  public:    
      DblValReader();
      virtual ~DblValReader();
      virtual bool IsValue(const char_type *a_szExpr, int &a_iPos, Value &a_fVal) override;
      virtual IValueReader* Clone(TokenReader *pTokenReader) const override;
  };

  //------------------------------------------------------------------------------
  //
  //  Reader for boolean values
  //
  //------------------------------------------------------------------------------

  /** \brief A class for reading boolean values from an expression string.
      \ingroup valreader
  */
  class BoolValReader : public IValueReader
  {
  public:    
      BoolValReader();
      virtual ~BoolValReader();
      virtual bool IsValue(const char_type *a_szExpr, int &a_iPos, Value &a_fVal) override;
      virtual IValueReader* Clone(TokenReader *pTokenReader) const override;
  };

  //------------------------------------------------------------------------------
  //
  //  Reader for hex values
  //
  //------------------------------------------------------------------------------

  /** \brief A class for reading hex values from an expression string.
      \ingroup valreader
  */
  class HexValReader : public IValueReader
  {
  public:    
      HexValReader();
      virtual bool IsValue(const char_type *a_szExpr, int &a_iPos, Value &a_fVal) override;
      virtual IValueReader* Clone(TokenReader *pTokenReader) const override;
  };

  //------------------------------------------------------------------------------
  //
  //  Reader for binary values
  //
  //------------------------------------------------------------------------------

  /** \brief A class for reading binary values from an expression string.
      \ingroup valreader
  */
  class BinValReader : public IValueReader
  {
  public:    
      BinValReader();
      virtual ~BinValReader();
      virtual bool IsValue(const char_type *a_szExpr, int &a_iPos, Value &a_fVal) override;
      virtual IValueReader* Clone(TokenReader *pTokenReader) const override;
  };

  //------------------------------------------------------------------------------
  //
  //  Reader for string values
  //
  //------------------------------------------------------------------------------

  /** \brief A class for reading strings from an expression string.
      \ingroup valreader
  */
  class StrValReader : public IValueReader
  {
  public:    
      StrValReader();
      virtual ~StrValReader();
      virtual bool IsValue(const char_type *a_szExpr, int &a_iPos, Value &a_fVal) override;
      virtual IValueReader* Clone(TokenReader *pTokenReader) const override;

  private:
      string_type Unescape(const char_type *szExpr, int &len);
  };

MUP_NAMESPACE_END

#endif
