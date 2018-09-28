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
#include "mpIValReader.h"
#include "mpTokenReader.h"

#include <cassert>


MUP_NAMESPACE_START

//--------------------------------------------------------------------------------------------
    IValueReader::IValueReader()
      :m_pTokenReader(nullptr)
    {}

    //--------------------------------------------------------------------------------------------
    IValueReader::~IValueReader() 
    {}

    //--------------------------------------------------------------------------------------------
    IValueReader::IValueReader(const IValueReader &ref)
    {
      m_pTokenReader = ref.m_pTokenReader;
    }

    //--------------------------------------------------------------------------------------------
    void IValueReader::SetParent(TokenReader *pTokenReader)
    {
      assert(pTokenReader);
      m_pTokenReader = pTokenReader;
    }

    //--------------------------------------------------------------------------------------------
    const IToken* IValueReader::TokenHistory(std::size_t pos) const
    {
      const TokenReader::token_buf_type &buf = m_pTokenReader->GetTokens();
      std::size_t size = buf.size();
      return (pos>=size) ? nullptr : buf[size-1-pos].Get();
    }
}
