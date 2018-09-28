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
#ifndef MUP_FUNC_STR_H
#define MUP_FUNC_STR_H

#include "mpICallback.h"


MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  /** \brief Callback object for determining the length of a string. 
      \ingroup functions
  */
  class FunStrLen : public ICallback
  {
  public:
    FunStrLen();
    virtual void Eval(ptr_val_type& ret, const ptr_val_type *a_pArg, int a_iArgc) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  };

  //------------------------------------------------------------------------------
  /** \brief Convert a string to upper case letters. 
      \ingroup functions  
  */
  class FunStrToUpper : public ICallback
  {
  public:
    FunStrToUpper();
    virtual void Eval(ptr_val_type& ret, const ptr_val_type *a_pArg, int a_iArgc) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  };

  //------------------------------------------------------------------------------
  /** \brief Convert a string to lower case letters.
      \ingroup functions
  */
  class FunStrToLower : public ICallback
  {
  public:
    FunStrToLower();
    virtual void Eval(ptr_val_type& ret, const ptr_val_type *a_pArg, int a_iArgc) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  };

  //------------------------------------------------------------------------------
  /** \brief Parse string to a floating point value. 
      \ingroup functions  
  */
  class FunStrToDbl : public ICallback
  {
  public:
    FunStrToDbl ();
    virtual void Eval(ptr_val_type& ret, const ptr_val_type *a_pArg, int a_iArgc) override;
    virtual const char_type* GetDesc() const override;
    virtual IToken* Clone() const override;
  }; // class FunStrToDbl
}  // namespace mu

#endif
