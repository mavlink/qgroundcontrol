/** \file
    \brief Definition of the interface for parser callback objects.

<pre>
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
</pre>
*/
#ifndef MU_IPARSER_CALLBACK_H
#define MU_IPARSER_CALLBACK_H

//--- muParserX framework --------------------------------------------
#include "mpIToken.h"
#include "mpIPackage.h"


MUP_NAMESPACE_START

  /** \brief Interface for callback objects. 
    
    All Parser functions and operators must implement this interface.
  */
  class ICallback : public IToken
  {
  public:
      typedef ParserXBase parent_type;  

      ICallback(ECmdCode a_iCode, 
                const char_type *a_szName, 
                int a_nArgNum = 1);
      virtual ~ICallback();

      virtual ICallback* AsICallback();
      virtual IValue* AsIValue();

      virtual void Eval(ptr_val_type& ret, const ptr_val_type *arg, int argc) = 0;
      virtual const char_type* GetDesc() const = 0;
      virtual string_type AsciiDump() const;
        
      int GetArgc() const;
      int GetArgsPresent() const;
      void  SetParent(parent_type *a_pParent);
      void  SetNumArgsPresent(int argc);

  protected:
      parent_type* GetParent();
      void  SetArgc(int argc);

  private:
      parent_type *m_pParent;      ///< Pointer to the parser object using this callback
      int  m_nArgc;                ///< Number of this function can take Arguments.
      int  m_nArgsPresent;         ///< Number of arguments actually submitted
  }; // class ICallback

MUP_NAMESPACE_END

#endif

