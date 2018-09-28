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
#include "mpPackageNonCmplx.h"

#include "mpParserBase.h"
#include "mpFuncNonCmplx.h"
#include "mpOprtNonCmplx.h"
#include "mpOprtBinCommon.h"


MUP_NAMESPACE_START

//------------------------------------------------------------------------------
std::unique_ptr<PackageNonCmplx> PackageNonCmplx::s_pInstance;

//------------------------------------------------------------------------------
IPackage* PackageNonCmplx::Instance()
{
  if (s_pInstance.get()==nullptr)
  {
    s_pInstance.reset(new PackageNonCmplx);
  }

  return s_pInstance.get();
}

//------------------------------------------------------------------------------
void PackageNonCmplx::AddToParser(ParserXBase *pParser)
{
  pParser->DefineFun(new FunSin());
  pParser->DefineFun(new FunCos());
  pParser->DefineFun(new FunTan());
  pParser->DefineFun(new FunSinH());
  pParser->DefineFun(new FunCosH());
  pParser->DefineFun(new FunTanH());
  pParser->DefineFun(new FunASin());
  pParser->DefineFun(new FunACos());
  pParser->DefineFun(new FunATan());
  pParser->DefineFun(new FunASinH());
  pParser->DefineFun(new FunACosH());
  pParser->DefineFun(new FunATanH());
  pParser->DefineFun(new FunLog());
  pParser->DefineFun(new FunLog10());
  pParser->DefineFun(new FunLog2());
  pParser->DefineFun(new FunLn());
  pParser->DefineFun(new FunExp());
  pParser->DefineFun(new FunSqrt());
  pParser->DefineFun(new FunCbrt());
  pParser->DefineFun(new FunAbs());

  // binary functions
  pParser->DefineFun(new FunPow());
  pParser->DefineFun(new FunHypot());
  pParser->DefineFun(new FunAtan2());
  pParser->DefineFun(new FunFmod());
  pParser->DefineFun(new FunRemainder());

  // Operator callbacks
  pParser->DefineInfixOprt(new OprtSign());
  pParser->DefineInfixOprt(new OprtSignPos());
  pParser->DefineOprt(new OprtAdd());
  pParser->DefineOprt(new OprtSub());
  pParser->DefineOprt(new OprtMul());
  pParser->DefineOprt(new OprtDiv());
  pParser->DefineOprt(new OprtPow);
}

//------------------------------------------------------------------------------
string_type PackageNonCmplx::GetDesc() const
{
  return _T("");
}

//------------------------------------------------------------------------------
string_type PackageNonCmplx::GetPrefix() const
{
  return _T("");
}

MUP_NAMESPACE_END
