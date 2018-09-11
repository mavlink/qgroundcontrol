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
#include "mpFuncStr.h"

#include <cmath>
#include <cassert>
#include <cstdio>
#include <cwchar>
#include <algorithm>

#include "mpValue.h"
#include "mpError.h"


MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  //
  // Strlen function
  //
  //------------------------------------------------------------------------------

  FunStrLen::FunStrLen()
    :ICallback(cmFUNC, _T("strlen"), 1)
  {}

  //------------------------------------------------------------------------------
  void FunStrLen::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    string_type str = a_pArg[0]->GetString();
    *ret = (float_type)str.length();
  }

  //------------------------------------------------------------------------------
  const char_type* FunStrLen::GetDesc() const
  {
    return _T("strlen(s) - Returns the length of the string s.");
  }

  //------------------------------------------------------------------------------
  IToken* FunStrLen::Clone() const
  {
    return new FunStrLen(*this);
  }

  //------------------------------------------------------------------------------
  //
  // ToUpper function
  //
  //------------------------------------------------------------------------------

  FunStrToUpper::FunStrToUpper()
    :ICallback(cmFUNC, _T("toupper"), 1)
  {}

  //------------------------------------------------------------------------------
  void FunStrToUpper::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    using namespace std;

    string_type str = a_pArg[0]->GetString();
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);

    *ret = str;
  }

  //------------------------------------------------------------------------------
  const char_type* FunStrToUpper::GetDesc() const
  {
    return _T("toupper(s) - Converts the string s to uppercase characters.");
  }

  //------------------------------------------------------------------------------
  IToken* FunStrToUpper::Clone() const
  {
    return new FunStrToUpper(*this);
  }

  //------------------------------------------------------------------------------
  //
  // ToLower function
  //
  //------------------------------------------------------------------------------

  FunStrToLower::FunStrToLower()
    :ICallback(cmFUNC, _T("tolower"), 1)
  {}

  //------------------------------------------------------------------------------
  void FunStrToLower::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    using namespace std;

    string_type str = a_pArg[0]->GetString();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    *ret = str;
  }

  //------------------------------------------------------------------------------
  const char_type* FunStrToLower::GetDesc() const
  {
    return _T("tolower(s) - Converts the string s to lowercase characters.");
  }

  //------------------------------------------------------------------------------
  IToken* FunStrToLower::Clone() const
  {
    return new FunStrToLower(*this);
  }

  //------------------------------------------------------------------------------
  //
  // String to double conversion
  //
  //------------------------------------------------------------------------------

  FunStrToDbl::FunStrToDbl()
    :ICallback(cmFUNC, _T("str2dbl"), 1)
  {}

  //------------------------------------------------------------------------------
  void FunStrToDbl::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int a_iArgc)
  {
    assert(a_iArgc==1);
    string_type in;
    double out;   // <- Ich will hier wirklich double, auch wenn der Type long double
                  // ist. sscanf und long double geht nicht mit GCC!

    in = a_pArg[0]->GetString();
    
#ifndef _UNICODE    
    sscanf(in.c_str(), "%lf", &out);
#else
    swscanf(in.c_str(), _T("%lf"), &out);
#endif

    *ret = (float_type)out;
  }

  //------------------------------------------------------------------------------
  const char_type* FunStrToDbl::GetDesc() const
  {
    return _T("str2dbl(s) - Converts the string stored in s into a floating foint value.");
  }

  //------------------------------------------------------------------------------
  IToken* FunStrToDbl::Clone() const
  {
    return new FunStrToDbl(*this);
  }
}  // namespace mu
