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
#include "mpPackageCommon.h"

#include "mpParserBase.h"
#include "mpFuncNonCmplx.h"
#include "mpFuncCommon.h"
#include "mpOprtBinCommon.h"
#include "mpOprtBinAssign.h"
#include "mpOprtPostfixCommon.h"
#include "mpValReader.h"

/** \brief Pi (what else?). */
#define MUP_CONST_PI  3.141592653589793238462643
//#define MUP_CONST_PI    3.14159265358979323846264338327950288419716939937510L

/** \brief The eulerian number. */
#define MUP_CONST_E   2.718281828459045235360287


MUP_NAMESPACE_START

//------------------------------------------------------------------------------
std::unique_ptr<PackageCommon> PackageCommon::s_pInstance;

//------------------------------------------------------------------------------
IPackage* PackageCommon::Instance()
{
  if (s_pInstance.get()==nullptr)
  {
    s_pInstance.reset(new PackageCommon);
  }

  return s_pInstance.get();
}

//------------------------------------------------------------------------------
void PackageCommon::AddToParser(ParserXBase *pParser)
{
  // Readers that need fancy decorations on their values must
  // be added first (i.e. hex -> "0x...") Otherwise the
  // zero in 0x will be read as a value of zero!
  pParser->AddValueReader(new HexValReader);
  pParser->AddValueReader(new BinValReader);
  pParser->AddValueReader(new DblValReader);
  pParser->AddValueReader(new BoolValReader);

  // Constants
  pParser->DefineConst( _T("pi"), (float_type)MUP_CONST_PI );
  pParser->DefineConst( _T("e"),  (float_type)MUP_CONST_E );

  // Vector
  pParser->DefineFun(new FunSizeOf());

  // Generic functions
  pParser->DefineFun(new FunMax());
  pParser->DefineFun(new FunMin());
  pParser->DefineFun(new FunSum());

  // misc
  pParser->DefineFun(new FunParserID);

  // integer package
  pParser->DefineOprt(new OprtLAnd);
  pParser->DefineOprt(new OprtLOr);
  pParser->DefineOprt(new OprtAnd);
  pParser->DefineOprt(new OprtOr);
  pParser->DefineOprt(new OprtShr);
  pParser->DefineOprt(new OprtShl);

  // booloean package
  pParser->DefineOprt(new OprtLE);
  pParser->DefineOprt(new OprtGE);
  pParser->DefineOprt(new OprtLT);
  pParser->DefineOprt(new OprtGT);
  pParser->DefineOprt(new OprtEQ);
  pParser->DefineOprt(new OprtNEQ);
  pParser->DefineOprt(new OprtLAnd(_T("and")));  // add logic and with a different identifier
  pParser->DefineOprt(new OprtLOr(_T("or")));    // add logic and with a different identifier
//  pParser->DefineOprt(new OprtBXor);

  // assignement operators
  pParser->DefineOprt(new OprtAssign);
  pParser->DefineOprt(new OprtAssignAdd);
  pParser->DefineOprt(new OprtAssignSub);
  pParser->DefineOprt(new OprtAssignMul);
  pParser->DefineOprt(new OprtAssignDiv);

  // infix operators
  pParser->DefineInfixOprt(new OprtCastToFloat);
  pParser->DefineInfixOprt(new OprtCastToInt);

  // postfix operators
  pParser->DefinePostfixOprt(new OprtFact);
// <ibg 20130708> commented: "%" is a reserved sign for either the 
//                modulo operator or comment lines. 
//  pParser->DefinePostfixOprt(new OprtPercentage);
// </ibg>
}

//------------------------------------------------------------------------------
string_type PackageCommon::GetDesc() const
{
  return _T("");
}

//------------------------------------------------------------------------------
string_type PackageCommon::GetPrefix() const
{
  return _T("");
}

MUP_NAMESPACE_END
