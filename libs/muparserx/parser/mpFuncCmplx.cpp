/** \file
    \brief Definition of functions for complex valued operations.

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
#include "mpFuncCmplx.h"

//--- Standard includes ----------------------------------------------------
#include <cmath>
#include <cassert>
#include <complex>
#include <iostream>

//--- Parser framework -----------------------------------------------------
#include "mpValue.h"
#include "mpError.h"


MUP_NAMESPACE_START

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxReal
  //
  //-----------------------------------------------------------------------

  FunCmplxReal::FunCmplxReal()
    :ICallback(cmFUNC, _T("real"), 1)
  {}

  //-----------------------------------------------------------------------
  FunCmplxReal::~FunCmplxReal()
  {}

  //-----------------------------------------------------------------------
  void FunCmplxReal::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    float_type v = a_pArg[0]->GetFloat();
    *ret = v;
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxReal::GetDesc() const
  {
    return _T("real(x) - Returns the real part of the complex number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxReal::Clone() const
  {
    return new FunCmplxReal(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxImag
  //
  //-----------------------------------------------------------------------

  FunCmplxImag::FunCmplxImag()
    :ICallback(cmFUNC, _T("imag"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxImag::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    float_type v = a_pArg[0]->GetImag();
    *ret = v;
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxImag::GetDesc() const
  {
    return _T("imag(x) - Returns the imaginary part of the complex number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxImag::Clone() const
  {
    return new FunCmplxImag(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxConj
  //
  //-----------------------------------------------------------------------

  FunCmplxConj::FunCmplxConj()
    :ICallback(cmFUNC, _T("conj"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxConj::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    *ret = cmplx_type(a_pArg[0]->GetFloat(), -a_pArg[0]->GetImag());
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxConj::GetDesc() const
  {
    return _T("conj(x) - Returns the complex conjugate of the complex number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxConj::Clone() const
  {
    return new FunCmplxConj(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxArg
  //
  //-----------------------------------------------------------------------

  FunCmplxArg::FunCmplxArg()
    :ICallback(cmFUNC, _T("arg"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxArg::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = std::arg(v);
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxArg::GetDesc() const
  {
    return _T("arg(x) - Returns the phase angle (or angular component) of the complex number x, expressed in radians.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxArg::Clone() const
  {
    return new FunCmplxArg(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxNorm
  //
  //-----------------------------------------------------------------------

  FunCmplxNorm::FunCmplxNorm()
    :ICallback(cmFUNC, _T("norm"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxNorm::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = std::norm(v);
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxNorm::GetDesc() const
  {
    return _T("norm(x) - Returns the norm value of the complex number x.")
           _T(" The norm value of a complex number is the squared magnitude,")
           _T(" defined as the addition of the square of both the real part")
           _T(" and the imaginary part (without the imaginary unit). This is")
           _T(" the square of abs (x).");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxNorm::Clone() const
  {
    return new FunCmplxNorm(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxCos
  //
  //-----------------------------------------------------------------------

  FunCmplxCos::FunCmplxCos()
    :ICallback(cmFUNC, _T("cos"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxCos::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    if (a_pArg[0]->IsNonComplexScalar())
    {
      *ret = std::cos(a_pArg[0]->GetFloat());
    }
    else
    {
      cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
      *ret = std::cos(v);
    }
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxCos::GetDesc() const
  {
    return _T("cos(x) - Returns the cosine of the number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxCos::Clone() const
  {
    return new FunCmplxCos(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxSin
  //
  //-----------------------------------------------------------------------

  FunCmplxSin::FunCmplxSin()
    :ICallback(cmFUNC, _T("sin"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxSin::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    if (a_pArg[0]->IsNonComplexScalar())
    {
      *ret = std::sin(a_pArg[0]->GetFloat());
    }
    else
    {
      cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
      *ret = std::sin(v);
    }
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxSin::GetDesc() const
  {
    return _T("sin(x) - Returns the sine of the number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxSin::Clone() const
  {
    return new FunCmplxSin(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxCosH
  //
  //-----------------------------------------------------------------------

  FunCmplxCosH::FunCmplxCosH()
    :ICallback(cmFUNC, _T("cosh"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxCosH::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = cosh(v);
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxCosH::GetDesc() const
  {
    return _T("cosh(x) - Returns the hyperbolic cosine of the number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxCosH::Clone() const
  {
    return new FunCmplxCosH(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxSinH
  //
  //-----------------------------------------------------------------------

  FunCmplxSinH::FunCmplxSinH()
    :ICallback(cmFUNC, _T("sinh"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxSinH::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = sinh(v);
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxSinH::GetDesc() const
  {
    return _T("sinh(x) - Returns the hyperbolic sine of the complex number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxSinH::Clone() const
  {
    return new FunCmplxSinH(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxTan
  //
  //-----------------------------------------------------------------------

  FunCmplxTan::FunCmplxTan()
    :ICallback(cmFUNC, _T("tan"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxTan::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    if (a_pArg[0]->IsNonComplexScalar())
    {
      *ret = std::tan(a_pArg[0]->GetFloat());
    }
    else
    {
      cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
      *ret = std::tan(v);
    }
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxTan::GetDesc() const
  {
    return _T("tan(x) - Returns the tangens of the number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxTan::Clone() const
  {
    return new FunCmplxTan(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxTanH
  //
  //-----------------------------------------------------------------------

  FunCmplxTanH::FunCmplxTanH()
    :ICallback(cmFUNC, _T("tanh"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxTanH::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = tanh(v);
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxTanH::GetDesc() const
  {
    return _T("tanh(x) - Returns the hyperbolic tangent of the complex number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxTanH::Clone() const
  {
    return new FunCmplxTanH(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxSqrt
  //
  //-----------------------------------------------------------------------

  FunCmplxSqrt::FunCmplxSqrt()
    :ICallback(cmFUNC, _T("sqrt"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxSqrt::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    *ret = sqrt((*a_pArg[0]).GetComplex());
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxSqrt::GetDesc() const
  {
    return _T("sqrt(x) - Returns the square root of x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxSqrt::Clone() const
  {
    return new FunCmplxSqrt(*this);
  }


  //-----------------------------------------------------------------------
  //
  //  class FunCmplxExp
  //
  //-----------------------------------------------------------------------

  FunCmplxExp::FunCmplxExp()
    :ICallback(cmFUNC, _T("exp"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxExp::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = exp(v);
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxExp::GetDesc() const
  {
    return _T("exp(x) - Returns the base-e exponential of the complex number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxExp::Clone() const
  {
    return new FunCmplxExp(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxLn
  //
  //-----------------------------------------------------------------------

  FunCmplxLn::FunCmplxLn()
    :ICallback(cmFUNC, _T("ln"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxLn::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = log(v);
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxLn::GetDesc() const
  {
    return _T("ln(x) - Returns the natural (base-e) logarithm of the complex number x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxLn::Clone() const
  {
    return new FunCmplxLn(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxLog
  //
  //-----------------------------------------------------------------------

  FunCmplxLog::FunCmplxLog()
    :ICallback(cmFUNC, _T("log"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxLog::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = log(v);
  }


  //-----------------------------------------------------------------------
  const char_type* FunCmplxLog::GetDesc() const
  {
    return _T("log(x) - Common logarithm of x, for values of x greater than zero.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxLog::Clone() const
  {
    return new FunCmplxLog(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxLog10
  //
  //-----------------------------------------------------------------------

  FunCmplxLog10::FunCmplxLog10()
    :ICallback(cmFUNC, _T("log10"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxLog10::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    cmplx_type v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = log10(v);
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxLog10::GetDesc() const
  {
    return _T("log10(x) - Common logarithm of x, for values of x greater than zero.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxLog10::Clone() const
  {
    return new FunCmplxLog10(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxLog2
  //
  //-----------------------------------------------------------------------

  FunCmplxLog2::FunCmplxLog2()
    :ICallback(cmFUNC, _T("log2"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxLog2::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    std::complex<float_type> v(a_pArg[0]->GetFloat(), a_pArg[0]->GetImag());
    *ret = std::log(v) * (float_type)1.0/std::log((float_type)2.0);
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxLog2::GetDesc() const
  {
    return _T("log2(x) - Logarithm to base 2 of x, for values of x greater than zero.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxLog2::Clone() const
  {
    return new FunCmplxLog2(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxAbs
  //
  //-----------------------------------------------------------------------

  FunCmplxAbs::FunCmplxAbs()
    :ICallback(cmFUNC, _T("abs"), 1)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxAbs::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    float_type v = sqrt(a_pArg[0]->GetFloat()*a_pArg[0]->GetFloat() + 
                        a_pArg[0]->GetImag()*a_pArg[0]->GetImag());
    *ret = v;
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxAbs::GetDesc() const
  {
    return _T("abs(x) - Returns the absolute value of x.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxAbs::Clone() const
  {
    return new FunCmplxAbs(*this);
  }

  //-----------------------------------------------------------------------
  //
  //  class FunCmplxPow
  //
  //-----------------------------------------------------------------------

  FunCmplxPow::FunCmplxPow()
    :ICallback(cmFUNC, _T("pow"), 2)
  {}

  //-----------------------------------------------------------------------
  void FunCmplxPow::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int)
  {
    *ret = std::pow(a_pArg[0]->GetComplex(), a_pArg[1]->GetComplex());
  }

  //-----------------------------------------------------------------------
  const char_type* FunCmplxPow::GetDesc() const
  {
    return _T("pox(x, y) - Raise x to the power of y.");
  }

  //-----------------------------------------------------------------------
  IToken* FunCmplxPow::Clone() const
  {
    return new FunCmplxPow(*this);
  }


MUP_NAMESPACE_END
