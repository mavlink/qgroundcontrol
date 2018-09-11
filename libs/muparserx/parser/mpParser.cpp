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

#include "mpParser.h"

//--- Standard includes ----------------------------------------------------
#include <cmath>
#include <algorithm>
#include <numeric>

//--- Parser framework -----------------------------------------------------
#include "mpPackageUnit.h"
#include "mpPackageStr.h"
#include "mpPackageCmplx.h"
#include "mpPackageNonCmplx.h"
#include "mpPackageCommon.h"
#include "mpPackageMatrix.h"

using namespace std;


/** \brief Namespace for mathematical applications. */
MUP_NAMESPACE_START

  //---------------------------------------------------------------------------
  /** \brief Default constructor. 

    Call ParserXBase class constructor and initiate function, operator 
    and constant initialization.
  */
  ParserX::ParserX(unsigned ePackages)
    :ParserXBase()
  {
    DefineNameChars(_T("0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
    DefineOprtChars(_T("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-*^/?<>=#!$%&|~'_µ{}"));
    DefineInfixOprtChars(_T("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ()/+-*^?<>=#!$%&|~'_"));

    if (ePackages & pckUNIT)
      AddPackage(PackageUnit::Instance());

    if (ePackages & pckSTRING)
      AddPackage(PackageStr::Instance());

    if (ePackages & pckCOMPLEX)
      AddPackage(PackageCmplx::Instance());

    if (ePackages & pckNON_COMPLEX)
      AddPackage(PackageNonCmplx::Instance());

    if (ePackages & pckCOMMON)
      AddPackage(PackageCommon::Instance());

    if (ePackages & pckMATRIX)
      AddPackage(PackageMatrix::Instance());
  }

  //------------------------------------------------------------------------------
  void ParserX::ResetErrorMessageProvider(ParserMessageProviderBase *pProvider)
  {
    ParserErrorMsg::Reset(pProvider);
  }

} // namespace mu
