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
#include "mpPackageMatrix.h"

#include "mpParserBase.h"
#include "mpFuncMatrix.h"
#include "mpOprtMatrix.h"

MUP_NAMESPACE_START

//------------------------------------------------------------------------------
std::unique_ptr<PackageMatrix> PackageMatrix::s_pInstance;

//------------------------------------------------------------------------------
IPackage* PackageMatrix::Instance()
{
  if (s_pInstance.get()==nullptr)
  {
    s_pInstance.reset(new PackageMatrix);
  }

  return s_pInstance.get();
}

//------------------------------------------------------------------------------
void PackageMatrix::AddToParser(ParserXBase *pParser)
{
  // Matrix functions
  pParser->DefineFun(new FunMatrixOnes());
  pParser->DefineFun(new FunMatrixZeros());
  pParser->DefineFun(new FunMatrixEye());
  pParser->DefineFun(new FunMatrixSize());
  
  // Matrix Operators
  pParser->DefinePostfixOprt(new OprtTranspose());

  // Colon operator
//pParser->DefineOprt(new OprtColon());
//pParser->DefineAggregator(new AggColon());
}

//------------------------------------------------------------------------------
string_type PackageMatrix::GetDesc() const
{
  return _T("Operators and functions for matrix operations");
}

//------------------------------------------------------------------------------
string_type PackageMatrix::GetPrefix() const
{
  return _T("");
}

MUP_NAMESPACE_END
