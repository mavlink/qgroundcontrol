/** \file
    \brief Implementation of basic functions used by muParserX.

<pre>
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
  </pre>
*/
#include "mpFuncNonCmplx.h"

//--- Standard includes ----------------------------------------------------
#include <cmath>
#include <cassert>
#include <iostream>

//--- muParserX framework --------------------------------------------------
#include "mpValue.h"
#include "mpError.h"

#undef log
#undef log2

MUP_NAMESPACE_START

  float_type log2(float_type v)  { return log(v) * 1.0/log(2.0); }
  float_type asinh(float_type v) { return log(v + sqrt(v * v + 1)); }
  float_type acosh(float_type v) { return log(v + sqrt(v * v - 1)); }
  float_type atanh(float_type v) { return (0.5 * log((1 + v) / (1 - v))); }

#define MUP_UNARY_FUNC(CLASS, IDENT, FUNC, DESC)                     \
    CLASS::CLASS()                                                   \
    :ICallback(cmFUNC, _T(IDENT), 1)                                 \
    {}                                                               \
                                                                     \
    void CLASS::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)        \
    {                                                                \
      *ret = FUNC(a_pArg[0]->GetFloat());                            \
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

    // trigonometric functions
    MUP_UNARY_FUNC(FunTan,   "sin",   std::sin,   "sine function")
    MUP_UNARY_FUNC(FunCos,   "cos",   std::cos,   "cosine function")
    MUP_UNARY_FUNC(FunSin,   "tan",   std::tan,   "tangens function")
    // arcus functions
    MUP_UNARY_FUNC(FunASin,  "asin",  std::asin,  "arcus sine")
    MUP_UNARY_FUNC(FunACos,  "acos",  std::acos,  "arcus cosine")
    MUP_UNARY_FUNC(FunATan,  "atan",  std::atan,  "arcus tangens")
    // hyperbolic functions
    MUP_UNARY_FUNC(FunSinH,  "sinh",  std::sinh,  "hyperbolic sine")
    MUP_UNARY_FUNC(FunCosH,  "cosh",  std::cosh,  "hyperbolic cosine")
    MUP_UNARY_FUNC(FunTanH,  "tanh",  std::tanh,  "hyperbolic tangens")
    // hyperbolic arcus functions
    MUP_UNARY_FUNC(FunASinH,  "asinh",  asinh,  "hyperbolic arcus sine")
    MUP_UNARY_FUNC(FunACosH,  "acosh",  acosh,  "hyperbolic arcus cosine")
    MUP_UNARY_FUNC(FunATanH,  "atanh",  atanh,  "hyperbolic arcus tangens")
    // logarithm functions
    MUP_UNARY_FUNC(FunLog,   "log",   std::log,   "Natural logarithm")
    MUP_UNARY_FUNC(FunLog10, "log10", std::log10, "Logarithm base 10")
    MUP_UNARY_FUNC(FunLog2,  "log2",  log2,  "Logarithm base 2")
    MUP_UNARY_FUNC(FunLn,    "ln",    std::log,   "Natural logarithm")
    // square root
    MUP_UNARY_FUNC(FunSqrt,  "sqrt",  std::sqrt,  "sqrt(x) - square root of x")
    MUP_UNARY_FUNC(FunCbrt,  "cbrt",  std::cbrt,  "cbrt(x) - cubic root of x")
    MUP_UNARY_FUNC(FunExp,   "exp",   std::exp,   "exp(x) - e to the power of x")
    MUP_UNARY_FUNC(FunAbs,   "abs",   std::fabs,  "abs(x) - absolute value of x")
#undef MUP_UNARY_FUNC

#define MUP_BINARY_FUNC(CLASS, IDENT, FUNC, DESC) \
    CLASS::CLASS()                                                   \
    :ICallback(cmFUNC, _T(IDENT), 2)                                 \
    {}                                                               \
                                                                     \
    void CLASS::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)        \
    {                                                                \
      *ret = FUNC(a_pArg[0]->GetFloat(), a_pArg[1]->GetFloat());     \
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

    MUP_BINARY_FUNC(FunPow,  "pow",  std::pow,  "pow(x, y) - raise x to the power of y")
    MUP_BINARY_FUNC(FunHypot,  "hypot",  std::hypot,  "hypot(x, y) - compute the length of the vector x,y")
    MUP_BINARY_FUNC(FunAtan2, "atan2", std::atan2, "arcus tangens with quadrant fix")
    MUP_BINARY_FUNC(FunFmod,  "fmod",  std::fmod,  "fmod(x, y) - floating point remainder of x / y")
    MUP_BINARY_FUNC(FunRemainder,  "remainder",  std::remainder,  "remainder(x, y) - IEEE remainder of x / y")
#undef MUP_BINARY_FUNC

MUP_NAMESPACE_END
