/** \file mpIOprt.h
    \brief Definition of base classes needed for parser operator definitions.

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
#ifndef MUP_IPARSER_OPERATOR_H
#define MUP_IPARSER_OPERATOR_H

#include "mpICallback.h"
#include "mpIPrecedence.h"


MUP_NAMESPACE_START

    //------------------------------------------------------------------------------
    /** \brief Interface for binary operators.
        \ingroup binop

      All classes representing binary operator callbacks must be derived from
      this base class.
    */
    class IOprtBin : public ICallback,
                     public IPrecedence
    {
    public:

      IOprtBin(const char_type *a_szIdent, int nPrec, EOprtAsct eAsc);
      virtual ~IOprtBin();
      virtual string_type AsciiDump() const;

      //------------------------------------------
      // IPrecedence implementation
      //------------------------------------------

      virtual IPrecedence* AsIPrecedence();
      virtual EOprtAsct GetAssociativity() const;
      virtual int GetPri() const;

    private:
      int m_nPrec;
      EOprtAsct m_eAsc;
    }; // class IOperator


    //------------------------------------------------------------------------------
    /** \brief Interface for unary postfix operators.
        \ingroup postfix
    */
    class IOprtPostfix : public ICallback
    {
    public:
        IOprtPostfix(const char_type *a_szIdent);
        virtual ~IOprtPostfix();
        virtual string_type AsciiDump() const;
    }; // class IOperator


    //------------------------------------------------------------------------------
    /** \brief Interface for unary infix operators.
        \ingroup infix
    */
    class IOprtInfix : public ICallback,
                       public IPrecedence
    {
    public:
      IOprtInfix(const char_type *a_szIdent, int nPrec);
      virtual ~IOprtInfix();
      virtual string_type AsciiDump() const;

      //------------------------------------------
      // IPrecedence implementation
      //------------------------------------------

      virtual IPrecedence* AsIPrecedence();
      virtual int GetPri() const;
      virtual EOprtAsct GetAssociativity() const;

    private:
      int m_nPrec;
    }; // class IOperator
}  // namespace mu

#endif

