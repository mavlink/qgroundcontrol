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
#include "mpPackageUnit.h"

#include "mpParserBase.h"


MUP_NAMESPACE_START

/** \brief This is a macro for defining scaling postfix operators.

  These operators can be used for unit conversions.
*/
#define MUP_POSTFIX_IMLP(CLASS, IDENT, MUL, DESC)                  \
  CLASS::CLASS(IPackage*)                                          \
    :IOprtPostfix(_T(IDENT))                                       \
  {}                                                               \
                                                                   \
  void CLASS::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int) \
  {                                                                \
    if (!a_pArg[0]->IsScalar())                                    \
    {                                                              \
      ErrorContext err(ecTYPE_CONFLICT,                            \
                       GetExprPos(),                               \
                       a_pArg[0]->ToString(),                      \
                       a_pArg[0]->GetType(),                       \
                       'c',                                        \
                       1);                                         \
      throw ParserError(err);                                      \
    }                                                              \
                                                                   \
    *ret = a_pArg[0]->GetComplex() * MUL;                          \
  }                                                                \
                                                                   \
  const char_type* CLASS::GetDesc() const                          \
  {                                                                \
    return _T(DESC);                                               \
  }                                                                \
                                                                   \
  IToken* CLASS::Clone() const                                     \
  {                                                                \
    return new CLASS(*this);                                       \
  }

  MUP_POSTFIX_IMLP(OprtNano,   "n",   (float_type)1e-9,   "n - unit multiplicator 1e-9")
  MUP_POSTFIX_IMLP(OprtMicro,  "u",   (float_type)1e-6,   "u - unit multiplicator 1e-6")
  MUP_POSTFIX_IMLP(OprtMilli,  "m",   (float_type)1e-3,   "m - unit multiplicator 1e-3")
  MUP_POSTFIX_IMLP(OprtKilo,   "k",   (float_type)1e3,    "k - unit multiplicator 1e3")
  MUP_POSTFIX_IMLP(OprtMega,   "M",   (float_type)1e6,    "M - unit multiplicator 1e6")
  MUP_POSTFIX_IMLP(OprtGiga,   "G",   (float_type)1e9,    "G - unit multiplicator 1e9")

#undef MUP_POSTFIX_IMLP

//------------------------------------------------------------------------------
std::unique_ptr<PackageUnit> PackageUnit::s_pInstance;

//------------------------------------------------------------------------------
IPackage* PackageUnit::Instance()
{
  if (s_pInstance.get()==nullptr)
  {
    s_pInstance.reset(new PackageUnit);
  }

  return s_pInstance.get();
}

//------------------------------------------------------------------------------
void PackageUnit::AddToParser(ParserXBase *pParser)
{
  pParser->DefinePostfixOprt(new OprtNano(this));
  pParser->DefinePostfixOprt(new OprtMicro(this));
  pParser->DefinePostfixOprt(new OprtMilli(this));
  pParser->DefinePostfixOprt(new OprtKilo(this));
  pParser->DefinePostfixOprt(new OprtMega(this));
  pParser->DefinePostfixOprt(new OprtGiga(this));
}

//------------------------------------------------------------------------------
string_type PackageUnit::GetDesc() const
{
  return _T("Postfix operators for basic unit conversions.");
}

//------------------------------------------------------------------------------
string_type PackageUnit::GetPrefix() const
{
  return _T("");
}

MUP_NAMESPACE_END
