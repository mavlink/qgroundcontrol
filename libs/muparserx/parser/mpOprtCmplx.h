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
#ifndef MP_OPRT_CMPLX_H
#define MP_OPRT_CMPLX_H

/** \file 
    \brief Definitions of classes used as callbacks for standard binary operators. 
*/

/** \defgroup binop Binary operator callbacks

  This group lists the objects representing the binary operators of muParserX.
*/

#include <cmath>
#include "mpIOprt.h"
#include "mpValue.h"
#include "mpError.h"


MUP_NAMESPACE_START

  //---------------------------------------------------------------------------
  /** \brief Callback for the negative sign operator.
      \ingroup infix
  */
  class OprtSignCmplx : public IOprtInfix
  {
  public:
    OprtSignCmplx();
    virtual void Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int a_iArgc) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  }; // class OprtSignCmplx

  //------------------------------------------------------------------------------
  /** \brief Parser callback for implementing an addition of two complex values.
      \ingroup binop
  */
  class OprtAddCmplx : public IOprtBin    
  {
  public:
    OprtAddCmplx();
    virtual void Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  };

  //------------------------------------------------------------------------------
  /** \brief Parser callback for implementing the subtraction of two complex values.
      \ingroup binop
  */
  class OprtSubCmplx : public IOprtBin    
  {
  public:
    OprtSubCmplx();
    virtual void Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  };

  //------------------------------------------------------------------------------
  /** \brief Callback object for implementing the multiplications of complex values.
      \ingroup binop
  */
  class OprtMulCmplx : public IOprtBin    
  {
  public:
    OprtMulCmplx();
    virtual void Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  };

  //------------------------------------------------------------------------------
  /** \brief Callback object for implementing the division of complex values.
      \ingroup binop
  */
  class OprtDivCmplx : public IOprtBin    
  {
  public:
    OprtDivCmplx();
    virtual void Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  };

  //------------------------------------------------------------------------------
  /** \brief Raise x to the power of y.
      \ingroup binop
  */
  class OprtPowCmplx : public IOprtBin
  {
  public:
    OprtPowCmplx();
    virtual void Eval(ptr_val_type& ret, const ptr_val_type *arg, int argc) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  }; 
}  // namespace mu

#endif
