#ifndef MU_PACKAGE_UNIT_H
#define MU_PACKAGE_UNIT_H

/*
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
*/

#include <memory>
#include "mpIPackage.h"
#include "mpIOprt.h"

MUP_NAMESPACE_START

#define MUP_POSTFIX_DEF(CLASS)                                             \
    class CLASS : public IOprtPostfix                                      \
    {                                                                      \
    public:                                                                \
      CLASS(IPackage* pPackage=nullptr);                                      \
      virtual void Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int a_iArgc);  \
      virtual const char_type* GetDesc() const;                            \
      virtual IToken* Clone() const;                                       \
    }; 

MUP_POSTFIX_DEF(OprtNano)
MUP_POSTFIX_DEF(OprtMicro)
MUP_POSTFIX_DEF(OprtMilli)
MUP_POSTFIX_DEF(OprtKilo)
MUP_POSTFIX_DEF(OprtMega)
MUP_POSTFIX_DEF(OprtGiga)

#undef MUP_POSTFIX_DEF

//------------------------------------------------------------------------------
/** \brief Package for installing unit postfix operators into muParserX. */
class PackageUnit : public IPackage
{
friend class std::unique_ptr<PackageUnit>;

public:
  
  static IPackage* Instance();
  virtual void AddToParser(ParserXBase *pParser);
  virtual string_type GetDesc() const;
  virtual string_type GetPrefix() const;

private:

  static std::unique_ptr<PackageUnit> s_pInstance;
};

MUP_NAMESPACE_END

#endif
